#pragma optimize("", off)
#include "overmap_generation.h"

#include "overmap.h"
#include "overmap_connection.h"
#include "overmap_location.h"

extern bool debug_connection_lay = false;

/*
pf::directed_path<point_om_omt> overmap::lay_out_connection(
    const overmap_connection &connection, const point_om_omt &source, const point_om_omt &dest,
    int z, const bool must_be_unexplored ) const
{
    const pf::two_node_scoring_fn<point_om_omt> estimate =
    [&]( pf::directed_node<point_om_omt> cur, cata::optional<pf::directed_node<point_om_omt>> prev ) {
        if( debug_connection_lay ) {
            std::cout << string_format(
                          "\nNode %s %s <= %s %s",
                          cur.pos.to_string(),
                          om_direction::name( cur.dir ),
                          prev ? prev->pos.to_string() : "?",
                          prev ? om_direction::name( prev->dir ) : "?"
                      );
        }

        const oter_id &id( ter( tripoint_om_omt( cur.pos, z ) ) );

        if( debug_connection_lay ) {
            std::cout << string_format( "  ter: %s", id.id().str() );
        }

        const overmap_connection::subtype *subtype = connection.pick_subtype_for( id );

        if( !subtype ) {
            if( debug_connection_lay ) {
                std::cout << "\n  Rejected: terrain not supported";
            }
            return pf::node_score::rejected;  // No option for this terrain.
        }

        if( debug_connection_lay ) {
            std::cout << string_format( "  subtype: %s", subtype->terrain.str() );
        }

        const bool existing_connection = connection.has( id );

        // Only do this check if it needs to be unexplored and there isn't already a connection.
        if( must_be_unexplored && !existing_connection ) {
            // If this must be unexplored, check if we've already got a submap generated.
            const bool existing_submap = is_omt_generated( tripoint_om_omt( cur.pos, z ) );

            // If there is an existing submap, this area has already been explored and this
            // isn't a valid placement.
            if( existing_submap ) {
                return pf::node_score::rejected;
            }
        }

        if( existing_connection && id->is_rotatable() && cur.dir != om_direction::type::invalid &&
            !om_direction::are_parallel( id->get_dir(), cur.dir ) ) {
            if( debug_connection_lay ) {
                std::cout << "\n  Rejected: can't intersect existing terrain";
            }
            return pf::node_score::rejected; // Can't intersect.
        }

        if( prev && prev->dir != om_direction::type::invalid && prev->dir != cur.dir ) {
            // Direction has changed.
            const oter_id &prev_id = ter( tripoint_om_omt( prev->pos, z ) );
            const overmap_connection::subtype *prev_subtype = connection.pick_subtype_for( prev_id );

            if( debug_connection_lay ) {
                std::cout << string_format(
                              "  prev_subtype: %s ",
                              prev_subtype ? prev_subtype->terrain.str() : "?"
                          );
            }

            if( !prev_subtype || !prev_subtype->allows_turns() ) {
                if( debug_connection_lay ) {
                    std::cout << "\n  Rejected: can't make a turn";
                }
                return pf::node_score::rejected;
            }
        }

        const int dist = subtype->is_orthogonal() ?
                         manhattan_dist( dest, cur.pos ) :
                         trig_dist( dest, cur.pos );
        const int existency_mult = existing_connection ? 1 : 5; // Prefer existing connections.

        const int node_score = subtype->basic_cost;
        const int dist_score = existency_mult * dist;

        if( debug_connection_lay ) {
            std::cout << string_format(
                          "\n  Allowed with node_score %s  dist_score %s",
                          node_score,
                          dist_score
                      );
        }

        return pf::node_score( node_score, dist_score );
    };

    if( debug_connection_lay ) {
        std::cout << string_format( "SEARCHING PATH %s => %s\n", source.to_string(), dest.to_string() );
    }

    auto ret = pf::greedy_path( source, dest, point_om_omt( OMAPX, OMAPY ), estimate );

    if( debug_connection_lay ) {
        std::cout << string_format( "\n\nDONE nodes_total: %d\n\n", ret.nodes.size() );
    }

    return ret;
}

static pf::directed_path<point_om_omt> straight_path( const point_om_omt &source,
        om_direction::type dir, size_t len )
{
    pf::directed_path<point_om_omt> res;
    if( len == 0 ) {
        return res;
    }
    point_om_omt p = source;
    res.nodes.reserve( len );
    for( size_t i = 0; i + 1 < len; ++i ) {
        res.nodes.emplace_back( p, dir );
        p += om_direction::displace( dir );
    }
    res.nodes.emplace_back( p, om_direction::type::invalid );
    return res;
}

pf::directed_path<point_om_omt> overmap::lay_out_street( const overmap_connection &connection,
        const point_om_omt &source, om_direction::type dir, size_t len ) const
{
    const tripoint_om_omt from( source, 0 );
    // See if we need to make another one "step" further.
    const tripoint_om_omt en_pos = from + om_direction::displace( dir, len + 1 );
    if( inbounds( en_pos, 1 ) && connection.has( ter( en_pos ) ) ) {
        ++len;
    }

    size_t actual_len = 0;

    while( actual_len < len ) {
        const tripoint_om_omt pos = from + om_direction::displace( dir, actual_len );

        if( !inbounds( pos, 1 ) ) {
            break;  // Don't approach overmap bounds.
        }

        const oter_id &ter_id = ter( pos );

        if( ter_id->is_river() || !connection.pick_subtype_for( ter_id ) ) {
            break;
        }

        bool collided = false;
        int collisions = 0;
        for( int i = -1; i <= 1; i++ ) {
            if( collided ) {
                break;
            }
            for( int j = -1; j <= 1; j++ ) {
                const tripoint_om_omt checkp = pos + tripoint( i, j, 0 );

                if( checkp != pos + om_direction::displace( dir, 1 ) &&
                    checkp != pos + om_direction::displace( om_direction::opposite( dir ), 1 ) &&
                    checkp != pos ) {
                    if( is_ot_match( "road", ter( checkp ), ot_match_type::type ) ) {
                        collisions++;
                    }
                }
            }

            //Stop roads from running right next to eachother
            if( collisions >= 3 ) {
                collided = true;
                break;
            }
        }
        if( collided ) {
            break;
        }

        ++actual_len;

        if( actual_len > 1 && connection.has( ter_id ) ) {
            break;  // Stop here.
        }
    }

    return straight_path( source, dir, actual_len );
}

void overmap::build_connection(
    const overmap_connection &connection, const pf::directed_path<point_om_omt> &path, int z,
    const om_direction::type &initial_dir )
{
    if( path.nodes.empty() ) {
        return;
    }

    om_direction::type prev_dir = initial_dir;

    const pf::directed_node<point_om_omt> start = path.nodes.front();
    const pf::directed_node<point_om_omt> end = path.nodes.back();

    for( const auto &node : path.nodes ) {
        const tripoint_om_omt pos( node.pos, z );
        const oter_id &ter_id = ter( pos );
        const om_direction::type new_dir = node.dir;
        const overmap_connection::subtype *subtype = connection.pick_subtype_for( ter_id );

        if( !subtype ) {
            debugmsg( "No suitable subtype of connection \"%s\" found for \"%s\".", connection.id.c_str(),
                      ter_id.id().c_str() );
            return;
        }

        if( subtype->terrain->is_linear() ) {
            size_t new_line = connection.has( ter_id ) ? ter_id->get_line() : 0;

            if( new_dir != om_direction::type::invalid ) {
                new_line = om_lines::set_segment( new_line, new_dir );
            }

            if( prev_dir != om_direction::type::invalid ) {
                new_line = om_lines::set_segment( new_line, om_direction::opposite( prev_dir ) );
            }

            for( const om_direction::type dir : om_direction::all ) {
                const tripoint_om_omt np( pos + om_direction::displace( dir ) );

                if( inbounds( np ) ) {
                    const oter_id &near_id = ter( np );

                    if( connection.has( near_id ) ) {
                        if( near_id->is_linear() ) {
                            const size_t near_line = near_id->get_line();

                            if( om_lines::is_straight( near_line ) || om_lines::has_segment( near_line, new_dir ) ) {
                                // Mutual connection.
                                const size_t new_near_line = om_lines::set_segment( near_line, om_direction::opposite( dir ) );
                                ter_set( np, near_id->get_type_id()->get_linear( new_near_line ) );
                                new_line = om_lines::set_segment( new_line, dir );
                            }
                        } else if( near_id->is_rotatable() && om_direction::are_parallel( dir, near_id->get_dir() ) ) {
                            new_line = om_lines::set_segment( new_line, dir );
                        }
                    }
                } else if( pos.xy() == start.pos || pos.xy() == end.pos ) {
                    // Only automatically connect to out of bounds locations if we're the start or end of this path.
                    new_line = om_lines::set_segment( new_line, dir );

                    // Add this connection point to our connections out.
                    std::vector<tripoint_om_omt> &outs = connections_out[connection.id];
                    const auto existing_out = std::find_if( outs.begin(),
                    outs.end(), [pos]( const tripoint_om_omt & c ) {
                        return c == pos;
                    } );
                    if( existing_out == outs.end() ) {
                        outs.emplace_back( pos );
                    }
                }
            }

            if( new_line == om_lines::invalid ) {
                debugmsg( "Invalid path for connection \"%s\".", connection.id.c_str() );
                return;
            }

            ter_set( pos, subtype->terrain->get_linear( new_line ) );
        } else if( new_dir != om_direction::type::invalid ) {
            ter_set( pos, subtype->terrain->get_rotated( new_dir ) );
        }

        prev_dir = new_dir;
    }
}

void overmap::build_connection( const point_om_omt &source, const point_om_omt &dest, int z,
                                const overmap_connection &connection, const bool must_be_unexplored,
                                const om_direction::type &initial_dir )
{
    build_connection(
        connection, lay_out_connection( connection, source, dest, z, must_be_unexplored ),
        z, initial_dir );
}
*/

template<typename T>
using array_2d = std::array<std::array<T, OMAPY>, OMAPX>;

template<typename T>
using dir_array = std::array<T, om_direction::size>;

struct piece_link {
    int tgt_piece_idx = -1;
    tripoint tgt_pos;
    om_direction::type tgt_dir = om_direction::type::invalid;
    int tgt_conn_idx = -1;
    int src_conn_idx = -1;
};

struct single_piece_placement {
    int cost = -1;
    std::vector<piece_link> links;

    bool is_valid() const {
        return cost >= 0;
    }
};

/**
 * Holds possible placements of all pieces at all positions and rotations.
 */
struct piece_placements {
    array_2d< std::vector< dir_array<single_piece_placement> > > data;

    explicit piece_placements( const int num_pieces ) {
        for( auto &ref_row : data ) {
            for( auto &ref : ref_row ) {
                ref.resize( num_pieces );
            }
        }
    }
    ~piece_placements() = default;

    single_piece_placement &get( int piece_idx, point pos, om_direction::type dir ) {
        return data[pos.x][pos.y][piece_idx][om_direction::get_num_cw_rotations( dir )];
    }

    const single_piece_placement &get( int piece_idx, point pos, om_direction::type dir ) const {
        return data[pos.x][pos.y][piece_idx][om_direction::get_num_cw_rotations( dir )];
    }
};

/**
 * Check whether piece can be placed at given position with given rotation.
 * Returns cata::nullopt if placement is impossible, placement cost otherwise.
 */
static cata::optional<int> test_placement_at(
    const overmap &om,
    const om_connection_piece &piece,
    const tripoint &pos,
    om_direction::type dir
)
{
    const int num_ters = static_cast<int>( piece.terrains.size() );
    for( const omcp_placement &placement : piece.placements ) {
        bool placement_ok = true;
        if( piece.is_linear ) {
            const omcp_location &location = placement.locations.front();
            const oter_id &ter = om.ter( tripoint_om_omt( pos ) );
            if( !location.loc->test( ter ) ) {
                placement_ok = false;
            }
        } else {
            for( int i = 0; i < num_ters; i++ ) {
                const omcp_location &location = placement.locations[i];
                const omcp_terrain &terrain = piece.terrains[i];
                tripoint_om_omt ter_pos( pos + om_direction::rotate( terrain.pos, dir ) );
                const oter_id &ter = om.ter( ter_pos );
                if( !location.loc->test( ter ) ) {
                    placement_ok = false;
                    break;
                }
            }
        }
        if( placement_ok ) {
            return placement.basic_cost;
        }
    }
    return cata::nullopt;
}

static omcp_connection_exit conn_exit_rotated( const omcp_connection_exit &exit,
        om_direction::type dir )
{
    omcp_connection_exit ret;
    ret.dir = om_direction::add( exit.dir, dir );
    ret.pos = om_direction::rotate( exit.pos, dir );
    // Copying ret.conn_type may cost a string allocation, but it's not used down the line, so skip it
    return ret;
}

static std::vector<piece_link> resolve_candidates(
    const om_connection_piece &piece,
    om_direction::type piece_dir,
    const overmap_connection &connection
)
{
    std::vector<piece_link> ret;

    const int num_pieces = static_cast<int>( connection.pieces.size() );
    const int num_piece_conns = static_cast<int>( piece.connections.size() );
    for( int candidate_idx = 0; candidate_idx < num_pieces; candidate_idx++ ) {
        const om_connection_piece &candidate = connection.pieces[candidate_idx].obj();
        const int num_cand_conns = static_cast<int>( candidate.connections.size() );
        for( int piece_conn_idx = 0; piece_conn_idx < num_piece_conns; piece_conn_idx++ ) {
            for( int cand_conn_idx = 0; cand_conn_idx < num_cand_conns; cand_conn_idx++ ) {
                omcp_connection piece_conn = piece.connections[piece_conn_idx];
                omcp_connection cand_conn = candidate.connections[cand_conn_idx];
                for( const omcp_connection_exit &piece_exit_base : piece_conn.exits ) {
                    for( const omcp_connection_exit &cand_exit_base : cand_conn.exits ) {
                        if( piece_exit_base.conn_type != cand_exit_base.conn_type ) {
                            // Exit type mismatch
                            continue;
                        }
                        omcp_connection_exit piece_exit = conn_exit_rotated( piece_exit_base, piece_dir );
                        for( om_direction::type cand_dir : candidate.allowed_rotations ) {
                            omcp_connection_exit cand_exit = conn_exit_rotated( cand_exit_base, cand_dir );
                            if( piece_exit.dir != om_direction::opposite( cand_exit.dir ) ) {
                                // Exit direction mismatch
                                continue;
                            }
                            tripoint delta_pos_piece = piece_exit.pos + om_direction::rotate( point_north, piece_exit.dir );
                            tripoint delta_pos_cand = -cand_exit.pos;
                            tripoint delta_total = delta_pos_piece + delta_pos_cand;

                            if( delta_total == tripoint_zero ) {
                                // Can't overwrite self
                                continue;
                            }

                            piece_link link;
                            link.src_conn_idx = piece_conn_idx;
                            link.tgt_conn_idx = cand_conn_idx;
                            link.tgt_piece_idx = candidate_idx;
                            link.tgt_dir = cand_dir;
                            link.tgt_pos = delta_total;
                            ret.push_back( std::move( link ) );
                        }
                    }
                }
            }
        }
    }

    return ret;
}

static void iter_matching_candidates(
    const std::vector<piece_link> &candidates,
    const tripoint &pos,
    const piece_placements &placements,
    std::function < void( piece_link && ) > callback
)
{
    for( const piece_link &candidate : candidates ) {
        tripoint cand_pos = pos + candidate.tgt_pos;
        if( !overmap::inbounds( tripoint_om_omt( cand_pos ) ) ) {
            // Can't place piece outside map boundaries
            continue;
        }
        const single_piece_placement &opp =
            placements.get( candidate.tgt_piece_idx, cand_pos.xy(), candidate.tgt_dir );
        if( !opp.is_valid() ) {
            // Can't place piece there due to other reasons (bad location)
            continue;
        }

        piece_link link = candidate; // Make a copy here
        link.tgt_pos += pos; // Convert relative pos to absolute within overmap
        callback( std::move( link ) );
    }
}

static std::unique_ptr<piece_placements> gen_piece_placements(
    const overmap &om,
    const overmap_connection &connection,
    const int zlev
)
{
    const int num_pieces = static_cast<int>( connection.pieces.size() );
    std::unique_ptr<piece_placements> ret = std::make_unique<piece_placements>( num_pieces );

    tripoint pos;
    pos.z = zlev;

    // Find where it's possible to place the pieces
    for( int piece_idx = 0; piece_idx < num_pieces; piece_idx++ ) {
        const om_connection_piece &piece = connection.pieces[piece_idx].obj();
        for( pos.x = 0; pos.x < OMAPX; pos.x++ ) {
            for( pos.y = 0; pos.y < OMAPY; pos.y++ ) {
                for( om_direction::type dir : piece.allowed_rotations ) {
                    const cata::optional<int> place_cost = test_placement_at( om, piece, pos, dir );
                    ret->get( piece_idx, pos.xy(), dir ).cost = place_cost ? *place_cost : -1;
                }
            }
        }
    }

    // Find possible connections between pieces
    for( int piece_idx = 0; piece_idx < num_pieces; piece_idx++ ) {
        const om_connection_piece &piece = connection.pieces[piece_idx].obj();

        for( om_direction::type dir : om_direction::all ) {
            std::vector<piece_link> candidates = resolve_candidates( piece, dir, connection );

            if( candidates.empty() ) {
                // Shortcut
                continue;
            }

            for( pos.x = 0; pos.x < OMAPX; pos.x++ ) {
                for( pos.y = 0; pos.y < OMAPY; pos.y++ ) {
                    single_piece_placement &spp = ret->get( piece_idx, pos.xy(), dir );
                    iter_matching_candidates( candidates, pos, *ret, [&]( piece_link && link ) {
                        spp.links.push_back( std::move( link ) );
                    } );
                }
            }
        }
    }

    return ret;
}

std::vector<overmap_generation::ConnNode>
find_matching_nodes(
    const overmap_connection &connection,
    const piece_placements &placements,
    const point &exit_pos,
    om_direction::type exit_dir
)
{
    if( exit_dir == om_direction::type::invalid ) {
        // TODO: get rid of this
        std::abort();
    }
    // Act like we're a piece with single connection

    omcp_connection_exit pseudo_exit;
    pseudo_exit.conn_type = connection.default_exit_type;
    pseudo_exit.dir = om_direction::opposite( exit_dir );
    pseudo_exit.pos = tripoint_zero;

    omcp_connection pseudo_conn;
    pseudo_conn.exits.push_back( std::move( pseudo_exit ) );

    om_connection_piece pseudo_piece;
    pseudo_piece.is_linear = false;
    pseudo_piece.connections.push_back( std::move( pseudo_conn ) );
    pseudo_piece.allowed_rotations.push_back( om_direction::type::north );

    std::vector<piece_link> candidates = resolve_candidates( pseudo_piece, exit_dir, connection );

    const tripoint exit_pseudo_pos( exit_pos + om_direction::rotate( point_north, exit_dir ), 0 );

    std::vector<overmap_generation::ConnNode> ret;

    iter_matching_candidates( candidates, exit_pseudo_pos, placements, [&]( piece_link && link ) {
        overmap_generation::ConnNode n;
        n.pos = tripoint_om_omt( link.tgt_pos );
        n.piece = connection.pieces[link.tgt_piece_idx];
        n.rot = link.tgt_dir;
        n.conn_idx = link.tgt_conn_idx;
        ret.push_back( std::move( n ) );
    } );

    return ret;
}

static void debug_print_node( const overmap_generation::ConnNode &node )
{
    std::cout << string_format( "  pos:%s  piece:%s  dir:%s  conn:%d\n",
                                node.piece,
                                node.pos.to_string(),
                                om_direction::name( node.rot ),
                                node.conn_idx
                              );
}

static overmap_generation::ConnPath
find_path_a_star(
    const std::vector<overmap_generation::ConnNode> &start_nodes,
    const std::vector<overmap_generation::ConnNode> &end_nodes,
    const piece_placements &placements,
    const overmap_connection &connection
)
{
    if( true ) {
        std::cout << "START_NODES:\n";
        for( const overmap_generation::ConnNode &node : start_nodes ) {
            debug_print_node( node );
        }
        std::cout << "\n";

        std::cout << "END_NODES:\n";
        for( const overmap_generation::ConnNode &node : end_nodes ) {
            debug_print_node( node );
        }
        std::cout << "\n";

        std::cout << "\nPLACEMENT_CACHE:\n";
        const int num_pieces = static_cast<int>( connection.pieces.size() );
        point pos;
        for( pos.x = 0; pos.x < 12; pos.x++ ) {
            for( pos.y = 0; pos.y < 13; pos.y++ ) {
                std::cout << pos.to_string() << ":\n";
                for( int piece_idx = 0; piece_idx < num_pieces; piece_idx++ ) {
                    const om_connection_piece &piece = connection.pieces[piece_idx].obj();
                    for( om_direction::type dir : om_direction::all ) {
                        const single_piece_placement &spp = placements.get( piece_idx, pos, dir );
                        if( !spp.is_valid() ) {
                            continue;
                        }
                        std::cout << string_format( "   piece:%s  dir:%s  cost:%d  links:%d\n",
                                                    piece.id,
                                                    om_direction::name( dir ),
                                                    spp.cost,
                                                    spp.links.size()
                                                  );
                        for( const piece_link &link : spp.links ) {
                            const om_connection_piece &tgt_piece = connection.pieces[link.tgt_piece_idx].obj();
                            std::cout << string_format( "    - link pos:%s dir:%s piece:%s conn:%d src_conn:%d\n",
                                                        link.tgt_pos.to_string(),
                                                        om_direction::name( link.tgt_dir ),
                                                        tgt_piece.id,
                                                        link.tgt_conn_idx,
                                                        link.src_conn_idx
                                                      );
                        }
                    }
                }
            }
        }
        std::cout << "\n";
    }

    // TODO
    return overmap_generation::ConnPath{};
}

overmap_generation::ConnPath
overmap_generation::lay_out_connection(
    const overmap &om,
    const overmap_connection &connection,
    const tripoint_om_omt &source,
    om_direction::type source_dir,
    const tripoint_om_omt &dest,
    om_direction::type dest_dir,
    bool must_be_unexplored
)
{
    if( connection.pieces.empty() ) {
        // Legacy connections
        return ConnPath{};
    }

    if( source.z() != dest.z() ) {
        // TODO: connections between z levels
        return ConnPath{};
    }

    if( !debug_connection_lay ) {
        // TODO: remove this
        return ConnPath{};
    }

    // Possible placements for pieces in the overmap
    std::unique_ptr<piece_placements> placements_container = gen_piece_placements( om, connection,
            source.z() );
    // Alias for ease of use
    piece_placements &placements = *placements_container;

    std::vector<overmap_generation::ConnNode> start_nodes =
        find_matching_nodes( connection, placements, source.raw().xy(), source_dir );
    std::vector<overmap_generation::ConnNode> end_nodes =
        find_matching_nodes( connection, placements, dest.raw().xy(), dest_dir );

    // Find a path from any start node to any end node
    return find_path_a_star( start_nodes, end_nodes, placements, connection );
}

overmap_generation::ConnPath
straight_path( const tripoint_om_omt &source, om_direction::type dir, int len )
{
    overmap_generation::ConnPath res;
    if( len == 0 ) {
        return res;
    }
    tripoint_om_omt p = source;
    res.nodes.reserve( len );
    for( int i = 0; i + 1 < len; i++ ) {
        res.nodes.emplace_back( p );
        p += om_direction::displace( dir );
    }
    res.nodes.emplace_back( p );
    return res;
}

overmap_generation::ConnPath
overmap_generation::lay_out_street(
    const overmap &om,
    const overmap_connection &connection,
    const tripoint_om_omt &from,
    om_direction::type dir,
    int len
)
{
    // See if we need to make another one "step" further.
    const tripoint_om_omt en_pos = from + om_direction::displace( dir, len + 1 );
    if( overmap::inbounds( en_pos, 1 ) && connection.has( om.ter( en_pos ) ) ) {
        len++;
    }

    size_t actual_len = 0;

    while( actual_len < len ) {
        const tripoint_om_omt pos = from + om_direction::displace( dir, actual_len );

        if( !overmap::inbounds( pos, 1 ) ) {
            break;  // Don't approach overmap bounds.
        }

        const oter_id &ter_id = om.ter( pos );

        if( ter_id->is_river() || !connection.pick_subtype_for( ter_id ) ) {
            break;
        }

        bool collided = false;
        int collisions = 0;
        for( int i = -1; i <= 1; i++ ) {
            if( collided ) {
                break;
            }
            for( int j = -1; j <= 1; j++ ) {
                const tripoint_om_omt checkp = pos + tripoint( i, j, 0 );

                if( checkp != pos + om_direction::displace( dir, 1 ) &&
                    checkp != pos + om_direction::displace( om_direction::opposite( dir ), 1 ) &&
                    checkp != pos ) {
                    if( is_ot_match( "road", om.ter( checkp ), ot_match_type::type ) ) {
                        collisions++;
                    }
                }
            }

            //Stop roads from running right next to eachother
            if( collisions >= 3 ) {
                collided = true;
                break;
            }
        }
        if( collided ) {
            break;
        }

        actual_len++;

        if( actual_len > 1 && connection.has( ter_id ) ) {
            break;  // Stop here.
        }
    }

    return straight_path( from, dir, actual_len );
}

void overmap_generation::build_connection(
    overmap &om,
    const overmap_connection &connection,
    const ConnPath &path
)
{
    // TODO
}

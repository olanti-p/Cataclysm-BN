#include "overmap_generation.h"

#include <queue>

#include "hash_utils.h"
#include "om_lines.h"
#include "overmap_connection.h"
#include "overmap_location.h"
#include "overmap.h"

static bool debug_connection_lay = false;

void overmap_generation::set_debug_output( bool val )
{
    debug_connection_lay = val;
}

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
static cata::optional<int> test_can_place_at(
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

static bool test_already_placed_at(
    const overmap &om,
    const om_connection_piece &piece,
    const tripoint &pos,
    om_direction::type dir
)
{
    const int num_ters = static_cast<int>( piece.terrains.size() );

    if( piece.is_linear ) {
        const oter_id &ter = om.ter( tripoint_om_omt( pos ) );
        if( ter->get_type_id() != piece.linear_terrain ) {
            return false;
        }
    } else {
        for( int i = 0; i < num_ters; i++ ) {
            const omcp_terrain &terrain = piece.terrains[i];
            tripoint_om_omt ter_pos( pos + om_direction::rotate( terrain.pos, dir ) );
            oter_id desired_oter = terrain.terrain->get_rotated( dir );
            const oter_id &ter = om.ter( ter_pos );
            if( ter != desired_oter ) {
                return false;
            }
        }
    }

    return true;
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

    // Find where it's possible to place the pieces, or where pieces are already placed
    for( int piece_idx = 0; piece_idx < num_pieces; piece_idx++ ) {
        const om_connection_piece &piece = connection.pieces[piece_idx].obj();
        for( pos.x = 0; pos.x < OMAPX; pos.x++ ) {
            for( pos.y = 0; pos.y < OMAPY; pos.y++ ) {
                for( om_direction::type dir : piece.allowed_rotations ) {
                    cata::optional<int> place_cost;
                    if( test_already_placed_at( om, piece, pos, dir ) ) {
                        place_cost = 1;
                    }
                    if( !place_cost ) {
                        place_cost = test_can_place_at( om, piece, pos, dir );
                    }
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

struct pfnode {
    point pos;
    int8_t piece_idx = -1;
    om_direction::type rot;
    int8_t conn_idx = -1;
};

static bool operator==( const pfnode &l, const pfnode &r )
{
    return l.pos == r.pos && l.piece_idx == r.piece_idx && l.rot == r.rot && l.conn_idx == r.conn_idx;
}

static bool operator<( const pfnode &l, const pfnode &r )
{
    if( l.pos < r.pos ) {
        return true;
    } else if( l.pos != r.pos ) {
        return false;
    }
    if( l.piece_idx < r.piece_idx ) {
        return true;
    } else if( l.piece_idx != r.piece_idx ) {
        return false;
    }
    if( l.rot < r.rot ) {
        return true;
    } else if( l.rot != r.rot ) {
        return false;
    }
    return l.conn_idx < r.conn_idx;
}

template<>
struct std::hash<pfnode> {
    std::size_t operator()( const pfnode &n ) const noexcept {
        std::size_t seed = 0xD7E9FC1AB649C981; // Random number
        cata::hash_combine( seed, n.pos );
        cata::hash_combine( seed, n.rot );
        cata::hash_combine( seed, n.piece_idx );
        cata::hash_combine( seed, n.conn_idx );
        return seed;
    }
};

std::vector<pfnode>
static find_matching_nodes(
    const overmap_connection &connection,
    const piece_placements &placements,
    const point &exit_pos,
    om_direction::type exit_dir
)
{
    if( exit_dir == om_direction::type::invalid ) {
        for( om_direction::type dir : om_direction::all ) {
            const single_piece_placement &spp =
                placements.get( connection.default_piece_idx, exit_pos, dir );
            if( spp.is_valid() ) {
                // TODO: properly implement this
                pfnode n;
                n.piece_idx = connection.default_piece_idx;
                n.pos = exit_pos;
                n.rot = dir;
                n.conn_idx = 0;

                return {{ n }};
            }
        }
        // Can't place default node there
        return {};
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

    std::vector<piece_link> candidates =
        resolve_candidates( pseudo_piece, om_direction::type::north, connection );

    const tripoint exit_pseudo_pos( exit_pos + om_direction::rotate( point_north, exit_dir ), 0 );

    std::vector<pfnode> ret;

    iter_matching_candidates( candidates, exit_pseudo_pos, placements, [&]( piece_link && link ) {
        pfnode n;
        n.pos = link.tgt_pos.xy();
        n.piece_idx = link.tgt_piece_idx;
        n.rot = link.tgt_dir;
        n.conn_idx = link.tgt_conn_idx;
        ret.push_back( std::move( n ) );
    } );

    return ret;
}

static void debug_print_node( const pfnode &node, const overmap_connection &connection )
{
    std::cout << string_format( "  pos:%s  piece:%s  dir:%s  conn:%d  HASH:%ud\n",
                                node.pos.to_string(),
                                connection.pieces[node.piece_idx],
                                om_direction::name( node.rot ),
                                node.conn_idx,
                                std::hash<pfnode> {}( node )
                              );
}

static std::vector<pfnode>
find_path_breadth_first(
    const std::vector<pfnode> &start_nodes,
    const std::vector<pfnode> &end_nodes,
    const piece_placements &placements,
    const overmap_connection &connection
)
{
    // TODO: all start nodes must be viable
    pfnode start = start_nodes[0];

    // TODO: all end nodes must be viable
    pfnode goal = end_nodes[0];

    std::queue<pfnode> frontier;
    frontier.push( start );

    std::unordered_map<pfnode, pfnode> came_from;
    came_from[start] = start;

    int num_iters = 0;
    bool path_found = false;

    while( !frontier.empty() ) {
        num_iters++;

        pfnode current = frontier.front();
        frontier.pop();

        if( current == goal ) {
            path_found = true;
            break;
        }

        if( false ) {
            std::cout << "Visiting  ";
            debug_print_node( current, connection );
        }

        const single_piece_placement &current_pl =
            placements.get( current.piece_idx, current.pos, current.rot );
        for( const piece_link &link : current_pl.links ) {
            if( link.src_conn_idx != current.conn_idx ) {
                // Can't connect from this connection
                continue;
            }
            pfnode next;
            next.pos = link.tgt_pos.xy();
            next.conn_idx = link.tgt_conn_idx;
            next.rot = link.tgt_dir;
            next.piece_idx = link.tgt_piece_idx;

            if( came_from.find( next ) == came_from.end() ) {
                frontier.push( next );
                came_from[next] = current;
            }
        }
    }

    std::vector<pfnode> ret;

    if( path_found ) {
        pfnode cursor = goal;
        while( true ) {
            pfnode prev = came_from[cursor];
            ret.push_back( cursor );
            if( prev == cursor ) {
                break;
            }
            cursor = prev;
        }
    }

    std::cout << string_format( "Path finding done in %d iterations.\n", num_iters );

    return ret;
}

template<typename T, typename Priority>
struct priority_queue {
    private:
        typedef std::pair<Priority, T> Elem;
        std::priority_queue<Elem, std::vector<Elem>, std::greater<Elem>> elements;

    public:
        inline bool empty() {
            return elements.empty();
        }

        inline void put( T &&item, Priority priority ) {
            elements.emplace( priority, std::move( item ) );
        }

        T get() {
            T best_item = elements.top().second;
            elements.pop();
            return best_item;
        }
};

static std::vector<pfnode>
find_path_dijkstra(
    const std::vector<pfnode> &start_nodes,
    const std::vector<pfnode> &end_nodes,
    const piece_placements &placements,
    const overmap_connection &connection
)
{
    // TODO: all start nodes must be viable
    pfnode start = start_nodes[0];

    // TODO: all end nodes must be viable
    pfnode goal = end_nodes[0];

    priority_queue<pfnode, int> frontier;
    frontier.put( pfnode( start ), 0 );

    std::unordered_map<pfnode, pfnode> came_from;
    came_from[start] = start;

    std::unordered_map<pfnode, int> cost_so_far;
    cost_so_far[start] = 0;

    int num_iters = 0;
    bool path_found = false;

    while( !frontier.empty() ) {
        num_iters++;

        pfnode current = frontier.get();

        if( current == goal ) {
            path_found = true;
            break;
        }

        if( false ) {
            std::cout << "Visiting  ";
            debug_print_node( current, connection );
        }

        const single_piece_placement &current_pl =
            placements.get( current.piece_idx, current.pos, current.rot );
        int cost_this = cost_so_far[current];
        for( const piece_link &link : current_pl.links ) {
            if( link.src_conn_idx != current.conn_idx ) {
                // Can't connect from this connection
                continue;
            }

            int new_cost = cost_this + placements.get( link.tgt_piece_idx, link.tgt_pos.xy(),
                           link.tgt_dir ).cost;

            pfnode next;
            next.pos = link.tgt_pos.xy();
            next.conn_idx = link.tgt_conn_idx;
            next.rot = link.tgt_dir;
            next.piece_idx = link.tgt_piece_idx;

            if( cost_so_far.find( next ) == cost_so_far.end() || new_cost < cost_so_far[next] ) {
                cost_so_far[next] = new_cost;
                came_from[next] = current;
                frontier.put( std::move( next ), new_cost );
            }
        }
    }

    std::vector<pfnode> ret;

    if( path_found ) {
        pfnode cursor = goal;
        while( true ) {
            pfnode prev = came_from[cursor];
            ret.push_back( cursor );
            if( prev == cursor ) {
                break;
            }
            cursor = prev;
        }
    }

    std::cout << string_format( "Path finding done in %d iterations.\n", num_iters );

    return ret;
}

overmap_generation::ConnPath
overmap_generation::lay_out_connection(
    const overmap &om,
    const overmap_connection &connection,
    const tripoint_om_omt &source,
    om_direction::type source_dir,
    const tripoint_om_omt &dest,
    om_direction::type dest_dir,
    bool /*must_be_unexplored*/
)
{
    ConnPath ret;
    ret.connection = &connection;
    ret.source = source;
    ret.source_dir = source_dir;
    ret.dest = dest;
    ret.dest_dir = dest_dir;

    if( connection.pieces.empty() ) {
        // Legacy connections
        return ret;
    }

    if( source.z() != dest.z() ) {
        // TODO: connections between z levels
        return ret;
    }

    if( !debug_connection_lay ) {
        // TODO: remove this
        return ret;
    }

    std::cout << string_format( "LAYING OUT CONNECTION\nconn: %s\nsrc: %s %s\ndst: %s %s\n\n",
                                connection.id,
                                source.to_string(),
                                om_direction::name( source_dir ),
                                dest.to_string(),
                                om_direction::name( dest_dir )
                              );

    // Possible placements for pieces in the overmap
    std::unique_ptr<piece_placements> placements_container =
        gen_piece_placements( om, connection, source.z() );
    // Alias for ease of use
    piece_placements &placements = *placements_container;

    std::vector<pfnode> start_nodes =
        find_matching_nodes( connection, placements, source.raw().xy(), source_dir );
    std::vector<pfnode> end_nodes =
        find_matching_nodes( connection, placements, dest.raw().xy(), dest_dir );

    std::vector<pfnode> nodes;

    // Shortcut: either start or exit is blocked
    bool found_cheap_path = false;
    if( start_nodes.empty() || end_nodes.empty() ) {
        found_cheap_path = true;
    } else {
        // Shortcut: one of the nodes is both start and end node
        // TODO: decide which one is cheaper
        for( const pfnode &snode : start_nodes ) {
            for( const pfnode &enode : end_nodes ) {
                if( snode == enode ) {
                    nodes.push_back( snode );
                    found_cheap_path = true;
                    break;
                }
            }
            if( found_cheap_path ) {
                break;
            }
        }
    }

    if( true ) {
        std::cout << "START_NODES:\n";
        for( const pfnode &node : start_nodes ) {
            debug_print_node( node, connection );
        }
        std::cout << "\n";

        std::cout << "END_NODES:\n";
        for( const pfnode &node : end_nodes ) {
            debug_print_node( node, connection );
        }
        std::cout << "\n";

        if( false ) {
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
    }

    if( !found_cheap_path ) {
        // Find a path from any start node to any end node
        if( true ) {
            nodes = find_path_dijkstra( start_nodes, end_nodes, placements,
                                        connection );
        } else {
            nodes = find_path_breadth_first( start_nodes, end_nodes, placements,
                                             connection );
        }
    }

    std::reverse( nodes.begin(), nodes.end() );

    std::cout << "FINAL_PATH:\n";
    for( const pfnode &node : nodes ) {
        debug_print_node( node, connection );

        overmap_generation::ConnNode n;
        n.conn_idx = node.conn_idx;
        n.piece = connection.pieces[node.piece_idx];
        n.pos = tripoint_om_omt( node.pos.x, node.pos.y, source.z() );
        n.rot = node.rot;

        ret.nodes.push_back( std::move( n ) );
    }
    std::cout << "\n";

    return ret;
}

overmap_generation::ConnPath
static straight_path(
    const overmap &om,
    const overmap_connection &connection,
    const tripoint_om_omt &source,
    om_direction::type dir,
    int len
)
{
    overmap_generation::ConnPath res;
    res.connection = &connection;
    res.source = source;
    res.source_dir = om_direction::type::invalid;
    res.dest_dir = om_direction::type::invalid;
    res.dest = source + om_direction::displace( dir ) * len;
    if( len == 0 ) {
        return res;
    }
    res.nodes.reserve( len );
    for( int i = 0; i < len; i++ ) {
        tripoint_om_omt p = source + om_direction::displace( dir ) * i;
        const oter_id &ter = om.ter( p );

        overmap_generation::ConnNode node;
        node.pos = p;
        node.rot = om_direction::type::north;
        node.conn_idx = 0;
        node.piece = connection.pick_linear_piece_for( ter )->id;

        res.nodes.emplace_back( std::move( node ) );
    }
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
    std::cout << string_format( "laying street %s -> %d %s\n", from.to_string(), len,
                                om_direction::name( dir ) );

    // See if we need to make another one "step" further.
    const tripoint_om_omt en_pos = from + om_direction::displace( dir, len + 1 );
    if( overmap::inbounds( en_pos, 1 ) && connection.has_linear_piece( om.ter( en_pos ) ) ) {
        len++;
    }

    int actual_len = 0;

    while( actual_len < len ) {
        const tripoint_om_omt pos = from + om_direction::displace( dir, actual_len );
        std::cout << string_format( "  scanning %s ", pos.to_string() );

        if( !overmap::inbounds( pos, 1 ) ) {
            std::cout << "too close to bounds.\n";
            break;  // Don't approach overmap bounds.
        }

        const oter_id &ter_id = om.ter( pos );
        std::cout << string_format( "%s ", ter_id.id() );

        if( !connection.pick_linear_piece_for( ter_id ) ) {
            std::cout << "no linear piece for terrain.\n";
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
                    if( connection.has_linear_piece( om.ter( checkp ) ) ) {
                        collisions++;
                    }
                }
            }

            // Stop roads from running right next to eachother
            if( collisions >= 3 ) {
                collided = true;
                break;
            }
        }
        if( collided ) {
            std::cout << "too many nearby streets.\n";
            break;
        }

        actual_len++;
        std::cout << "ok\n";

        if( actual_len > 1 && connection.has_linear_piece( ter_id ) ) {
            std::cout << "  collides with existing street, ending here.\n";
            break; // Stop here.
        }
    }

    std::cout << string_format( "deferring to straight path, len:%d actual_len:%d\n", len, actual_len );

    auto ret = straight_path( om, connection, from, dir, actual_len );

    std::cout << string_format( "done laying street, %d steps.\n", ret.nodes.size() );

    return ret;
}

static std::vector<om_direction::type> find_exits_for_linear(
    const overmap_generation::ConnNode &node,
    const overmap_generation::ConnNode &other
)
{
    std::vector<om_direction::type> ret;

    tripoint_om_omt node_pos = node.pos;
    const std::string &exit_type = node.piece->linear_conn_type;

    const om_connection_piece &piece = other.piece.obj();
    const omcp_connection &piece_conn = piece.connections[other.conn_idx];
    for( const omcp_connection_exit &exit_base : piece_conn.exits ) {
        if( exit_base.conn_type != exit_type ) {
            // Exit type mismatch
            continue;
        }
        omcp_connection_exit exit = conn_exit_rotated( exit_base, other.rot );
        tripoint_om_omt exit_on_map( other.pos + exit.pos );
        tripoint_om_omt exit_to = exit_on_map + om_direction::displace( exit.dir );
        if( node_pos == exit_to ) {
            ret.push_back( om_direction::opposite( exit.dir ) );
        }
    }

    return ret;
}

void overmap_generation::build_connection(
    overmap &om,
    const ConnPath &path
)
{
    if( !debug_connection_lay ) {
        // TODO: remove this
        return;
    }

    std::cout << string_format( "building connection, %d steps\n", path.nodes.size() );

    const size_t line_none = 0;

    for( size_t node_idx = 0; node_idx < path.nodes.size(); node_idx++ ) {
        const ConnNode &node = path.nodes[node_idx];
        const ConnNode *node_prev = node_idx > 0 ? &path.nodes[node_idx - 1] : nullptr;
        const ConnNode *node_next = node_idx < path.nodes.size() - 1 ? &path.nodes[node_idx + 1] : nullptr;
        const om_connection_piece &piece = node.piece.obj();

        std::cout << string_format( "  placing piece %s at %s\n", node.piece, node.pos.to_string() );

        if( !piece.is_linear ) {
            for( const omcp_terrain &ter : piece.terrains ) {
                tripoint_om_omt ter_pos =
                    tripoint_om_omt( om_direction::rotate( ter.pos, node.rot ) + node.pos.raw() );
                oter_id tid = ter.terrain->get_rotated( node.rot );
                om.ter_set( ter_pos, tid );
            }
        } else {
            // TODO: connect to nearby unconnected roads
            size_t line = line_none;
            if( om.ter( node.pos )->get_type_id() == piece.linear_terrain ) {
                line = om.ter( node.pos )->get_line();
            }
            if( node_prev ) {
                if( node_prev->piece->is_linear ) {
                    std::cout << "    has linear node_prev\n";
                    point v = node_prev->pos.raw().xy() - node.pos.raw().xy();
                    line = om_lines::set_segment( line, om_direction::from_vec( v ) );
                } else {
                    std::vector<om_direction::type> dirs = find_exits_for_linear( node, *node_prev );
                    for( om_direction::type dir : dirs ) {
                        line = om_lines::set_segment( line, dir );
                    }
                }
            } else if( path.source_dir != om_direction::type::invalid ) {
                line = om_lines::set_segment( line, path.source_dir );
            }
            if( node_next ) {
                if( node_next->piece->is_linear ) {
                    std::cout << "    has linear node_next\n";
                    point v = node_next->pos.raw().xy() - node.pos.raw().xy();
                    line = om_lines::set_segment( line, om_direction::from_vec( v ) );
                } else {
                    std::vector<om_direction::type> dirs = find_exits_for_linear( node, *node_next );
                    for( om_direction::type dir : dirs ) {
                        line = om_lines::set_segment( line, dir );
                    }
                }
            } else if( path.dest_dir != om_direction::type::invalid ) {
                line = om_lines::set_segment( line, path.dest_dir );
            }
            oter_id tid = piece.linear_terrain->get_linear( line );

            om.ter_set( node.pos, tid );
        }
    }

    std::cout << "done building connection.\n";
}

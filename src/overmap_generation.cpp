#include "overmap_generation.h"

#include "overmap.h"
#include "overmap_connection.h"

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


    // TODO
    return ConnPath{};
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

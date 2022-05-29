#include "overmap_connection.h"

#include <cstddef>
#include <algorithm>
#include <cassert>
#include <map>
#include <memory>

#include "generic_factory.h"
#include "json.h"
#include "overmap_location.h"
#include "debug.h"

namespace
{

generic_factory<overmap_connection> connections( "overmap connection" );

} // namespace

static const std::map<std::string, overmap_connection::subtype::flag> connection_subtype_flag_map
= {
    { "ORTHOGONAL", overmap_connection::subtype::flag::orthogonal },
};

template<>
bool string_id<overmap_connection>::is_valid() const
{
    return connections.is_valid( *this );
}

template<>
const overmap_connection &string_id<overmap_connection>::obj() const
{
    return connections.obj( *this );
}

template<>
int_id<overmap_connection> string_id<overmap_connection>::id() const
{
    int_id<overmap_connection> null_id( -1 );
    return connections.convert( *this, null_id );
}

template<>
const overmap_connection &int_id<overmap_connection>::obj() const
{
    return connections.obj( *this );
}

bool overmap_connection::subtype::allows_terrain( const int_id<oter_t> &oter ) const
{
    if( oter->type_is( terrain ) ) {
        return true;    // Can be built on similar terrains.
    }

    return std::any_of( locations.cbegin(),
    locations.cend(), [&oter]( const string_id<overmap_location> &elem ) {
        return elem->test( oter );
    } );
}

void overmap_connection::subtype::load( const JsonObject &jo )
{
    const auto flag_reader = make_flag_reader( connection_subtype_flag_map, "connection subtype flag" );

    mandatory( jo, false, "terrain", terrain );
    mandatory( jo, false, "locations", locations );

    optional( jo, false, "basic_cost", basic_cost, 0 );
    optional( jo, false, "flags", flags, flag_reader );
}

void overmap_connection::subtype::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();
    load( jo );
}

const overmap_connection::subtype *overmap_connection::pick_subtype_for(
    const int_id<oter_t> &ground ) const
{
    if( !ground ) {
        return nullptr;
    }

    const size_t cache_index = ground.to_i();
    assert( cache_index < cached_subtypes.size() );

    if( cached_subtypes[cache_index] ) {
        return cached_subtypes[cache_index].value;
    }

    const auto iter = std::find_if( subtypes.cbegin(),
    subtypes.cend(), [&ground]( const subtype & elem ) {
        return elem.allows_terrain( ground );
    } );

    const overmap_connection::subtype *result = iter != subtypes.cend() ? &*iter : nullptr;

    cached_subtypes[cache_index].value = result;
    cached_subtypes[cache_index].assigned = true;

    return result;
}

bool overmap_connection::has( const int_id<oter_t> &oter ) const
{
    return std::find_if( subtypes.cbegin(), subtypes.cend(), [&oter]( const subtype & elem ) {
        return oter->type_is( elem.terrain );
    } ) != subtypes.cend();
}

void overmap_connection::load( const JsonObject &jo, const std::string & )
{
    data_new.id = id;
    mandatory( jo, false, "subtypes", subtypes );
    optional( jo, false, "use_new_method", use_new_method );
    if( use_new_method ) {
        mandatory( jo, false, "data_new", data_new );
    } else {
        optional( jo, false, "data_new", data_new );
    }
}

void om_conn_segment::load( const JsonObject &jo )
{
    mandatory( jo, false, "terrain", terrain );
    optional( jo, false, "n", get_edge_mut( om_direction::type::north ) );
    optional( jo, false, "e", get_edge_mut( om_direction::type::east ) );
    optional( jo, false, "s", get_edge_mut( om_direction::type::south ) );
    optional( jo, false, "w", get_edge_mut( om_direction::type::west ) );
    optional( jo, false, "complexity_cost", complexity_cost );
    optional( jo, false, "rotates", rotates, 1 );
    optional( jo, false, "upgrades", upgrades_str );
    optional( jo, false, "conns", connections_str );
}

void om_conn_segment::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();
    load( jo );
}

const std::vector<int> &
om_conn_segment::get_edge_of_rotated(
    om_direction::type side,
    om_direction::type rot,
    int conn_id
) const
{
    om_direction::type fin = static_cast<om_direction::type>(
                                 om_direction::size - static_cast<size_t>( rot )
                             );
    return connections[conn_id][static_cast<int>( om_direction::add( side, fin ) )];
}

void om_conn_location::load( const JsonObject &jo )
{
    mandatory( jo, false, "id", id );
    mandatory( jo, false, "basic_cost", basic_cost );
}

void om_conn_location::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();
    load( jo );
}

void om_conn_placement::load( const JsonObject &jo )
{
    mandatory( jo, false, "locations", locations );
    mandatory( jo, false, "segments", segments_str );
}

void om_conn_placement::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();
    load( jo );
}

void om_connection_new::load( const JsonObject &jo )
{
    mandatory( jo, false, "segments", segments );
    mandatory( jo, false, "placement", placements );
    mandatory( jo, false, "default_segment", default_segment_str );
    optional( jo, false, "follow_cost", follow_cost );
}

void om_connection_new::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();
    load( jo );
}

void om_connection_new::check() const
{
    if( !default_segment_str.is_valid() ) {
        debugmsg( R"(In overmap connection "%s", default segment "%s" is invalid.)",
                  id, default_segment_str );
    }
    for( const om_conn_segment &it : segments ) {
        if( it.rotates != 1 &&
            it.rotates != 2 &&
            it.rotates != 4 ) {
            debugmsg( R"(In overmap connection "%s", rotates value "%d" is invalid.)", id, it.rotates );
        }
        if( !it.terrain.is_valid() ) {
            debugmsg( R"(In overmap connection "%s", segment terrain "%s" is invalid.)", id, it.terrain );
        }
        for( const auto &up : it.upgrades_str ) {
            if( !up.is_valid() ) {
                debugmsg( R"(In overmap connection "%s", segment upgrade "%s" is invalid.)", id, up );
            }
        }
    }
    for( const om_conn_placement &it : placements ) {
        for( const auto &loc : it.locations ) {
            if( !loc.id.is_valid() ) {
                debugmsg( R"(In overmap connection "%s", placement location "%s" is invalid.)", id, loc.id );
            }
        }
        for( const auto &seg : it.segments_str ) {
            if( !seg.is_valid() ) {
                debugmsg( R"(In overmap connection "%s", placement segment "%s" is invalid.)", id, seg );
            }
        }
    }
}

void om_connection_new::finalize()
{
    default_segment = find_segment_by_terr( default_segment_str );
    for( om_conn_placement &it_pl : placements ) {
        it_pl.segments.reserve( it_pl.segments_str.size() );
        for( const auto &it : it_pl.segments_str ) {
            it_pl.segments.push_back( find_segment_by_terr( it ) );
        }
    }
    for( om_conn_segment &it_seg : segments ) {
        it_seg.upgrades.reserve( it_seg.upgrades_str.size() );
        for( const auto &it : it_seg.upgrades_str ) {
            it_seg.upgrades.push_back( find_segment_by_terr( it ) );
        }
        it_seg.connections.reserve( it_seg.connections_str.size() );
        for( const std::string &conn_str : it_seg.connections_str ) {
            it_seg.connections.emplace_back();
            std::array<std::vector<int>, 4> &sides = it_seg.connections.back();
            for( char c : conn_str ) {
                om_direction::type dir = om_direction::type::invalid;
                switch( c ) {
                    case 'n': {
                        dir = om_direction::type::north;
                        break;
                    }
                    case 'e': {
                        dir = om_direction::type::east;
                        break;
                    }
                    case 's': {
                        dir = om_direction::type::south;
                        break;
                    }
                    case 'w': {
                        dir = om_direction::type::west;
                        break;
                    }
                    default: {
                        debugmsg( R"(In overmap connection "%s", connection side '%c' is invalid.)", id, c );
                        continue;
                    }
                }
                std::vector<int> &side_conn = sides[static_cast<int>( dir )];
                const std::vector<std::string> &side = it_seg.edges[static_cast<int>( dir )];
                side_conn.reserve( side.size() );
                for( const std::string &edge_s : side ) {
                    int edge_idx = -1;
                    auto it = edge_string_hash.find( edge_s );
                    if( it == edge_string_hash.end() ) {
                        edge_idx = static_cast<int>( edge_string_hash.size() );
                        edge_string_hash[edge_s] = edge_idx;
                    } else {
                        edge_idx = it->second;
                    }
                    side_conn.push_back( edge_idx );
                }
            }
        }
    }
}

int om_connection_new::find_segment_by_terr( const oter_type_str_id &seg ) const
{
    for( size_t i = 0; i < segments.size(); i++ ) {
        if( segments[i].terrain == seg ) {
            return static_cast<int>( i );
        }
    }
    return -1;
}

const std::vector<int> &
om_connection_new::find_candidate_segments( const oter_id &t ) const
{
    for( const om_conn_placement &it_pl : placements ) {
        for( const om_conn_location &it_loc : it_pl.locations ) {
            if( it_loc.id->test( t ) ) {
                return it_pl.segments;
            }
        }
    }
    static const std::vector<int> no_segments;
    return no_segments;
}

float om_connection_new::get_terrain_cost( const oter_id &t ) const
{
    for( const om_conn_placement &it_pl : placements ) {
        for( const om_conn_location &it_loc : it_pl.locations ) {
            if( it_loc.id->test( t ) ) {
                return it_loc.basic_cost;
            }
        }
    }
    return 0;
}

bool test_segment_connectivity(
    const std::vector<int> &edge_src,
    const std::vector<int> &edge_dest
)
{
    return std::find_first_of(
               edge_src.cbegin(), edge_src.cend(),
               edge_dest.cbegin(), edge_dest.cend()
           ) != edge_src.cend();
}

void overmap_connection::check() const
{
    if( subtypes.empty() ) {
        debugmsg( "Overmap connection \"%s\" doesn't have subtypes.", id.c_str() );
    }
    for( const auto &subtype : subtypes ) {
        if( !subtype.terrain.is_valid() ) {
            debugmsg( "In overmap connection \"%s\", terrain \"%s\" is invalid.", id.c_str(),
                      subtype.terrain.c_str() );
        }
        for( const auto &location : subtype.locations ) {
            if( !location.is_valid() ) {
                debugmsg( "In overmap connection \"%s\", location \"%s\" is invalid.", id.c_str(),
                          location.c_str() );
            }
        }
    }
    if( use_new_method ) {
        data_new.check();
    }
}

void overmap_connection::finalize()
{
    cached_subtypes.resize( overmap_terrains::get_all().size() );
    if( use_new_method ) {
        data_new.finalize();
    }
}

void overmap_connections::load( const JsonObject &jo, const std::string &src )
{
    connections.load( jo, src );
}

void overmap_connections::finalize()
{
    connections.finalize();
    for( const auto &elem : connections.get_all() ) {
        const_cast<overmap_connection &>( elem ).finalize(); // This cast is ugly, but safe.
    }
}

void overmap_connections::check_consistency()
{
    connections.check();
}

void overmap_connections::reset()
{
    connections.reset();
}

const std::vector<overmap_connection> &overmap_connections::get_all()
{
    return connections.get_all();
}

string_id<overmap_connection> overmap_connections::guess_for( const int_id<oter_t> &oter_id )
{
    const auto &all = connections.get_all();
    const auto iter = std::find_if( all.cbegin(),
    all.cend(), [&oter_id]( const overmap_connection & elem ) {
        return elem.pick_subtype_for( oter_id ) != nullptr;
    } );

    return iter != all.cend() ? iter->id : string_id<overmap_connection>::NULL_ID();
}

string_id<overmap_connection> overmap_connections::guess_for( const int_id<oter_type_t> &oter_id )
{
    return guess_for( oter_id->get_first() );
}

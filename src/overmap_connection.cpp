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

static const std::map<std::string, om_conn_flag> connection_subtype_flag_map
= {
    { "ORTHOGONAL", om_conn_flag::orthogonal },
};

static const std::map<std::string, om_conn_method> om_conn_method_type_map
= {
    { "modular", om_conn_method::modular },
    { "linear", om_conn_method::linear },
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

bool om_conn_subtype::allows_terrain( const oter_id &oter ) const
{
    if( oter->type_is( terrain ) ) {
        return true;    // Can be built on similar terrains.
    }

    return std::any_of( locations.cbegin(),
    locations.cend(), [&oter]( const string_id<overmap_location> &elem ) {
        return elem->test( oter );
    } );
}

void om_conn_subtype::load( const JsonObject &jo )
{
    const auto flag_reader = make_flag_reader( connection_subtype_flag_map, "connection subtype flag" );

    mandatory( jo, false, "terrain", terrain );
    mandatory( jo, false, "locations", locations );

    optional( jo, false, "basic_cost", basic_cost, 0 );
    optional( jo, false, "flags", flags, flag_reader );
}

void om_conn_subtype::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();
    load( jo );
}

const om_conn_subtype *om_connection_linear::pick_subtype_for( const oter_id &ground ) const
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
    subtypes.cend(), [&ground]( const om_conn_subtype & elem ) {
        return elem.allows_terrain( ground );
    } );

    const om_conn_subtype *result = iter != subtypes.cend() ? &*iter : nullptr;

    cached_subtypes[cache_index].value = result;
    cached_subtypes[cache_index].assigned = true;

    return result;
}

bool om_connection_linear::has( const oter_id &oter ) const
{
    return std::find_if( subtypes.cbegin(), subtypes.cend(), [&oter]( const om_conn_subtype & elem ) {
        return oter->type_is( elem.terrain );
    } ) != subtypes.cend();
}

void om_connection_linear::load( const JsonObject &jo )
{
    mandatory( jo, false, "subtypes", subtypes );
    mandatory( jo, false, "default_terrain", default_terrain );
}

void overmap_connection::load( const JsonObject &jo, const std::string & )
{
    data_modular.id = id;
    optional( jo, false, "disable_city_hubs", disable_city_hubs );

    const auto method_reader = make_flag_reader( om_conn_method_type_map, "overmap connection method" );

    mandatory( jo, false, "method", method, method_reader );
    if( method == om_conn_method::modular ) {
        data_modular.load( jo );
    } else {
        data_linear.load( jo );
    }
}

void om_conn_upgrade::load( const JsonObject &jo )
{
    mandatory( jo, false, "id", segment_str );
    optional( jo, false, "rot", rot );
}

void om_conn_upgrade::deserialize( JsonIn &jsin )
{
    if( jsin.test_string() ) {
        segment_str = oter_type_str_id( jsin.get_string() );
        rot = om_direction::type::none;
    } else {
        JsonObject jo = jsin.get_object();
        load( jo );
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
    optional( jo, false, "upgrades", upgrades );
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

void om_connection_modular::load( const JsonObject &jo )
{
    mandatory( jo, false, "segments", segments );
    mandatory( jo, false, "placement", placements );
    mandatory( jo, false, "default_segment", default_segment_str );
    mandatory( jo, false, "default_conn", default_conn_str );
    optional( jo, false, "follow_cost", follow_cost );
}

void om_connection_modular::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();
    load( jo );
}

void om_connection_modular::check() const
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
        if( it.connections.empty() ) {
            debugmsg( R"(In overmap connection "%s", segment "%s" has no connections.)", id, it.terrain );
        }
        for( const om_conn_upgrade &up : it.upgrades ) {
            if( !up.segment_str.is_valid() ) {
                debugmsg( R"(In overmap connection "%s", segment upgrade "%s" is invalid.)", id, up.segment );
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

void om_connection_modular::finalize()
{
    default_segment = find_segment_by_terr( default_segment_str );
    for( om_conn_placement &it_pl : placements ) {
        it_pl.segments.reserve( it_pl.segments_str.size() );
        for( const auto &it : it_pl.segments_str ) {
            it_pl.segments.push_back( find_segment_by_terr( it ) );
        }
    }
    for( om_conn_segment &it_seg : segments ) {
        for( auto &up : it_seg.upgrades ) {
            up.segment = find_segment_by_terr( up.segment_str );
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
    auto it = edge_string_hash.find( default_conn_str );
    if( it == edge_string_hash.end() ) {
        debugmsg( R"(In overmap connection "%s", default_conn "%s" is never used by segments.)", id,
                  default_conn_str );
        default_conn = 0;
    } else {
        default_conn = it->second;
    }
}

int om_connection_modular::find_segment_by_terr( const oter_type_str_id &seg ) const
{
    for( size_t i = 0; i < segments.size(); i++ ) {
        if( segments[i].terrain == seg ) {
            return static_cast<int>( i );
        }
    }
    return -1;
}

const std::vector<int> &
om_connection_modular::find_candidate_segments( const oter_id &t ) const
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

float om_connection_modular::get_terrain_cost( const oter_id &t ) const
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

void om_connection_linear::check() const
{
    if( subtypes.empty() ) {
        debugmsg( "Overmap connection \"%s\" doesn't have subtypes.", id );
    }
    for( const auto &subtype : subtypes ) {
        if( !subtype.terrain.is_valid() ) {
            debugmsg( "In overmap connection \"%s\", terrain \"%s\" is invalid.", id, subtype.terrain );
        }
        for( const auto &location : subtype.locations ) {
            if( !location.is_valid() ) {
                debugmsg( "In overmap connection \"%s\", location \"%s\" is invalid.", id, location );
            }
        }
    }
    if( !default_terrain.is_valid() ) {
        debugmsg( "In overmap connection \"%s\", default terrain \"%s\" is invalid.", id, default_terrain );
    }
}

void overmap_connection::check() const
{
    if( method == om_conn_method::modular ) {
        data_modular.check();
    } else {
        data_linear.check();
    }
}

void om_connection_linear::finalize()
{
    cached_subtypes.resize( overmap_terrains::get_all().size() );
}

void overmap_connection::finalize()
{
    if( method == om_conn_method::modular ) {
        data_modular.finalize();
    } else {
        data_linear.finalize();
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

string_id<overmap_connection> overmap_connections::guess_for( const oter_id &oter_id )
{
    const auto &all = connections.get_all();
    const auto iter = std::find_if( all.cbegin(),
    all.cend(), [&oter_id]( const overmap_connection & elem ) {
        if( elem.method == om_conn_method::linear ) {
            return elem.data_linear.pick_subtype_for( oter_id ) != nullptr;
        } else {
            return false;
        }
    } );

    return iter != all.cend() ? iter->id : string_id<overmap_connection>::NULL_ID();
}

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

generic_factory<om_connection_piece> conn_pieces( "overmap connection piece" );
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

template<>
bool string_id<om_connection_piece>::is_valid() const
{
    return conn_pieces.is_valid( *this );
}

template<>
const om_connection_piece &string_id<om_connection_piece>::obj() const
{
    return conn_pieces.obj( *this );
}

bool overmap_connection::subtype::allows_terrain( const oter_id &oter ) const
{
    if( oter->type_is( terrain ) ) {
        return true;    // Can be built on similar terrains.
    }

    return std::any_of( locations.cbegin(),
    locations.cend(), [&oter]( const overmap_location_id & elem ) {
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
    const oter_id &ground ) const
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

bool overmap_connection::has( const oter_id &oter ) const
{
    return std::find_if( subtypes.cbegin(), subtypes.cend(), [&oter]( const subtype & elem ) {
        return oter->type_is( elem.terrain );
    } ) != subtypes.cend();
}

bool overmap_connection::has_linear_piece( const oter_id &t ) const
{
    for( const string_id<om_connection_piece> &piece : pieces ) {
        if( piece->is_linear && piece->linear_terrain == t->get_type_id() ) {
            return true;
        }
    }
    return false;
}

const om_connection_piece *overmap_connection::pick_linear_piece_for( const oter_id &t ) const
{
    for( const string_id<om_connection_piece> &piece : pieces ) {
        if( !piece->is_linear ) {
            continue;
        }
        if( piece->linear_terrain == t->get_type_id() ) {
            return &piece.obj();
        }
        for( const omcp_placement &place : piece->placements ) {
            if( place.locations.front().loc->test( t ) ) {
                return &piece.obj();
            }
        }
    }
    return nullptr;
}

void overmap_connection::load( const JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "default_terrain", default_terrain );
    mandatory( jo, was_loaded, "default_exit_type", default_exit_type );
    optional( jo, was_loaded, "subtypes", subtypes );
    optional( jo, was_loaded, "pieces", pieces );
    optional( jo, was_loaded, "default_piece", default_piece );
    optional( jo, was_loaded, "is_ortho", is_ortho );
}

void overmap_connection::check() const
{
    if( subtypes.empty() && pieces.empty() ) {
        debugmsg( "Overmap connection \"%s\" doesn't have subtypes or pieces.", id.c_str() );
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
    for( const auto &piece : pieces ) {
        if( !piece.is_valid() ) {
            debugmsg( "Overmap connection \"%s\" refers to non-existent piece \"%s\".", id.c_str(), piece );
        }
    }
}

void overmap_connection::finalize()
{
    cached_subtypes.resize( overmap_terrains::get_all().size() );

    if( !pieces.empty() ) {
        if( default_piece.is_empty() ) {
            debugmsg( "Overmap connection \"%s\" must define a default piece.", id );
        } else {
            auto it = std::find( pieces.cbegin(), pieces.cend(), default_piece );
            if( it == pieces.end() ) {
                debugmsg( "Overmap connection \"%s\" refers to default piece \"%s\" which is absent from 'pieces' array.",
                          id, default_piece );
                default_piece_idx = 0;
            } else {
                default_piece_idx = std::distance( pieces.cbegin(), it );
            }
        }
    }
}

static void deserialize( omcp_location &obj, JsonIn &jsin )
{
    jsin.start_array();
    jsin.read( obj.pos );
    jsin.read( obj.loc );
    jsin.end_array();
}

static void deserialize( omcp_placement &obj, JsonIn &jsin )
{
    JsonObject jso = jsin.get_object();

    jso.read( "basic_cost", obj.basic_cost );
    if( jso.has_member( "location" ) ) {
        omcp_location loc;
        loc.pos = tripoint_zero;
        jso.read( "location", loc.loc );
        obj.locations.push_back( std::move( loc ) );
    } else {
        jso.read( "locations", obj.locations );
    }
}

static om_direction::type read_dir( JsonIn &jsin )
{
    static std::map<std::string, om_direction::type> dir_map{
        { std::string( "n" ), om_direction::type::north },
        { std::string( "e" ), om_direction::type::east },
        { std::string( "s" ), om_direction::type::south },
        { std::string( "w" ), om_direction::type::west }
    };
    std::string tmp_dir;
    jsin.read( tmp_dir );
    auto it = dir_map.find( tmp_dir );
    if( it == dir_map.end() ) {
        jsin.error( string_format( "Unknown direction '%s', valid values are: n, e, s, w", tmp_dir ) );
    } else {
        return it->second;
    }
}

static void deserialize( omcp_connection_exit &obj, JsonIn &jsin )
{
    jsin.start_array();
    jsin.read( obj.pos );
    obj.dir = read_dir( jsin );
    jsin.read( obj.conn_type );
    jsin.end_array();
}

static void deserialize( omcp_connection &obj, JsonIn &jsin )
{
    jsin.read( obj.exits );
}

static void deserialize( omcp_terrain &obj, JsonIn &jsin )
{
    jsin.start_array();
    jsin.read( obj.pos );
    jsin.read( obj.terrain );
    jsin.end_array();
}

void om_connection_piece::load( const JsonObject &jo, const std::string & )
{
    optional( jo, was_loaded, "is_linear", is_linear );
    optional( jo, was_loaded, "piece_cost", piece_cost );
    if( is_linear ) {
        mandatory( jo, was_loaded, "terrain", linear_terrain );
        mandatory( jo, was_loaded, "conn_type", linear_conn_type );
    } else {
        mandatory( jo, was_loaded, "terrains", terrains );
        mandatory( jo, was_loaded, "connections", connections );

        if( jo.has_member( "allowed_rotations" ) ) {
            allowed_rotations.reserve( om_direction::size );
            JsonIn &jsin = *jo.get_raw( "allowed_rotations" );
            jsin.start_array();
            while( !jsin.end_array() ) {
                om_direction::type dir = read_dir( jsin );
                allowed_rotations.push_back( dir );
            }
        }
    }
    mandatory( jo, was_loaded, "placements", placements );
}

void om_connection_piece::check() const
{
    if( is_linear && ( !linear_terrain || !linear_terrain->is_linear() ) ) {
        debugmsg( "In conn piece %s, terrain must be linear.", id );
    }
    if( !is_linear ) {
        if( terrains.empty() ) {
            debugmsg( "Conn piece %s has no terrains.", id );
        }
        for( const omcp_terrain &ter : terrains ) {
            if( !ter.terrain.is_valid() ) {
                debugmsg( "Conn piece %s refers to invalid overmap terrain '%s'.  Did you specify wrong rotation suffix?",
                          id, ter.terrain );
            }
        }
        for( size_t idx = 0; idx < placements.size(); idx++ ) {
            const omcp_placement &placement = placements[idx];
            if( placement.basic_cost <= 0 ) {
                debugmsg( "In conn piece %s, basic_cost must be >= 1 at placement_idx=%d (got %d)",
                          id, idx, placement.basic_cost );
            }
            if( placement.locations.size() != terrains.size() ) {
                debugmsg( "In conn piece %s, number of locations must match number of terrains at placement_idx=%d",
                          id, idx );
            } else {
                for( size_t loc_idx = 0; loc_idx < placement.locations.size(); loc_idx++ ) {
                    const omcp_location &loc = placement.locations[loc_idx];
                    const omcp_terrain &ter = terrains[loc_idx];
                    if( loc.pos != ter.pos ) {
                        debugmsg( "In conn piece %s, location pos doesn't match terrain pos at placement_idx=%d loc_idx=%d",
                                  id, idx, loc_idx );
                    }
                    if( !loc.loc.is_valid() ) {
                        debugmsg( "Conn piece %s refers to invalid overmap location '%s'.", id, loc.loc );
                    }
                }
            }
        }
    }
}

void om_connection_piece::finalize()
{
    if( is_linear ) {
        // Generate single connection with same exit on all sides
        omcp_connection pseudo_conn;
        for( om_direction::type dir : om_direction::all ) {
            omcp_connection_exit exit;
            exit.conn_type = linear_conn_type;
            exit.dir = dir;
            exit.pos = tripoint_zero;
            pseudo_conn.exits.push_back( std::move( exit ) );
        }
        connections.push_back( std::move( pseudo_conn ) );

        // Can't rotate
        allowed_rotations.push_back( om_direction::type::north );
    } else if( allowed_rotations.empty() ) {
        // Can rotate freely.
        // TODO: restrict this.
        allowed_rotations.reserve( om_direction::size );
        for( om_direction::type dir : om_direction::all ) {
            allowed_rotations.push_back( dir );
        }
    }
}

namespace overmap_connections
{

void load( const JsonObject &jo, const std::string &src )
{
    connections.load( jo, src );
}

void load_piece( const JsonObject &jo, const std::string &src )
{
    conn_pieces.load( jo, src );
}

void finalize()
{
    conn_pieces.finalize();
    connections.finalize();
    for( const auto &elem : conn_pieces.get_all() ) {
        const_cast<om_connection_piece &>( elem ).finalize(); // This cast is ugly, but safe.
    }
    for( const auto &elem : connections.get_all() ) {
        const_cast<overmap_connection &>( elem ).finalize(); // This cast is ugly, but safe.
    }
}

void check_consistency()
{
    conn_pieces.check();
    connections.check();
}

void reset()
{
    conn_pieces.reset();
    connections.reset();
}

const std::vector<overmap_connection> &get_all()
{
    return connections.get_all();
}

overmap_connection_id guess_for( const oter_id &oter )
{
    const auto &all = connections.get_all();
    const auto iter = std::find_if( all.cbegin(),
    all.cend(), [&oter]( const overmap_connection & elem ) {
        return elem.pick_subtype_for( oter ) != nullptr;
    } );

    return iter != all.cend() ? iter->id : overmap_connection_id::NULL_ID();
}

overmap_connection_id guess_for( const oter_type_id &oter )
{
    return guess_for( oter->get_first() );
}

} // namespace overmap_connections

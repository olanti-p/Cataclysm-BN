#include "active_tile_data.h"
#include "active_tile_data_impl.h"
#include "coordinate_conversions.h"
#include "debug.h"
#include "distribution_grid.h"
#include "item.h"
#include "itype.h"
#include "json.h"
#include "map.h"
#include "mapbuffer.h"
#include "vehicle.h"
#include "vpart_range.h"
#include "weather.h"

// TODO: Shouldn't use
#include "submap.h"

namespace active_tiles
{

template<typename T>
T *furn_at( const tripoint &p )
{
    tripoint xy_offset = p;
    // TODO: A "ms_to_sm" with explicit offset result
    submap *sm = MAPBUFFER.lookup_submap( ms_to_sm_remain( xy_offset ) );
    if( sm == nullptr ) {
        return nullptr;
    }
    auto iter = sm->active_furniture.find( xy_offset.xy() );
    if( iter == sm->active_furniture.end() ) {
        return nullptr;
    }

    return dynamic_cast<T *>( &*iter->second );
}

template active_tile_data *furn_at<active_tile_data>( const tripoint & );
template vehicle_connector_tile *furn_at<vehicle_connector_tile>( const tripoint & );
template battery_tile *furn_at<battery_tile>( const tripoint & );

} // namespace active_tiles

active_tile_data::~active_tile_data() {}

void active_tile_data::serialize( JsonOut &jsout ) const
{
    jsout.member( "last_updated", last_updated );
    store( jsout );
}

void active_tile_data::deserialize( JsonIn &jsin )
{
    JsonObject jo( jsin );
    jo.read( "last_updated", last_updated );
    load( jo );
}

class null_tile_data : public active_tile_data
{
        void update_internal( time_point, const tripoint &, distribution_grid & ) override
        {}
        active_tile_data *clone() const override {
            return new null_tile_data( *this );
        }

        const std::string &get_type() const override {
            static const std::string type( "null" );
            return type;
        }
        void store( JsonOut & ) const override
        {}
        void load( JsonObject & ) override
        {}
};

static std::map<std::string, std::unique_ptr<active_tile_data>> build_type_map()
{
    std::map<std::string, std::unique_ptr<active_tile_data>> type_map;
    const auto add_type = [&type_map]( active_tile_data * arg ) {
        type_map[arg->get_type()].reset( arg );
    };
    add_type( new solar_tile() );
    add_type( new battery_tile() );
    add_type( new charger_tile() );
    add_type( new vehicle_connector_tile() );
    return type_map;
}

active_tile_data *active_tile_data::create( const std::string &id )
{
    static const auto type_map = build_type_map();
    const auto iter = type_map.find( id );
    if( iter == type_map.end() ) {
        debugmsg( "Invalid active_tile_data id %s", id.c_str() );
        return new null_tile_data();
    }

    active_tile_data *new_tile = iter->second->clone();
    new_tile->last_updated = calendar::start_of_cataclysm;
    return new_tile;
}

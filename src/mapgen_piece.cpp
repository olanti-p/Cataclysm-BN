#include "mapgen_piece.h"

#include "clzones.h"
#include "debug.h"
#include "game.h"
#include "item_factory.h"
#include "json.h"
#include "magic_ter_furn_transform.h"
#include "map.h"
#include "mapgen_factory.h"
#include "mapgen.h"
#include "mapgendata.h"
#include "mission.h"
#include "npc.h"
#include "options.h"
#include "overmapbuffer.h"
#include "rng.h"
#include "string_utils.h"
#include "text_snippets.h"
#include "vpart_position.h"

jmapgen_field::jmapgen_field( const JsonObject &jsi ) :
    ftype( field_type_id( jsi.get_string( "field" ) ) )
    , intensity( jsi.get_int( "intensity", 1 ) )
    , age( time_duration::from_turns( jsi.get_int( "age", 0 ) ) )
{
    if( !ftype.id() ) {
        set_mapgen_defer( jsi, "field", "invalid field type" );
    }
}

void jmapgen_field::apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const
{
    dat.m.add_field( tripoint( x.get(), y.get(), dat.m.get_abs_sub().z ), ftype, intensity, age );
}

jmapgen_npc::jmapgen_npc( const JsonObject &jsi ) :
    npc_class( jsi.get_string( "class" ) )
    , target( jsi.get_bool( "target", false ) )
{
    if( !npc_class.is_valid() ) {
        set_mapgen_defer( jsi, "class", "unknown npc class" );
    }
    if( jsi.has_string( "add_trait" ) ) {
        std::string new_trait = jsi.get_string( "add_trait" );
        traits.emplace_back( new_trait );
    } else if( jsi.has_array( "add_trait" ) ) {
        for( const std::string new_trait : jsi.get_array( "add_trait" ) ) {
            traits.emplace_back( new_trait );
        }
    }
}

void jmapgen_npc::apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const
{
    character_id npc_id = dat.m.place_npc( point( x.get(), y.get() ), npc_class );
    if( dat.mission() && target ) {
        dat.mission()->set_target_npc_id( npc_id );
    }
    npc *p = g->find_npc( npc_id );
    if( p != nullptr ) {
        for( const std::string &new_trait : traits ) {
            p->set_mutation( trait_id( new_trait ) );
        }
    }
}

jmapgen_faction::jmapgen_faction( const JsonObject &jsi )
{
    if( jsi.has_string( "id" ) ) {
        id = faction_id( jsi.get_string( "id" ) );
    }
}

void jmapgen_faction::apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const
{
    dat.m.apply_faction_ownership( point( x.val, y.val ), point( x.valmax, y.valmax ), id );
}

jmapgen_sign::jmapgen_sign( const JsonObject &jsi ) :
    signage( jsi.get_string( "signage", "" ) )
    , snippet( jsi.get_string( "snippet", "" ) )
{
    if( signage.empty() && snippet.empty() ) {
        jsi.throw_error( "jmapgen_sign: needs either signage or snippet" );
    }
}

void jmapgen_sign::apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const
{
    const point r{ x.get(), y.get() };
    dat.m.furn_set( r, f_null );
    dat.m.furn_set( r, furn_str_id( "f_sign" ) );

    std::string signtext;

    if( !snippet.empty() ) {
        // select a snippet from the category
        signtext = SNIPPET.random_from_category( snippet ).value_or( translation() ).translated();
    } else if( !signage.empty() ) {
        signtext = signage;
    }
    if( !signtext.empty() ) {
        // replace tags
        signtext = _( signtext );

        std::string cityname = "illegible city name";
        tripoint abs_sub = dat.m.get_abs_sub();
        // TODO: fix point types
        const city *c = overmap_buffer.closest_city( tripoint_abs_sm( abs_sub ) ).city;
        if( c != nullptr ) {
            cityname = c->name;
        }
        signtext = apply_all_tags( signtext, cityname );
    }
    dat.m.set_signage( tripoint( r, dat.m.get_abs_sub().z ), signtext );
}

std::string jmapgen_sign::apply_all_tags( std::string signtext, const std::string &cityname ) const
{
    replace_city_tag( signtext, cityname );
    replace_name_tags( signtext );
    return signtext;
}

bool jmapgen_sign::has_vehicle_collision( mapgendata &dat, const point &p ) const
{
    return dat.m.veh_at( tripoint( p, dat.zlevel() ) ).has_value();
}

jmapgen_graffiti::jmapgen_graffiti( const JsonObject &jsi ) :
    text( jsi.get_string( "text", "" ) )
    , snippet( jsi.get_string( "snippet", "" ) )
{
    if( text.empty() && snippet.empty() ) {
        jsi.throw_error( "jmapgen_graffiti: needs either text or snippet" );
    }
}

void jmapgen_graffiti::apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const
{
    const point r{ x.get(), y.get() };

    std::string graffiti;

    if( !snippet.empty() ) {
        // select a snippet from the category
        graffiti = SNIPPET.random_from_category( snippet ).value_or( translation() ).translated();
    } else if( !text.empty() ) {
        graffiti = text;
    }
    if( !graffiti.empty() ) {
        // replace tags
        graffiti = _( graffiti );

        std::string cityname = "illegible city name";
        tripoint abs_sub = dat.m.get_abs_sub();
        // TODO: fix point types
        const city *c = overmap_buffer.closest_city( tripoint_abs_sm( abs_sub ) ).city;
        if( c != nullptr ) {
            cityname = c->name;
        }
        graffiti = apply_all_tags( graffiti, cityname );
    }
    dat.m.set_graffiti( tripoint( r, dat.m.get_abs_sub().z ), graffiti );
}

std::string jmapgen_graffiti::apply_all_tags( std::string graffiti,
        const std::string &cityname ) const
{
    replace_city_tag( graffiti, cityname );
    replace_name_tags( graffiti );
    return graffiti;
}

jmapgen_vending_machine::jmapgen_vending_machine( const JsonObject &jsi ) :
    reinforced( jsi.get_bool( "reinforced", false ) )
    , item_group( jsi.get_string( "item_group", "default_vending_machine" ) )
{
    if( !item_group::group_is_defined( item_group ) ) {
        set_mapgen_defer( jsi, "item_group", "no such item group" );
    }
}

void jmapgen_vending_machine::apply( mapgendata &dat, const jmapgen_int &x,
                                     const jmapgen_int &y ) const
{
    point r{ x.get(), y.get() };
    dat.m.furn_set( r, f_null );
    dat.m.place_vending( r, item_group, reinforced );
}

bool jmapgen_vending_machine::has_vehicle_collision( mapgendata &dat, const point &p ) const
{
    return dat.m.veh_at( tripoint( p, dat.zlevel() ) ).has_value();
}

jmapgen_toilet::jmapgen_toilet( const JsonObject &jsi ) :
    amount( jsi, "amount", 0, 0 )
{
}

void jmapgen_toilet::apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const
{
    const point r{ x.get(), y.get() };
    const int charges = amount.get();
    dat.m.furn_set( r, f_null );
    if( charges == 0 ) {
        dat.m.place_toilet( r ); // Use the default charges supplied as default values
    } else {
        dat.m.place_toilet( r, charges );
    }
}

bool jmapgen_toilet::has_vehicle_collision( mapgendata &dat, const point &p ) const
{
    return dat.m.veh_at( tripoint( p, dat.zlevel() ) ).has_value();
}

jmapgen_gaspump::jmapgen_gaspump( const JsonObject &jsi ) :
    amount( jsi, "amount", 0, 0 )
{
    if( jsi.has_string( "fuel" ) ) {
        fuel = jsi.get_string( "fuel" );

        // may want to not force this, if we want to support other fuels for some reason
        if( fuel != "gasoline" && fuel != "diesel" ) {
            jsi.throw_error( "invalid fuel", "fuel" );
        }
    }
}

void jmapgen_gaspump::apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const
{
    const point r{ x.get(), y.get() };
    int charges = amount.get();
    dat.m.furn_set( r, f_null );
    if( charges == 0 ) {
        charges = rng( 10000, 50000 );
    }
    if( !fuel.empty() ) {
        dat.m.place_gas_pump( r, charges, fuel );
    } else {
        dat.m.place_gas_pump( r, charges );
    }
}

bool jmapgen_gaspump::has_vehicle_collision( mapgendata &dat, const point &p ) const
{
    return dat.m.veh_at( tripoint( p, dat.zlevel() ) ).has_value();
}

jmapgen_liquid_item::jmapgen_liquid_item( const JsonObject &jsi ) :
    amount( jsi, "amount", 0, 0 )
    , liquid( jsi.get_string( "liquid" ) )
    , chance( jsi, "chance", 1, 1 )
{
    // Itemgroups apply migrations when being loaded, but we need to migrate
    // individual items here.
    liquid = item_controller->migrate_id( liquid );
    if( !liquid.is_valid() ) {
        set_mapgen_defer( jsi, "liquid", "no such item type '" + liquid.str() + "'" );
    }
}

void jmapgen_liquid_item::apply( mapgendata &dat, const jmapgen_int &x,
                                 const jmapgen_int &y ) const
{
    if( one_in( chance.get() ) ) {
        item newliquid( liquid, calendar::start_of_cataclysm );
        if( amount.valmax > 0 ) {
            newliquid.charges = amount.get();
        }
        dat.m.add_item_or_charges( tripoint( x.get(), y.get(), dat.m.get_abs_sub().z ), newliquid );
    }
}

jmapgen_item_group::jmapgen_item_group( const JsonObject &jsi ) : chance( jsi, "chance", 1, 1 )
{
    JsonValue group = jsi.get_member( "item" );
    group_id = item_group::load_item_group( group, "collection" );
    repeat = jmapgen_int( jsi, "repeat", 1, 1 );
}

void jmapgen_item_group::apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const
{
    dat.m.place_items( group_id, chance.get(), point( x.val, y.val ), point( x.valmax, y.valmax ), true,
                       calendar::start_of_cataclysm );
}

jmapgen_loot::jmapgen_loot( const JsonObject &jsi ) :
    result_group( Item_group::Type::G_COLLECTION, 100, jsi.get_int( "ammo", 0 ),
                  jsi.get_int( "magazine", 0 ) )
    , chance( jsi.get_int( "chance", 100 ) )
{
    const item_group_id group = item_group_id( jsi.get_string( "group", std::string() ) );
    const itype_id ity = itype_id( jsi.get_string( "item", std::string() ) );

    if( group.is_empty() == ity.is_empty() ) {
        jsi.throw_error( "must provide either item or group" );
    }
    if( !group.is_empty() && !group.is_valid() ) {
        set_mapgen_defer( jsi, "group", "no such item group" );
    }
    if( !ity.is_empty() && !ity.is_valid() ) {
        set_mapgen_defer( jsi, "item", "no such item type '" + ity.str() + "'" );
    }

    // All the probabilities are 100 because we do the roll in @ref apply.
    if( !group ) {
        // Migrations are applied to item *groups* on load, but single item spawns must be
        // migrated individually
        result_group.add_item_entry( item_controller->migrate_id( ity ), 100 );
    } else {
        result_group.add_group_entry( group, 100 );
    }
}

void jmapgen_loot::apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const
{
    if( rng( 0, 99 ) < chance ) {
        const Item_spawn_data *const isd = &result_group;
        const std::vector<item> spawn = isd->create( calendar::start_of_cataclysm );
        dat.m.spawn_items( tripoint( rng( x.val, x.valmax ), rng( y.val, y.valmax ),
                                     dat.m.get_abs_sub().z ), spawn );
    }
}

jmapgen_monster_group::jmapgen_monster_group( const JsonObject &jsi ) :
    id( jsi.get_string( "monster" ) )
    , density( jsi.get_float( "density", -1.0f ) )
    , chance( jsi, "chance", 1, 1 )
{
    if( !id.is_valid() ) {
        set_mapgen_defer( jsi, "monster", "no such monster group" );
    }
}

void jmapgen_monster_group::apply( mapgendata &dat, const jmapgen_int &x,
                                   const jmapgen_int &y ) const
{
    dat.m.place_spawns( id, chance.get(), point( x.val, y.val ), point( x.valmax, y.valmax ),
                        density == -1.0f ? dat.monster_density() : density );
}

jmapgen_monster::jmapgen_monster( const JsonObject &jsi ) :
    chance( jsi, "chance", 100, 100 )
    , pack_size( jsi, "pack_size", 1, 1 )
    , one_or_none( jsi.get_bool( "one_or_none",
                                 !( jsi.has_member( "repeat" ) || jsi.has_member( "pack_size" ) ) ) )
    , friendly( jsi.get_bool( "friendly", false ) )
    , name( jsi.get_string( "name", "NONE" ) )
    , target( jsi.get_bool( "target", false ) )
{
    if( jsi.has_string( "group" ) ) {
        m_id = mongroup_id( jsi.get_string( "group" ) );
        if( !m_id.is_valid() ) {
            set_mapgen_defer( jsi, "group", "no such monster group" );
            return;
        }
    } else if( jsi.has_array( "monster" ) ) {
        for( const JsonValue entry : jsi.get_array( "monster" ) ) {
            mtype_id id;
            int weight = 100;
            if( entry.test_array() ) {
                JsonArray inner = entry.get_array();
                id = mtype_id( inner.get_string( 0 ) );
                weight = inner.get_int( 1 );
            } else {
                id = mtype_id( entry.get_string() );
            }
            if( !id.is_valid() ) {
                set_mapgen_defer( jsi, "monster", "no such monster" );
                return;
            }
            ids.add( id, weight );
        }
    } else {
        mtype_id id = mtype_id( jsi.get_string( "monster" ) );
        if( !id.is_valid() ) {
            set_mapgen_defer( jsi, "monster", "no such monster" );
            return;
        }
        ids.add( id, 100 );
    }
}

void jmapgen_monster::apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const
{

    int raw_odds = chance.get();

    // Handle spawn density: Increase odds, but don't let the odds of absence go below half the odds at density 1.
    // Instead, apply a multipler to the number of monsters for really high densities.
    // For example, a 50% chance at spawn density 4 becomes a 75% chance of ~2.7 monsters.
    int odds_after_density = raw_odds * get_option<float>( "SPAWN_DENSITY" );
    int max_odds = ( 100 + raw_odds ) / 2;
    float density_multiplier = 1;
    if( odds_after_density > max_odds ) {
        density_multiplier = 1.0f * odds_after_density / max_odds;
        odds_after_density = max_odds;
    }

    int mission_id = -1;
    if( dat.mission() && target ) {
        mission_id = dat.mission()->get_id();
    }

    int spawn_count = roll_remainder( density_multiplier );

    if( one_or_none ) { // don't let high spawn density alone cause more than 1 to spawn.
        spawn_count = std::min( spawn_count, 1 );
    }
    if( raw_odds == 100 ) { // don't spawn less than 1 if odds were 100%, even with low spawn density.
        spawn_count = std::max( spawn_count, 1 );
    } else {
        if( !x_in_y( odds_after_density, 100 ) ) {
            return;
        }
    }

    if( m_id != mongroup_id::NULL_ID() ) {
        MonsterGroupResult spawn_details = MonsterGroupManager::GetResultFromGroup( m_id );
        dat.m.add_spawn( spawn_details.name, spawn_count * pack_size.get(),
        { x.get(), y.get(), dat.m.get_abs_sub().z },
        friendly, -1, mission_id, name );
    } else {
        dat.m.add_spawn( *( ids.pick() ), spawn_count * pack_size.get(),
        { x.get(), y.get(), dat.m.get_abs_sub().z },
        friendly, -1, mission_id, name );
    }
}

jmapgen_vehicle::jmapgen_vehicle( const JsonObject &jsi ) :
    type( jsi.get_string( "vehicle" ) )
    , chance( jsi, "chance", 1, 1 )
    //, rotation( jsi.get_int( "rotation", 0 ) ) // unless there is a way for the json parser to
    // return a single int as a list, we have to manually check this in the constructor below
    , fuel( jsi.get_int( "fuel", -1 ) )
    , status( jsi.get_int( "status", -1 ) )
{
    if( jsi.has_array( "rotation" ) ) {
        for( const JsonValue &elt : jsi.get_array( "rotation" ) ) {
            rotation.push_back( units::from_degrees( elt.get_int() ) );
        }
    } else {
        rotation.push_back( units::from_degrees( jsi.get_int( "rotation", 0 ) ) );
    }

    if( !type.is_valid() ) {
        set_mapgen_defer( jsi, "vehicle", "no such vehicle type or group" );
    }
}
void jmapgen_vehicle::apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const
{
    if( !x_in_y( chance.get(), 100 ) ) {
        return;
    }
    dat.m.add_vehicle( type, point( x.get(), y.get() ), random_entry( rotation ), fuel, status );
}
bool jmapgen_vehicle::has_vehicle_collision( mapgendata &dat, const point &p ) const
{
    return dat.m.veh_at( tripoint( p, dat.zlevel() ) ).has_value();
}

jmapgen_spawn_item::jmapgen_spawn_item( const JsonObject &jsi ) :
    type( jsi.get_string( "item" ) )
    , amount( jsi, "amount", 1, 1 )
    , chance( jsi, "chance", 100, 100 )
{
    // Itemgroups apply migrations when being loaded, but we need to migrate
    // individual items here.
    type = item_controller->migrate_id( type );
    if( !type.is_valid() ) {
        set_mapgen_defer( jsi, "item", "no such item" );
    }
    repeat = jmapgen_int( jsi, "repeat", 1, 1 );
}

void jmapgen_spawn_item::apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const
{
    const int c = chance.get();

    // 100% chance = exactly 1 item, otherwise scale by item spawn rate.
    const float spawn_rate = get_option<float>( "ITEM_SPAWNRATE" );
    int spawn_count = ( c == 100 ) ? 1 : roll_remainder( c * spawn_rate / 100.0f );
    for( int i = 0; i < spawn_count; i++ ) {
        dat.m.spawn_item( point( x.get(), y.get() ), type, amount.get() );
    }
}

jmapgen_trap::jmapgen_trap( const JsonObject &jsi ) :
    id( 0 )
{
    const trap_str_id sid( jsi.get_string( "trap" ) );
    if( !sid.is_valid() ) {
        set_mapgen_defer( jsi, "trap", "no such trap" );
    }
    id = sid.id();
}

jmapgen_trap::jmapgen_trap( const std::string &tid ) :
    id( 0 )
{
    const trap_str_id sid( tid );
    if( !sid.is_valid() ) {
        throw std::runtime_error( "unknown trap type" );
    }
    id = sid.id();
}

void jmapgen_trap::apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const
{
    const tripoint actual_loc = tripoint( x.get(), y.get(), dat.m.get_abs_sub().z );
    dat.m.trap_set( actual_loc, id );
}

bool jmapgen_trap::has_vehicle_collision( mapgendata &dat, const point &p ) const
{
    return dat.m.veh_at( tripoint( p, dat.zlevel() ) ).has_value();
}

jmapgen_furniture::jmapgen_furniture( const JsonObject &jsi ) : jmapgen_furniture(
        jsi.get_string( "furn" ) ) {}

jmapgen_furniture::jmapgen_furniture( const std::string &fid ) : id( furn_id( fid ) ) {}

void jmapgen_furniture::apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const
{
    dat.m.furn_set( point( x.get(), y.get() ), id );
}

bool jmapgen_furniture::has_vehicle_collision( mapgendata &dat, const point &p ) const
{
    return dat.m.veh_at( tripoint( p, dat.zlevel() ) ).has_value();
}

jmapgen_terrain::jmapgen_terrain( const JsonObject &jsi ) : jmapgen_terrain(
        jsi.get_string( "ter" ) ) {}

jmapgen_terrain::jmapgen_terrain( const std::string &tid ) : id( ter_id( tid ) ) {}

void jmapgen_terrain::apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const
{
    dat.m.ter_set( point( x.get(), y.get() ), id );
    // Delete furniture if a wall was just placed over it. TODO: need to do anything for fluid, monsters?
    if( dat.m.has_flag_ter( "WALL", point( x.get(), y.get() ) ) ) {
        dat.m.furn_set( point( x.get(), y.get() ), f_null );
        // and items, unless the wall has PLACE_ITEM flag indicating it stores things.
        if( !dat.m.has_flag_ter( "PLACE_ITEM", point( x.get(), y.get() ) ) ) {
            dat.m.i_clear( tripoint( x.get(), y.get(), dat.m.get_abs_sub().z ) );
        }
    }
}

bool jmapgen_terrain::has_vehicle_collision( mapgendata &dat, const point &p ) const
{
    return dat.m.veh_at( tripoint( p, dat.zlevel() ) ).has_value();
}

jmapgen_ter_furn_transform::jmapgen_ter_furn_transform( const JsonObject &jsi ) :
    jmapgen_ter_furn_transform(
        jsi.get_string( "transform" ) ) {}

jmapgen_ter_furn_transform::jmapgen_ter_furn_transform( const std::string &rid ) : id(
        ter_furn_transform_id( rid ) ) {}

void jmapgen_ter_furn_transform::apply( mapgendata &dat, const jmapgen_int &x,
                                        const jmapgen_int &y ) const
{
    id->transform( dat.m, tripoint( x.get(), y.get(), dat.m.get_abs_sub().z ) );
}

jmapgen_make_rubble::jmapgen_make_rubble( const JsonObject &jsi )
{
    if( jsi.has_string( "rubble_type" ) ) {
        rubble_type = furn_id( jsi.get_string( "rubble_type" ) );
    }
    jsi.read( "items", items );
    if( jsi.has_string( "floor_type" ) ) {
        floor_type = ter_id( jsi.get_string( "floor_type" ) );
    }
    jsi.read( "overwrite", overwrite );
}

void jmapgen_make_rubble::apply( mapgendata &dat, const jmapgen_int &x,
                                 const jmapgen_int &y ) const
{
    dat.m.make_rubble( tripoint( x.get(), y.get(), dat.m.get_abs_sub().z ), rubble_type, items,
                       floor_type, overwrite );
}

jmapgen_computer::jmapgen_computer( const JsonObject &jsi )
{
    jsi.read( "name", name );
    jsi.read( "access_denied", access_denied );
    security = jsi.get_int( "security", 0 );
    target = jsi.get_bool( "target", false );
    if( jsi.has_array( "options" ) ) {
        for( JsonObject jo : jsi.get_array( "options" ) ) {
            options.emplace_back( computer_option::from_json( jo ) );
        }
    }
    if( jsi.has_array( "failures" ) ) {
        for( JsonObject jo : jsi.get_array( "failures" ) ) {
            failures.emplace_back( computer_failure::from_json( jo ) );
        }
    }
}

void jmapgen_computer::apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const
{
    const point r{ x.get(), y.get() };
    dat.m.ter_set( r, t_console );
    dat.m.furn_set( r, f_null );
    computer *cpu = dat.m.add_computer( tripoint( r, dat.m.get_abs_sub().z ), name.translated(),
                                        security );
    for( const auto &opt : options ) {
        cpu->add_option( opt );
    }
    for( const auto &opt : failures ) {
        cpu->add_failure( opt );
    }
    if( target && dat.mission() ) {
        cpu->set_mission( dat.mission()->get_id() );
    }

    // The default access denied message is defined in computer's constructor
    if( !access_denied.empty() ) {
        cpu->set_access_denied_msg( access_denied.translated() );
    }
}

bool jmapgen_computer::has_vehicle_collision( mapgendata &dat, const point &p ) const
{
    return dat.m.veh_at( tripoint( p, dat.zlevel() ) ).has_value();
}

jmapgen_sealed_item::jmapgen_sealed_item( const JsonObject &jsi )
    : furniture( jsi.get_string( "furniture" ) )
    , chance( jsi, "chance", 100, 100 )
{
    if( jsi.has_object( "item" ) ) {
        JsonObject item_obj = jsi.get_object( "item" );
        item_spawner = jmapgen_spawn_item( item_obj );
    }
    if( jsi.has_object( "items" ) ) {
        JsonObject items_obj = jsi.get_object( "items" );
        item_group_spawner = jmapgen_item_group( items_obj );
    }
}

void jmapgen_sealed_item::check( const std::string &oter_name ) const
{
    const furn_t &furn = furniture.obj();
    std::string summary = string_format(
                              "sealed_item special in json mapgen for overmap terrain %s using furniture %s",
                              oter_name, furn.id.str() );

    if( !furniture.is_valid() ) {
        debugmsg( "%s which is not valid furniture", summary );
    }

    if( !item_spawner && !item_group_spawner ) {
        debugmsg( "%s specifies neither an item nor an item group.  "
                  "It should specify at least one.",
                  summary );
        return;
    }

    if( furn.has_flag( "PLANT" ) ) {
        // plant furniture requires exactly one seed item within it
        if( item_spawner && item_group_spawner ) {
            debugmsg( "%s (with flag PLANT) specifies both an item and an item group.  "
                      "It should specify exactly one.",
                      summary );
            return;
        }

        if( item_spawner ) {
            int count = item_spawner->amount.get();
            if( count != 1 ) {
                debugmsg( "%s (with flag PLANT) spawns %d items; it should spawn exactly "
                          "one.", summary, count );
                return;
            }
            int item_chance = item_spawner->chance.get();
            if( item_chance != 100 ) {
                debugmsg( "%s (with flag PLANT) spawns an item (%s) with probability %d%%; "
                          "it should always spawn.  You can move the \"chance\" up to the "
                          "sealed_item instead of the \"item\" within.",
                          summary, item_spawner->type, item_chance );
                return;
            }
            if( !item_spawner->type->seed ) {
                debugmsg( "%s (with flag PLANT) spawns item type %s which is not a seed.",
                          summary, item_spawner->type );
                return;
            }
        }

        if( item_group_spawner ) {
            int ig_chance = item_group_spawner->chance.get();
            if( ig_chance != 100 ) {
                debugmsg( "%s (with flag PLANT) spawns item group %s with chance %d.  "
                          "It should have chance 100.  You can move the \"chance\" up to the "
                          "sealed_item instead of the \"items\" within.",
                          summary, item_group_spawner->group_id.str(), ig_chance );
                return;
            }
            item_group_id group_id = item_group_spawner->group_id;
            for( const itype *type : item_group::every_possible_item_from( group_id ) ) {
                if( !type->seed ) {
                    debugmsg( "%s (with flag PLANT) spawns item group %s which can "
                              "spawn item %s which is not a seed.",
                              summary, group_id.str(), type->get_id() );
                    return;
                }
            }

            /// TODO: Somehow check that the item group always produces exactly one item.
        }
    }
}

void jmapgen_sealed_item::apply( mapgendata &dat, const jmapgen_int &x,
                                 const jmapgen_int &y ) const
{
    const int c = chance.get();

    // 100% chance = always generate, otherwise scale by item spawn rate.
    // (except is capped at 1)
    const float spawn_rate = get_option<float>( "ITEM_SPAWNRATE" );
    if( c != 100 && !x_in_y( c * spawn_rate / 100.0f, 1 ) ) {
        return;
    }

    dat.m.furn_set( point( x.get(), y.get() ), f_null );
    if( item_spawner ) {
        item_spawner->apply( dat, x, y );
    }
    if( item_group_spawner ) {
        item_group_spawner->apply( dat, x, y );
    }
    dat.m.furn_set( point( x.get(), y.get() ), furniture );
}

bool jmapgen_sealed_item::has_vehicle_collision( mapgendata &dat, const point &p ) const
{
    return dat.m.veh_at( tripoint( p, dat.zlevel() ) ).has_value();
}

jmapgen_translate::jmapgen_translate( const JsonObject &jsi )
{
    if( jsi.has_string( "from" ) && jsi.has_string( "to" ) ) {
        const std::string from_id = jsi.get_string( "from" );
        const std::string to_id = jsi.get_string( "to" );
        from = ter_id( from_id );
        to = ter_id( to_id );
    }
}

void jmapgen_translate::apply( mapgendata &dat, const jmapgen_int &/*x*/,
                               const jmapgen_int &/*y*/ ) const
{
    dat.m.translate( from, to );
}

jmapgen_zone::jmapgen_zone( const JsonObject &jsi )
{
    if( jsi.has_string( "faction" ) && jsi.has_string( "type" ) ) {
        std::string fac_id = jsi.get_string( "faction" );
        faction = faction_id( fac_id );
        std::string zone_id = jsi.get_string( "type" );
        zone_type = zone_type_id( zone_id );
        if( jsi.has_string( "name" ) ) {
            name = jsi.get_string( "name" );
        }
    }
}

void jmapgen_zone::apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const
{
    zone_manager &mgr = zone_manager::get_manager();
    const tripoint start = dat.m.getabs( tripoint( x.val, y.val, 0 ) );
    const tripoint end = dat.m.getabs( tripoint( x.valmax, y.valmax, 0 ) );
    mgr.add( name, zone_type, faction, false, true, start, end );
}

static void load_weighted_entries( const JsonObject &jsi, const std::string &json_key,
                                   weighted_int_list<std::string> &list )
{
    for( const JsonValue entry : jsi.get_array( json_key ) ) {
        if( entry.test_array() ) {
            JsonArray inner = entry.get_array();
            list.add( inner.get_string( 0 ), inner.get_int( 1 ) );
        } else {
            list.add( entry.get_string(), 100 );
        }
    }
}

neighborhood_check::neighborhood_check( const JsonObject &jsi )
{
    for( om_direction::type dir : om_direction::all ) {
        int index = static_cast<int>( dir );
        neighbors[index] = jsi.get_tags<oter_str_id>( om_direction::id( dir ) );
        has_any |= !neighbors[index].empty();

        above = jsi.get_tags<oter_str_id>( "above" );
        has_any |= !above.empty();
    }
}

bool neighborhood_check::test( mapgendata &dat ) const
{
    if( !has_any ) {
        return true;
    }

    bool all_directions_match  = true;
    for( om_direction::type dir : om_direction::all ) {
        int index = static_cast<int>( dir );
        const std::set<oter_str_id> &allowed_neighbors = neighbors[index];

        if( allowed_neighbors.empty() ) {
            continue;  // no constraints on this direction, skip.
        }

        bool this_direction_matches = false;
        for( const oter_str_id &allowed_neighbor : allowed_neighbors ) {
            this_direction_matches |= is_ot_match( allowed_neighbor.str(), dat.neighbor_at( dir ).id(),
                                                   ot_match_type::contains );
        }
        all_directions_match &= this_direction_matches;
    }

    if( !above.empty() ) {
        bool above_matches = false;
        for( const oter_str_id &allowed_neighbor : above ) {
            above_matches |= is_ot_match( allowed_neighbor.str(), dat.above().id(), ot_match_type::contains );
        }
        all_directions_match &= above_matches;
    }

    return all_directions_match;
}

jmapgen_nested::jmapgen_nested( const JsonObject &jsi ) : neighbors(
        jsi.get_object( "neighbors" ) )
{
    load_weighted_entries( jsi, "chunks", entries );
    load_weighted_entries( jsi, "else_chunks", else_entries );
}

void jmapgen_nested::apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const
{
    const std::string *res = neighbors.test( dat ) ? entries.pick() : else_entries.pick();
    if( res == nullptr || res->empty() || *res == "null" ) {
        // This will be common when neighbors.test(...) is false, since else_entires is often empty.
        return;
    }

    const auto &nested_mapgen = get_all_nested_mapgen();

    const auto iter = nested_mapgen.find( *res );
    if( iter == nested_mapgen.end() ) {
        debugmsg( "Unknown nested mapgen function id %s", res->c_str() );
        return;
    }

    // A second roll? Let's allow it for now
    const auto &ptr = iter->second.pick();
    if( ptr == nullptr ) {
        return;
    }

    ( *ptr )->nest( dat, point( x.get(), y.get() ) );
}

bool jmapgen_nested::has_vehicle_collision( mapgendata &dat, const point &p ) const
{
    const weighted_int_list<std::string> &selected_entries = neighbors.test(
                dat ) ? entries : else_entries;
    if( selected_entries.empty() ) {
        return false;
    }

    const auto &nested_mapgen = get_all_nested_mapgen();

    for( auto &entry : selected_entries ) {
        if( entry.obj == "null" ) {
            continue;
        }
        const auto iter = nested_mapgen.find( entry.obj );
        if( iter == nested_mapgen.end() ) {
            return false;
        }
        for( const auto &nest : iter->second ) {
            if( nest.obj->has_vehicle_collision( dat, p ) ) {
                return true;
            }
        }
    }

    return false;
}

template<>
JmPieceType jmapgen_alternativly_trap::get_type() const
{
    return JmPieceType::AltTrap;
}

template<>
JmPieceType jmapgen_alternativly_furniture::get_type() const
{
    return JmPieceType::AltFurniture;
}

template<>
JmPieceType jmapgen_alternativly_terrain::get_type() const
{
    return JmPieceType::AltTerrain;
}

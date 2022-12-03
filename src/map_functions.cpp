#include "map_functions.h"

#include "character.h"
#include "game.h"
#include "map.h"
#include "debug.h"
#include "map_iterator.h"
#include "messages.h"
#include "submap.h"
#include "monster.h"
#include "overmapbuffer.h"
#include "sounds.h"
#include "mapbuffer.h"

static const mtype_id mon_mi_go_myrmidon( "mon_mi_go_myrmidon" );

namespace map_funcs
{

int climbing_cost( const map &m, const tripoint &from, const tripoint &to )
{
    // TODO: All sorts of mutations, equipment weight etc. for characters
    if( !m.valid_move( from, to, false, true ) ) {
        return 0;
    }
    const int diff = m.climb_difficulty( from );
    if( diff > 5 ) {
        return 0;
    }
    return 50 + diff * 100;
}

void migo_nerve_cage_removal( map &m, const tripoint &p, bool spawn_damaged )
{
    bool open = false;
    for( const tripoint &tmp : m.points_in_radius( p, 12 ) ) {
        if( m.ter( tmp ) == ter_id( "t_wall_resin_cage" ) ) {
            m.ter_set( tmp, ter_id( "t_floor_resin" ) );
            open = true;
        }
    }
    if( open ) {
        add_msg( m_good, _( "The nerve cluster collapses in on itself, and the nearby cages open!" ) );
    } else {
        add_msg( _( "The nerve cluster collapses in on itself, to no discernible effect." ) );
    }
    sounds::sound( p, 120, sounds::sound_t::combat,
                   _( "a loud alien shriek reverberating through the structure!" ), true,
                   "shout", "scream_tortured" );
    monster *const spawn = g->place_critter_around( mon_mi_go_myrmidon, p, 1 );
    if( spawn_damaged ) {
        spawn->set_hp( spawn->get_hp_max() / 2 );
    }
    if( get_player_character().sees( p ) ) {
        add_msg( m_bad, _( "Something stirs and clambers out of the ruined mass of flesh and nerves!" ) );
    }
}

// Optimized mapgen function that only works properly for very simple overmap types
// Does not create or require a temporary map and does its own saving
static void generate_uniform( const tripoint_abs_sm &p_rounded, const ter_id &terrain_type )
{
    DebugLog( DL::Info, DC::MapGen )
            << string_format( "Generating uniform %s terrain_type: %s",
                              p_rounded.to_string(), terrain_type.id() );

    for( int xd = 0; xd <= 1; xd++ ) {
        for( int yd = 0; yd <= 1; yd++ ) {
            submap *sm = new submap();
            sm->is_uniform = true;
            sm->set_all_ter( terrain_type );
            sm->last_touched = calendar::turn;
            MAPBUFFER.add_submap( ( p_rounded + point( xd, yd ) ).raw(), sm );
        }
    }
}

submap *fetch_or_generate_submap( const tripoint_abs_sm &p )
{
    // Cache empty overmap types
    static const oter_id rock( "empty_rock" );
    static const oter_id air( "open_air" );

    submap *tmpsub = MAPBUFFER.lookup_submap( p );

    if( tmpsub ) {
        return tmpsub;
    }

    // It doesn't exist; we must generate it!
    DebugLogFL( DL::Info, DC::MapGen )
            << string_format( "Missing mapbuffer data for %s.  Regenerating.", p.to_string() );

    // Each overmap square is two nonants; to prevent overlap,
    // generate only at squares divisible by 2.

    coords::coord_point<point, coords::origin::overmap_terrain, coords::sm> _unused;
    tripoint_abs_omt p_omt_abs;
    std::tie( p_omt_abs, _unused ) = coords::project_remain<coords::omt>( p );
    tripoint_abs_sm p_rounded = coords::project_to<coords::sm>( p_omt_abs );

    const oter_id terrain_type = overmap_buffer.ter( p_omt_abs );

    // Short-circuit if the map tile is uniform
    // TODO: Replace with json mapgen functions.
    if( terrain_type == air ) {
        generate_uniform( p_rounded, t_open_air );
    } else if( terrain_type == rock ) {
        generate_uniform( p_rounded, t_rock );
    } else {
        tinymap tmp_map;
        tmp_map.generate( p_rounded.raw(), calendar::turn );
    }

    // This is the same call to MAPBUFFER as above!
    tmpsub = MAPBUFFER.lookup_submap( p );
    if( tmpsub == nullptr ) {
        debugmsg( "failed to generate a submap at %s", p.to_string() );
    }
    return tmpsub;
}

} // namespace map_funcs

#include "catch/catch.hpp"

#include "avatar.h"
#include "map.h"
#include "map_helpers.h"
#include "map_setup_helpers.h"
#include "point.h"
#include "player_helpers.h"
#include "string_formatter.h"
#include "type_id.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"

static map_helpers::canvas_legend legend = {{
        { U'.', "t_pavement" },
        { U'x', "t_railroad_track" },
    }
};

const efftype_id effect_blind( "blind" );

static void clear_game( const ter_id &terrain )
{
    // Set to turn 0 to prevent solars from producing power
    calendar::turn = calendar::turn_zero;
    clear_avatar();
    clear_creatures();
    clear_npcs();
    clear_vehicles();

    avatar &u = get_avatar();
    // Move player somewhere safe
    REQUIRE_FALSE( u.in_vehicle );
    u.setpos( tripoint_zero );
    // Blind the player to avoid needless drawing-related overhead
    u.add_effect( effect_blind, 365_days, num_bp );

    build_test_map( terrain );
}

static void build_map_from_canvas( const map_helpers::canvas &canvas, const tripoint &canvas_pos )
{
    auto adapter = map_helpers::canvas_adapter( legend )
    .with_setter( [canvas_pos]( const point & p, const std::string & s ) {
        get_map().ter_set( p + canvas_pos, ter_str_id( s ).id() );
    } )
    .with_getter( [canvas_pos]( const point & p ) {
        return get_map().ter( p + canvas_pos ).id().str();
    } );

    adapter.set_all( canvas );

    // Sanity check
    adapter.check_matches_expected( canvas, true );
}

static void test_rail_movement( const std::string &veh_id, int num_cycles,
                                const tripoint &canvas_pos,
                                map_helpers::canvas &canvas )
{
    tripoint vehicle_pos = canvas_pos + canvas.replace_unique( U'*', U'x' );
    point pos_center = canvas.replace_unique( U'o', U'x' );
    point pos_l = canvas.replace_opt( U'l', U'x' ).value_or( pos_center );
    point pos_r = canvas.replace_opt( U'r', U'x' ).value_or( pos_center );

    clear_game( t_floor );
    build_map_from_canvas( canvas, canvas_pos );

    map &here = get_map();
    vehicle *veh_ptr = here.add_vehicle( vproto_id( veh_id ), vehicle_pos, -90_degrees, 100, 0 );

    REQUIRE( veh_ptr != nullptr );
    if( veh_ptr == nullptr ) {
        return;
    }

    vehicle &veh = *veh_ptr;

    // Remove all items from cargo to normalize weight.
    for( const vpart_reference vp : veh.get_all_parts() ) {
        veh_ptr->get_items( vp.part_index() ).clear();
        vp.part().ammo_consume( vp.part().ammo_remaining(), vp.pos() );
    }
    for( const vpart_reference vp : veh.get_avail_parts( "OPENABLE" ) ) {
        veh.close( vp.part_index() );
    }

    veh.refresh_insides();

    const tripoint starting_point = veh.global_pos3();
    veh.tags.insert( "IN_CONTROL_OVERRIDE" );
    veh.engine_on = true;

    int tgt_velocity = 5000;
    REQUIRE( veh.safe_velocity( false ) >= tgt_velocity );
    veh.cruise_on = true;
    veh.cruise_velocity = tgt_velocity;
    veh.velocity = tgt_velocity;
    veh.vertical_velocity = 0;

    int cycles_left = num_cycles;
    while( cycles_left > 0 ) {
        tripoint pos_before = veh.global_pos3();
        cycles_left -= 1;
        here.vehmove();
        veh.idle( true );
        // If the vehicle starts skidding, the effects become random and test is RUINED
        REQUIRE( !veh.skidding );
        for( const tripoint &pos : veh.get_points() ) {
            REQUIRE( here.ter( pos ) );
        }
        cata_printf( "pos: %s dir: %d vel: %d/%d\n",
                     veh.global_pos3().to_string(),
                     static_cast<int>( units::to_degrees( veh.face.dir() ) ),
                     veh.velocity,
                     veh.vertical_velocity
                   );
        tripoint pos_after = veh.global_pos3();
        veh.velocity = tgt_velocity;
        here.displace_vehicle( veh, pos_before - pos_after );
    }

    FAIL();
}

map_helpers::canvas rails_straight()
{
    return { {
            U".x..x..x.",
            U".x..x..x.",
            U".x..x..x.",
            U".x..x..x.",
            U".x..o..x.",
            U".x..x..x.",
            U".x..x..x.",
            U".x..x..x.",
            U".x..x..x.",
            U".x..x..x.",
            U".x..x..x.",
            U".x..x..x.",
            U".x..x..x.",
            U".x..x..x.",
            U".x..x..x.",
            U".x..x..x.",
            U".x..x..x.",
            U".x..*..x.",
            U".x..x..x.",
            U".x..x..x.",
            U".x..x..x.",
            U".x..x..x.",
        }
    };
}

map_helpers::canvas rails_diag_start()
{
    return { {
            U"................x..x..x.",
            U"...............x..x..x..",
            U"..............x..x..x...",
            U".............x..x..x....",
            U"............x..o..x.....",
            U"...........x..x..x......",
            U"..........x..x..x.......",
            U".........x..x..x........",
            U"........x..x..x.........",
            U".......x..x..x..........",
            U"......x..x..x...........",
            U".....x..x..x............",
            U"....x..x..x.............",
            U"...x..x..x..............",
            U"..x..x..x...............",
            U".x..x..x................",
            U".x..x..x................",
            U".x..x..x................",
            U".x..x..x................",
            U".x..x..x................",
            U".x..x..x................",
            U".x..*..x................",
            U".x..x..x................",
            U".x..x..x................",
            U".x..x..x................",
            U".x..x..x................",
        }
    };
}

map_helpers::canvas rails_diag_end()
{
    return { {
            U"..............................",
            U".....................xxxxxxxxx",
            U"....................x.........",
            U"...................x..........",
            U"..................x..xxxxoxxxx",
            U".................x..x.........",
            U"................x..x..........",
            U"...............x..x..xxxxxxxxx",
            U"..............x..x..x.........",
            U".............x..x..x..........",
            U"............x..x..x...........",
            U"...........x..x..x............",
            U"..........x..x..x.............",
            U".........x..x..x..............",
            U"........x..x..x...............",
            U".......x..x..x................",
            U"......x..x..x.................",
            U".....x..x..x..................",
            U"....x..*..x...................",
            U"...x..x..x....................",
            U"..x..x..x.....................",
            U".x..x..x......................",
            U"x..x..x.......................",
            U"..x..x........................",
            U".x..x.........................",
            U"x..x..........................",
            U"..x...........................",
            U".x............................",
            U"x.............................",
            U"..............................",
        }
    };
}

map_helpers::canvas rails_cross()
{
    return { {
            U"..............x..x..x.............",
            U"..............x..x..x.............",
            U"..............x..x..x.............",
            U"..............x..x..x.............",
            U"..............x..o..x.............",
            U"..............x..x..x.............",
            U"..............x..x..x.............",
            U"..............x..x..x.............",
            U"..............x..x..x.............",
            U"..............x..x..x.............",
            U"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
            U"..............x..x..x.............",
            U"..............x..x..x.............",
            U"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
            U"..............x..x..x.............",
            U"..............x..x..x.............",
            U"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
            U"..............x..x..x.............",
            U"..............x..x..x.............",
            U"..............x..x..x.............",
            U"..............x..x..x.............",
            U"..............x..x..x.............",
            U"..............x..*..x.............",
            U"..............x..x..x.............",
            U"..............x..x..x.............",
            U"..............x..x..x.............",
            U"..............x..x..x.............",
        }
    };
}

map_helpers::canvas rails_tee_straight()
{
    return { {
            U".x..x..x........x..x..x........x..x..x.",
            U"..x..x..x.......x..x..x.......x..x..x..",
            U"...x..x..x......x..x..x......x..x..x...",
            U"....x..x..x.....x..x..x.....x..x..x....",
            U".....x..l..x....x..o..x....x..r..x.....",
            U"......x..x..x...x..x..x...x..x..x......",
            U".......x..x..x..x..x..x..x..x..x.......",
            U"........x..x..x.x..x..x.x..x..x........",
            U".........x..x..xx..x..xx..x..x.........",
            U"..........x..x..x..x..x..x..x..........",
            U"...........x..x.xx.x.xx.x..x...........",
            U"............x..xx.xxx.xx..x............",
            U".............x..x..x..x..x.............",
            U"..............x.xxxxxxx.x..............",
            U"...............xxxxxxxxx...............",
            U"................x..x..x................",
            U"................x..x..x................",
            U"................x..x..x................",
            U"................x..x..x................",
            U"................x..x..x................",
            U"................x..x..x................",
            U"................x..*..x................",
            U"................x..x..x................",
            U"................x..x..x................",
            U"................x..x..x................",
            U"................x..x..x................",
        }
    };
}

map_helpers::canvas rails_tee_diag()
{
    return { {
            U"..................x..x..x.........x..x..x..",
            U"..................x..x..x........x..x..x...",
            U"..................x..x..x.......x..x..x....",
            U"..................x..x..x......x..x..x.....",
            U"..................x..l..x.....x..o..x......",
            U"..................x..x..x....x..x..x.......",
            U"..................x..x..x...x..x..x........",
            U"..................x..x..x..x..x..x.........",
            U"..................x..x..x.x..x..x..........",
            U"..................x..x..xx..x..x...........",
            U"..................x..x..x..x..x............",
            U"..................x..x.xx.x..x.............",
            U"..................x..xx.xx..x..............",
            U"..................x..xxxxxxxxxxxxxxxxxxxxxx",
            U"..................x.xx.xx.x................",
            U"..................xx.xx.xx.................",
            U"..................x..xxxxxxxxxxxxxxxxxrxxxx",
            U".................x..x..x...................",
            U"................x..x..x....................",
            U"...............x..x..xxxxxxxxxxxxxxxxxxxxxx",
            U"..............x..x..x......................",
            U".............x..x..x.......................",
            U"............x..x..x........................",
            U"...........x..x..x.........................",
            U"..........x..x..x..........................",
            U".........x..x..x...........................",
            U"........x..x..x............................",
            U".......x..x..x.............................",
            U"......x..x..x..............................",
            U".....x..x..x...............................",
            U"....x..*..x................................",
            U"...x..x..x.................................",
            U"..x..x..x..................................",
            U".x..x..x...................................",
            U"x..x..x....................................",
            U"..x..x.....................................",
            U".x..x......................................",
            U"x..x.......................................",
            U"..x........................................",
            U".x.........................................",
            U"x..........................................",
            U"...........................................",
        }
    };
}

TEST_CASE( "canvas_stuff", "[vehicle][railroad]" )
{
    map_helpers::canvas ccc = { {
            U"..",
            U"ab",
            U"cd",
            U"..",
        }
    };

    ccc.rotated( 1 );

    SECTION( "even_sides" ) {
        map_helpers::canvas canvas = { {
                U"..",
                U"ab",
                U"cd",
                U"..",
            }
        };

        canvas.rotated( 1 );

        REQUIRE( canvas.rotated( 0 ) == canvas );
        REQUIRE( canvas.rotated( 1 ) == map_helpers::canvas( { {
                U".ca.",
                U".db.",
            }
        } ) );
        REQUIRE( canvas.rotated( 2 ) == map_helpers::canvas( { {
                U"..",
                U"dc",
                U"ba",
                U".."
            }
        } ) );
        REQUIRE( canvas.rotated( 3 ) == map_helpers::canvas( { {
                U".bd.",
                U".ac.",
            }
        } ) );
        REQUIRE( canvas.rotated( 4 ) == canvas );
    }
    SECTION( "not_even_sides" ) {
        map_helpers::canvas canvas = { {
                U"...",
                U"abc",
                U"...",
                U"def",
                U"...",
            }
        };

        REQUIRE( canvas.rotated( 0 ) == canvas );
        REQUIRE( canvas.rotated( 1 ) == map_helpers::canvas( { {
                U".d.a.",
                U".e.b.",
                U".d.c.",
            }
        } ) );
        REQUIRE( canvas.rotated( 2 ) == map_helpers::canvas( { {
                U"...",
                U"fed",
                U"...",
                U"cba",
                U"..."
            }
        } ) );
        REQUIRE( canvas.rotated( 3 ) == map_helpers::canvas( { {
                U".c.f.",
                U".b.e.",
                U".a.d.",
            }
        } ) );

        REQUIRE( canvas.rotated( 4 ) == canvas );
    }
}

TEST_CASE( "vehicle_rail_movement", "[vehicle][railroad]" )
{
    //tripoint canvas_pos( 10, 10, 0 );
    //test_rail_movement( "4x4_car", 20, canvas_pos, map_helpers::canvas() );
    //
    //test_rail_movement( "4x4_car", 20, canvas_pos, rails_straight() );

}

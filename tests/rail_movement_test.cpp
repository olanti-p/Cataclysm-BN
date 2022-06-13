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
#include "units_utility.h"

namespace Catch
{
template<>
struct StringMaker<map_helpers::canvas> {
    static std::string convert( const map_helpers::canvas &c ) {
        return c.to_string();
    }
};
} // namespace Catch

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

static void run_test_case( const std::string &veh_id, int num_cycles,
                           tripoint vehicle_pos,
                           units::angle face_dir,
                           units::angle turn_dir,
                           tripoint expected_pos,
                           units::angle expected_dir )
{
    map &here = get_map();
    vehicle *veh_ptr = here.add_vehicle( vproto_id( veh_id ), vehicle_pos, face_dir, 100, 0 );

    REQUIRE( veh_ptr != nullptr );

    vehicle &veh = *veh_ptr;

    // Position passed to add_vehicle is the desired position of the vehicle's (0,0) part.
    // However, for ease of testing we want to deal with positions of pivot.
    // As such, shift the vehicle as necessary so vehicle_pos is the pivot pos.
    //tripoint pivot_fix_delta = vehicle_pos - veh.global_pos3();
    tripoint pivot_fix_delta;
    bool displaced_ok = here.displace_vehicle( veh, pivot_fix_delta );
    if( !displaced_ok ) {
        CAPTURE( vehicle_pos );
        CAPTURE( veh.global_pos3() );
        CAPTURE( pivot_fix_delta );
        REQUIRE( displaced_ok );
    }

    // Check that pivot pos is right where we want it
    //REQUIRE( veh.global_pos3() == vehicle_pos );

    CAPTURE( vehicle_pos );
    CAPTURE( expected_pos );
    CAPTURE( veh.global_pos3() );

    // Remove all items from cargo to normalize weight.
    for( const vpart_reference vp : veh.get_all_parts() ) {
        veh_ptr->get_items( vp.part_index() ).clear();
        vp.part().ammo_consume( vp.part().ammo_remaining(), vp.pos() );
    }
    for( const vpart_reference vp : veh.get_avail_parts( "OPENABLE" ) ) {
        veh.close( vp.part_index() );
    }

    veh.refresh_insides();

    veh.tags.insert( "IN_CONTROL_OVERRIDE" );
    veh.engine_on = true;

    int tgt_velocity = 5000;
    REQUIRE( veh.safe_velocity( false ) >= tgt_velocity );
    veh.cruise_on = true;
    veh.cruise_velocity = tgt_velocity;
    veh.velocity = tgt_velocity;
    veh.vertical_velocity = 0;
    veh.turn_dir = turn_dir;

    CAPTURE( vehicle_pos );
    CAPTURE( expected_pos );
    CAPTURE( veh.global_pos3() );

    int cycles_left = num_cycles;
    while( cycles_left > 0 ) {
        //tripoint pos_before = veh.global_pos3();
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
        //tripoint pos_after = veh.global_pos3();
        veh.velocity = tgt_velocity;
        //here.displace_vehicle( veh, pos_before - pos_after );

        if( veh.global_pos3() == expected_pos ) {
            break;
        }
    }

    CHECK( veh.global_pos3() == expected_pos );
    CHECK( normalize( veh.face.dir() ) == expected_dir );
}

static void test_rail_movement( const std::string &veh_id, int num_cycles,
                                const tripoint &canvas_pos,
                                map_helpers::canvas &canvas )
{
    tripoint vehicle_pos = canvas_pos + canvas.replace_unique( U'*', U'x' );
    point p_center = canvas.replace_unique( U'o', U'x' );
    point p_l = canvas.replace_opt( U'l', U'x' ).value_or( p_center );
    point p_r = canvas.replace_opt( U'r', U'x' ).value_or( p_center );

    tripoint pos_center = canvas_pos + p_center;
    tripoint pos_l = canvas_pos + p_l;
    tripoint pos_r = canvas_pos + p_r;

    ( void )pos_l;
    ( void )pos_r;

    clear_game( t_floor );
    build_map_from_canvas( canvas, canvas_pos );

    units::angle face_dir = normalize( -90_degrees );
    units::angle turn_dir = normalize( face_dir );
    units::angle expected_dir = normalize( face_dir );

    run_test_case( veh_id, num_cycles, vehicle_pos, -90_degrees,
                   turn_dir, pos_center, expected_dir );
}

map_helpers::canvas empty_terrain()
{
    return { {
            U".........",
            U".........",
            U".........",
            U".........",
            U"....o....",
            U".........",
            U".........",
            U".........",
            U".........",
            U".........",
            U".........",
            U".........",
            U".........",
            U".........",
            U".........",
            U".........",
            U"....*....",
            U".........",
            U".........",
            U".........",
            U".........",
        }
    };
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
    SECTION( "even_sides" ) {
        map_helpers::canvas canvas = { {
                U"..",
                U"ab",
                U"cd",
                U"..",
            }
        };

        REQUIRE( canvas.rotated( 0 ) == canvas );
        {
            map_helpers::canvas exp = { {
                    U".ca.",
                    U".db.",
                }
            };
            REQUIRE( canvas.rotated( 1 ) == exp );
        }
        {
            map_helpers::canvas exp = { {
                    U"..",
                    U"dc",
                    U"ba",
                    U".."
                }
            };
            REQUIRE( canvas.rotated( 2 ) == exp );
        }
        {
            map_helpers::canvas exp = { {
                    U".bd.",
                    U".ac.",
                }
            };
            REQUIRE( canvas.rotated( 3 ) == exp );
        }
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
        {
            map_helpers::canvas exp = { {
                    U".d.a.",
                    U".e.b.",
                    U".f.c.",
                }
            };
            REQUIRE( canvas.rotated( 1 ) == exp );
        }
        {
            map_helpers::canvas exp = { {
                    U"...",
                    U"fed",
                    U"...",
                    U"cba",
                    U"..."
                }
            };
            REQUIRE( canvas.rotated( 2 ) == exp );
        }
        {
            map_helpers::canvas exp = { {
                    U".c.f.",
                    U".b.e.",
                    U".a.d.",
                }
            };
            REQUIRE( canvas.rotated( 3 ) == exp );
        }
        REQUIRE( canvas.rotated( 4 ) == canvas );
    }
}

TEST_CASE( "vehicle_rail_movement", "[vehicle][railroad]" )
{
    tripoint canvas_pos( 10, 10, 0 );
    {
        auto c = empty_terrain();
        test_rail_movement( "4x4_car", 30, canvas_pos, c );
    }
    /*
    {
        auto c = rails_straight();
        test_rail_movement( "4x4_car", 20, canvas_pos, c );
    }
    */
}

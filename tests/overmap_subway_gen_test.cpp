#include "catch/catch.hpp"

#include "overmap.h"
#include "point.h"
#include "string_formatter.h"
#include "map_setup_helpers.h"
#include "overmap_generation.h"

extern bool debug_connection_lay;

static map_helpers::canvas_legend legend = {{
        { U'.', "empty_rock" },
        { U'#', "rock_border" },
        { U'v', "subway_end_north" },
        { U'<', "subway_end_east" },
        { U'^', "subway_end_south" },
        { U'>', "subway_end_west" },
        { U'│', "subway_ns" },
        { U'─', "subway_ew" },
        { U'┘', "subway_wn" },
        { U'└', "subway_ne" },
        { U'┌', "subway_es" },
        { U'┐', "subway_sw" },
        { U'├', "subway_nes" },
        { U'┬', "subway_esw" },
        { U'┤', "subway_nsw" },
        { U'┴', "subway_new" },
        { U'+', "subway_nesw" },
        /*
        { U'║', "bridge_north" },
        { U'═', "bridge_east" },
        { U'╎', "bridge_south" },
        { U'╌', "bridge_west" },
        */
    }
};

class subway_gen_tester
{
    private:
        int test_num = -1;
        std::unique_ptr<overmap> om;
        map_helpers::canvas_adapter adapter;
        tripoint sz;

    public:
        subway_gen_tester( int test_num, const map_helpers::canvas &initial ) :
            test_num( test_num ), om( std::make_unique<overmap>( point_abs_om( 0, 0 ) ) ) {
            CAPTURE( test_num );

            sz = initial.size();

            oter_str_id block_str( "rock_border" );
            oter_id block = block_str.id();

            for( int x = 0; x < sz.x + 2; x++ ) {
                om->ter_set( tripoint_om_omt( x, 0, 0 ), block );
                om->ter_set( tripoint_om_omt( x, sz.y + 1, 0 ), block );
            }
            for( int y = 0; y < sz.y + 2; y++ ) {
                om->ter_set( tripoint_om_omt( 0, y, 0 ), block );
                om->ter_set( tripoint_om_omt( sz.x + 1, y, 0 ), block );
            }

            adapter = map_helpers::canvas_adapter( legend )
            .with_getter( [&]( const tripoint & p ) {
                return om->ter( tripoint_om_omt( p ) + point( 1, 1 ) ).id().str();
            } )
            .with_setter( [&]( const tripoint & p, const std::string & id ) {
                om->ter_set( tripoint_om_omt( p ) + point( 1, 1 ), oter_str_id( id ).id() );
            } );

            adapter.set_all( initial );

            // Sanity check
            adapter.check_matches_expected( initial, true );
        }

        ~subway_gen_tester() = default;

        subway_gen_tester &run_gen(
            point start,
            om_direction::type start_dir,
            point dest,
            om_direction::type dest_dir
        ) {
            CAPTURE( test_num );

            const overmap_connection &connection = string_id<overmap_connection>( "subway_tunnel" ).obj();

            overmap_generation::set_debug_output( true );
            auto _restore = on_out_of_scope( [] {
                overmap_generation::set_debug_output( false );
            } );

            const overmap_generation::ConnPath path =
                overmap_generation::lay_out_connection(
                    *om,
                    connection,
                    tripoint_om_omt( start.x + 1, start.y + 1, 0 ),
                    start_dir,
                    tripoint_om_omt( dest.x + 1, dest.y + 1, 0 ),
                    dest_dir,
                    false
                );

            overmap_generation::build_connection( *om, path );

            return *this;
        }

        subway_gen_tester &expect( const map_helpers::canvas &expected ) {
            CAPTURE( test_num );

            adapter.check_matches_expected( expected, false );

            return *this;
        }
};

static map_helpers::canvas empty_10_11 = {{
        U"..........",
        U"..........",
        U"..........",
        U"..........",
        U"..........",
        U"..........",
        U"..........",
        U"..........",
        U"..........",
        U"..........",
        U".........."
    }
};

TEST_CASE( "subway_gen_straight", "[mapgen][connects][subway]" )
{
    subway_gen_tester( 1, empty_10_11 )
    // Horizontal w->e
    .run_gen( point( 1, 1 ), om_direction::type::invalid, point( 3, 1 ), om_direction::type::invalid )
    // Horizontal e->w
    .run_gen( point( 3, 3 ), om_direction::type::invalid, point( 1, 3 ), om_direction::type::invalid )
    // Vertical n->s
    .run_gen( point( 8, 1 ), om_direction::type::invalid, point( 8, 3 ), om_direction::type::invalid )
    // Vertical s->n
    .run_gen( point( 8, 8 ), om_direction::type::invalid, point( 8, 6 ), om_direction::type::invalid )
    // Crossing
    .run_gen( point( 3, 5 ), om_direction::type::invalid, point( 3, 9 ), om_direction::type::invalid )
    .run_gen( point( 1, 7 ), om_direction::type::invalid, point( 5, 7 ), om_direction::type::invalid )
    .expect( {
        {
            U"..........",
            U".>─<....v.",
            U"........│.",
            U".>─<....^.",
            U"..........",
            U"...v......",
            U"...│....v.",
            U".>─+─<..│.",
            U"...│....^.",
            U"...^......",
            U".........."
        }
    } );
}

TEST_CASE( "subway_gen_no_path", "[mapgen][connects][subway]" )
{
    static map_helpers::canvas two_rooms = {{
            U"#########",
            U"#.......#",
            U"#.......#",
            U"#.......#",
            U"#########",
            U"#.......#",
            U"#.......#",
            U"#.......#",
            U"#########"
        }
    };

    subway_gen_tester( 0, two_rooms )
    // Can't path from top part into bottom part
    .run_gen( point( 2, 2 ), om_direction::type::invalid, point( 6, 6 ), om_direction::type::invalid )
    // Can't path from bottom part into top part
    .run_gen( point( 2, 6 ), om_direction::type::invalid, point( 6, 2 ), om_direction::type::invalid )
    .expect( two_rooms );
}

static map_helpers::canvas empty_10_10 = {{
        U"..........",
        U"..........",
        U"..........",
        U"..........",
        U"..........",
        U"..........",
        U"..........",
        U"..........",
        U"..........",
        U".........."
    }
};

TEST_CASE( "subway_gen_curves_and_connections", "[mapgen][connects][subway]" )
{
    subway_gen_tester( 2, empty_10_10 )
    // Straight with connections towards n and s
    .run_gen( point( 9, 0 ), om_direction::type::north, point( 9, 9 ), om_direction::type::south )
    // Straight with connections towards e and w, also intersects at dest
    .run_gen( point( 0, 9 ), om_direction::type::west, point( 9, 9 ), om_direction::type::east )
    // Straight with connections towards n and s, also intersects at src
    .run_gen( point( 0, 9 ), om_direction::type::south, point( 0, 7 ), om_direction::type::north )
    // Bend with connections at n and w
    .run_gen( point( 0, 2 ), om_direction::type::west, point( 2, 0 ), om_direction::type::north )
    // Bend with connections at n and w
    .run_gen( point( 3, 6 ), om_direction::type::south, point( 5, 4 ), om_direction::type::east )
    .expect( {
        {
            U"┌─┘......│",
            U"│........│",
            U"┘........│",
            U".........│",
            U"...┌──...│",
            U"...│.....│",
            U"...│.....│",
            U"│........│",
            U"│........│",
            U"+────────+"
        }
    } );
}

TEST_CASE( "subway_gen_extend", "[mapgen][connects][subway]" )
{
    subway_gen_tester( 3, empty_10_10 )
    // Horizontal w->e extends existing w->e
    .run_gen( point( 3, 1 ), om_direction::type::invalid, point( 5, 1 ), om_direction::type::invalid )
    .run_gen( point( 3, 1 ), om_direction::type::invalid, point( 7, 1 ), om_direction::type::invalid )
    // Horizontal e->w extends existing w->e
    .run_gen( point( 7, 1 ), om_direction::type::invalid, point( 1, 1 ), om_direction::type::invalid )
    // Horizontal w->e is extended into a w->s bend
    .run_gen( point( 1, 5 ), om_direction::type::invalid, point( 3, 5 ), om_direction::type::invalid )
    .run_gen( point( 1, 5 ), om_direction::type::invalid, point( 5, 8 ), om_direction::type::invalid )
    .expect( {
        {
            U"..........",
            U".>─────<..",
            U"..........",
            U"..........",
            U"..........",
            U".>─┐......",
            U"...│......",
            U"...│......",
            U"...└─<....",
            U".........."
        }
    } );
}

TEST_CASE( "subway_gen_join", "[mapgen][connects][subway]" )
{
    subway_gen_tester( 4, empty_10_10 )
    // Vertical s->n
    .run_gen( point( 4, 0 ), om_direction::type::invalid, point( 4, 9 ), om_direction::type::invalid )
    // Is joined from nw
    .run_gen( point( 1, 0 ), om_direction::type::invalid, point( 4, 9 ), om_direction::type::invalid )
    // Is joined from se, follows path backwards to nw
    .run_gen( point( 7, 9 ), om_direction::type::invalid, point( 1, 0 ), om_direction::type::invalid )
    // Is joined from ne, leaves towards sw
    .run_gen( point( 7, 2 ), om_direction::type::invalid, point( 1, 7 ), om_direction::type::invalid )
    .expect( {
        {
            U".v..v.....",
            U".│..│.....",
            U".│..├──<..",
            U".│..│.....",
            U".│..│.....",
            U".│..│.....",
            U".│..│.....",
            U".│..│.....",
            U".│..│.....",
            U".└──┴──<.."
        }
    } );
}

TEST_CASE( "subway_gen_s_bend", "[mapgen][connects][subway]" )
{
    static map_helpers::canvas s_bend = {{
            U"#######",
            U"#...###",
            U"###...#",
            U"#######"
        }
    };

    subway_gen_tester( 7, s_bend )
    // Path can take an S-bend
    .run_gen( point( 1, 1 ), om_direction::type::invalid, point( 5, 2 ), om_direction::type::invalid )
    .expect( {{
            U"#######",
            U"#>─┐###",
            U"###└─<#",
            U"#######"
        }
    } );
}

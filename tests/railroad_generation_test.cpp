#include "catch/catch.hpp"

#include "overmap.h"
#include "point.h"
#include "string_formatter.h"
#include "map_setup_helpers.h"

static map_helpers::canvas_legend legend = {{
        { U'.', "field" },
        { U'R', "river_center" },
        { U'#', "rock_border" },
        { U'^', "railroad_end_2_north" },
        { U'>', "railroad_end_2_east" },
        { U'v', "railroad_end_2_south" },
        { U'<', "railroad_end_2_west" },
        { U'│', "railroad_straight_north" },
        { U'─', "railroad_straight_east" },
        { U'┊', "railroad_straight_south" },
        { U'┈', "railroad_straight_west" },
        { U'┃', "railroad_straight_end_north" },
        { U'━', "railroad_straight_end_east" },
        { U'┋', "railroad_straight_end_south" },
        { U'┉', "railroad_straight_end_west" },
        { U'║', "railroad_bridge_north" },
        { U'═', "railroad_bridge_east" },
        { U'╎', "railroad_bridge_south" },
        { U'╌', "railroad_bridge_west" },
        { U'+', "railroad_cross_north" },
        { U'*', "railroad_cross_east" },
        { U'┘', "railroad_diag_north" },
        { U'└', "railroad_diag_east" },
        { U'┌', "railroad_diag_south" },
        { U'┐', "railroad_diag_west" },
        { U'┛', "railroad_diag_end_1_north" },
        { U'┗', "railroad_diag_end_1_east" },
        { U'┏', "railroad_diag_end_1_south" },
        { U'┓', "railroad_diag_end_1_west" },
        { U'╝', "railroad_diag_end_2_north" },
        { U'╚', "railroad_diag_end_2_east" },
        { U'╔', "railroad_diag_end_2_south" },
        { U'╗', "railroad_diag_end_2_west" },
        { U'┩', "railroad_tee_1_north" },
        { U'┺', "railroad_tee_1_east" },
        { U'┢', "railroad_tee_1_south" },
        { U'┱', "railroad_tee_1_west" },
        { U'┡', "railroad_tee_2_north" },
        { U'┲', "railroad_tee_2_east" },
        { U'┪', "railroad_tee_2_south" },
        { U'┹', "railroad_tee_2_west" },
    }
};

class railroad_gen_tester
{
    private:
        int test_num = -1;
        std::unique_ptr<overmap> om;
        map_helpers::canvas_adapter adapter;
        tripoint sz;

    public:
        railroad_gen_tester( int test_num, const map_helpers::canvas &initial ) :
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

        ~railroad_gen_tester() = default;

        railroad_gen_tester &run_gen(
            point start,
            om_direction::type start_dir,
            point dest,
            om_direction::type dest_dir
        ) {
            CAPTURE( test_num );

            const overmap_connection &connection = string_id<overmap_connection>( "railroad_new" ).obj();

            om->build_connection(
                point_om_omt( start.x + 1, start.y + 1 ),
                point_om_omt( dest.x + 1, dest.y + 1 ),
                0,
                connection,
                false,
                start_dir,
                dest_dir
            );

            return *this;
        }

        railroad_gen_tester &expect( const map_helpers::canvas &expected ) {
            CAPTURE( test_num );

            adapter.check_matches_expected( expected, false );

            return *this;
        }
};

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

TEST_CASE( "railroad_gen_straight", "[mapgen][connects][railroad]" )
{
    railroad_gen_tester( 1, empty_10_10 )
    // Horizontal w->e
    .run_gen( point( 1, 1 ), om_direction::type::invalid, point( 3, 1 ), om_direction::type::invalid )
    // Horizontal e->w
    .run_gen( point( 3, 2 ), om_direction::type::invalid, point( 1, 2 ), om_direction::type::invalid )
    // Vertical n->s
    .run_gen( point( 8, 1 ), om_direction::type::invalid, point( 8, 3 ), om_direction::type::invalid )
    // Vertical s->n
    .run_gen( point( 8, 7 ), om_direction::type::invalid, point( 8, 5 ), om_direction::type::invalid )
    // Crossing
    .run_gen( point( 3, 4 ), om_direction::type::invalid, point( 3, 8 ), om_direction::type::invalid )
    .run_gen( point( 1, 6 ), om_direction::type::invalid, point( 5, 6 ), om_direction::type::invalid )
    .expect( {
        {
            U"..........",
            U".>─<....v.",
            U".>─<....│.",
            U"........^.",
            U"...v......",
            U"...│....v.",
            U".>─+─<..│.",
            U"...│....^.",
            U"...^......",
            U".........."
        }
    } );
}

TEST_CASE( "railroad_gen_curves_and_connections", "[mapgen][connects][railroad]" )
{
    railroad_gen_tester( 2, empty_10_10 )
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
            U"..┃......│",
            U".┌╝......│",
            U"┉┛.......│",
            U".........│",
            U"....┏━...│",
            U"...╔┘....│",
            U"...┋.....│",
            U"│........│",
            U"│........│",
            U"*────────+"
        }
    } );
}

TEST_CASE( "railroad_gen_extend", "[mapgen][connects][railroad]" )
{
    railroad_gen_tester( 3, empty_10_10 )
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
            U".>───┈─<..",
            U"..........",
            U"..........",
            U"..........",
            U".>─┉╗.....",
            U"....└┓....",
            U".....┋....",
            U".....^....",
            U".........."
        }
    } );
}

TEST_CASE( "railroad_gen_join", "[mapgen][connects][railroad]" )
{
    railroad_gen_tester( 4, empty_10_10 )
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
            U".>┉╗v.....",
            U"...└┪.....",
            U"....┋┏━<..",
            U"....┢┘....",
            U"....┋.....",
            U"....┃.....",
            U"...┌┩.....",
            U".>┉┛┃.....",
            U"....┡┐....",
            U"....^╚━<.."
        }
    } );
}

TEST_CASE( "railroad_gen_join_diag", "[mapgen][connects][railroad]" )
{
    railroad_gen_tester( 4, {
        {
            U"...#......",
            U"#.........",
            U"..........",
            U".........#",
            U"..........",
            U"..........",
            U".........#",
            U"..........",
            U"..........",
            U"........#."
        }
    } )
    // Diagonal nw->se
    .run_gen( point( 0, 0 ), om_direction::type::invalid, point( 9, 9 ), om_direction::type::invalid )
    // Joined by w->se
    .run_gen( point( 0, 2 ), om_direction::type::invalid, point( 9, 9 ), om_direction::type::invalid )
    // Joined by s->nw
    .run_gen( point( 7, 9 ), om_direction::type::invalid, point( 0, 0 ), om_direction::type::invalid )
    // Joined by e->nw
    .run_gen( point( 9, 4 ), om_direction::type::invalid, point( 0, 0 ), om_direction::type::invalid )
    .expect( {
        {
            U">┉╗#......",
            U"#.└┐......",
            U">──┺╗.....",
            U"....└┐...#",
            U".....╚┱──<",
            U"......└┓..",
            U".......┡┐#",
            U".......│└┓",
            U".......│.┋",
            U".......^#^"
        }
    } );
}

TEST_CASE( "railroad_gen_no_self_crossing", "[mapgen][connects][railroad]" )
{
    static map_helpers::canvas empty_10_10_4_blocked = {{
            U"#.#.......",
            U"..........",
            U"#.#.......",
            U"..........",
            U"..........",
            U"..........",
            U"..........",
            U"..........",
            U"..........",
            U".........."
        }
    };

    railroad_gen_tester( 5, empty_10_10_4_blocked )
    // Path can't cross itself
    .run_gen( point( 1, 0 ), om_direction::type::invalid, point( 0, 1 ), om_direction::type::invalid )
    .expect( empty_10_10_4_blocked );
}

TEST_CASE( "railroad_gen_no_hard_turns", "[mapgen][connects][railroad]" )
{
    static map_helpers::canvas hard_turn = {{
            U"#######",
            U"#.....#",
            U"#.###.#",
            U"#.###.#",
            U"#.###.#",
            U"#.....#",
            U"#######",
        }
    };

    railroad_gen_tester( 6, hard_turn )
    // Path can't take hard turns
    .run_gen( point( 1, 1 ), om_direction::type::invalid, point( 5, 5 ), om_direction::type::invalid )
    .run_gen( point( 5, 1 ), om_direction::type::invalid, point( 1, 5 ), om_direction::type::invalid )
    .run_gen( point( 5, 5 ), om_direction::type::invalid, point( 1, 1 ), om_direction::type::invalid )
    .run_gen( point( 1, 5 ), om_direction::type::invalid, point( 5, 1 ), om_direction::type::invalid )
    .expect( hard_turn );
}

TEST_CASE( "railroad_gen_s_bend", "[mapgen][connects][railroad]" )
{
    static map_helpers::canvas s_bend = {{
            U"#######",
            U"#...###",
            U"###...#",
            U"#######"
        }
    };

    railroad_gen_tester( 7, s_bend )
    // Path can take an S-bend
    .run_gen( point( 1, 1 ), om_direction::type::invalid, point( 5, 2 ), om_direction::type::invalid )
    .expect( {{
            U"#######",
            U"#>┉╗###",
            U"###╚━<#",
            U"#######"
        }
    } );
}

TEST_CASE( "railroad_gen_bridges", "[mapgen][connects][railroad]" )
{
    static map_helpers::canvas riverside = {{
            U"..........",
            U"..........",
            U"..........",
            U"..........",
            U"RRRRRRRRRR",
            U"RRRRRRRRRR",
            U".....RRRRR",
            U".....RRRRR",
            U".....RRRRR",
            U".....RRRRR"
        }
    };

    railroad_gen_tester( 7, riverside )
    // When railroad crosses river, it turns to bridge
    .run_gen( point( 2, 1 ), om_direction::type::invalid, point( 2, 8 ), om_direction::type::invalid )
    // If railroad starts over water, build bridge to land
    .run_gen( point( 8, 9 ), om_direction::type::north, point( 8, 2 ), om_direction::type::invalid )
    // If railroad end over water, build bridge to land
    .run_gen( point( 6, 2 ), om_direction::type::invalid, point( 6, 9 ), om_direction::type::south )
    .expect( {{
            U"..........",
            U"..v.......",
            U"..│...v.v.",
            U"..│...│.│.",
            U"RR║RRR║R║R",
            U"RR║RRR║R║R",
            U"..│..R║R║R",
            U"..│..R║R║R",
            U"..^..R║R║R",
            U".....R║R║R"
        }
    } );
}

TEST_CASE( "railroad_gen_no_bridge_crossing", "[mapgen][connects][railroad]" )
{
    static map_helpers::canvas lake_4_sides = {{
            U".....#.....",
            U".....#.....",
            U".....#.....",
            U"...RRRRR...",
            U"...RRRRR...",
            U"###RRRRR###",
            U"...RRRRR...",
            U"...RRRRR...",
            U".....#.....",
            U".....#.....",
            U".....#....."
        }
    };

    railroad_gen_tester( 8, lake_4_sides )
    // 2 bridges can't intersect
    .run_gen( point( 4, 1 ), om_direction::type::invalid, point( 4, 9 ), om_direction::type::invalid )
    .run_gen( point( 1, 4 ), om_direction::type::invalid, point( 9, 4 ), om_direction::type::invalid )
    .expect( {{
            U".....#.....",
            U"....v#.....",
            U"....│#.....",
            U"...R║RRR...",
            U"...R║RRR...",
            U"###R║RRR###",
            U"...R║RRR...",
            U"...R║RRR...",
            U"....│#.....",
            U"....^#.....",
            U".....#....."
        }
    } );
}

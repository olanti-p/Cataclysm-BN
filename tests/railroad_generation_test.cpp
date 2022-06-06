#include "catch/catch.hpp"

#include "overmap.h"
#include "point.h"
#include "string_formatter.h"

static std::unordered_map<char32_t, std::string> char_legend = {{
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

using canvas = std::vector<std::u32string>;

static point get_canvas_size( const canvas &c )
{
    if( c.empty() ) {
        return point_zero;
    } else {
        return point(
                   static_cast<int>( c[0].size() ),
                   static_cast<int>( c.size() )
               );
    }
}

static void validate_size( const canvas &c, const point &sz )
{
    REQUIRE( static_cast<int>( c.size() ) == sz.y );
    for( const auto &line : c ) {
        REQUIRE( static_cast<int>( line.size() ) == sz.x );
    }
}

static std::string fmt_canvas( const canvas &c )
{
    std::string res;
    res += "\n";
    for( const auto &line : c ) {
        res += utf32_to_utf8( line );
        res += "\n";
    }
    return res;
}

static void check_equals( const overmap &om, const point &sz, const canvas &expected,
                          bool require )
{
    struct entry {
        int x;
        int y;
        std::string ter_id;
        std::string expected_id;
    };
    std::vector<entry> fails;
    for( int y = 0; y < sz.y; y++ ) {
        const auto &line = expected[y];
        for( int x = 0; x < sz.x; x++ ) {
            oter_id ter = om.ter( tripoint_om_omt( x + 1, y + 1, 0 ) );
            const std::string &ter_id = ter->id.str();
            const std::string &expected_id = char_legend[line[x]];
            if( ter_id != expected_id ) {
                fails.push_back( { x, y, ter_id, expected_id} );
            }
        }
    }

    if( !fails.empty() ) {
        canvas cr;

        for( int y = 0; y < sz.y; y++ ) {
            cr.emplace_back();
            for( int x = 0; x < sz.x; x++ ) {
                char32_t c = U'?';
                const std::string &ter_id = om.ter( tripoint_om_omt( x + 1, y + 1, 0 ) )->id.str();
                for( const auto &it : char_legend ) {
                    if( it.second == ter_id ) {
                        c = it.first;
                        break;
                    }
                }
                cr.back().push_back( c );
            }
        }

        CAPTURE( fmt_canvas( expected ) );
        CAPTURE( fmt_canvas( cr ) );
        CAPTURE( fails.size() );

        for( const entry &e : fails ) {
            cata_print_stdout( string_format( "(%d,%d) exp:%s got:%s\n", e.x, e.y, e.expected_id, e.ter_id ) );
        }
        if( require ) {
            FAIL();
        } else {
            FAIL_CHECK();
        }
    } else {
        SUCCEED();
    }
}

class railroad_gen_tester
{
    private:
        int test_num = -1;
        overmap om;
        point sz;

    public:
        railroad_gen_tester( int test_num, const canvas &initial ) :
            test_num( test_num ), om( point_abs_om( 0, 0 ) ) {
            CAPTURE( test_num );

            sz = get_canvas_size( initial );
            validate_size( initial, sz );

            oter_str_id block_str( "rock_border" );
            oter_id block = block_str.id();

            for( int x = 0; x < sz.x + 2; x++ ) {
                om.ter_set( tripoint_om_omt( x, 0, 0 ), block );
                om.ter_set( tripoint_om_omt( x, sz.y + 1, 0 ), block );
            }
            for( int y = 0; y < sz.y + 2; y++ ) {
                om.ter_set( tripoint_om_omt( 0, y, 0 ), block );
                om.ter_set( tripoint_om_omt( sz.x + 1, y, 0 ), block );
            }

            for( int y = 0; y < sz.y; y++ ) {
                const auto &line = initial[y];
                for( int x = 0; x < sz.x; x++ ) {
                    oter_str_id this_id( char_legend[line[x]] );
                    om.ter_set( tripoint_om_omt( x + 1, y + 1, 0 ), this_id.id() );
                }
            }

            check_equals( om, sz, initial, true );
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

            om.build_connection(
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

        railroad_gen_tester &expect( const canvas &expected ) {
            CAPTURE( test_num );

            validate_size( expected, sz );
            check_equals( om, sz, expected, false );

            return *this;
        }
};

TEST_CASE( "railroad_gen", "[mapgen][connects]" )
{
    static canvas empty_10_10 = {{
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

    static canvas empty_10_10_4_blocked = {{
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

    static canvas hard_turn = {{
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

    static canvas s_bend = {{
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

    static canvas riverside = {{
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

    static canvas lake_4_sides = {{
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

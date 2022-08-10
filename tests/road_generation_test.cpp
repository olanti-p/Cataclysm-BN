#include "catch/catch.hpp"

#include "overmap.h"
#include "point.h"
#include "string_formatter.h"
#include "map_setup_helpers.h"

static map_helpers::canvas_legend legend = {{
        { U'.', "field" },
        { U'R', "river_center" },
        { U'#', "rock_border" },
        { U'^', "road_end_north" },
        { U'>', "road_end_east" },
        { U'v', "road_end_south" },
        { U'<', "road_end_west" },
        { U'│', "road_ns" },
        { U'─', "road_ew" },
        { U'+', "road_nesw" }
    }
};

class road_gen_tester
{
    private:
        int test_num = -1;
        std::unique_ptr<overmap> om;
        map_helpers::canvas_adapter adapter;
        tripoint sz;

    public:
        road_gen_tester( int test_num, const map_helpers::canvas &initial ) :
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

        ~road_gen_tester() = default;

        road_gen_tester &run_gen(
            point start,
            om_direction::type start_dir,
            point dest,
            om_direction::type dest_dir
        ) {
            CAPTURE( test_num );

            const overmap_connection &connection = string_id<overmap_connection>( "local_road" ).obj();

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

        road_gen_tester &expect( const map_helpers::canvas &expected ) {
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

TEST_CASE( "road_gen_straight", "[mapgen][connects][road]" )
{
    road_gen_tester( 1, empty_10_10 )
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

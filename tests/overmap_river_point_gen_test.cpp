#include "catch/catch.hpp"

#include <array>
#include <vector>

#include "coordinates.h"
#include "overmap_generation.h"
#include "regional_settings.h"
#include "rng_seedable.h"
#include "rng.h"
#include "stringmaker.h"

// Generates seed from global generator initialized from Catch session seed
static u64 generate_seed_from_session()
{
    return rng_bits();
}

/*
static RNG get_overmap_rng( u64 wg_seed, const point_abs_om &p )
{
    u64 seed = wg_seed;
    hash_combine( seed, p );
    return RNG( seed );
}
*/

struct river_points {
    std::vector<int> n;
    std::vector<int> e;
    std::vector<int> s;
    std::vector<int> w;
};

/*
constexpr int river_spacing = 300;
constexpr int river_pos_varience = 50;

static point_abs_omt gen_river_offset( u64 seed, const point_abs_omt &orig_river_p )
{
    om_noise::om_noise_layer_river noise( orig_river_p, seed );
    float d = noise.noise_at( point_om_omt( 0, 0 ) );
    int candidate_y = orig_river_p.y() + static_cast<int>( d * river_pos_varience );

    return orig_river_p + point_rel_omt( 0, candidate_y );
};

static std::vector<int> get_intersecting_river_templates( const point_abs_om &p )
{
    // Get NW point in abs coords
    point_abs_omt p_omt = coords::project_combine( p, point_om_omt( 0, 0 ) );
    // Get coords of river intersections on W border that could intersect this overmap
    int p_omt_y_min = p_omt.y() - OMAPX - river_pos_varience - 1;
    int p_omt_y_max = p_omt.y() + OMAPY + river_pos_varience + 1;

    int y_min_proj_x0 = p_omt_y_min - OMAPX * p.x();
    int y_max_proj_x0 = p_omt_y_max - OMAPX * p.x();

    if( y_min_proj_x0 > y_max_proj_x0 ) {
        std::swap( y_min_proj_x0, y_max_proj_x0 );
    }

    int yr_min = y_min_proj_x0 / river_spacing;
    if( y_min_proj_x0 < 0 ) {
        yr_min -= 1;
    }
    int yr_max = y_max_proj_x0 / river_spacing;
    if( y_max_proj_x0 > 0 ) {
        yr_max += 1;
    }

    std::vector<int> ret;
    for( int yr = yr_min; yr <= yr_max; yr++ ) {
        ret.push_back( yr );
    }
    return ret;
};

static std::vector<std::pair<point_om_omt, point_om_omt>> get_potential_intersections( u64 seed,
        const point_abs_om &p )
{
    std::vector<std::pair<point_om_omt, point_om_omt>> ret;

    std::vector<int> templs = get_intersecting_river_templates( p );

    for( int ry : templs ) {
        point_rel_omt x_step_size( OMAPX, OMAPX );
        point_abs_omt riv_start( 0, ry * river_spacing );
        point_abs_omt p_w = riv_start + x_step_size * p.x();
        point_abs_omt p_e = p_w + x_step_size;

        point_abs_omt p_w_sh = gen_river_offset( seed, p_w );
        point_abs_omt p_e_sh = gen_river_offset( seed, p_e );

        point_abs_om aom_w;
        point_om_omt omt_w;
        std::tie( aom_w, omt_w ) = coords::project_remain<coords::om>( p_w_sh );
        point w_raw = omt_w.raw() + ( aom_w - p ).raw() * OMAPX;

        point_abs_om aom_e;
        point_om_omt omt_e;
        std::tie( aom_e, omt_e ) = coords::project_remain<coords::om>( p_e_sh );
        point e_raw = omt_e.raw() + ( aom_e - p ).raw() * OMAPY;
        e_raw.x -= 1;

        if( ( w_raw.y < 0 && e_raw.y < 0 ) || ( w_raw.y >= OMAPY && e_raw.y >= OMAPY ) ) {
            // River does not intersect this overmap
            continue;
        }

        if( w_raw.y > 0 && w_raw.y < OMAPY && e_raw.y > 0 && e_raw.y < OMAPY ) {
            // River goes through this overmap from east to west,
            // no additional calculations required.
        } else {
            // Calculate where river intersects north/south border(s) of the overmap
            // by building line equation for the river and then solvng it for
            // north and/or south border(s).
            // y = k * x + b
            constexpr int x1 = 0;
            constexpr int x2 = OMAPX;
            // Normalize y1 and y2 to avoid floating-point math inconsistencies between
            // overmaps that are intersected by this river segment.
            const int delta_y = divide_round_to_minus_infinity( w_raw.y, OMAPY ) * OMAPY;
            const int y1 = w_raw.y - delta_y;
            const int y2 = e_raw.y - delta_y;
            // y1 = k * x1 + b
            // y2 = k * x2 + b
            // y2 - y1 = k * ( x2 - x1)
            // k = (y2 - y1) / ( x2 - x1 )
            const float k = static_cast<float>( y2 - y1 ) / static_cast<float>( x2 - x1 );
            // b = y1 - k * x1
            const float b = static_cast<float>( y1 ) - k * static_cast<float>( x1 );

            if( w_raw.y < 0 ) {
                w_raw.y = 0;
                // x = ( y - b ) / k
                w_raw.x = std::trunc( ( -delta_y - b ) / k );
            }
            if( e_raw.y >= OMAPY ) {
                e_raw.y = OMAPY - 1;
                // x = ( y - b ) / k
                e_raw.x = std::trunc( ( static_cast<float>( OMAPY - delta_y ) - b ) / k );
            }

            ( void )b;
        }

        ret.emplace_back( point_om_omt( w_raw ), point_om_omt( e_raw ) );
    }

    return ret;
};
*/

static river_points gen_river_points( u64 wg_seed, const point_abs_om &p )
{
    river_points ret;

    river_settings spec;
    spec.spacing = 120;
    spec.pos_variance = 50;

    std::vector<river_data> rivs = gen_rivers( wg_seed, spec, p );

    const auto add_pt = [&]( const point_om_omt & p ) {
        if( p.x() == 0 ) {
            ret.w.push_back( p.y() );
        } else if( p.y() == 0 ) {
            ret.n.push_back( p.x() );
        } else if( p.x() == OMAPX - 1 ) {
            ret.e.push_back( p.y() );
        } else {
            ret.s.push_back( p.x() );
        }
    };

    for( const river_data &riv : rivs ) {
        add_pt( riv.start );
        add_pt( riv.end );
    }

    return ret;
}

static void check_river_points_for( u64 wg_seed, const point_abs_om &origin )
{
    CAPTURE( origin );

    river_points pts = gen_river_points( wg_seed, origin );
    river_points pts_n = gen_river_points( wg_seed, origin + point_north );
    river_points pts_s = gen_river_points( wg_seed, origin + point_south );
    river_points pts_e = gen_river_points( wg_seed, origin + point_east );
    river_points pts_w = gen_river_points( wg_seed, origin + point_west );

    CAPTURE( pts.n );
    CAPTURE( pts.e );
    CAPTURE( pts.s );
    CAPTURE( pts.w );

    CAPTURE( pts_n.n );
    CAPTURE( pts_n.e );
    CAPTURE( pts_n.s );
    CAPTURE( pts_n.w );

    CAPTURE( pts_e.n );
    CAPTURE( pts_e.e );
    CAPTURE( pts_e.s );
    CAPTURE( pts_e.w );

    CAPTURE( pts_s.n );
    CAPTURE( pts_s.e );
    CAPTURE( pts_s.s );
    CAPTURE( pts_s.w );

    CAPTURE( pts_w.n );
    CAPTURE( pts_w.e );
    CAPTURE( pts_w.s );
    CAPTURE( pts_w.w );

    // Adjacent overmaps must share same points
    REQUIRE( pts.n == pts_n.s );
    REQUIRE( pts.s == pts_s.n );
    REQUIRE( pts.e == pts_e.w );
    REQUIRE( pts.w == pts_w.e );

    // The overmap must have either no river points at all
    // or at least 1 start and 1 exit point
    size_t num_start = pts.n.size() + pts.w.size();
    size_t num_exit = pts.s.size() + pts.e.size();
    CAPTURE( num_start );
    CAPTURE( num_exit );
    REQUIRE( ( ( num_start == 0 && num_exit == 0 ) || ( num_start != 0 && num_exit != 0 ) ) );

    // River points should not be close to overmap corners
    constexpr int EXP_MIN = 10;
    constexpr int EXP_MAX = OMAPX - 10;
    for( int x : pts.n ) {
        REQUIRE( x >= EXP_MIN );
        REQUIRE( x <= EXP_MAX );
    }
    for( int x : pts.s ) {
        REQUIRE( x >= EXP_MIN );
        REQUIRE( x <= EXP_MAX );
    }
    for( int y : pts.e ) {
        REQUIRE( y >= EXP_MIN );
        REQUIRE( y <= EXP_MAX );
    }
    for( int y : pts.w ) {
        REQUIRE( y >= EXP_MIN );
        REQUIRE( y <= EXP_MAX );
    }
}

TEST_CASE( "overmap_river_point_generation", "[overmapgen]" )
{
    constexpr u64 WG_SEED = 1337;

    CAPTURE( WG_SEED );

    // Check central overmap, it's important to get right
    SECTION( "central overmap" ) {
        check_river_points_for( WG_SEED, point_abs_om( 0, 0 ) );
    }
    // Check neighbours of central overmap, those are the starting area and are important as well
    SECTION( "starting area" ) {
        for (int x = -10; x <= 10; x++) {
            for (int y = -10; y <= 10; y++) {
                if (x == 0 && y == 0) {
                    continue;
                }
                point_abs_om origin( x, y );
                check_river_points_for( WG_SEED, origin );
            }
        }
        /*
        check_river_points_for( WG_SEED, point_abs_om( 1, 1 ) );
        check_river_points_for( WG_SEED, point_abs_om( 1, 0 ) );
        check_river_points_for( WG_SEED, point_abs_om( 1, -1 ) );
        check_river_points_for( WG_SEED, point_abs_om( 0, 1 ) );
        check_river_points_for( WG_SEED, point_abs_om( 0, -1 ) );
        check_river_points_for( WG_SEED, point_abs_om( -1, 1 ) );
        check_river_points_for( WG_SEED, point_abs_om( -1, 0 ) );
        check_river_points_for( WG_SEED, point_abs_om( -1, -1 ) );
        */
    }
    // Randomly check some far-off overmaps
    SECTION( "random check" ) {
        RNG rng_setup( generate_seed_from_session() );
        for( int i = 0; i < 10; i++ ) {
            int x = rng_setup.gen_int( -1000, 1000 );
            int y = rng_setup.gen_int( -1000, 1000 );
            point_abs_om origin( x, y );
            check_river_points_for( WG_SEED, origin );
        }
    }
}

/*
TEST_CASE( "overmap_river_point_generation_2", "[overmapgen]" )
{
    constexpr u64 WG_SEED = 1337;

    CAPTURE( "offset at x=0" );
    CAPTURE( gen_river_offset( WG_SEED, point_abs_omt( 0, 100 ) ) );
    CAPTURE( gen_river_offset( WG_SEED, point_abs_omt( 0, 200 ) ) );
    CAPTURE( gen_river_offset( WG_SEED, point_abs_omt( 0, 300 ) ) );
    CAPTURE( gen_river_offset( WG_SEED, point_abs_omt( 0, 400 ) ) );
    CAPTURE( gen_river_offset( WG_SEED, point_abs_omt( 0, 500 ) ) );
    CAPTURE( gen_river_offset( WG_SEED, point_abs_omt( 0, 600 ) ) );
    CAPTURE( gen_river_offset( WG_SEED, point_abs_omt( 0, 700 ) ) );
    CAPTURE( gen_river_offset( WG_SEED, point_abs_omt( 0, 800 ) ) );

    CAPTURE( "offset at x=100" );
    CAPTURE( gen_river_offset( WG_SEED, point_abs_omt( 100, 100 ) ) );
    CAPTURE( gen_river_offset( WG_SEED, point_abs_omt( 100, 200 ) ) );
    CAPTURE( gen_river_offset( WG_SEED, point_abs_omt( 100, 300 ) ) );
    CAPTURE( gen_river_offset( WG_SEED, point_abs_omt( 100, 400 ) ) );
    CAPTURE( gen_river_offset( WG_SEED, point_abs_omt( 100, 500 ) ) );
    CAPTURE( gen_river_offset( WG_SEED, point_abs_omt( 100, 600 ) ) );
    CAPTURE( gen_river_offset( WG_SEED, point_abs_omt( 100, 700 ) ) );
    CAPTURE( gen_river_offset( WG_SEED, point_abs_omt( 100, 800 ) ) );

    CAPTURE( "dx=0 dy=0" );
    CAPTURE( get_potential_intersections( WG_SEED, point_abs_om( -2, -2 ) ) );
    CAPTURE( get_potential_intersections( WG_SEED, point_abs_om( -1, -1 ) ) );
    CAPTURE( get_potential_intersections( WG_SEED, point_abs_om( 0, 0 ) ) );
    CAPTURE( get_potential_intersections( WG_SEED, point_abs_om( 1, 1 ) ) );
    CAPTURE( get_potential_intersections( WG_SEED, point_abs_om( 2, 2 ) ) );

    CAPTURE( "dx=+1 dy=0" );
    CAPTURE( get_potential_intersections( WG_SEED, point_abs_om( -1, -2 ) ) );
    CAPTURE( get_potential_intersections( WG_SEED, point_abs_om( 0, -1 ) ) );
    CAPTURE( get_potential_intersections( WG_SEED, point_abs_om( 1, 0 ) ) );
    CAPTURE( get_potential_intersections( WG_SEED, point_abs_om( 2, 1 ) ) );
    CAPTURE( get_potential_intersections( WG_SEED, point_abs_om( 3, 2 ) ) );

    CAPTURE( "dx=0 dy=+1" );
    CAPTURE( get_potential_intersections( WG_SEED, point_abs_om( -2, -1 ) ) );
    CAPTURE( get_potential_intersections( WG_SEED, point_abs_om( -1, 0 ) ) );
    CAPTURE( get_potential_intersections( WG_SEED, point_abs_om( 0, 1 ) ) );
    CAPTURE( get_potential_intersections( WG_SEED, point_abs_om( 1, 2 ) ) );
    CAPTURE( get_potential_intersections( WG_SEED, point_abs_om( 2, 3 ) ) );

    FAIL();
}
*/

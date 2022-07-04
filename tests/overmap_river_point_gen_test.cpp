#include "catch/catch.hpp"

#include <array>
#include <vector>

#include "rng_seedable.h"
#include "coordinates.h"

static RNG get_overmap_rng( u64 wg_seed, const point_abs_om &p )
{
    u64 seed = wg_seed;
    hash_combine( seed, p );
    return RNG( seed );
}

struct river_points {
    std::vector<int> n;
    std::vector<int> e;
    std::vector<int> s;
    std::vector<int> w;
};

static river_points gen_river_points( u64 wg_seed, const point_abs_om &p )
{
    river_points ret;

    int riv_chance_x = 7;
    int riv_chance_y = 7;

    RNG rng_n = get_overmap_rng( wg_seed, p );
    RNG rng_e = get_overmap_rng( wg_seed, p );
    RNG rng_s = get_overmap_rng( wg_seed, p + point_south );
    RNG rng_w = get_overmap_rng( wg_seed, p + point_west );

    const auto gen_nw_points = [ = ]( RNG & rng ) {
        std::array<std::vector<int>, 2> points;

        std::vector<std::pair<int, int>> candidates;

        for( ;; ) {
            if( rng.x_in_y( riv_chance_x, riv_chance_y ) ) {
                // North point
                candidates.emplace_back( 0, rng.gen_int( 10, OMAPX - 11 ) );
            }
            if( rng.x_in_y( riv_chance_x, riv_chance_y ) ) {
                // West point
                candidates.emplace_back( 1, rng.gen_int( 10, OMAPY - 11 ) );
            }
            if( candidates.empty() ) {
                continue;
            }
            std::pair<int, int> res = rng.random_entry_removed( candidates );
            points[res.first].push_back( res.second );
            break;
        }

        return points;
    };

    const auto gen_north = [ = ]( RNG & rng ) {
        return gen_nw_points( rng )[0];
    };
    const auto gen_west = [ = ]( RNG & rng ) {
        return gen_nw_points( rng )[1];
    };

    ret.w = gen_west( rng_w );
    ret.n = gen_north( rng_n );
    ret.s = gen_north( rng_s );
    ret.e = gen_west( rng_e );

    return ret;
}

TEST_CASE( "overmap_river_point_generation", "[overmapgen]" )
{
    constexpr u64 SEED = 1337;

    river_points pts = gen_river_points( SEED, point_abs_om( 0, 0 ) );
    river_points pts_n = gen_river_points( SEED, point_abs_om( 1, 0 ) );
    river_points pts_s = gen_river_points( SEED, point_abs_om( -1, 0 ) );
    river_points pts_e = gen_river_points( SEED, point_abs_om( 0, 1 ) );
    river_points pts_w = gen_river_points( SEED, point_abs_om( 0, -1 ) );



}

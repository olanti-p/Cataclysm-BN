#include "overmap_generation.h"
#include "regional_settings.h"
#include "overmap_noise.h"

/**
 * RIVERS
 *
 * The goal of river generator is to provide overmapgen with entry and exit points for rivers
 * within the overmap under the following rules:
 *   1. Generation should only be affected by world seed and settings
 *   2. Any 2 adjacent overmaps should have their rivers' entry/exit points match up
 *   3. River entry/exit points should not be very close to overmap corners
 *   4. Some overmaps should be able to have no rivers
 *   5. Player should be able to tweak the generation:
 *      - Specify average amount of rivers per overmap as non-integer value
 *      - Specify how likely rivers are to intersect
 *      - Disable rivergen completely
 *
 *
 *
 * First rivers are generated as lines on global scale, described by the equation
 *   y = x + riv_idx * river_spacing
 * where
 *   y and x are absolute overmaptile coords of the river
 *   riv_idx is an integer number accociated with the river
 *   river_spacing is the [river_spacing] value received from region settings
 *
 * Then the lines are broken up into segments wherever they cross vertical
 * boundaries of overmaps ( x = OMAPX * n ), and each segment has its
 * y coord adjusted by no more than [pos_variance] according to global noise,
 * and then a bit more to keep away from overmap corners.
 *
 * That means that if [pos_variance] is higher than [river_spacing] / 2,
 * the rivers have a chance to intersect.
 *
 * After that each segment is additionally broken up wherever it crosses horizontal
 * boundaries of overmaps ( y = OMAPY * n ), and each segment (including new ones)
 * has its x coord adjacent a bit to keep away from overmap corners.
 *
 * Now, each segment fits in an overmap, ends at overmap borders far enough from overmap corners
 * and has 2 connected segments in adjacent overmaps - exactly what we need.
 */

// Minimum distance from overmap corner to river start/end point
constexpr int RIVER_OM_CORNER_SPACING = 10;

static int offset_away_from_corvers( int val )
{
    return clamp( val, RIVER_OM_CORNER_SPACING, OMAPY - RIVER_OM_CORNER_SPACING );
}

static point_abs_omt offset_river_segment_y( u64 seed, const river_settings &spec,
        const point_abs_omt &orig_river_p )
{
    om_noise::om_noise_layer_river noise( orig_river_p, seed );
    const float d = noise.noise_at( point_om_omt( 0, 0 ) );
    const int dy = static_cast<int>( d * spec.pos_variance );
    point_abs_omt candidate = orig_river_p + point_rel_omt( 0, orig_river_p.y() + dy );

    // Ensure we're far enough from overmap corners
    point_abs_om global;
    point_om_omt local;
    std::tie( global, local ) = coords::project_remain<coords::om>( candidate );
    local.y() = offset_away_from_corvers( local.y() );

    return project_combine( global, local );
};

static std::vector<int> get_intersecting_river_lines( const river_settings &spec,
        const point_abs_om &p )
{
    // Get NW point in abs coords
    point_abs_omt p_omt = coords::project_combine( p, point_om_omt( 0, 0 ) );
    // Get coords of river intersections on W border that could intersect this overmap
    const int p_omt_y_min = p_omt.y() - OMAPX - spec.pos_variance - 1;
    const int p_omt_y_max = p_omt.y() + OMAPY + spec.pos_variance + 1;

    int y_min_proj_x0 = p_omt_y_min - OMAPX * p.x();
    int y_max_proj_x0 = p_omt_y_max - OMAPX * p.x();

    if( y_min_proj_x0 > y_max_proj_x0 ) {
        std::swap( y_min_proj_x0, y_max_proj_x0 );
    }

    const int yr_min = divide_round_to_minus_infinity( y_min_proj_x0, spec.spacing ) - 1;
    const int yr_max = divide_round_to_minus_infinity( y_max_proj_x0, spec.spacing ) + 1;

    std::vector<int> ret;
    for( int yr = yr_min; yr <= yr_max; yr++ ) {
        ret.push_back( yr );
    }
    return ret;
};

std::vector<river_data> gen_rivers( u64 wg_seed, const river_settings &spec,
                                    const point_abs_om &p )
{
    std::vector<river_data> ret;

    if( spec.spacing == 0 ) {
        return ret;
    }

    std::vector<int> templs = get_intersecting_river_lines( spec, p );

    for( int ry : templs ) {
        point_rel_omt x_step_size( OMAPX, OMAPX );
        point_abs_omt riv_start( 0, ry * spec.spacing );
        point_abs_omt p_w = riv_start + x_step_size * p.x();
        point_abs_omt p_e = p_w + x_step_size;

        point_abs_omt p_w_sh = offset_river_segment_y( wg_seed, spec, p_w );
        point_abs_omt p_e_sh = offset_river_segment_y( wg_seed, spec, p_e );

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
                w_raw.x = offset_away_from_corvers( std::trunc(
                                                        ( -delta_y - b ) / k
                                                    ) );
            }
            if( e_raw.y >= OMAPY ) {
                e_raw.y = OMAPY - 1;
                // x = ( y - b ) / k
                e_raw.x = offset_away_from_corvers( std::trunc(
                                                        ( static_cast<float>( OMAPY - delta_y ) - b ) / k
                                                    ) );
            }
        }

        river_data riv;
        riv.start = point_om_omt( w_raw );
        riv.end = point_om_omt( e_raw );
        ret.emplace_back( std::move( riv ) );
    }

    return ret;
};

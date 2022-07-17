#pragma once
#ifndef CATA_SRC_OVERMAP_GENERATION_H
#define CATA_SRC_OVERMAP_GENERATION_H

#include "coordinates.h"
#include "integer.h"

struct river_settings;

struct river_data {
    point_om_omt start;
    point_om_omt end;
};

/**
 * @brief Generate rough river data for given overmap.
 *
 * @param wg_seed Worldgen seed
 * @param spec Worldgen river settings
 * @param p Overmap to generate for
 * @return std::vector<river_data> List of produced rivers
 */
std::vector<river_data> gen_rivers( u64 wg_seed, const river_settings &spec,
                                    const point_abs_om &p );

#endif // CATA_SRC_OVERMAP_GENERATION_H

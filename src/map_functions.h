#pragma once
#ifndef CATA_SRC_MAP_FUNCTIONS_H
#define CATA_SRC_MAP_FUNCTIONS_H

#include "coordinates.h"

struct tripoint;
class map;
class submap;

namespace map_funcs
{

/**
 * Checks both the neighborhoods of from and to for climbable surfaces,
 * returns move cost of climbing from `from` to `to`.
 * 0 means climbing is not possible.
 * Return value can depend on the orientation of the terrain.
 */
int climbing_cost( const map &m, const tripoint &from, const tripoint &to );

void migo_nerve_cage_removal( map &m, const tripoint &p, bool spawn_damaged );

/** Fetch or generate submap at given point. */
submap *fetch_or_generate_submap( const tripoint_abs_sm &p );

} // namespace map_funcs

#endif // CATA_SRC_MAP_FUNCTIONS_H

#ifndef CATA_SRC_EDITOR_ALGO_H
#define CATA_SRC_EDITOR_ALGO_H

#include "uuid.h"
#include "canvas_2d.h"

#include <vector>

namespace editor
{

/**
 * Find all tiles that match predicate.
*/
std::vector<point> find_tiles_via_global(
    const Canvas2D<UUID> &canvas,
    std::function<bool( point p, const UUID & )> predicate
);

/**
 * Find via floodfill all tiles that match predicate.
*/
std::vector<point> find_tiles_via_floodfill(
    const Canvas2D<UUID> &canvas,
    const point &initial_pos,
    std::function<bool( point p, const UUID & )> predicate
);

} // namespace editor

#endif // CATA_SRC_EDITOR_ALGO_H

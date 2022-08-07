#pragma once
#ifndef CATA_SRC_SDLTILES_EDITOR_H
#define CATA_SRC_SDLTILES_EDITOR_H

#include <utility>

#include "optional.h"
#include "point.h"

namespace editor
{
/**
 * @brief Convert screen coords to tile pos.
 */
cata::optional<tripoint> screen_to_tile( point mouse_pos );

/**
 * @brief Convert tile pos to bounding screen coords.
 */
std::pair<point, point> tile_to_screen( point pos );

} // namespace editor

#endif // CATA_SRC_SDLTILES_EDITOR_H

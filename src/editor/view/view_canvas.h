#ifndef CATA_SRC_EDITOR_VIEW_CANVAS_H
#define CATA_SRC_EDITOR_VIEW_CANVAS_H

#include "coordinates.h"

#include "imgui.h"

namespace editor
{
struct State;
struct Camera;
struct Mapgen;

/**
 * ============ Mouse helpers ============
 */
point_abs_screen get_mouse_pos();
point_abs_etile get_mouse_tile_pos( const Camera &cam );

/**
 * ========== Rendering helpers ==========
 */
void draw_frame(
    ImDrawList *draw_list,
    const Camera &cam,
    const point_abs_etile &p1,
    const point_abs_etile &p2,
    ImVec4 col,
    bool filled
);
void highlight_tile(
    ImDrawList *draw_list,
    const Camera &cam,
    point_abs_etile tile,
    ImVec4 col
);
void fill_tile(
    ImDrawList *draw_list,
    const Camera &cam,
    point_abs_etile tile,
    ImVec4 col
);
void highlight_region(
    ImDrawList *draw_list,
    const Camera &cam,
    point_abs_etile p1,
    point_abs_etile p2,
    ImVec4 col_bg,
    ImVec4 col_border
);
void fill_region(
    ImDrawList *draw_list,
    const Camera &cam,
    point_abs_etile p1,
    point_abs_etile p2,
    ImVec4 col
);

/**
 * =============== Windows ===============
 */
void show_canvas( State &state, Mapgen *mapgen_ptr );

} // namespace editor

#endif // CATA_SRC_EDITOR_VIEW_CANVAS_H

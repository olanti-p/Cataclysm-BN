#ifndef CATA_SRC_EDITOR_EDITOR_ME_STATE_H
#define CATA_SRC_EDITOR_EDITOR_ME_STATE_H

#include "../options.h"
#include "../coordinates.h"

struct ImDrawList;
struct ImVec4;

namespace editor
{

constexpr int MIN_SCALE = 8;
constexpr int MAX_SCALE = 128;
constexpr int DEFAULT_SCALE = 32;

struct me_camera {
    point_abs_epos pos;
    int scale = DEFAULT_SCALE;

    point_abs_epos screen_to_world( const point_abs_screen &p ) const;
    point_abs_screen world_to_screen( const point_abs_epos &p ) const;
};

struct me_state {
    me_camera camera;
    bool do_loop = true; // Setting this to false will quit the editor
    bool show_demo_wnd = false; // Whether to show ImGui Demo window
};

/**
 * ============ Mouse helpers ============
 */
point_abs_screen get_mouse_pos();
point_abs_etile get_mouse_tile_pos( const me_camera &cam );

/**
 * ========== Rendering helpers ==========
 */
void draw_frame(
    ImDrawList *draw_list,
    const me_camera &cam,
    const point_abs_etile &p1,
    const point_abs_etile &p2,
    ImVec4 col,
    bool filled
);
void highlight_tile(
    ImDrawList *draw_list,
    const me_camera &cam,
    point_abs_etile tile,
    ImVec4 col
);
void highlight_region(
    ImDrawList *draw_list,
    const me_camera &cam,
    point_abs_etile p1,
    point_abs_etile p2,
    ImVec4 col_bg,
    ImVec4 col_border
);

/**
 * =============== Windows ===============
 */
void show_canvas( me_state &state );
void show_control_window( me_state &state );

/**
 * ============= Entry point =============
 */
void show_me_ui( me_state &state );

} // namespace editor

#endif // CATA_SRC_EDITOR_EDITOR_ME_STATE_H

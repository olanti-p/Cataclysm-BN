#ifndef CATA_SRC_EDITOR_EDITOR_ME_STATE_H
#define CATA_SRC_EDITOR_EDITOR_ME_STATE_H

#include "../options.h"
#include "../coordinates.h"

#include "editor_assets.h"

struct ImDrawList;
struct ImVec4;

namespace editor
{

constexpr int MIN_SCALE = 8;
constexpr int MAX_SCALE = 128;
constexpr int DEFAULT_SCALE = 32;

struct me_camera {
    point_abs_epos pos;
    point_rel_epos drag_delta;
    int scale = DEFAULT_SCALE;

    point_abs_epos screen_to_world( const point_abs_screen &p ) const;
    point_abs_screen world_to_screen( const point_abs_epos &p ) const;
    point_rel_epos screen_to_world( const point_rel_screen &p ) const;
    point_rel_screen world_to_screen( const point_rel_epos &p ) const;
};

struct me_state {
    me_state();
    me_state( const me_state & ) = delete;
    me_state( me_state && ) = default;
    ~me_state();

    me_state &operator=( const me_state & ) = delete;
    me_state &operator=( me_state && ) = default;

    me_camera camera;
    bool do_loop = true; // Setting this to false will quit the editor
    bool show_demo_wnd = false; // Whether to show ImGui Demo window
    bool show_asset_lib = false; // Whether to show asset library
    asset_library assets;
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
void show_asset_lib( asset_library &assets, bool &show );

/**
 * ============= Entry point =============
 */
void show_me_ui( me_state &state );

} // namespace editor

#endif // CATA_SRC_EDITOR_EDITOR_ME_STATE_H

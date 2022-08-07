#include "editor_main.h"

#include "imgui.h"

#include "../avatar.h"
#include "../game.h"
#include "../input.h"
#include "../map.h"
#include "../output.h"
#include "../ui_manager.h"
#include "../string_formatter.h"
#include "../sdltiles_editor.h"

static bool st_ui_exists = false;
static bool do_loop = false;
static bool show_demo_wnd = false;
static int loops = 0;
static int frames = 0;

static cata::optional<tripoint> get_mouse_tile_pos()
{
    ImVec2 mouse_pos = ImGui::GetMousePos();
    point mouse_pos_p( mouse_pos.x, mouse_pos.y );
    return editor::screen_to_tile( mouse_pos_p );
}

static void show_control_window()
{
    ImGui::Begin( "Advanced Map Editor", &do_loop );
    ImGui::Text( "Close this window to exit the editor. %d/%d", loops, frames );
    frames++;
    // Debugging
    if( ImGui::Button( "Toggle Demo Window" ) ) {
        show_demo_wnd = !show_demo_wnd;
    }

    // Camera zoom
    int zoom_now = g->get_zoom();
    int zoom_old = zoom_now;
    ImGui::DragInt( "Zoom", &zoom_now, 0.2f, 4, 64 );
    if( zoom_now != zoom_old ) {
        g->set_zoom( zoom_now );
        g->mark_main_ui_adaptor_resize();
    }

    // Camera offset
    std::vector<int> offs = {{
            g->u.view_offset.x,
            g->u.view_offset.y
        }
    };
    std::vector<int> offs_old = offs;
    ImGui::DragInt2( "Offset", &offs[0], 0.2f, -60, 60 );
    if( offs != offs_old ) {
        g->u.view_offset.x = offs[0];
        g->u.view_offset.y = offs[1];
    }

    // Mouse position
    ImVec2 mouse_pos = ImGui::GetMousePos();
    cata::optional<tripoint> tile_pos = get_mouse_tile_pos();
    ImGui::Text( "Mouse pos, px: (%f,%f)", mouse_pos.x, mouse_pos.y );
    if( tile_pos ) {
        ImGui::Text( "Mouse pos, tile: %s", tile_pos->to_string().c_str() );
    } else {
        ImGui::Text( "Mouse pos, tile: ???" );
    }

    ImGui::End();
}

static void show_canvas_overlay_window()
{
    ImVec2 disp_size = ImGui::GetIO().DisplaySize;

    ImGui::SetNextWindowPos( ImVec2( 0, 0 ) );
    ImGui::SetNextWindowSize( disp_size );
    ImGui::Begin( "<canvas>", nullptr,
                  ImGuiWindowFlags_NoInputs |
                  ImGuiWindowFlags_NoDecoration |
                  ImGuiWindowFlags_NoFocusOnAppearing |
                  ImGuiWindowFlags_NoBackground |
                  ImGuiWindowFlags_NoBringToFrontOnFocus
                );

    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    const ImVec4 colf = ImVec4( 1.0f, 1.0f, 0.4f, 1.0f );
    const ImU32 col = ImColor( colf );

    cata::optional<tripoint> tile_pos = get_mouse_tile_pos();
    if( tile_pos ) {
        std::pair<point, point> rect = editor::tile_to_screen( tile_pos->xy() );
        ImVec2 p_min( rect.first.x, rect.first.y );
        ImVec2 p_max( rect.second.x, rect.second.y );
        draw_list->AddRect( p_min, p_max, col, 0.0f, ImDrawFlags_None, 1.0f );
    }

    ImGui::End();
}

namespace editor
{
void advanced_editor_run()
{
    st_ui_exists = true;
    do_loop = true;

    bool old_submap_grid = g->debug_submap_grid_overlay;
    g->debug_submap_grid_overlay = true;
    on_out_of_scope _close_ui( [&]() {
        st_ui_exists = false;
        g->debug_submap_grid_overlay = old_submap_grid;
    } );

    g->invalidate_main_ui_adaptor();
    ui_manager::redraw();
    refresh_display();

    while( do_loop ) {
        loops++;

        inp_mngr.get_input_event();
        g->invalidate_main_ui_adaptor();
        ui_manager::redraw();
        refresh_display();
    }

    g->invalidate_main_ui_adaptor();
    ui_manager::redraw();
    refresh_display();
}

bool ui_exists()
{
    return st_ui_exists;
}

void show_ui()
{
    show_canvas_overlay_window();
    show_control_window();
    if( show_demo_wnd ) {
        ImGui::ShowDemoWindow( &show_demo_wnd );
    }
}
}

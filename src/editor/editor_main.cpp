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

static void show_control_window()
{
    ImGui::Begin( "Advanced Map Editor", &do_loop );
    ImGui::Text( "Close this window to exit the editor. %d/%d", loops, frames );
    frames++;
    if( ImGui::Button( "Toggle Demo Window" ) ) {
        show_demo_wnd = !show_demo_wnd;
    }

    int zoom_now = g->get_zoom();
    int zoom_old = zoom_now;
    ImGui::DragInt( "Zoom", &zoom_now, 0.2f, 4, 64 );
    if( zoom_now != zoom_old ) {
        g->set_zoom( zoom_now );
        g->mark_main_ui_adaptor_resize();
    }

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

    ImVec2 mouse_pos = ImGui::GetMousePos();
    point mouse_pos_p( mouse_pos.x, mouse_pos.y );
    cata::optional<tripoint> tile_pos = editor::screen_to_tile( mouse_pos_p );
    ImGui::Text( "Mouse pos, px: %s", mouse_pos_p.to_string().c_str() );
    if( tile_pos ) {
        ImGui::Text( "Mouse pos, tile: %s", tile_pos->to_string().c_str() );
    } else {
        ImGui::Text( "Mouse pos, tile: ???" );
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
    show_control_window();
    if( show_demo_wnd ) {
        ImGui::ShowDemoWindow( &show_demo_wnd );
    }
}
}

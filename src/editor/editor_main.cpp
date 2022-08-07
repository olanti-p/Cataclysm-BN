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

struct editor_state {
    bool do_loop = true;
    bool show_demo_wnd = false;
    int loops = 0;
    int frames = 0;
};

static point get_mouse_screen_pos( const editor_state & /*state*/ )
{
    ImVec2 mouse_pos = ImGui::GetMousePos();
    return point( mouse_pos.x, mouse_pos.y );
}

static cata::optional<tripoint> get_mouse_tile_pos( const editor_state &state )
{
    return editor::screen_to_tile( get_mouse_screen_pos( state ) );
}

static void show_control_window( editor_state &state )
{
    ImGui::Begin( "Advanced Map Editor", &state.do_loop );
    ImGui::Text( "Close this window to exit the editor. %d/%d", state.loops, state.frames );
    state.frames++;
    // Debugging
    if( ImGui::Button( "Toggle Demo Window" ) ) {
        state.show_demo_wnd = !state.show_demo_wnd;
    }

    avatar &u = get_avatar();

    // Camera zoom
    {
        int zoom_now = g->get_zoom();
        int zoom_old = zoom_now;
        ImGui::DragInt( "Zoom", &zoom_now, 0.2f, 4, 64 );
        if( zoom_now != zoom_old && zoom_now >= 4 && zoom_now <= 64 ) {
            g->set_zoom( zoom_now );
            g->mark_main_ui_adaptor_resize();
        }
    }

    // Camera offset
    {
        std::vector<int> offs = {{
                u.view_offset.x,
                u.view_offset.y
            }
        };
        std::vector<int> offs_old = offs;
        ImGui::DragInt2( "Offset", &offs[0], 0.2f, -60, 60 );
        if( offs != offs_old ) {
            u.view_offset.x = offs[0];
            u.view_offset.y = offs[1];
        }
    }

    // Mouse position
    {
        point mouse_pos = get_mouse_screen_pos( state );
        cata::optional<tripoint> tile_pos = get_mouse_tile_pos( state );
        ImGui::Text( "Mouse pos, px: %s", mouse_pos.to_string().c_str() );
        if( tile_pos ) {
            ImGui::Text( "Mouse pos, tile: %s", tile_pos->to_string().c_str() );
        } else {
            ImGui::Text( "Mouse pos, tile: ???" );
        }
    }

    ImGui::End();
}

static void highlight_tile( ImDrawList *draw_list, point tile, ImU32 col )
{
    std::pair<point, point> rect = editor::tile_to_screen( tile );
    ImVec2 p_min( rect.first.x, rect.first.y );
    ImVec2 p_max( rect.second.x, rect.second.y );
    draw_list->AddRect( p_min, p_max, col, 0.0f, ImDrawFlags_None, 1.0f );
}

static void show_canvas_overlay_window( editor_state &state )
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

    cata::optional<tripoint> tile_pos = get_mouse_tile_pos( state );
    if( tile_pos ) {
        highlight_tile( draw_list, tile_pos->xy(), col );
    }

    ImGui::End();
}

static void show_editor_ui( editor_state &state )
{
    show_canvas_overlay_window( state );
    show_control_window( state );
    if( state.show_demo_wnd ) {
        ImGui::ShowDemoWindow( &state.show_demo_wnd );
    }
}

static editor_state *current_state = nullptr;

namespace editor
{
void advanced_editor_run()
{
    editor_state state;
    current_state = &state;

    bool old_submap_grid = g->debug_submap_grid_overlay;
    g->debug_submap_grid_overlay = true;
    on_out_of_scope _close_ui( [&]() {
        current_state = nullptr;
        g->debug_submap_grid_overlay = old_submap_grid;
    } );

    g->invalidate_main_ui_adaptor();
    ui_manager::redraw();
    refresh_display();

    while( state.do_loop ) {
        state.loops++;

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
    return current_state != nullptr;
}

void show_ui()
{
    show_editor_ui( *current_state );
}
}

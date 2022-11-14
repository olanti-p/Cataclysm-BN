#include "editor_me_uistate.h"

#include "editor_me_assetlib.h"
#include "editor_me_camera.h"
#include "editor_me_canvas_tool.h"
#include "editor_me_canvas.h"
#include "editor_me_file.h"
#include "editor_me_project.h"
#include "editor_me_history.h"
#include "editor_me_save_export.h"
#include "editor_me_state.h"
#include "editor_me_uistate.h"
#include "editor_widgets.h"

namespace editor
{

void show_ui_control_window( me_state &state )
{
    bool keep_open = true;
    ImGui::Begin( "Advanced Map Editor", &keep_open );
    ImGui::Text( "Close this window to close the project." );

    save_on_close_widget_block( state, keep_open );

    me_uistate &uistate = *state.uistate;

    // Controls
    if( ImGui::Button( "Toggle Demo Window" ) ) {
        uistate.show_demo_wnd = !uistate.show_demo_wnd;
    }
    ImGui::SameLine();
    if( ImGui::Button( "Toggle Asset Library" ) ) {
        uistate.show_asset_lib = !uistate.show_asset_lib;
    }

    if( ImGui::Button( "Toggle File Info" ) ) {
        uistate.show_file_info = !uistate.show_file_info;
    }
    ImGui::SameLine();
    if( ImGui::Button( "Toggle History" ) ) {
        uistate.show_file_history = !uistate.show_file_history;
    }
    ImGui::SameLine();
    if( ImGui::Button( "Toggle Toolbar" ) ) {
        uistate.show_toolbar = !uistate.show_toolbar;
    }

    save_and_export_widget_block( state );

    // Camera
    {
        ImGui::TextDisabled( "(?: Camera contols)" );
        ImGui::HelpPopup(
            "Camera controls:\n\n"
            "- Drag the view with RMB to pan.\n"
            "- Scroll over the view to zoom.\n"
            "- Use widgets below to manually control zoom and position.\n"
            "\nIn canvas mode:\n"
            "- Press MMB (mouse wheel) on tile to select it.\n"
            "- Press MMB outside bounds (or on empty tile) to clear selection."
        );
        ImGui::DragInt( "Zoom", &state.camera->scale, 0.2f, MIN_SCALE, MAX_SCALE );
        ImGui::DragPoint( "Pos", &state.camera->pos, 1.0f, -10000, 10000 );
    }

    // Mouse position
    {
        point_abs_screen screen_pos = get_mouse_pos();
        point_abs_etile etile_pos = get_mouse_tile_pos( *state.camera );
        ImGui::Text( "Mouse pos, px: %s", screen_pos.to_string().c_str() );
        ImGui::Text( "Mouse pos, tile: %s", etile_pos.to_string().c_str() );
    }

    ImGui::End();
}

void run_ui_for_state( me_state &state )
{
    me_project &proj = state.project();
    show_project_ui( state, proj );

    me_file *active_file = nullptr;
    if( state.uistate->active_file_id ) {
        active_file = proj.get_file_by_uuid( *state.uistate->active_file_id );
        if( !active_file ) {
            state.uistate->active_file_id.reset();
        }
    }

    // TODO: multiple files on same canvas
    show_canvas( state, active_file );
    show_ui_control_window( state );

    me_uistate &uistate = *state.uistate;

    if( uistate.show_demo_wnd ) {
        ImGui::ShowDemoWindow( &uistate.show_demo_wnd );
    }
    if( uistate.show_asset_lib ) {
        show_asset_lib( *state.assets, uistate.show_asset_lib );
    }
    if( uistate.show_file_info && active_file ) {
        show_file_info( state, *active_file, uistate.show_file_info );
    }
    if( uistate.show_file_history ) {
        show_file_history( *state.histate, uistate.show_file_history );
    }
    if( uistate.show_toolbar ) {
        show_toolbar( *state.tools_state, uistate.show_toolbar );
    }

    handle_revision_change( *state.histate, *state.tools_state );
}

} // namespace editor

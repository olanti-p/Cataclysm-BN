#include "editor_me_state.h"
#include "editor_widgets.h"
#include "editor_me_state_export.h"
#include "editor_me_canvas.h"

#include "../fstream_utils.h"
#include "../game_constants.h"
#include "../game.h"
#include "../string_utils.h"
#include "../text_snippets.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include <unordered_set>

namespace editor
{

void show_control_window( me_state &state )
{
    bool keep_open = true;
    ImGui::Begin( "Advanced Map Editor", &keep_open );
    ImGui::Text( "Close this window to close the project." );

    save_on_close_widget_block( state, keep_open );

    // Controls
    if( ImGui::Button( "Toggle Demo Window" ) ) {
        state.show_demo_wnd = !state.show_demo_wnd;
    }
    ImGui::SameLine();
    if( ImGui::Button( "Toggle Asset Library" ) ) {
        state.show_asset_lib = !state.show_asset_lib;
    }

    if( ImGui::Button( "Toggle File Info" ) ) {
        state.show_file_info = !state.show_file_info;
    }
    ImGui::SameLine();
    if( ImGui::Button( "Toggle History" ) ) {
        state.show_file_history = !state.show_file_history;
    }
    ImGui::SameLine();
    if( ImGui::Button( "Toggle Toolbar" ) ) {
        state.show_toolbar = !state.show_toolbar;
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
        ImGui::DragInt( "Zoom", &state.camera.scale, 0.2f, MIN_SCALE, MAX_SCALE );
        ImGui::DragPoint( "Pos", &state.camera.pos, 1.0f, -10000, 10000 );
    }

    // Mouse position
    {
        point_abs_screen screen_pos = get_mouse_pos();
        point_abs_etile etile_pos = get_mouse_tile_pos( state.camera );
        ImGui::Text( "Mouse pos, px: %s", screen_pos.to_string().c_str() );
        ImGui::Text( "Mouse pos, tile: %s", etile_pos.to_string().c_str() );
    }

    ImGui::End();
}

void show_me_ui( me_state &state )
{
    show_canvas( state );
    show_control_window( state );
    if( state.show_demo_wnd ) {
        ImGui::ShowDemoWindow( &state.show_demo_wnd );
    }
    if( state.show_asset_lib ) {
        show_asset_lib( state.assets, state.show_asset_lib );
    }
    if( state.show_file_info ) {
        show_file_info( state, state.file(), state.show_file_info );
    }
    if( state.show_file_history ) {
        show_file_history( state.histate, state.show_file_history );
    }
    if( state.show_toolbar ) {
        show_toolbar( state.tools_state, state.show_toolbar );
    }

    handle_revision_change( state.histate, state.tools_state );
}

me_state::me_state() : me_state( std::make_unique<me_file>() ) { }

me_state::me_state( std::unique_ptr<me_file> &&file ) : me_state( std::move( file ), nullptr ) { }

me_state::me_state( std::unique_ptr<me_file> &&file,
                    const std::string *loaded_from_path ) : histate( std::move( file ), !!loaded_from_path )
{
    if( loaded_from_path ) {
        sestate.file_save_path = *loaded_from_path;
    }
    init_assets( assets );
}

me_state::~me_state() = default;

} // namespace editor

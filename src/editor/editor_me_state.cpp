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
#include "ImGuiFileDialog.h"

#include <unordered_set>

namespace editor
{
static void handle_file_saving( me_state &state )
{
    if( state.tools_state.ongoing_tool_operation ) {
        return;
    }

    if( ImGui::IsKeyDown( ImGuiKey_LeftCtrl ) && ImGui::IsKeyPressed( ImGuiKey_S ) ) {
        if( ImGui::IsKeyDown( ImGuiKey_LeftShift ) || !state.file_save_path ) {
            state.open_save_as = true;
        } else {
            state.do_save = true;
        }
    }

    if( state.open_save_as ) {
        state.open_save_as = false;
        ImGuiFileDialog::Instance()->OpenDialog( "SaveToFile",
                "Save As...", ".json",
                state.file_save_path ? *state.file_save_path : ".",
                1, nullptr, ImGuiFileDialogFlags_ConfirmOverwrite );
    }

    if( ImGuiFileDialog::Instance()->Display( "SaveToFile" ) ) {
        if( ImGuiFileDialog::Instance()->IsOk() ) {
            state.file_save_path = ImGuiFileDialog::Instance()->GetFilePathName();
            state.do_save = true;
        } else {
            state.do_exit_after_save = false;
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if( state.do_save ) {
        state.do_save = false;
        assert( state.file_save_path );
        write_to_file( *state.file_save_path, [&]( std::ostream & oss ) {
            oss << serialize( state.file() );
        } );
        state.histate.last_saved_revision = state.histate.current_revision.num;
        if( state.do_exit_after_save ) {
            state.do_loop = false;
        }
    }
}

static void handle_file_exporting( me_state &state )
{
    if( state.tools_state.ongoing_tool_operation ) {
        return;
    }

    if( state.open_export_as ) {
        state.open_export_as = false;
        ImGuiFileDialog::Instance()->OpenDialog( "ExportToFile",
                "Export As...", ".json",
                state.file_export_path ? *state.file_export_path : ".",
                1, nullptr, ImGuiFileDialogFlags_ConfirmOverwrite );
    }

    if( ImGuiFileDialog::Instance()->Display( "ExportToFile" ) ) {
        if( ImGuiFileDialog::Instance()->IsOk() ) {
            state.file_export_path = ImGuiFileDialog::Instance()->GetFilePathName();
            state.do_export = true;
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if( g->export_editor_project_on_start ) {
        state.file_export_path = *g->export_editor_project_on_start;
        state.do_export = true;
        g->export_editor_project_on_start.reset();
    }

    if( state.do_export ) {
        state.do_export = false;
        assert( state.file_export_path );
        write_to_file( *state.file_export_path, [&]( std::ostream & oss ) {
            std::string s = editor_export::to_string( state.file() );
            oss << editor_export::format_string( s );
        } );
        state.histate.last_exported_revision = state.histate.current_revision.num;
    }
}

void show_control_window( me_state &state )
{
    bool keep_open = true;
    ImGui::Begin( "Advanced Map Editor", &keep_open );
    ImGui::Text( "Close this window to close the project." );

    if( !keep_open ) {
        if( state.histate.has_unsaved_changes() ) {
            ImGui::OpenPopup( "###warn-unsaved-on-close" );
        } else {
            state.do_loop = false;
        }
    }

    if( ImGui::BeginPopupModal( "###warn-unsaved-on-close", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize ) ) {
        ImGui::TextCentered( "Do you want to save the changes?" );
        ImGui::Text( " " );
        ImVec2 btn_sz( ImGui::GetFrameHeight() * 5.0f, ImGui::GetFrameHeight() );
        if( ImGui::Button( "Don't Save", btn_sz ) ) {
            ImGui::CloseCurrentPopup();
            state.do_loop = false;
        }
        ImGui::SameLine();
        if( ImGui::Button( "Cancel", btn_sz ) ) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if( ImGui::Button( "Save", btn_sz ) ) {
            ImGui::CloseCurrentPopup();
            state.do_exit_after_save = true;
            if( state.file_save_path ) {
                state.do_save = true;
            } else {
                state.open_save_as = true;
            }
        }
        ImGui::EndPopup();
    }

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

    std::string save_btn = string_format( "%sSave###save-button",
                                          state.histate.has_unsaved_changes() ? "* " : "" );
    if( ImGui::Button( save_btn.c_str() ) ) {
        if( !state.file_save_path ) {
            state.open_save_as = true;
        } else {
            state.do_save = true;
        }
    }
    ImGui::SameLine();
    if( ImGui::Button( "Save As..." ) ) {
        state.open_save_as = true;
    }

    handle_file_saving( state );

    std::string export_btn = string_format( "%sExport###export-button",
                                            state.histate.has_unexported_changes() ? "* " : "" );
    if( ImGui::Button( export_btn.c_str() ) ) {
        if( !state.file_export_path ) {
            state.open_export_as = true;
        } else {
            state.do_export = true;
        }
    }
    ImGui::SameLine();
    if( ImGui::Button( "Export As..." ) ) {
        state.open_export_as = true;
    }

    handle_file_exporting( state );

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

static bool filter_matches( const std::string &s, const std::string &filter )
{
    return lcmatch( s, filter );
}

static void show_assetlib_tab( asset_library_cat &cat, asset_library &assets )
{
    const char *cat_name = get_asset_type_name( cat.atype );
    if( !ImGui::BeginTabItem( cat_name ) ) {
        cat.is_active_tab = false;
        return;
    }
    cat.is_active_tab = true;

    ImGui::Text( "%s - %d entries", cat_name, cat.get_num() );

    ImGui::InputText( "##filter", &cat.filter );
    std::string lb_name = string_format( "##lb-%s", cat_name );
    if( ImGui::BeginListBox( lb_name.c_str(), ImVec2( -1.0f, -1.0f ) ) ) {
        for( int i = 0; i < cat.get_num(); i++ ) {
            const asset_lib_entry &entry = cat.get( i );
            const char *id = entry.get_id();
            if( !filter_matches( id, cat.filter ) ) {
                continue;
            }
            const bool is_selected = i == cat.selected;
            if( ImGui::Selectable( id, is_selected ) ) {
                cat.selected = i;
            }
            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if( is_selected ) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndListBox();
    }

    ImGui::EndTabItem();
}

void show_asset_lib( asset_library &assets, bool &show )
{
    if( ImGui::Begin( "Asset Library", &show ) ) {
        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_FittingPolicyResizeDown;
        if( ImGui::BeginTabBar( "Asset Types", tab_bar_flags ) ) {
            for( asset_library_cat &cat : assets.categories ) {
                show_assetlib_tab( cat, assets );
            }

            ImGui::EndTabBar();
        }
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
        file_save_path = *loaded_from_path;
    }
    init_assets( assets );
}

me_state::~me_state() = default;

} // namespace editor

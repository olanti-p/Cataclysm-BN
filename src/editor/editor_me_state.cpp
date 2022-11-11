#include "editor_me_state.h"
#include "editor_widgets.h"
#include "editor_me_state_export.h"
#include "editor_me_canvas.h"

#include "../fstream_utils.h"
#include "../game_constants.h"
#include "../game.h"
#include "../string_utils.h"
#include "../text_snippets.h"

#ifdef DebugLog
#  undef DebugLog
#endif

#include "imgui.h"
#include "imgui_internal.h"
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
        state.last_saved_revision = state.current_revision.num;
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
        state.last_exported_revision = state.current_revision.num;
    }
}

void show_control_window( me_state &state )
{
    bool keep_open = true;
    ImGui::Begin( "Advanced Map Editor", &keep_open );
    ImGui::Text( "Close this window to close the project." );

    if( !keep_open ) {
        if( state.has_unsaved_changes() ) {
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
                                          state.has_unsaved_changes() ? "* " : "" );
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
                                            state.has_unexported_changes() ? "* " : "" );
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

void show_file_history( me_state &state, bool &show )
{
    if( !ImGui::Begin( "File history", &show ) ) {
        ImGui::End();
        return;
    }

    ImGui::HelpMarkerInline(
        "Undo/redo support.\n\n"
        "In order to enable undo and redo, the editor has to keep track of the old versions (revisions) of the file.  "
        "This is done entirely in memory, so remembering too much revisions may exhaust available RAM at some point "
        "and trigger program termination by the OS.  You can manually control how much revisions will be kept alive "
        "using the widget below.\n"
        "\nHotkeys:\n"
        "  Ctrl+Z - Undo (advance to older revision)\n"
        "  Ctrl+Shift+Z - Redo (advance to newer revision)\n"
    );

    ImGui::SetNextItemWidth( ImGui::GetFrameHeight() * 4.0f );
    ImGui::InputIntClamped( "History limit", state.history_capacity, 10, 10000,
                            ImGuiInputTextFlags_AutoSelectAll );

    ImGui::HelpMarkerInline(
        "The list below keeps track of file revisions.\n\n"
        "Click on a revision to make it active.  "
        "Every edit automatically generates a new revision and places it at the top.\n"
        "\nMarkers use in the list:\n"
        "  [S] This revision is the one saved in the project file.\n"
        "  [E] This revision is the one that was used for export.\n"
    );
    ImGui::Text( "Edit counter (debug): %d", state.edit_counter );

    for( const me_file_revision &entry : state.file_history ) {
        bool is_saved = state.last_saved_revision && *state.last_saved_revision == entry.num;
        bool is_exported = state.last_exported_revision && *state.last_exported_revision == entry.num;
        std::string fname = string_format(
                                "Version %d%s%s",
                                entry.num,
                                is_saved ? " [S]" : "",
                                is_exported ? " [E]" : ""
                            );
        if( ImGui::Selectable( fname.c_str(), entry.num == state.current_revision.num ) ) {
            state.switch_to_revision = entry.num;
        }
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

static void handle_revision_change( me_state &state )
{
    if( state.tools_state.ongoing_tool_operation ) {
        return;
    }
    if( ImGui::IsKeyDown( ImGuiKey_LeftCtrl ) && ImGui::IsKeyPressed( ImGuiKey_Z ) ) {
        if( ImGui::IsKeyDown( ImGuiKey_LeftShift ) ) {
            if( state.can_redo() ) {
                state.queue_redo();
            }
        } else {
            if( state.can_undo() ) {
                state.queue_undo();
            }
        }
    }
    if( state.switch_to_revision ) {
        auto it = std::find_if( state.file_history.cbegin(),
        state.file_history.cend(), [&]( const me_file_revision & rev ) {
            return rev.num == *state.switch_to_revision;
        } );
        assert( it != state.file_history.cend() );
        state.current_revision = it->make_copy();
        state.switch_to_revision.reset();
    } else if( state.file_has_changes ) {
        state.file_has_changes = false;

        const bool is_changing_same = state.last_widget_changed && state.current_widget_changed &&
                                      *state.last_widget_changed == *state.current_widget_changed;

        state.current_widget_changed_str.clear();
        state.last_widget_changed = state.current_widget_changed;
        state.current_widget_changed = cata::nullopt;

        bool is_alt_history = false;

        // Erase alternative history
        while( state.file_history[0].num != state.current_revision.num ) {
            // TODO: optimize this to use dequeue
            state.file_history.erase( state.file_history.cbegin() );
            is_alt_history = true;
        }

        const bool is_rev_saved = state.last_saved_revision ? *state.last_saved_revision ==
                                  state.current_revision.num : false;
        const bool is_rev_exported = state.last_exported_revision ? *state.last_exported_revision ==
                                     state.current_revision.num : false;
        const bool collapse_change = is_changing_same && !is_alt_history && !is_rev_saved &&
                                     !is_rev_exported && !state.file_history.empty();

        if( collapse_change ) {
            state.file_history.erase( state.file_history.cbegin() );
        } else {
            state.current_revision.num++;
        }
        state.file_history.insert( state.file_history.cbegin(), state.current_revision.make_copy() );

        // Erase old entries
        if( static_cast<int>( state.file_history.size() ) > state.history_capacity ) {
            state.file_history.resize( state.history_capacity );
        }
    }
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
        show_file_history( state, state.show_file_history );
    }
    if( state.show_toolbar ) {
        show_toolbar( state.tools_state, state.show_toolbar );
    }

    handle_revision_change( state );
}

me_state::me_state() : me_state( std::make_unique<me_file>() ) { }

me_state::me_state( std::unique_ptr<me_file> &&file ) : me_state( std::move( file ), nullptr ) { }

me_state::me_state( std::unique_ptr<me_file> &&file,
                    const std::string *loaded_from_path )
{
    current_revision = me_file_revision();

    if( loaded_from_path ) {
        file_save_path = *loaded_from_path;
        last_saved_revision = current_revision.num;
    }

    if( file ) {
        current_revision.file = std::move( file );
    }

    file_history.reserve( history_capacity + 1 );
    file_history.emplace_back( current_revision.make_copy() );

    init_assets( assets );
}

me_state::~me_state() = default;

void me_state::mark_changed( const char *id )
{
    std::string new_widget_changed_str = id ? id : "<nullptr>";
    if( file_has_changes ) {
        std::cerr << string_format(
                      "Tried to invoke mark_changed( \"%s\" ), but the file has already been marked as changed with id \"%s\".",
                      new_widget_changed_str,
                      current_widget_changed_str
                  ) << std::endl;
        std::abort();
    }
    current_widget_changed_str = new_widget_changed_str;
    if( id ) {
        ImGuiWindow *wnd = ImGui::GetCurrentWindow();
        current_widget_changed = wnd->GetID( id );
    }
    file_has_changes = true;
    edit_counter++;
}

bool me_state::has_unsaved_changes() const
{
    return !last_saved_revision || current_revision.num != *last_saved_revision;
}

bool me_state::has_unexported_changes() const
{
    return !last_exported_revision || current_revision.num != *last_exported_revision;
}

} // namespace editor

#include "save_export_state.h"

#include "fstream_utils.h"
#include "game.h"

#include "history_state.h"
#include "project/project.h"
#include "project/project_export.h"
#include "state.h"
#include "state/control_state.h"
#include "tools_state.h"
#include "ui_state.h"
#include "widget/ImGuiFileDialog.h"
#include "widget/widgets.h"

namespace editor
{
void handle_project_saving( State &state )
{
    ControlState &control = *state.control;
    SaveExportState &sestate = *state.save_export;

    if( state.control->has_ongoing_tool_operation() ) {
        control.want_save = false;
        control.want_save_as = false;
        control.want_exit_after_save = false;
        return;
    }

    if( control.want_save && !sestate.project_save_path ) {
        control.want_save = false;
        control.want_save_as = true;
    }

    if( control.want_save_as ) {
        control.want_save_as = false;
        ImGui::SetNextWindowSize( ImVec2( 580, 380 ), ImGuiCond_FirstUseEver );
        ImGuiFileDialog::Instance()->OpenDialog( "SaveToFile",
                "Save As...", ".json",
                sestate.project_save_path ? *sestate.project_save_path : ".",
                1, nullptr, ImGuiFileDialogFlags_ConfirmOverwrite );
    }

    if( ImGuiFileDialog::Instance()->Display( "SaveToFile" ) ) {
        if( ImGuiFileDialog::Instance()->IsOk() ) {
            sestate.project_save_path = ImGuiFileDialog::Instance()->GetFilePathName();
            control.want_save = true;
        } else {
            control.want_exit_after_save = false;
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if( control.want_save ) {
        control.want_save = false;
        assert( sestate.project_save_path );
        write_to_file( *sestate.project_save_path, [&]( std::ostream & oss ) {
            oss << serialize( state.project() );
        } );
        state.history->last_saved_snapshot = state.history->current_snapshot.num;
        if( control.want_exit_after_save ) {
            control.want_exit_after_save = false;
            control.is_editor_running = false;
        }
    }
}

void handle_project_exporting( State &state )
{
    ControlState &control = *state.control;
    SaveExportState &sestate = *state.save_export;

    if( state.control->has_ongoing_tool_operation() ) {
        control.want_export = false;
        control.want_export_as = false;
        return;
    }

    if( control.want_export && !sestate.project_export_path ) {
        control.want_export = false;
        control.want_export_as = true;
    }

    if( control.want_export_as ) {
        control.want_export_as = false;
        ImGui::SetNextWindowSize( ImVec2( 580, 380 ), ImGuiCond_FirstUseEver );
        ImGuiFileDialog::Instance()->OpenDialog( "ExportToFile",
                "Export As...", ".json",
                sestate.project_export_path ? *sestate.project_export_path : ".",
                1, nullptr, ImGuiFileDialogFlags_ConfirmOverwrite );
    }

    if( ImGuiFileDialog::Instance()->Display( "ExportToFile" ) ) {
        if( ImGuiFileDialog::Instance()->IsOk() ) {
            sestate.project_export_path = ImGuiFileDialog::Instance()->GetFilePathName();
            control.want_export = true;
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if( g->export_editor_project_on_start ) {
        sestate.project_export_path = *g->export_editor_project_on_start;
        control.want_export = true;
        g->export_editor_project_on_start.reset();
    }

    if( control.want_export ) {
        control.want_export = false;
        assert( sestate.project_export_path );
        write_to_file( *sestate.project_export_path, [&]( std::ostream & oss ) {
            std::string s = editor_export::to_string( state.project() );
            oss << editor_export::format_string( s );
        } );
        state.history->last_exported_snapshot = state.history->current_snapshot.num;
    }
}

void handle_project_exiting( State &state )
{
    ControlState &control = *state.control;

    if( state.control->has_ongoing_tool_operation() ) {
        control.want_close = false;
        return;
    }

    if( control.want_close ) {
        if( state.history->has_unsaved_changes() ) {
            ImGui::OpenPopup( "###warn-unsaved-on-close" );
            control.want_close = false;
        } else {
            control.is_editor_running = false;
        }
    }

    if( ImGui::BeginPopupModal( "###warn-unsaved-on-close", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize ) ) {
        ImGui::TextCentered( "Do you want to save the changes?" );
        ImGui::Text( " " );
        ImVec2 btn_sz( ImGui::GetFrameHeight() * 5.0f, ImGui::GetFrameHeight() );
        if( ImGui::Button( "Don't Save", btn_sz ) ) {
            ImGui::CloseCurrentPopup();
            control.is_editor_running = false;
        }
        ImGui::SameLine();
        if( ImGui::Button( "Cancel", btn_sz ) ) {
            control.want_close = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if( ImGui::Button( "Save", btn_sz ) ) {
            ImGui::CloseCurrentPopup();
            control.want_exit_after_save = true;
            control.want_save = true;
        }
        ImGui::EndPopup();
    }
}

} // namespace editor

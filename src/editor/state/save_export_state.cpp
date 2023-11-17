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
void handle_file_saving( me_state &state )
{
    me_control_state &control = *state.cstate;
    me_save_export_state &sestate = *state.sestate;

    if( state.uistate->tools_state->has_ongoing_tool_operation() ) {
        control.want_save = false;
        control.want_save_as = false;
        control.want_exit_after_save = false;
        return;
    }

    if( control.want_save && !sestate.file_save_path ) {
        control.want_save = false;
        control.want_save_as = true;
    }

    if( control.want_save_as ) {
        control.want_save_as = false;
        ImGui::SetNextWindowSize( ImVec2( 580, 380 ), ImGuiCond_FirstUseEver );
        ImGuiFileDialog::Instance()->OpenDialog( "SaveToFile",
                "Save As...", ".json",
                sestate.file_save_path ? *sestate.file_save_path : ".",
                1, nullptr, ImGuiFileDialogFlags_ConfirmOverwrite );
    }

    if( ImGuiFileDialog::Instance()->Display( "SaveToFile" ) ) {
        if( ImGuiFileDialog::Instance()->IsOk() ) {
            sestate.file_save_path = ImGuiFileDialog::Instance()->GetFilePathName();
            control.want_save = true;
        } else {
            control.want_exit_after_save = false;
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if( control.want_save ) {
        control.want_save = false;
        assert( sestate.file_save_path );
        write_to_file( *sestate.file_save_path, [&]( std::ostream & oss ) {
            oss << serialize( state.project() );
        } );
        state.histate->last_saved_revision = state.histate->current_revision.num;
        if( control.want_exit_after_save ) {
            control.want_exit_after_save = false;
            control.is_editor_running = false;
        }
    }
}

void handle_file_exporting( me_state &state )
{
    me_control_state &control = *state.cstate;
    me_save_export_state &sestate = *state.sestate;

    if( state.uistate->tools_state->has_ongoing_tool_operation() ) {
        control.want_export = false;
        control.want_export_as = false;
        return;
    }

    if( control.want_export && !sestate.file_export_path ) {
        control.want_export = false;
        control.want_export_as = true;
    }

    if( control.want_export_as ) {
        control.want_export_as = false;
        ImGui::SetNextWindowSize( ImVec2( 580, 380 ), ImGuiCond_FirstUseEver );
        ImGuiFileDialog::Instance()->OpenDialog( "ExportToFile",
                "Export As...", ".json",
                sestate.file_export_path ? *sestate.file_export_path : ".",
                1, nullptr, ImGuiFileDialogFlags_ConfirmOverwrite );
    }

    if( ImGuiFileDialog::Instance()->Display( "ExportToFile" ) ) {
        if( ImGuiFileDialog::Instance()->IsOk() ) {
            sestate.file_export_path = ImGuiFileDialog::Instance()->GetFilePathName();
            control.want_export = true;
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if( g->export_editor_project_on_start ) {
        sestate.file_export_path = *g->export_editor_project_on_start;
        control.want_export = true;
        g->export_editor_project_on_start.reset();
    }

    if( control.want_export ) {
        control.want_export = false;
        assert( sestate.file_export_path );
        write_to_file( *sestate.file_export_path, [&]( std::ostream & oss ) {
            std::string s = editor_export::to_string( state.project() );
            oss << editor_export::format_string( s );
        } );
        state.histate->last_exported_revision = state.histate->current_revision.num;
    }
}

void handle_project_exiting( me_state &state )
{
    me_control_state &control = *state.cstate;

    if( state.uistate->tools_state->has_ongoing_tool_operation() ) {
        control.want_close = false;
        return;
    }

    if( control.want_close ) {
        if( state.histate->has_unsaved_changes() ) {
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

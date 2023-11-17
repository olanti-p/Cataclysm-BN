#include "state_export.h"

#include "fstream_utils.h"
#include "game.h"

#include "canvas/canvas_tools.h"
#include "project.h"
#include "history.h"
#include "save_and_export.h"
#include "state.h"
#include "uistate.h"
#include "widget/widgets.h"
#include "widget/ImGuiFileDialog.h"

namespace editor
{
void handle_file_saving( me_state &state )
{
    me_save_export_state &sestate = *state.sestate;

    if( state.uistate->tools_state->has_ongoing_tool_operation() ) {
        sestate.want_save = false;
        sestate.want_save_as = false;
        sestate.want_exit_after_save = false;
        return;
    }

    if( sestate.want_save && !sestate.file_save_path ) {
        sestate.want_save = false;
        sestate.want_save_as = true;
    }

    if( sestate.want_save_as ) {
        sestate.want_save_as = false;
        ImGui::SetNextWindowSize( ImVec2( 580, 380 ), ImGuiCond_FirstUseEver );
        ImGuiFileDialog::Instance()->OpenDialog( "SaveToFile",
                "Save As...", ".json",
                sestate.file_save_path ? *sestate.file_save_path : ".",
                1, nullptr, ImGuiFileDialogFlags_ConfirmOverwrite );
    }

    if( ImGuiFileDialog::Instance()->Display( "SaveToFile" ) ) {
        if( ImGuiFileDialog::Instance()->IsOk() ) {
            sestate.file_save_path = ImGuiFileDialog::Instance()->GetFilePathName();
            sestate.want_save = true;
        } else {
            sestate.want_exit_after_save = false;
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if( sestate.want_save ) {
        sestate.want_save = false;
        assert( sestate.file_save_path );
        write_to_file( *sestate.file_save_path, [&]( std::ostream & oss ) {
            oss << serialize( state.project() );
        } );
        state.histate->last_saved_revision = state.histate->current_revision.num;
        if( sestate.want_exit_after_save ) {
            sestate.want_exit_after_save = false;
            state.uistate->do_loop = false;
        }
    }
}

void handle_file_exporting( me_state &state )
{
    me_save_export_state &sestate = *state.sestate;

    if( state.uistate->tools_state->has_ongoing_tool_operation() ) {
        sestate.want_export = false;
        sestate.want_export_as = false;
        return;
    }

    if( sestate.want_export && !sestate.file_export_path ) {
        sestate.want_export = false;
        sestate.want_export_as = true;
    }

    if( sestate.want_export_as ) {
        sestate.want_export_as = false;
        ImGui::SetNextWindowSize( ImVec2( 580, 380 ), ImGuiCond_FirstUseEver );
        ImGuiFileDialog::Instance()->OpenDialog( "ExportToFile",
                "Export As...", ".json",
                sestate.file_export_path ? *sestate.file_export_path : ".",
                1, nullptr, ImGuiFileDialogFlags_ConfirmOverwrite );
    }

    if( ImGuiFileDialog::Instance()->Display( "ExportToFile" ) ) {
        if( ImGuiFileDialog::Instance()->IsOk() ) {
            sestate.file_export_path = ImGuiFileDialog::Instance()->GetFilePathName();
            sestate.want_export = true;
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if( g->export_editor_project_on_start ) {
        sestate.file_export_path = *g->export_editor_project_on_start;
        sestate.want_export = true;
        g->export_editor_project_on_start.reset();
    }

    if( sestate.want_export ) {
        sestate.want_export = false;
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
    if( state.uistate->tools_state->has_ongoing_tool_operation() ) {
        state.uistate->want_close = false;
        return;
    }

    if( state.uistate->want_close ) {
        if( state.histate->has_unsaved_changes() ) {
            ImGui::OpenPopup( "###warn-unsaved-on-close" );
            state.uistate->want_close = false;
        } else {
            state.uistate->do_loop = false;
        }
    }

    me_save_export_state &sestate = *state.sestate;

    if( ImGui::BeginPopupModal( "###warn-unsaved-on-close", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize ) ) {
        ImGui::TextCentered( "Do you want to save the changes?" );
        ImGui::Text( " " );
        ImVec2 btn_sz( ImGui::GetFrameHeight() * 5.0f, ImGui::GetFrameHeight() );
        if( ImGui::Button( "Don't Save", btn_sz ) ) {
            ImGui::CloseCurrentPopup();
            state.uistate->do_loop = false;
        }
        ImGui::SameLine();
        if( ImGui::Button( "Cancel", btn_sz ) ) {
            state.uistate->want_close = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if( ImGui::Button( "Save", btn_sz ) ) {
            ImGui::CloseCurrentPopup();
            sestate.want_exit_after_save = true;
            sestate.want_save = true;
        }
        ImGui::EndPopup();
    }
}

} // namespace editor

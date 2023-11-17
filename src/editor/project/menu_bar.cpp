#include "menu_bar.h"

#include "imgui.h"
#include "project/history.h"
#include "project/save_and_export.h"
#include "uistate.h"

namespace editor
{
void show_main_menu_bar( me_state &state )
{
    me_save_export_state &sestate = *state.sestate;

    if( ImGui::BeginMainMenuBar() ) {
        if( ImGui::BeginMenu( "File" ) ) {
            if( ImGui::MenuItem( "Save", "Ctrl+S" ) ) {
                sestate.want_save = true;
            }
            if( ImGui::MenuItem( "Save As...", "Ctrl+Shift+S" ) ) {
                sestate.want_save_as = true;
            }
            ImGui::Separator();
            if( ImGui::MenuItem( "Export", "Ctrl+E" ) ) {
                sestate.want_export = true;
            }
            if( ImGui::MenuItem( "Export As...", "Ctrl+Shift+E" ) ) {
                sestate.want_export_as = true;
            }
            ImGui::Separator();
            if( ImGui::MenuItem( "Exit", "Ctrl+Q" ) ) {
                state.uistate->want_close = true;
            }
            ImGui::EndMenu();
        }
        if( ImGui::BeginMenu( "Edit" ) ) {
            if( ImGui::MenuItem( "Undo", "Ctrl+Z", nullptr, state.histate->can_undo() ) ) {
                state.histate->queue_undo();
            }
            if( ImGui::MenuItem( "Redo", "Ctrl+Shift+Z", nullptr, state.histate->can_redo() ) ) {
                state.histate->queue_redo();
            }
            ImGui::EndMenu();
        }
        if( ImGui::BeginMenu( "View" ) ) {
            ImGui::MenuItem( "Project Overview", nullptr, &state.uistate->show_project_overview );
            ImGui::MenuItem( "File Info", nullptr, &state.uistate->show_file_info );
            ImGui::MenuItem( "History", nullptr, &state.uistate->show_file_history );
            ImGui::MenuItem( "Toolbar", nullptr, &state.uistate->show_toolbar );
            ImGui::MenuItem( "Camera Controls", nullptr, &state.uistate->show_camera_controls );
            ImGui::Separator();
            ImGui::MenuItem( "ImGui Demo", nullptr, &state.uistate->show_demo_wnd );
            ImGui::MenuItem( "Debug/Metrics", nullptr, &state.uistate->show_metrics_wnd );
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if( state.uistate->tools_state->has_ongoing_tool_operation() ) {
        return;
    }

    if( ImGui::IsKeyDown( ImGuiKey_LeftCtrl ) && ImGui::IsKeyPressed( ImGuiKey_S ) ) {
        if( ImGui::IsKeyDown( ImGuiKey_LeftShift ) ) {
            sestate.want_save_as = true;
        } else {
            sestate.want_save = true;
        }
    }

    if( ImGui::IsKeyDown( ImGuiKey_LeftCtrl ) && ImGui::IsKeyPressed( ImGuiKey_E ) ) {
        if( ImGui::IsKeyDown( ImGuiKey_LeftShift ) ) {
            sestate.want_export_as = true;
        } else {
            sestate.want_export = true;
        }
    }

    if( ImGui::IsKeyDown( ImGuiKey_LeftCtrl ) && ImGui::IsKeyPressed( ImGuiKey_Q ) ) {
        state.uistate->want_close = true;
    }

    if( ImGui::IsKeyDown( ImGuiKey_LeftCtrl ) && ImGui::IsKeyPressed( ImGuiKey_Z ) ) {
        if( ImGui::IsKeyDown( ImGuiKey_LeftShift ) ) {
            if( state.histate->can_redo() ) {
                state.histate->queue_redo();
            }
        } else {
            if( state.histate->can_undo() ) {
                state.histate->queue_undo();
            }
        }
    }
}

} // namespace editor

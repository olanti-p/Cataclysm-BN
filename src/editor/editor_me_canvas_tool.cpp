#include "editor_me_canvas_tool.h"

#include "editor_widgets.h"
#include "imgui.h"

namespace editor
{

void show_toolbar( me_canvas_tools_state &tools, bool &show )
{
    if( !ImGui::Begin( "Toolbar", &show,
                       ImGuiWindowFlags_AlwaysAutoResize |
                       ImGuiWindowFlags_NoCollapse |
                       ImGuiWindowFlags_NoResize
                     ) ) {
        ImGui::End();
        return;
    }

    if( ImGui::RadioButton( "Brush", tools.tool == CanvasTool::Brush ) ) {
        tools.tool = CanvasTool::Brush;
    }
    ImGui::HelpPopup( "Hold LMB to draw with selected tile." );
    if( ImGui::RadioButton( "Bucket", tools.tool == CanvasTool::Bucket ) ) {
        tools.tool = CanvasTool::Bucket;
    }
    ImGui::HelpPopup( "Click LMB to flood fill with selected tile." );
    if( ImGui::RadioButton( "Bucket (global)", tools.tool == CanvasTool::BucketGlobal ) ) {
        tools.tool = CanvasTool::BucketGlobal;
    }
    ImGui::HelpPopup( "Click LMB to replace all such tiles with selected tile." );

    ImGui::End();
}

} // namespace editor

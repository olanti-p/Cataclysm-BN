#include "editor_widgets.h"
#include "editor_main.h"

#include <string>

#include "../color.h"
#include "../catacharset.h"
#include "../mapgen.h"

namespace ImGui
{
void SymbolColored( const std::string &sym, nc_color col )
{
    ImGui::TextColored( editor::curses_color_to_imgui( col ), "%s", sym.c_str() );
}
void SymbolColored( int sym, nc_color col )
{
    SymbolColored( utf32_to_utf8( sym ), col );
}

void JmapgenInt( const std::string &label, const jmapgen_int &jmi )
{
    ImGui::Text( "%s:[%d,%d]", label.c_str(), jmi.val, jmi.valmax );
}

void JmapgenPlace( const std::string &label, const jmapgen_place &jmp )
{
    ImGui::Text( "%s", label.c_str() );
    ImGui::SameLine();
    JmapgenInt( "x", jmp.x );
    ImGui::SameLine();
    JmapgenInt( "y", jmp.y );
    ImGui::SameLine();
    JmapgenInt( "repeat", jmp.repeat );
}

bool detail::InputId( const char *label, std::string &data, bool is_valid,
                      ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void *user_data )
{
    if( !is_valid ) {
        ImGui::PushStyleColor( ImGuiCol_FrameBg, ImVec4( 0.8f, 0.3f, 0.3f, 1.0f ) );
    }
    bool ret = ImGui::InputText( label, &data, flags, callback, user_data );
    if( !is_valid ) {
        ImGui::PopStyleColor();
    }
    return ret;
}

bool InputJmapgenInt( const char *label, jmapgen_int &jmi )
{
    ImGui::Text( "%s", label );
    ImGui::SameLine();
    ImGui::PushID( label );
    ImGui::SetNextItemWidth( GetFrameHeight() * 1.5f );
    bool ret1 = ImGui::InputInt( "##min", &jmi.val, -1, -1, ImGuiInputTextFlags_AutoSelectAll );
    ImGui::SameLine();
    ImGui::SetNextItemWidth( GetFrameHeight() * 1.5f );
    bool ret2 = ImGui::InputInt( "##max", &jmi.valmax, -1, -1, ImGuiInputTextFlags_AutoSelectAll );
    ImGui::PopID();
    return ret1 || ret2;
}

} // namespace ImGui

#include "editor_widgets.h"
#include "editor_main.h"

#include <string>

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

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

} // namespace ImGui

#ifndef CATA_SRC_EDITOR_EDITOR_WIDGETS_H
#define CATA_SRC_EDITOR_EDITOR_WIDGETS_H

struct ImVec4;
class nc_color;
struct jmapgen_int;
class jmapgen_place;

namespace ImGui
{
void SymbolColored( const std::string &sym, nc_color col );
void SymbolColored( int sym, nc_color col );
void JmapgenInt( const std::string &label, const jmapgen_int &jmi );
void JmapgenPlace( const std::string &label, const jmapgen_place &jmp );
} // namespace ImGui

#endif // CATA_SRC_EDITOR_EDITOR_WIDGETS_H

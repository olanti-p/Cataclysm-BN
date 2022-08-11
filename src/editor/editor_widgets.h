#ifndef CATA_SRC_EDITOR_EDITOR_WIDGETS_H
#define CATA_SRC_EDITOR_EDITOR_WIDGETS_H

#include <string>

struct ImVec4;
class nc_color;
struct jmapgen_int;
class jmapgen_place;

namespace editor
{
enum class AssetType : int;
struct editor_state;
} // namespace editor

namespace ImGui
{
void SymbolColored( const std::string &sym, nc_color col );
void SymbolColored( int sym, nc_color col );
void JmapgenInt( const std::string &label, const jmapgen_int &jmi );
void JmapgenPlace( const std::string &label, const jmapgen_place &jmp );

bool InputAssetId( editor::editor_state &state, const std::string &label, std::string &buf,
                   editor::AssetType atype );
} // namespace ImGui

#endif // CATA_SRC_EDITOR_EDITOR_WIDGETS_H

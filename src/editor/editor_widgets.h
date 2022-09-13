#ifndef CATA_SRC_EDITOR_EDITOR_WIDGETS_H
#define CATA_SRC_EDITOR_EDITOR_WIDGETS_H

#include <string>

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

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

template<typename Point>
bool DragPoint( const char *label, Point *p, float v_speed = 1.0f, int v_min = 0,
                int v_max = 0, const char *format = "%d", ImGuiSliderFlags flags = 0 )
{
    int v[2];
    v[0] = p->x();
    v[1] = p->y();
    bool ret = DragScalarN( label, ImGuiDataType_S32, v, 2, v_speed, &v_min, &v_max, format, flags );
    p->x() = v[0];
    p->y() = v[1];
    return ret;
}
} // namespace ImGui

#endif // CATA_SRC_EDITOR_EDITOR_WIDGETS_H

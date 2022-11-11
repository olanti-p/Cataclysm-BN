#ifndef CATA_SRC_EDITOR_EDITOR_WIDGETS_H
#define CATA_SRC_EDITOR_EDITOR_WIDGETS_H

#include <string>
#include <vector>

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include "editor_me_state.h"
#include "editor_widget_combofilter.h"
#include "editor_sprite_ref.h"

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

void Image( const SpriteRef &img, const ImVec2 &size );
bool ImageButton( const char *wid, const SpriteRef &img );
bool ImageButton( const char *wid, const SpriteRef &img, const ImVec2 &size );
bool ImageButton( const char *wid, const std::string &tile_id );
bool ImageButton( const char *wid, const std::string &tile_id, const ImVec2 &size );

/** WARNING: crashes when closing parent window! */
bool InputTextCompleting( const char *label, std::string &input,
                          const std::vector<std::string> &opts );

bool InputSymbol( const char *label, std::string &input, const char *fallback = "." );

namespace detail
{
bool InputId(
    const char *label,
    std::string &data,
    const std::vector<std::string> &opts,
    bool is_valid,
    ImGuiInputTextFlags flags,
    ImGuiInputTextCallback callback,
    void *user_data
);
}

template<typename T>
bool InputId( const char *label, editor::editable_id<T> &id, ImGuiInputTextFlags flags = 0,
              ImGuiInputTextCallback callback = NULL, void *user_data = NULL )
{
    return detail::InputId( label, id.data, editor::editable_id<T>::get_all_opts(), id.is_valid(),
                            flags, callback, user_data );
}

bool InputIntRange( const char *label, editor::me_int_range &r );

bool InputIntClamped( const char *label, int &val, int min, int max,
                      ImGuiInputTextFlags flags = 0 );

bool InputDuration( const char *label, time_duration &dur, ImGuiInputTextFlags flags = 0 );

void TextCentered( const std::string &text );

void BeginErrorArea();
void EndErrorArea();

/**
 * Helper to display a little (?) mark which shows a tooltip when hovered.
 *
 * Copied from ImGui's demo example.
 */
void HelpMarker( const char *desc );
void HelpMarkerInline( const char *desc );
void HelpPopup( const char *desc );

} // namespace ImGui

#endif // CATA_SRC_EDITOR_EDITOR_WIDGETS_H

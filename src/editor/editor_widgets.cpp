#include "editor_widgets.h"
#include "editor_main.h"

#include <string>

#include "../color.h"
#include "../catacharset.h"
#include "../mapgen.h"
#include "../sdltiles.h"
#include "../cata_tiles.h"

SpriteRef::SpriteRef( const std::string &id )
{
    const tileset &tset = tilecontext->get_tileset();
    const tile_type *t = tset.find_tile_type( id );
    if( t ) {
        tile_idx = t->fg.begin()->obj[0];
    }
}

std::pair<ImVec2, ImVec2> SpriteRef::make_uvs() const
{
    if( tile_idx < 0 ) {
        return std::make_pair( ImVec2( 0, 0 ), ImVec2( 1, 1 ) );
    }

    const tileset &tset = tilecontext->get_tileset();

    const texture *tex = tset.get_tile( tile_idx );

    auto rect = tex->rect();

    auto fullsize = tex->getsize();

    float w = rect.w;
    float h = rect.h;
    float x = rect.x;
    float y = rect.y;

    float fw = fullsize.x;
    float fh = fullsize.y;

    ImVec2 uv0( x / fw, y / fh );
    ImVec2 uv1( ( x + w ) / fw, ( y + h ) / fh );

    return std::make_pair( uv0, uv1 );
}

ImTextureID SpriteRef::get_tex_id() const
{
    if( tile_idx < 0 ) {
        return nullptr;
    }

    const tileset &tset = tilecontext->get_tileset();

    const texture *tex = tset.get_tile( tile_idx );

    return static_cast<void *>( tex->get_ptr() );
}

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

void Image( const SpriteRef &img, const ImVec2 &size )
{
    auto uvs = img.make_uvs();
    ImGui::Image( img.get_tex_id(), size, uvs.first, uvs.second );
}

bool ImageButton( const char *wid, const SpriteRef &img )
{
    ImVec2 sz( ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight() );
    return ImageButton( wid, img, sz );
}

bool ImageButton( const char *wid, const SpriteRef &img, const ImVec2 &size )
{
    auto uvs = img.make_uvs();
    ImGui::PushID( wid );
    bool ret = ImGui::ImageButton( img.get_tex_id(), size, uvs.first, uvs.second );
    ImGui::PopID();
    return ret;
}

bool ImageButton( const char *wid, const std::string &tile_id )
{
    return ImageButton( wid, SpriteRef( tile_id ) );
}

bool ImageButton( const char *wid, const std::string &tile_id, const ImVec2 &size )
{
    return ImageButton( wid, SpriteRef( tile_id ), size );
}

} // namespace ImGui

#include "imgui_internal.h"

#include "editor_widgets.h"
#include "editor_main.h"

#include "../color.h"
#include "../catacharset.h"
#include "../mapgen.h"
#include "../sdltiles.h"
#include "../cata_tiles.h"
#include "../string_utils.h"

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

bool detail::InputId( const char *label,
                      std::string &data,
                      const std::vector<std::string> &opts,
                      bool is_valid,
                      ImGuiInputTextFlags /*flags*/,
                      ImGuiInputTextCallback /*callback*/,
                      void * /*user_data*/ )
{
    // TODO: implement or remove args
    if( !is_valid ) {
        BeginErrorArea();
    }
    int current_item = -1;
    for( size_t i = 0; i < opts.size(); i++ ) {
        if( opts[i] == data ) {
            current_item = static_cast<int>( i );
            break;
        }
    }
    bool ret = ImGui::ComboWithFilter( label, &current_item, opts, 15 );
    if( current_item >= 0 ) {
        data = opts[ current_item ];
    }
    if( !is_valid ) {
        EndErrorArea();
    }
    return ret;
}

bool InputIntRange( const char *label, editor::me_int_range &r )
{
    ImGui::BeginGroup();
    ImGui::Text( "%s", label );
    ImGui::SameLine();
    ImGui::PushID( label );
    ImGui::SetNextItemWidth( GetFrameHeight() * 1.5f );
    bool ret1 = ImGui::InputInt( "##min", &r.min, -1, -1, ImGuiInputTextFlags_AutoSelectAll );
    ImGui::SameLine();
    ImGui::SetNextItemWidth( GetFrameHeight() * 1.5f );
    bool ret2 = ImGui::InputInt( "##max", &r.max, -1, -1, ImGuiInputTextFlags_AutoSelectAll );
    ImGui::PopID();
    ImGui::EndGroup();
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

/**
 * Text input with autocomplete suggestions.
 *
 * Authors: EricStancliff & ocornut
 *
 * Source: https://github.com/ocornut/imgui/issues/718#issuecomment-1249822993
 */
bool InputTextCompleting( const char *label, std::string &input,
                          const std::vector<std::string> &opts )
{
    // Code
    const bool is_input_text_enter_pressed = ImGui::InputText( label, &input,
            ImGuiInputTextFlags_EnterReturnsTrue );
    const bool is_input_text_active = ImGui::IsItemActive();
    const bool is_input_text_activated = ImGui::IsItemActivated();

    if( is_input_text_activated ) {
        ImGui::OpenPopup( "##popup_text_completion" );
    }

    bool ret = false;

    ImGui::SetNextWindowPos( ImVec2( ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y ) );
    ImGui::SetNextWindowSize( { ImGui::GetItemRectSize().x, 0 } );
    ImGui::SetNextWindowSizeConstraints( { ImGui::GetItemRectSize().x, 0 }, { ImGui::GetItemRectSize().x, ImGui::GetFrameHeight() * 15.0f } );
    if( ImGui::BeginPopup( "##popup_text_completion",
                           ImGuiWindowFlags_NoTitleBar |
                           ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoResize |
                           ImGuiWindowFlags_ChildWindow ) ) {

        if( input.size() < 1 ) {
            ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.3, 0.3f, 0.3f, 1.0f ) );
            ImGui::Text( "> Type more" );
            ImGui::PopStyleColor();
        } else {
            bool has_matches = false;
            for( const std::string &opt : opts ) {
                if( !lcmatch( opt, input ) ) {
                    continue;
                }
                has_matches = true;
                if( ImGui::Selectable( opt.c_str() ) ) {
                    ImGui::ClearActiveID();
                    input = opt;
                }
            }
            if( !has_matches ) {
                ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.3, 0.3f, 0.3f, 1.0f ) );
                ImGui::Text( "> Not found" );
                ImGui::PopStyleColor();
            }
        }

        if( is_input_text_enter_pressed || ( !is_input_text_active && !ImGui::IsWindowFocused() ) ) {
            ImGui::CloseCurrentPopup();
            ret = true;
        }

        ImGui::EndPopup();
    }

    return ret;
}

bool InputSymbol( const char *label, std::string &input, const char *fallback )
{
    if( input.empty() ) {
        input = fallback;
    }
    if( ImGui::InputText( label, &input, ImGuiInputTextFlags_AutoSelectAll ) ) {
        if( input.empty() ) {
            input = fallback;
        } else {
            // TODO: optionally forbid wide characters
            // TODO: accept combining characters
            std::u32string s32 = utf8_to_utf32( input );
            input = utf32_to_utf8( s32[0] );
        }
        return true;
    } else {
        return false;
    }
}

bool InputIntClamped( const char *label, int &val, int min, int max, ImGuiInputTextFlags flags )
{
    int val_new = val;
    if( InputInt( label, &val_new, -1, -1, flags ) ) {
        int clamped = clamp( val_new, min, max );
        if( clamped != val ) {
            val = clamped;
            return true;
        }
    }
    return false;
}

bool InputDuration( const char *label, time_duration &dur, ImGuiInputTextFlags flags )
{
    bool ret = false;
    ImGui::PushID( label );

    static bool show_decomposed = false;
    float checkbox_x_start = GetCursorPosX();
    ImGui::Checkbox( "###show-decomposed", &show_decomposed );
    ImGui::SameLine();
    float checkbox_x_size = GetCursorPosX() - checkbox_x_start;
    if( show_decomposed ) {
        int d = to_days<int>( dur );
        int h = to_hours<int>( dur % 1_days );
        int m = to_minutes<int>( dur % 1_hours );
        int s = to_seconds<int>( dur % 1_minutes );

        int d_max = to_days<int>( calendar::INDEFINITELY_LONG_DURATION ) - 1;
        int h_max = to_hours<int>( 1_days ) - 1;
        int m_max = to_minutes<int>( 1_hours ) - 1;
        int s_max = to_seconds<int>( 1_minutes ) - 1;

        float w_total = CalcItemWidth() -
                        checkbox_x_size -
                        ImGui::CalcTextSize( "d" ).x -
                        ImGui::CalcTextSize( "h" ).x -
                        ImGui::CalcTextSize( "m" ).x -
                        ImGui::CalcTextSize( "s" ).x -
                        ImGui::GetStyle().ItemSpacing.x * 7.0f;

        float w = w_total / 5.0f;

        ImGui::Text( "d" );
        ImGui::SameLine();
        ImGui::SetNextItemWidth( w * 2.0f );
        ret = ImGui::InputIntClamped( "###d", d, 0, d_max ) || ret;
        ImGui::SameLine();

        ImGui::Text( "h" );
        ImGui::SameLine();
        ImGui::SetNextItemWidth( w );
        ret = ImGui::InputIntClamped( "###h", h, 0, h_max ) || ret;
        ImGui::SameLine();

        ImGui::Text( "m" );
        ImGui::SameLine();
        ImGui::SetNextItemWidth( w );
        ret = ImGui::InputIntClamped( "###m", m, 0, m_max ) || ret;
        ImGui::SameLine();

        ImGui::Text( "s" );
        ImGui::SameLine();
        ImGui::SetNextItemWidth( w );
        ret = ImGui::InputIntClamped( "###s", s, 0, s_max ) || ret;
        ImGui::SameLine();

        ImGui::Text( "%s", label );

        dur = time_duration::from_days( d ) +
              time_duration::from_hours( h ) +
              time_duration::from_minutes( m ) +
              time_duration::from_seconds( s );
    } else {
        int t = to_turns<int>( dur );
        int t_max = to_turns<int>( calendar::INDEFINITELY_LONG_DURATION );

        float w = CalcItemWidth() -
                  checkbox_x_size -
                  ImGui::CalcTextSize( "turns" ).x -
                  ImGui::GetStyle().ItemSpacing.x;

        ImGui::Text( "turns" );
        ImGui::SameLine();
        ImGui::SetNextItemWidth( w );
        ret = ImGui::InputIntClamped( "###t", t, 0, t_max, flags );
        ImGui::SameLine();

        ImGui::Text( "%s",  label );

        dur = time_duration::from_turns( t );
    }
    ImGui::PopID();
    return ret;
}

void TextCentered( const std::string &text )
{
    float wnd_w = ImGui::GetWindowSize().x;
    float text_w = ImGui::CalcTextSize( text.c_str() ).x;
    ImGui::SetCursorPosX( wnd_w / 2.0f - text_w / 2.0f );
    ImGui::Text( "%s", text.c_str() );
}

void BeginErrorArea()
{
    ImGui::PushStyleColor( ImGuiCol_FrameBg, ImVec4( 0.8f, 0.3f, 0.3f, 1.0f ) );
}

void EndErrorArea()
{
    ImGui::PopStyleColor();
}

static void help_popup_common( const char *desc, ImGuiHoveredFlags flags )
{
    if( ImGui::IsItemHovered( flags ) ) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos( ImGui::GetFontSize() * 35.0f );
        ImGui::TextUnformatted( desc );
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void HelpMarker( const char *desc )
{
    ImGui::TextDisabled( "(?)" );
    help_popup_common( desc, ImGuiHoveredFlags_DelayShort );
}

void HelpMarkerInline( const char *desc )
{
    HelpMarker( desc );
    SameLine();
}

void HelpPopup( const char *desc )
{
    help_popup_common( desc, ImGuiHoveredFlags_DelayNormal );
}

} // namespace ImGui

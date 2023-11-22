#include "palette.h"

#include "common/color.h"
#include "state/state.h"
#include "state/ui_state.h"
#include "widget/widgets.h"

namespace editor
{

static void show_palette_entries_simple( State &state, Palette &palette )
{
    UUID selected = state.ui->tools->get_main_tile();
    ImGuiStyle &style = ImGui::GetStyle();
    int buttons_count = palette.entries.size();
    float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
    ImVec2 button_sz( 40, 40 );
    ImVec2 button_sz_text = button_sz + ImGui::GetStyle().FramePadding * 2;
    for( int n = 0; n < buttons_count; n++ ) {
        const PaletteEntry &entry = palette.entries[n];
        const SpriteRef *img = palette.sprite_from_uuid( entry.uuid );
        ImGui::PushID( n );
        bool is_selected = selected == entry.uuid;
        if( is_selected ) {
            ImGui::PushStyleColor( ImGuiCol_Button, col_selected_palette_entry );
            ImGui::PushStyleColor( ImGuiCol_ButtonHovered, col_selected_palette_entry );
            ImGui::PushStyleColor( ImGuiCol_ButtonActive, col_selected_palette_entry );
        }
        bool btn_result;
        if( img ) {
            btn_result = ImGui::ImageButton( "button", *img, button_sz );
        } else {
            std::string label = string_format( "%s###button", entry.key.str );
            btn_result = ImGui::Button( label.c_str(), button_sz_text );
        }
        if( btn_result && !is_selected ) {
            state.ui->tools->set_main_tile( entry.uuid );
        }
        if( ImGui::IsItemHovered( ImGuiHoveredFlags_DelayShort ) ) {
            ImGui::BeginTooltip();
            show_palette_entry_tooltip( entry );
            ImGui::EndTooltip();
        }
        if( is_selected ) {
            ImGui::PopStyleColor( 3 );
        }
        float last_button_x2 = ImGui::GetItemRectMax().x;
        float next_button_x2 = last_button_x2 + style.ItemSpacing.x + button_sz.x;
        if( n + 1 < buttons_count && next_button_x2 < window_visible_x2 ) {
            ImGui::SameLine();
        }
        ImGui::PopID();
    }
}

void show_palette_simple( State &state, Palette &p, bool &show )
{
    ImGui::SetNextWindowSize( ImVec2( 670.0f, 120.0f ), ImGuiCond_FirstUseEver );
    ImGui::SetNextWindowPos( ImVec2( 50.0f, 50.0f ), ImGuiCond_FirstUseEver );

    std::string name = p.display_name();
    std::string wnd_id = string_format( "Palette %s###palette-%d-simple", name, p.uuid );
    if( !ImGui::Begin( wnd_id.c_str(), &show ) ) {
        ImGui::End();
        return;
    }
    ImGui::PushID( p.uuid );
    ImGui::Text( "%s", name.c_str() );
    if( ImGui::Button( "Toggle verbose mode" ) ) {
        state.ui->toggle_show_palette_verbose( p.uuid );
    }

    show_palette_entries_simple( state, p );

    ImGui::PopID();
    ImGui::End();
}

} // namespace editor

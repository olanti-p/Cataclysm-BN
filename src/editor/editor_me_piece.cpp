#include "editor_me_piece.h"

#include "editor_me_state.h"
#include "editor_widgets.h"

#define REG_PIECE( piece_class ) ret.push_back( std::make_unique<piece_class>() )

namespace editor
{

void me_piece_field::show_ui( me_state &state )
{
    ImGui::HelpMarkerInline( "Type of the field." );
    if( ImGui::InputId( "ftype", ftype ) ) {
        state.mark_changed();
    }
    ImGui::HelpMarkerInline( "Intensity of the field." );
    if( ImGui::InputIntClamped( "intensity", intensity, 1, 3 ) ) {
        state.mark_changed();
    }
    ImGui::HelpMarkerInline( "Age of the field at the moment of spawn.  Affects decay rate." );
    if( ImGui::InputDuration( "age", age ) ) {
        state.mark_changed();
    }
}

void me_piece_npc::show_ui( me_state &state )
{
    ImGui::HelpMarkerInline( "NPC template to use." );
    if( ImGui::InputId( "npc_class", npc_class ) ) {
        state.mark_changed();
    }
    ImGui::HelpMarkerInline( "Whether this NPC is the target of a quest." );
    if( ImGui::Checkbox( "target", &target ) ) {
        state.mark_changed();
    }
    ImGui::HelpMarkerInline( "List of additional character traits applied on spawn." );
    ImGui::Text( "TODO: traits" );
}

void me_piece_faction::show_ui( me_state &state )
{
    ImGui::Text( "TODO" );
}

static void sign_or_graffiti(
    me_state &state,
    bool is_sign,
    bool &use_snippet,
    snippet_category_eid &snippet,
    std::string &text
)
{
    ImGui::HelpMarkerInline( "Signs and graffiti can use either exact text string or a random snippet from category." );
    if( ImGui::Checkbox( "Use snippet from category", &use_snippet ) ) {
        state.mark_changed();
    }
    if( !use_snippet ) {
        ImGui::BeginDisabled();
    }
    ImGui::HelpMarkerInline( "Snippet category to draw from." );
    if( ImGui::InputId( "snippet", snippet ) ) {
        state.mark_changed();
    }
    if( !use_snippet ) {
        ImGui::EndDisabled();
    } else {
        ImGui::BeginDisabled();
    }
    ImGui::HelpMarkerInline( "Exact text to use." );
    if( ImGui::InputText( "text", &text ) ) {
        state.mark_changed( is_sign ? "me-piece-sign-text-entry" : "me-piece-graffiti-text-entry" );
    }
    if( use_snippet ) {
        ImGui::EndDisabled();
    }
}

void me_piece_sign::show_ui( me_state &state )
{
    sign_or_graffiti( state, true, use_snippet, snippet, text );
}

void me_piece_graffiti::show_ui( me_state &state )
{
    sign_or_graffiti( state, false, use_snippet, snippet, text );
}

void me_piece_vending_machine::show_ui( me_state &state )
{
    ImGui::Text( "TODO" );
}

void me_piece_toilet::show_ui( me_state &state )
{
    ImGui::Text( "TODO" );
}

void me_piece_gaspump::show_ui( me_state &state )
{
    ImGui::Text( "TODO" );
}

void me_piece_liquid::show_ui( me_state &state )
{
    ImGui::Text( "TODO" );
}

void me_piece_igroup::show_ui( me_state &state )
{
    ImGui::Text( "TODO" );
}

void me_piece_loot::show_ui( me_state &state )
{
    ImGui::Text( "TODO" );
}

void me_piece_mgroup::show_ui( me_state &state )
{
    ImGui::Text( "TODO" );
}

void me_piece_monster::show_ui( me_state &state )
{
    ImGui::Text( "TODO" );
}

void me_piece_vehicle::show_ui( me_state &state )
{
    ImGui::Text( "TODO" );
}

void me_piece_item::show_ui( me_state &state )
{
    ImGui::Text( "TODO" );
}

void me_piece_trap::show_ui( me_state &state )
{
    ImGui::Text( "TODO" );
}

void me_piece_furniture::show_ui( me_state &state )
{
    ImGui::Text( "TODO" );
}

void me_piece_terrain::show_ui( me_state &state )
{
    ImGui::Text( "TODO" );
}

void me_piece_ter_furn_transform::show_ui( me_state &state )
{
    ImGui::Text( "TODO" );
}

void me_piece_make_rubble::show_ui( me_state &state )
{
    ImGui::Text( "TODO" );
}

void me_piece_computer::show_ui( me_state &state )
{
    ImGui::Text( "TODO" );
}

void me_piece_sealed_item::show_ui( me_state &state )
{
    ImGui::Text( "TODO" );
}

void me_piece_translate::show_ui( me_state &state )
{
    ImGui::Text( "TODO" );
}

void me_piece_zone::show_ui( me_state &state )
{
    ImGui::Text( "TODO" );
}

void me_piece_nested::show_ui( me_state &state )
{
    ImGui::Text( "TODO" );
}

void me_piece_alt_trap::show_ui( me_state &state )
{
    ImGui::Text( "TODO" );
}

void me_piece_alt_furniture::show_ui( me_state &state )
{
    ImGui::Text( "TODO" );
}

void me_piece_alt_terrain::show_ui( me_state &state )
{
    ImGui::Text( "TODO" );
}

const std::vector<std::unique_ptr<me_piece>> &get_piece_templates()
{
    static std::vector<std::unique_ptr<me_piece>> ret;
    if( ret.empty() ) {
        ret.reserve( static_cast<int>( PieceType::NumJmTypes ) );
        REG_PIECE( me_piece_field );
        REG_PIECE( me_piece_npc );
        REG_PIECE( me_piece_faction );
        REG_PIECE( me_piece_sign );
        REG_PIECE( me_piece_graffiti );
        REG_PIECE( me_piece_vending_machine );
        REG_PIECE( me_piece_toilet );
        REG_PIECE( me_piece_gaspump );
        REG_PIECE( me_piece_liquid );
        REG_PIECE( me_piece_igroup );
        REG_PIECE( me_piece_loot );
        REG_PIECE( me_piece_mgroup );
        REG_PIECE( me_piece_monster );
        REG_PIECE( me_piece_vehicle );
        REG_PIECE( me_piece_item );
        REG_PIECE( me_piece_trap );
        REG_PIECE( me_piece_furniture );
        REG_PIECE( me_piece_terrain );
        REG_PIECE( me_piece_ter_furn_transform );
        REG_PIECE( me_piece_make_rubble );
        REG_PIECE( me_piece_computer );
        REG_PIECE( me_piece_sealed_item );
        REG_PIECE( me_piece_translate );
        REG_PIECE( me_piece_zone );
        REG_PIECE( me_piece_nested );
        REG_PIECE( me_piece_alt_trap );
        REG_PIECE( me_piece_alt_furniture );
        REG_PIECE( me_piece_alt_terrain );
    }
    return ret;
}

std::unique_ptr<me_piece> make_new_piece( PieceType pt )
{
    for( const std::unique_ptr<me_piece> &it : get_piece_templates() ) {
        if( it->get_type() == pt ) {
            return it->clone();
        }
    }
    std::cerr << "Failed to generate piece with type " << static_cast<int>( pt ) << std::endl;
    std::abort();
}

} // namespace editor

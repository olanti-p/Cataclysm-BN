#include "editor_me_piece_impl.h"

#include "editor_me_state.h"
#include "editor_widgets.h"

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
    ImGui::HelpMarkerInline( "Faction id string.\n\nWARNING: no validation is done here." );
    // TODO: validation
    if( ImGui::InputText( "id", &id ) ) {
        state.mark_changed( "me-piece-faction-id-input" );
    }
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
        state.mark_changed( is_sign ? "me-piece-sign-text-input" : "me-piece-graffiti-text-input" );
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
    ImGui::HelpMarkerInline( "Whether this vending machine is reinforced." );
    if( ImGui::Checkbox( "reinforced", &reinforced ) ) {
        state.mark_changed();
    }
    ImGui::HelpMarkerInline( "Whether to use \"default_vending_machine\" group." );
    if( ImGui::Checkbox( "Use default item group", &use_default_group ) ) {
        state.mark_changed();
    }
    if( use_default_group ) {
        ImGui::BeginDisabled();
    }
    ImGui::HelpMarkerInline( "Item group id string.\n\nWARNING: no validation is done here." );
    // TODO: validation
    if( ImGui::InputText( "item_group", &item_group ) ) {
        state.mark_changed( "me-piece-vending-machine-item-group-input" );
    }
    if( use_default_group ) {
        ImGui::EndDisabled();
    }
}

void me_piece_toilet::show_ui( me_state &state )
{
    ImGui::HelpMarkerInline( "Whether to use default amount (24)." );
    if( ImGui::Checkbox( "Use default amount", &use_default_amount ) ) {
        state.mark_changed();
    }
    if( use_default_amount ) {
        ImGui::BeginDisabled();
    }
    ImGui::HelpMarkerInline( "Amount of water to spawn, [min, max]." );
    if( ImGui::InputIntRange( "amount", amount ) ) {
        state.mark_changed( "me-piece-toilet-amount-input" );
    }
    if( use_default_amount ) {
        ImGui::EndDisabled();
    }
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

} // namespace editor

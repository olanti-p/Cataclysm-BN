#include "editor_me_piece_impl.h"

#include "editor_me_style.h"
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
    ImGui::Text( "traits:" );
    ImGui::Indent( style::list_indent );
    bool changed = ImGui::VectorWidget()
    .with_for_each( [&]( size_t idx ) {
        if( ImGui::InputId( "##trait-input", traits[idx] ) ) {
            state.mark_changed();
        }
    } ).run( traits );
    if( changed ) {
        state.mark_changed();
    }
    ImGui::Indent( -style::list_indent );
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
    ImGui::HelpMarkerInline( "Item group to spawn items from." );
    if( ImGui::InputId( "item_group", item_group ) ) {
        state.mark_changed();
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
    ImGui::HelpMarkerInline( "Whether to use default amount\n( random value within [10000, 50000] )." );
    if( ImGui::Checkbox( "Use default amount", &use_default_amount ) ) {
        state.mark_changed();
    }
    if( use_default_amount ) {
        ImGui::BeginDisabled();
    }
    ImGui::HelpMarkerInline( "Amount of fuel to spawn, [min, max]." );
    if( ImGui::InputIntRange( "amount", amount ) ) {
        state.mark_changed( "me-piece-gaspump-amount-input" );
    }
    if( use_default_amount ) {
        ImGui::EndDisabled();
    }

    ImGui::HelpMarkerInline(
        "Type of fuel to spawn.\n\n"
        "Setting it to 'Random' will choose randomly between diesel (25% chance) and gasoline (75% chance)."
    );
    ImGui::Text( "Fuel type:" );
    if( ImGui::RadioButton( "Random", fuel == GasPumpFuel::Random ) ) {
        fuel = GasPumpFuel::Random;
    }
    if( ImGui::RadioButton( "Diesel", fuel == GasPumpFuel::Diesel ) ) {
        fuel = GasPumpFuel::Diesel;
    }
    if( ImGui::RadioButton( "Gasoline", fuel == GasPumpFuel::Gasoline ) ) {
        fuel = GasPumpFuel::Gasoline;
    }
}

void me_piece_liquid::show_ui( me_state &state )
{
    // TODO: show default amount from item
    ImGui::HelpMarkerInline( "Whether to use default amount\n( derived from item type )." );
    if( ImGui::Checkbox( "Use default amount", &use_default_amount ) ) {
        state.mark_changed();
    }
    if( use_default_amount ) {
        ImGui::BeginDisabled();
    }
    ImGui::HelpMarkerInline( "Amount of liquid to spawn, [min, max]." );
    if( ImGui::InputIntRange( "amount", amount ) ) {
        state.mark_changed( "me-piece-liquid-amount-input" );
    }
    if( use_default_amount ) {
        ImGui::EndDisabled();
    }

    ImGui::HelpMarkerInline( "Type of liquid to spawn.\n\nWARNING: no validation is done here." );
    if( ImGui::InputId( "liquid", liquid ) ) {
        state.mark_changed();
    }

    ImGui::HelpMarkerInline( "Whether to spawn always, or with a chance." );
    if( ImGui::Checkbox( "Always", &spawn_always ) ) {
        state.mark_changed();
    }
    if( spawn_always ) {
        ImGui::BeginDisabled();
    }
    ImGui::HelpMarkerInline(
        "Chance to spawn, non-linear.\n\n"
        "Formula is:  one_in( rng( [min, max] ) )"
    );
    if( ImGui::InputIntRange( "chance", chance ) ) {
        state.mark_changed( "me-piece-liquid-chance-input" );
    }
    if( spawn_always ) {
        ImGui::EndDisabled();
    }
}

void me_piece_igroup::show_ui( me_state &state )
{
    ImGui::HelpMarkerInline( "Whether to spawn always, or with a chance." );
    if( ImGui::Checkbox( "Always", &spawn_always ) ) {
        state.mark_changed();
    }
    ImGui::BeginDisabled( spawn_always );
    ImGui::HelpMarkerInline( "TODO" );
    if( ImGui::InputIntRange( "chance", chance ) ) {
        state.mark_changed( "me-piece-igroup-chance-input" );
    }
    ImGui::EndDisabled();

    ImGui::HelpMarkerInline( "Whether to spawn once, or multiple times." );
    if( ImGui::Checkbox( "Once", &spawn_once ) ) {
        state.mark_changed();
    }
    ImGui::BeginDisabled( spawn_once );
    ImGui::HelpMarkerInline( "TODO" );
    if( ImGui::InputIntRange( "repeat", repeat ) ) {
        state.mark_changed( "me-piece-igroup-repeat-input" );
    }
    ImGui::EndDisabled();

    // TODO: inline item groups
    ImGui::HelpMarkerInline( "Item group to spawn." );
    if( ImGui::InputId( "group_id", group_id ) ) {
        state.mark_changed();
    }
}

void me_piece_loot::show_ui( me_state &state )
{
    ImGui::Text( "TODO" );
}

void me_piece_mgroup::show_ui( me_state &state )
{
    ImGui::HelpMarkerInline( "Whether to spawn always, or with a chance." );
    if( ImGui::Checkbox( "Always", &spawn_always ) ) {
        state.mark_changed();
    }
    ImGui::BeginDisabled( spawn_always );
    ImGui::HelpMarkerInline( "TODO" );
    if( ImGui::InputIntRange( "chance", chance ) ) {
        state.mark_changed( "me-piece-mgroup-chance-input" );
    }
    ImGui::EndDisabled();

    ImGui::HelpMarkerInline( "Monster group to spawn." );
    if( ImGui::InputId( "group_id", group_id ) ) {
        state.mark_changed();
    }

    ImGui::HelpMarkerInline( "Whether to use default map density, or a custom one." );
    if( ImGui::Checkbox( "Use default density", &use_default_density ) ) {
        state.mark_changed();
    }
    ImGui::BeginDisabled( use_default_density );
    ImGui::HelpMarkerInline( "TODO" );
    // TODO: validation
    if( ImGui::InputFloat( "density", &density ) ) {
        state.mark_changed( "me-piece-mgroup-density-input" );
    }
    ImGui::EndDisabled();
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

template<typename T>
void show_piece_alt( me_state &state, editor::me_weighted_list<T> &list )
{
    ImGui::Indent( style::list_indent );

    const auto can_delete = [&]( size_t ) -> bool {
        return list.entries.size() > 1;
    };

    const auto show_val = [&]( size_t i ) {
        ImGui::SetNextItemWidth( ImGui::GetFrameHeight() * 5.0f );
        bool bad_weight = list.entries[i].weight <= 0;
        if( bad_weight ) {
            ImGui::BeginErrorArea();
        }
        if( ImGui::InputInt( "##weight", &list.entries[i].weight ) ) {
            state.mark_changed( "entry-weight" );
        }
        if( bad_weight ) {
            ImGui::EndErrorArea();
        }
        ImGui::HelpPopup( "Weight" );
        ImGui::SameLine();

        ImGui::SetNextItemWidth( ImGui::GetFrameHeight() * 10.0f );
        if( ImGui::InputId( "##ter", list.entries[i].val ) ) {
            state.mark_changed();
        }
        ImGui::HelpPopup( "Value" );
    };

    if(
        ImGui::VectorWidget()
        .with_for_each( show_val )
        .with_can_delete( can_delete )
        .run( list.entries ) ) {
        state.mark_changed();
    }

    ImGui::Indent( -style::list_indent );
}

void me_piece_alt_trap::init_new()
{
    list.entries.emplace_back();
    list.entries.back().weight = 1;
}

void me_piece_alt_trap::show_ui( me_state &state )
{
    show_piece_alt( state, list );
}

void me_piece_alt_furniture::init_new()
{
    list.entries.emplace_back();
    list.entries.back().weight = 1;
}

void me_piece_alt_furniture::show_ui( me_state &state )
{
    show_piece_alt( state, list );
}

void me_piece_alt_terrain::init_new()
{
    list.entries.emplace_back();
    list.entries.back().weight = 1;
}

void me_piece_alt_terrain::show_ui( me_state &state )
{
    show_piece_alt( state, list );
}

} // namespace editor

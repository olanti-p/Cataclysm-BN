#include "editor_assets.h"
#include "editor_main.h"
#include "editor_widgets.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include "../mapgen_piece.h"

void jmapgen_field::show_details() const
{
    ImGui::Text( "ftype: %s", ftype.id().c_str() );
    ImGui::Text( "intensity: %d", intensity );
    ImGui::Text( "age: %d", to_turns<int>( age ) );
}

void jmapgen_npc::show_details() const
{
    ImGui::Text( "npc_class: %s", npc_class.c_str() );
    ImGui::Text( "target: %d", target ? 1 : 0 );
    int num_traits = static_cast<int>( traits.size() );
    if( ImGui::TreeNode( "npc_piece_traits", "traits: %d elem(s)", num_traits ) ) {
        for( int i = 0; i < num_traits; i++ ) {
            ImGui::Text( "%s", traits[i].c_str() );
        }
        ImGui::TreePop();
    }
}

void jmapgen_faction::show_details() const
{
    ImGui::Text( "id: %s", id.c_str() );
}

void jmapgen_sign::show_details() const
{
    ImGui::Text( "signage: %s", signage.c_str() );
    ImGui::Text( "snippet: %s", snippet.c_str() );
}

void jmapgen_graffiti::show_details() const
{
    ImGui::Text( "text: %s", text.c_str() );
    ImGui::Text( "snippet: %s", snippet.c_str() );
}

void jmapgen_vending_machine::show_details() const
{
    ImGui::Text( "reinforced: %d", reinforced ? 1 : 0 );
    ImGui::Text( "item_group: %s", item_group.c_str() );
}

void jmapgen_toilet::show_details() const
{
    ImGui::JmapgenInt( "amount", amount );
}

void jmapgen_gaspump::show_details() const
{
    ImGui::JmapgenInt( "amount", amount );
    ImGui::Text( "fuel: %s", fuel.c_str() );
}

void jmapgen_liquid_item::show_details() const
{
    ImGui::JmapgenInt( "amount", amount );
    ImGui::Text( "liquid: %s", liquid.c_str() );
    ImGui::JmapgenInt( "chance", chance );
}

void jmapgen_item_group::show_details() const
{
    ImGui::Text( "group_id: %s", group_id.c_str() );
    ImGui::JmapgenInt( "chance", chance );
}

void jmapgen_loot::show_details() const
{
    ImGui::JmapgenInt( "chance", chance );
    ImGui::Text( "result_group: %d elem(s)", static_cast<int>( result_group.get_items().size() ) );
}

void jmapgen_monster_group::show_details() const
{
    ImGui::Text( "id: %s", id.c_str() );
    ImGui::Text( "density: %f", density );
    ImGui::JmapgenInt( "chance", chance );
}

void jmapgen_monster::show_details() const
{
    ImGui::Text( "ids: %d elem(s)", static_cast<int>( ids.size() ) );
    ImGui::Text( "m_id: %s", m_id.c_str() );
    ImGui::JmapgenInt( "chance", chance );
    ImGui::JmapgenInt( "pack_size", pack_size );
    ImGui::Text( "one_or_none: %d", one_or_none ? 1 : 0 );
    ImGui::Text( "friendly: %d", friendly ? 1 : 0 );
    ImGui::Text( "name: %s", name.c_str() );
    ImGui::Text( "target: %d", target ? 1 : 0 );
}

void jmapgen_vehicle::show_details() const
{
    ImGui::Text( "type: %s", type.c_str() );
    ImGui::JmapgenInt( "chance", chance );
    ImGui::Text( "rotation: %d elem(s)", static_cast<int>( rotation.size() ) );
    ImGui::Text( "fuel: %d", fuel );
    ImGui::Text( "status: %d", status );
}

void jmapgen_spawn_item::show_details() const
{
    ImGui::Text( "type: %s", type.c_str() );
    ImGui::JmapgenInt( "amount", amount );
    ImGui::JmapgenInt( "chance", chance );
}

void jmapgen_trap::show_details() const
{
    ImGui::Text( "id: %s", id.id().c_str() );
}

void jmapgen_furniture::show_details() const
{
    ImGui::Text( "id: %s", id.id().c_str() );
}

void jmapgen_terrain::show_details() const
{
    ImGui::Text( "id: %s", id.id().c_str() );
}

void jmapgen_ter_furn_transform::show_details() const
{
    ImGui::Text( "id: %s", id.c_str() );
}

void jmapgen_make_rubble::show_details() const
{
    ImGui::Text( "rubble_type: %s", rubble_type.id().c_str() );
    ImGui::Text( "items: %d", items ? 1 : 0 );
    ImGui::Text( "floor_type: %s", floor_type.id().c_str() );
    ImGui::Text( "overwrite: %d", overwrite ? 1 : 0 );
}

void jmapgen_computer::show_details() const
{
    ImGui::Text( "name: %s", name.raw.c_str() );
    ImGui::Text( "access_denied: %s", access_denied.raw.c_str() );
    ImGui::Text( "security: %d", security );
    ImGui::Text( "options: %d elem(s)", static_cast<int>( options.size() ) );
    ImGui::Text( "failures: %d elem(s)", static_cast<int>( failures.size() ) );
    ImGui::Text( "target: %d", target ? 1 : 0 );
}

void jmapgen_sealed_item::show_details() const
{
    ImGui::Text( "furniture: %s", furniture.id().c_str() );
    ImGui::JmapgenInt( "chance", chance );
    ImGui::Text( "item_spawner:" );
    ImGui::Separator();
    if( item_spawner ) {
        item_spawner->show_details();
    } else {
        ImGui::Text( "<None>" );
    }
    ImGui::Separator();
    ImGui::Text( "item_group_spawner:" );
    ImGui::Separator();
    if( item_group_spawner ) {
        item_group_spawner->show_details();
    } else {
        ImGui::Text( "<None>" );
    }
}

void jmapgen_translate::show_details() const
{
    ImGui::Text( "from: %s", from.id().c_str() );
    ImGui::Text( "to: %s", to.id().c_str() );
}

void jmapgen_zone::show_details() const
{
    ImGui::Text( "zone_type: %s", zone_type.c_str() );
    ImGui::Text( "faction: %s", faction.c_str() );
    ImGui::Text( "name: %s", name.c_str() );
}

void jmapgen_nested::show_details() const
{
    ImGui::Text( "entries: %d elem(s)", static_cast<int>( entries.size() ) );
    ImGui::Text( "else_entries: %d elem(s)", static_cast<int>( else_entries.size() ) );

    const auto show_set = []( const char *fmt, const std::set<oter_str_id> &val ) {
        ImGui::Text( fmt, static_cast<int>( val.size() ) );
    };

    show_set( "neighbours_n: %d elem(s)",
              neighbors.neighbors[static_cast<int>( om_direction::type::north )] );
    show_set( "neighbours_e: %d elem(s)",
              neighbors.neighbors[static_cast<int>( om_direction::type::east )] );
    show_set( "neighbours_s: %d elem(s)",
              neighbors.neighbors[static_cast<int>( om_direction::type::south )] );
    show_set( "neighbours_w: %d elem(s)",
              neighbors.neighbors[static_cast<int>( om_direction::type::west )] );
    show_set( "neighbours_above: %d elem(s)", neighbors.above );
}

template<>
void jmapgen_alternativly_trap::show_details() const
{
    ImGui::Text( "alternatives: %d elem(s)", static_cast<int>( alternatives.size() ) );
}

template<>
void jmapgen_alternativly_furniture::show_details() const
{
    ImGui::Text( "alternatives: %d elem(s)", static_cast<int>( alternatives.size() ) );
}

template<>
void jmapgen_alternativly_terrain::show_details() const
{
    ImGui::Text( "alternatives: %d elem(s)", static_cast<int>( alternatives.size() ) );
}

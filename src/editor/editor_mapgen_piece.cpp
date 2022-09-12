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
    ImGui::Text( "TODO" );
}

void jmapgen_faction::show_details() const
{
    ImGui::Text( "TODO" );
}

void jmapgen_sign::show_details() const
{
    ImGui::Text( "TODO" );
}

void jmapgen_graffiti::show_details() const
{
    ImGui::Text( "TODO" );
}

void jmapgen_vending_machine::show_details() const
{
    ImGui::Text( "TODO" );
}

void jmapgen_toilet::show_details() const
{
    ImGui::JmapgenInt( "amount", amount );
}

void jmapgen_gaspump::show_details() const
{
    ImGui::Text( "TODO" );
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
    ImGui::Text( "TODO" );
}

void jmapgen_monster::show_details() const
{
    ImGui::Text( "ids:%d", static_cast<int>( ids.size() ) );
    ImGui::Text( "m_id:%s", m_id.c_str() );
    ImGui::JmapgenInt( "chance", chance );
    ImGui::JmapgenInt( "pack_size", pack_size );
    ImGui::Text( "one_or_none:%d", one_or_none ? 1 : 0 );
    ImGui::Text( "friendly:%d", friendly ? 1 : 0 );
    ImGui::Text( "target:%d", target ? 1 : 0 );
    ImGui::Text( "name:%s", name.c_str() );
}

void jmapgen_vehicle::show_details() const
{
    ImGui::Text( "TODO" );
}

void jmapgen_spawn_item::show_details() const
{
    ImGui::Text( "TODO" );
}

void jmapgen_trap::show_details() const
{
    ImGui::Text( "TODO" );
}

void jmapgen_furniture::show_details() const
{
    ImGui::Text( "TODO" );
}

void jmapgen_terrain::show_details() const
{
    ImGui::Text( "TODO" );
}

void jmapgen_ter_furn_transform::show_details() const
{
    ImGui::Text( "TODO" );
}

void jmapgen_make_rubble::show_details() const
{
    ImGui::Text( "TODO" );
}

void jmapgen_computer::show_details() const
{
    ImGui::Text( "TODO" );
}

void jmapgen_sealed_item::show_details() const
{
    ImGui::Text( "TODO" );
}

void jmapgen_translate::show_details() const
{
    ImGui::Text( "TODO" );
}

void jmapgen_zone::show_details() const
{
    ImGui::Text( "TODO" );
}

void jmapgen_nested::show_details() const
{
    ImGui::Text( "TODO" );
}

template<>
void jmapgen_alternativly_trap::show_details() const
{
    ImGui::Text( "TODO" );
}

template<>
void jmapgen_alternativly_furniture::show_details() const
{
    ImGui::Text( "TODO" );
}

template<>
void jmapgen_alternativly_terrain::show_details() const
{
    ImGui::Text( "TODO" );
}

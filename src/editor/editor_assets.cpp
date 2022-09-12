#include "editor_assets.h"
#include "editor_main.h"
#include "editor_widgets.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include "../field.h"
#include "../game.h"
#include "../item_factory.h"
#include "../item_group.h"
#include "../mapdata.h"
#include "../mapgen.h"
#include "../mapgen_factory.h"
#include "../mapgen_piece.h"
#include "../mongroup.h"
#include "../monstergenerator.h"
#include "../string_formatter.h"
#include "../string_utils.h"
#include "../trap.h"

namespace editor
{

const char *get_asset_type_name( AssetType at )
{
    switch( at ) {
        case AssetType::Terrain:
            return "Terrain";
        case AssetType::Furniture:
            return "Furniture";
        case AssetType::Trap:
            return "Trap";
        case AssetType::Field:
            return "Field";
        case AssetType::Itype:
            return "Itype";
        case AssetType::Igroup:
            return "Igroup";
        case AssetType::Mtype:
            return "Mtype";
        case AssetType::Mgroup:
            return "Mgroup";
        case AssetType::Palette:
            return "Palette";
        case AssetType::NestedMapgen:
            return "NestedMapgen";
        case AssetType::UpdateMapgen:
            return "UpdateMapgen";
        case AssetType::OterMapgen:
            return "OterMapgen";
        default:
            std::abort();
    }
}

const char *asset_terrain::get_id() const
{
    return ref.id.c_str();
}
const char *asset_furniture::get_id() const
{
    return ref.id.c_str();
}
const char *asset_trap::get_id() const
{
    return ref.id.c_str();
}
const char *asset_field_type::get_id() const
{
    return ref.id.c_str();
}
const char *asset_itype::get_id() const
{
    return ref.id.c_str();
}
const char *asset_igroup::get_id() const
{
    return ref.id.c_str();
}
const char *asset_mtype::get_id() const
{
    return ref.id.c_str();
}
const char *asset_mgroup::get_id() const
{
    return ref.id.c_str();
}
const char *asset_palette::get_id() const
{
    return ref.id.c_str();
}
const char *asset_nested_mapgen::get_id() const
{
    return ref.id.c_str();
}
const char *asset_update_mapgen::get_id() const
{
    return ref.id.c_str();
}
const char *asset_oter_mapgen::get_id() const
{
    return ref.id.c_str();
}

static void show_common_furn_ter( const map_data_common_t &ref )
{
    ImGui::Text( "Name: %s", ref.name().c_str() );
    ImGui::Text( "Symbol:" );
    ImGui::SameLine();
    ImGui::SymbolColored( ref.symbol(), ref.color() );
    ImGui::SameLine();
    ImGui::Text( "Movecost: %d", ref.movecost );
}

void asset_terrain::show_details() const
{
    show_common_furn_ter( ref );
}
void asset_furniture::show_details() const
{
    show_common_furn_ter( ref );
}
void asset_trap::show_details() const
{
    ImGui::Text( "Name: %s", ref.name().c_str() );
}
void asset_field_type::show_details() const
{
    int max_int = ref.get_max_intensity();
    for( int i = 0; i < max_int; i++ ) {
        ImGui::Text( "[%d]", i + 1 );
        ImGui::SameLine();
        ImGui::SymbolColored( ref.get_symbol( i ), ref.get_color( i ) );
        ImGui::SameLine();
        ImGui::Text( "%s", ref.get_name( i ).c_str() );
    }
}
void asset_itype::show_details() const
{
    ImGui::Text( "Name: %s", ref.nname( 1 ).c_str() );
    ImGui::Text( "Symbol:" );
    ImGui::SameLine();
    ImGui::SymbolColored( ref.sym, ref.color );
}
void asset_igroup::show_details() const
{
    ImGui::Text( "TODO" );
}
void asset_mtype::show_details() const
{
    ImGui::Text( "Name: %s", ref.nname().c_str() );
    ImGui::Text( "Symbol:" );
    ImGui::SameLine();
    ImGui::SymbolColored( ref.sym, ref.color );
}
void asset_mgroup::show_details() const
{
    ImGui::Text( "TODO" );
}
void asset_palette::show_details() const
{
    ImGui::Text( "TODO" );
}
void asset_nested_mapgen::show_details() const
{
    ref.data->editor_show_details();
}
void asset_update_mapgen::show_details() const
{
    ref.data->editor_show_details();
}
void asset_oter_mapgen::show_details() const
{
    ref.data->editor_show_details();
}

void init_assets( asset_library &assets )
{
    for( int i = 0; i < static_cast<int>( AssetType::NumAssetTypes ); i++ ) {
        assets.add_cat( static_cast<AssetType>( i ) );
    }
    for( const ter_t &elem : ter_t::get_all() ) {
        assets.add_asset<asset_terrain>( elem );
    }
    for( const furn_t &elem : furn_t::get_all() ) {
        assets.add_asset<asset_furniture>( elem );
    }
    for( const trap &elem : trap::get_all() ) {
        assets.add_asset<asset_trap>( elem );
    }
    for( const field_type &elem : field_types::get_all() ) {
        assets.add_asset<asset_field_type>( elem );
    }
    for( const itype *elem : item_controller->all() ) {
        assets.add_asset<asset_itype>( *elem );
    }
    for( const item_group_id &elem : item_controller->get_all_group_names() ) {
        igroup_plug plug;
        plug.id = elem;
        assets.igroup_plugs.push_back( std::move( plug ) );
    }
    for( const igroup_plug &elem : assets.igroup_plugs ) {
        assets.add_asset<asset_igroup>( elem );
    }
    for( const mtype &elem : MonsterGenerator::generator().get_all_mtypes() ) {
        assets.add_asset<asset_mtype>( elem );
    }
    for( const auto &elem : MonsterGroupManager::get_all() ) {
        assets.add_asset<asset_mgroup>( elem.second );
    }
    for( const auto &elem : mapgen_palette::get_all() ) {
        assets.add_asset<asset_palette>( elem.second );
    }

    const auto &all_nested = get_all_nested_mapgen();
    for( auto &it : all_nested ) {
        const std::string &id = it.first;
        int i = 0;
        for( auto &obj : it.second ) {
            nested_mapgen_plug plug;
            plug.id = string_format( "%s:w=%d:i=%d", id, obj.weight, i );
            plug.data = obj.obj.get();
            assets.nested_mapgen_plugs.push_back( std::move( plug ) );
            i++;
        }
    }
    for( const nested_mapgen_plug &elem : assets.nested_mapgen_plugs ) {
        assets.add_asset<asset_nested_mapgen>( elem );
    }

    const auto &all_update = get_all_update_mapgen();
    for( const auto &it : all_update ) {
        const std::string &id = it.first;
        int i = 0;
        for( const auto &obj : it.second ) {
            update_mapgen_plug plug;
            plug.id = string_format( "%s:i=%d", id, i );
            plug.data = obj.get();
            assets.update_mapgen_plugs.push_back( std::move( plug ) );
            i++;
        }
    }
    for( const update_mapgen_plug &elem : assets.update_mapgen_plugs ) {
        assets.add_asset<asset_update_mapgen>( elem );
    }

    const mapgen_factory &all_oter = get_all_oter_mapgen();
    for( const auto &it : all_oter.mapgens_ ) {
        const std::string &id = it.first;
        int i = 0;
        for( const auto &obj : it.second.weights_ ) {
            oter_mapgen_plug plug;
            plug.id = string_format( "%s:w=%d:i=%d", id, obj.weight, i );
            plug.data = obj.obj.get();
            assets.oter_mapgen_plugs.push_back( std::move( plug ) );
            i++;
        }
    }
    for( const oter_mapgen_plug &elem : assets.oter_mapgen_plugs ) {
        assets.add_asset<asset_oter_mapgen>( elem );
    }
}

} // namespace editor

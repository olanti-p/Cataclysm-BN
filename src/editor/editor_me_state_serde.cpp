#include "editor_me_state.h"

#include "../json.h"
#include "../mapgen.h"

void serialize( const std::unique_ptr<editor::me_piece> &ptr, JsonOut &jsout )
{
    jsout.start_object();
    jsout.member( "piece_type", ptr->get_type() );
    ptr->serialize( jsout );
    jsout.end_object();
}

void deserialize( std::unique_ptr<editor::me_piece> &ptr, JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();

    editor::PieceType pt;
    jo.read( "piece_type", pt );

    std::unique_ptr<editor::me_piece> val = editor::make_new_piece( pt );
    val->deserialize( jo );
    ptr = std::move( val );
}

void serialize( const map_key &mk, JsonOut &jsout )
{
    jsout.start_object();
    jsout.member( "str", mk.str );
    jsout.end_object();
}

void deserialize( map_key &mk, JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();

    jo.read( "str", mk.str );
}

void serialize( const ImVec4 &v, JsonOut &jsout )
{
    jsout.start_object();
    jsout.member( "x", v.x );
    jsout.member( "y", v.y );
    jsout.member( "z", v.z );
    jsout.member( "w", v.w );
    jsout.end_object();
}

void deserialize( ImVec4 &v, JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();

    jo.read( "x", v.x );
    jo.read( "y", v.y );
    jo.read( "z", v.z );
    jo.read( "w", v.w );
}

namespace io
{

template<>
std::string enum_to_string<editor::OterMapgenBase>( editor::OterMapgenBase data )
{
    switch( data ) {
        // *INDENT-OFF*
        case editor::OterMapgenBase::FillTer: return "FillTer";
        case editor::OterMapgenBase::PredecessorMapgen: return "PredecessorMapgen";
        case editor::OterMapgenBase::Rows: return "Rows";
        // *INDENT-ON*
        case editor::OterMapgenBase::_Num:
            break;
    }
    debugmsg( "Invalid editor::OterMapgenBase" );
    abort();
}

template<>
std::string enum_to_string<editor::MapgenType>( editor::MapgenType data )
{
    switch( data ) {
        // *INDENT-OFF*
        case editor::MapgenType::Nested: return "Nested";
        case editor::MapgenType::Oter: return "Oter";
        case editor::MapgenType::Update: return "Update";
        // *INDENT-ON*
        case editor::MapgenType::_Num:
            break;
    }
    debugmsg( "Invalid editor::MapgenType" );
    abort();
}

template<>
std::string enum_to_string<editor::PieceType>( editor::PieceType data )
{
    switch( data ) {
        // *INDENT-OFF*
        case editor::PieceType::Field: return "Field";
        case editor::PieceType::NPC: return "NPC";
        case editor::PieceType::Faction: return "Faction";
        case editor::PieceType::Sign: return "Sign";
        case editor::PieceType::Graffiti: return "Graffiti";
        case editor::PieceType::VendingMachine: return "VendingMachine";
        case editor::PieceType::Toilet: return "Toilet";
        case editor::PieceType::GasPump: return "GasPump";
        case editor::PieceType::Liquid: return "Liquid";
        case editor::PieceType::Igroup: return "Igroup";
        case editor::PieceType::Loot: return "Loot";
        case editor::PieceType::Mgroup: return "Mgroup";
        case editor::PieceType::Monster: return "Monster";
        case editor::PieceType::Vehicle: return "Vehicle";
        case editor::PieceType::Item: return "Item";
        case editor::PieceType::Trap: return "Trap";
        case editor::PieceType::Furniture: return "Furniture";
        case editor::PieceType::Terrain: return "Terrain";
        case editor::PieceType::TerFurnTransform: return "TerFurnTransform";
        case editor::PieceType::MakeRubble: return "MakeRubble";
        case editor::PieceType::Computer: return "Computer";
        case editor::PieceType::SealedItem: return "SealedItem";
        case editor::PieceType::Translate: return "Translate";
        case editor::PieceType::Zone: return "Zone";
        case editor::PieceType::Nested: return "Nested";
        case editor::PieceType::AltTrap: return "AltTrap";
        case editor::PieceType::AltFurniture: return "AltFurniture";
        case editor::PieceType::AltTerrain: return "AltTerrain";
        // *INDENT-ON*
        default:
            break;
    }
    debugmsg( "Invalid editor::PieceType" );
    abort();
}

} // namespace io

namespace editor
{

void me_piece_field::serialize( JsonOut &jsout ) const
{
    jsout.member( "ftype", ftype );
    jsout.member( "intensity", intensity );
    jsout.member( "age", age );
}

void me_piece_field::deserialize( JsonObject &jsin )
{
    jsin.read( "ftype", ftype );
    jsin.read( "intensity", intensity );
    jsin.read( "age", age );
}

void me_piece_npc::serialize( JsonOut &jsout ) const
{
    jsout.member( "npc_class", npc_class );
    jsout.member( "target", target );
    jsout.member( "traits", traits );
}

void me_piece_npc::deserialize( JsonObject &jsin )
{
    jsin.read( "npc_class", npc_class );
    jsin.read( "target", target );
    jsin.read( "traits", traits );
}

void me_piece_faction::serialize( JsonOut &jsout ) const
{
    jsout.member( "id", id );
}

void me_piece_faction::deserialize( JsonObject &jsin )
{
    jsin.read( "id", id );
}

void me_piece_sign::serialize( JsonOut &jsout ) const
{
    jsout.member( "use_snippet", use_snippet );
    jsout.member( "snippet", snippet );
    jsout.member( "text", text );
}

void me_piece_sign::deserialize( JsonObject &jsin )
{
    jsin.read( "use_snippet", use_snippet );
    jsin.read( "snippet", snippet );
    jsin.read( "text", text );
}

void me_piece_graffiti::serialize( JsonOut &jsout ) const
{
    jsout.member( "use_snippet", use_snippet );
    jsout.member( "snippet", snippet );
    jsout.member( "text", text );
}

void me_piece_graffiti::deserialize( JsonObject &jsin )
{
    jsin.read( "use_snippet", use_snippet );
    jsin.read( "snippet", snippet );
    jsin.read( "text", text );
}

void me_piece_vending_machine::serialize( JsonOut &jsout ) const
{
    jsout.member( "reinforced", reinforced );
    jsout.member( "use_default_group", use_default_group );
    jsout.member( "item_group", item_group );
}

void me_piece_vending_machine::deserialize( JsonObject &jsin )
{
    jsin.read( "reinforced", reinforced );
    jsin.read( "use_default_group", use_default_group );
    jsin.read( "item_group", item_group );
}

void me_piece_toilet::serialize( JsonOut &jsout ) const
{
    jsout.member( "use_default_amount", use_default_amount );
    jsout.member( "amount", amount );
}

void me_piece_toilet::deserialize( JsonObject &jsin )
{
    jsin.read( "use_default_amount", use_default_amount );
    jsin.read( "amount", amount );
}

void me_piece_gaspump::serialize( JsonOut &jsout ) const
{
    // TODO
}

void me_piece_gaspump::deserialize( JsonObject &jsin )
{
    // TODO
}

void me_piece_liquid::serialize( JsonOut &jsout ) const
{
    // TODO
}

void me_piece_liquid::deserialize( JsonObject &jsin )
{
    // TODO
}

void me_piece_igroup::serialize( JsonOut &jsout ) const
{
    // TODO
}

void me_piece_igroup::deserialize( JsonObject &jsin )
{
    // TODO
}

void me_piece_loot::serialize( JsonOut &jsout ) const
{
    // TODO
}

void me_piece_loot::deserialize( JsonObject &jsin )
{
    // TODO
}

void me_piece_mgroup::serialize( JsonOut &jsout ) const
{
    // TODO
}

void me_piece_mgroup::deserialize( JsonObject &jsin )
{
    // TODO
}

void me_piece_monster::serialize( JsonOut &jsout ) const
{
    // TODO
}

void me_piece_monster::deserialize( JsonObject &jsin )
{
    // TODO
}

void me_piece_vehicle::serialize( JsonOut &jsout ) const
{
    // TODO
}

void me_piece_vehicle::deserialize( JsonObject &jsin )
{
    // TODO
}

void me_piece_item::serialize( JsonOut &jsout ) const
{
    // TODO
}

void me_piece_item::deserialize( JsonObject &jsin )
{
    // TODO
}

void me_piece_trap::serialize( JsonOut &jsout ) const
{
    // TODO
}

void me_piece_trap::deserialize( JsonObject &jsin )
{
    // TODO
}

void me_piece_furniture::serialize( JsonOut &jsout ) const
{
    // TODO
}

void me_piece_furniture::deserialize( JsonObject &jsin )
{
    // TODO
}

void me_piece_terrain::serialize( JsonOut &jsout ) const
{
    // TODO
}

void me_piece_terrain::deserialize( JsonObject &jsin )
{
    // TODO
}

void me_piece_ter_furn_transform::serialize( JsonOut &jsout ) const
{
    // TODO
}

void me_piece_ter_furn_transform::deserialize( JsonObject &jsin )
{
    // TODO
}

void me_piece_make_rubble::serialize( JsonOut &jsout ) const
{
    // TODO
}

void me_piece_make_rubble::deserialize( JsonObject &jsin )
{
    // TODO
}

void me_piece_computer::serialize( JsonOut &jsout ) const
{
    // TODO
}

void me_piece_computer::deserialize( JsonObject &jsin )
{
    // TODO
}

void me_piece_sealed_item::serialize( JsonOut &jsout ) const
{
    // TODO
}

void me_piece_sealed_item::deserialize( JsonObject &jsin )
{
    // TODO
}

void me_piece_translate::serialize( JsonOut &jsout ) const
{
    // TODO
}

void me_piece_translate::deserialize( JsonObject &jsin )
{
    // TODO
}

void me_piece_zone::serialize( JsonOut &jsout ) const
{
    // TODO
}

void me_piece_zone::deserialize( JsonObject &jsin )
{
    // TODO
}

void me_piece_nested::serialize( JsonOut &jsout ) const
{
    // TODO
}

void me_piece_nested::deserialize( JsonObject &jsin )
{
    // TODO
}

void me_piece_alt_trap::serialize( JsonOut &jsout ) const
{
    // TODO
}

void me_piece_alt_trap::deserialize( JsonObject &jsin )
{
    // TODO
}

void me_piece_alt_furniture::serialize( JsonOut &jsout ) const
{
    // TODO
}

void me_piece_alt_furniture::deserialize( JsonObject &jsin )
{
    // TODO
}

void me_piece_alt_terrain::serialize( JsonOut &jsout ) const
{
    // TODO
}

void me_piece_alt_terrain::deserialize( JsonObject &jsin )
{
    // TODO
}


namespace detail
{

void serialize_eid( JsonOut &jsout, const std::string &data )
{
    jsout.start_object();
    jsout.member( "data", data );
    jsout.end_object();
}

void deserialize_eid( JsonIn &jsin, std::string &data )
{
    JsonObject jo = jsin.get_object();

    jo.read( "data", data );
}

} // namespace detail

void uuid_generator::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "counter", counter );
    jsout.end_object();
}

void uuid_generator::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();

    jo.read( "counter", counter );
}

void me_int_range::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "min", min );
    jsout.member( "max", max );
    jsout.end_object();
}

void me_int_range::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();

    jo.read( "min", min );
    jo.read( "max", max );
}

void me_placing::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "pieces", pieces );
    jsout.end_object();
}

void me_placing::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();

    jo.read( "pieces", pieces );
}

void me_palette_entry::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "uuid", uuid );
    jsout.member( "key", key );
    jsout.member( "color", color );
    jsout.member( "ter", ter );
    jsout.member( "furn", furn );
    jsout.member( "placing", placing );
    jsout.end_object();
}

void me_palette_entry::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();

    jo.read( "uuid", uuid );
    jo.read( "key", key );
    jo.read( "color", color );
    jo.read( "ter", ter );
    jo.read( "furn", furn );
    jo.read( "placing", placing );
}

void me_palette::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "is_inline", is_inline );
    jsout.member( "id", id );
    jsout.member( "entries", entries );
    jsout.end_object();
}

void me_palette::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();

    jo.read( "is_inline", is_inline );
    jo.read( "id", id );
    jo.read( "entries", entries );
}

void me_mapgen_base::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "size", size );
    jsout.member( "rows", rows );
    jsout.member( "inline_palette", inline_palette );
    jsout.end_object();
}

void me_mapgen_base::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();

    jo.read( "size", size );
    jo.read( "rows", rows );
    jo.read( "inline_palette", inline_palette );
}

void me_mapgen_oter::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "om_terrain", om_terrain );
    jsout.member( "weight", weight );
    jsout.member( "mapgen_base", mapgen_base );
    jsout.member( "fill_ter", fill_ter );
    jsout.member( "predecessor_mapgen", predecessor_mapgen );
    jsout.member( "rotation", rotation );
    jsout.end_object();
}

void me_mapgen_oter::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();

    jo.read( "om_terrain", om_terrain );
    jo.read( "weight", weight );
    jo.read( "mapgen_base", mapgen_base );
    jo.read( "fill_ter", fill_ter );
    jo.read( "predecessor_mapgen", predecessor_mapgen );
    jo.read( "rotation", rotation );
}

void me_mapgen_update::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "update_mapgen_id", update_mapgen_id );
    jsout.member( "fill_ter", fill_ter );
    jsout.end_object();
}

void me_mapgen_update::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();

    jo.read( "update_mapgen_id", update_mapgen_id );
    jo.read( "fill_ter", fill_ter );
}

void me_mapgen_nested::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "nested_mapgen_id", nested_mapgen_id );
    jsout.member( "size", size );
    jsout.member( "rotation", rotation );
    jsout.end_object();
}

void me_mapgen_nested::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();

    jo.read( "nested_mapgen_id", nested_mapgen_id );
    jo.read( "size", size );
    jo.read( "rotation", rotation );
}

void me_file::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "uuid_gen", uuid_gen );
    jsout.member( "mtype", mtype );
    jsout.member( "base", base );
    jsout.member( "oter", oter );
    jsout.member( "update", update );
    jsout.member( "nested", nested );
    jsout.end_object();
}

void me_file::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();

    jo.read( "uuid_gen", uuid_gen );
    jo.read( "mtype", mtype );
    jo.read( "base", base );
    jo.read( "oter", oter );
    jo.read( "update", update );
    jo.read( "nested", nested );
}

} // namespace editor

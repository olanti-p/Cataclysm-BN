#include "editor_me_state_export.h"
#include "editor_me_piece.h"

#include "../fstream_utils.h"
#include "../json.h"
#include "../../tools/format/format.h"

#include <sstream>

namespace editor_export
{

/**
 * ============= EMIT DECLARATIONS =============
 */

void emit_key( JsonOut &jo, const std::string &key );

void emit_val( JsonOut &jo, int i );
void emit_val( JsonOut &jo, bool b );
void emit_val( JsonOut &jo, const char *str );
void emit_val( JsonOut &jo, const std::string &str );
void emit_val( JsonOut &jo, const editor::me_piece *piece );
template<typename T>
void emit_val( JsonOut &jo, const editor::editable_id<T> &eid );
void emit_val( JsonOut &jo, const editor::me_int_range &r );

template<typename T>
void emit( JsonOut &jo, const std::string &key, T value );

template<typename F>
void emit_array( JsonOut &jo, F func );
template<typename F>
void emit_array( JsonOut &jo, const std::string &key, F func );

template<typename F>
void emit_object( JsonOut &jo, F func );
template<typename F>
void emit_object( JsonOut &jo, const std::string &key, F func );

template<typename T>
void emit_single_or_array( JsonOut &jo, const std::vector<T> &vals );
template<typename T>
void emit_single_or_array( JsonOut &jo, const std::string &key, const std::vector<T> &vals );

/**
 * ============= EMIT DEFINITIONS =============
 */

void emit_key( JsonOut &jo, const std::string &key )
{
    jo.member( key );
}

void emit_val( JsonOut &jo, int i )
{
    jo.write( i );
}

void emit_val( JsonOut &jo, bool b )
{
    jo.write_bool( b );
}

void emit_val( JsonOut &jo, const char *str )
{
    jo.write( str );
}

void emit_val( JsonOut &jo, const std::string &str )
{
    jo.write( str );
}

void emit_val( JsonOut &jo, const editor::me_piece *piece )
{
    emit_object( jo, [&]() {
        piece->export_func( jo );
    } );
}

template<typename T>
void emit_val( JsonOut &jo, const editor::editable_id<T> &eid )
{
    jo.write( eid.data );
}

void emit_val( JsonOut &jo, const editor::me_int_range &r )
{
    if( r.min == r.max ) {
        emit_val( jo, r.min );
    } else {
        emit_array( jo, [&]() {
            emit_val( jo, r.min );
            emit_val( jo, r.max );
        } );
    }
}

template<typename T>
void emit( JsonOut &jo, const std::string &key, T value )
{
    emit_key( jo, key );
    emit_val( jo, value );
}

template<typename F>
void emit_array( JsonOut &jo, F func )
{
    jo.start_array();
    func();
    jo.end_array();
}

template<typename F>
void emit_array( JsonOut &jo, const std::string &key, F func )
{
    emit_key( jo, key );
    emit_array( jo, func );
}

template<typename F>
void emit_object( JsonOut &jo, F func )
{
    jo.start_object();
    func();
    jo.end_object();
}

template<typename F>
void emit_object( JsonOut &jo, const std::string &key, F func )
{
    emit_key( jo, key );
    emit_object( jo, func );
}

template<typename T>
void emit_single_or_array( JsonOut &jo, const std::vector<T> &vals )
{
    if( vals.size() == 1 ) {
        emit_val( jo, vals[0] );
    } else {
        emit_array( jo, [&]() {
            for( const T &itm : vals ) {
                emit_val( jo, itm );
            }
        } );
    }
}

template<typename T>
void emit_single_or_array( JsonOut &jo, const std::string &key, const std::vector<T> &vals )
{
    emit_key( jo, key );
    emit_single_or_array( jo, vals );
}

} // namespace editor_export

/**
 * ==================== PIECES ====================
 */

namespace editor
{

namespace ee = editor_export;

void me_piece_field::export_func( JsonOut &jo ) const
{
    ee::emit( jo, "field", ftype );
    if( intensity != 1 ) {
        ee::emit( jo, "intensity", intensity );
    }
    if( age != 0_turns ) {
        ee::emit( jo, "age", to_turns<int>( age ) );
    }
}

void me_piece_npc::export_func( JsonOut &jo ) const
{
    ee::emit( jo, "class", npc_class );
    if( target ) {
        ee::emit( jo, "target", target );
    }
    if( !traits.empty() ) {
        ee::emit_array( jo, "add_trait", [&]() {
            for( const auto &it : traits ) {
                ee::emit_val( jo, it );
            }
        } );
    }
}

void me_piece_faction::export_func( JsonOut &jo ) const
{
    ee::emit( jo, "id", id );
}

void me_piece_sign::export_func( JsonOut &jo ) const
{
    if( use_snippet ) {
        ee::emit( jo, "snippet", snippet );
    } else {
        ee::emit( jo, "signage", text );
    }
}

void me_piece_graffiti::export_func( JsonOut &jo ) const
{
    if( use_snippet ) {
        ee::emit( jo, "snippet", snippet );
    } else {
        ee::emit( jo, "text", text );
    }
}

void me_piece_vending_machine::export_func( JsonOut &jo ) const
{
    if( reinforced ) {
        ee::emit( jo, "reinforced", reinforced );
    }
    if( !use_default_group ) {
        ee::emit( jo, "item_group", item_group );
    }
}

void me_piece_toilet::export_func( JsonOut &jo ) const
{
    if( !use_default_amount ) {
        ee::emit( jo, "amount", amount );
    }
}

void me_piece_gaspump::export_func( JsonOut &jo ) const
{
    // TODO
}

void me_piece_liquid::export_func( JsonOut &jo ) const
{
    // TODO
}

void me_piece_igroup::export_func( JsonOut &jo ) const
{
    // TODO
}

void me_piece_loot::export_func( JsonOut &jo ) const
{
    // TODO
}

void me_piece_mgroup::export_func( JsonOut &jo ) const
{
    // TODO
}

void me_piece_monster::export_func( JsonOut &jo ) const
{
    // TODO
}

void me_piece_vehicle::export_func( JsonOut &jo ) const
{
    // TODO
}

void me_piece_item::export_func( JsonOut &jo ) const
{
    // TODO
}

void me_piece_trap::export_func( JsonOut &jo ) const
{
    // TODO
}

void me_piece_furniture::export_func( JsonOut &jo ) const
{
    // TODO
}

void me_piece_terrain::export_func( JsonOut &jo ) const
{
    // TODO
}

void me_piece_ter_furn_transform::export_func( JsonOut &jo ) const
{
    // TODO
}

void me_piece_make_rubble::export_func( JsonOut &jo ) const
{
    // TODO
}

void me_piece_computer::export_func( JsonOut &jo ) const
{
    // TODO
}

void me_piece_sealed_item::export_func( JsonOut &jo ) const
{
    // TODO
}

void me_piece_translate::export_func( JsonOut &jo ) const
{
    // TODO
}

void me_piece_zone::export_func( JsonOut &jo ) const
{
    // TODO
}

void me_piece_nested::export_func( JsonOut &jo ) const
{
    // TODO
}

void me_piece_alt_trap::export_func( JsonOut &jo ) const
{
    // TODO
}

void me_piece_alt_furniture::export_func( JsonOut &jo ) const
{
    // TODO
}

void me_piece_alt_terrain::export_func( JsonOut &jo ) const
{
    // TODO
}

} // namespace editor

namespace editor_export
{

std::string get_palette_category( editor::PieceType data )
{
    switch( data ) {
        // *INDENT-OFF*
        case editor::PieceType::Field: return "fields";
        case editor::PieceType::NPC: return "npcs";
        case editor::PieceType::Faction: return "faction_owner_character";
        case editor::PieceType::Sign: return "signs";
        case editor::PieceType::Graffiti: return "graffiti";
        case editor::PieceType::VendingMachine: return "vendingmachines";
        case editor::PieceType::Toilet: return "toilets";
        case editor::PieceType::GasPump: return "gaspumps";
        case editor::PieceType::Liquid: return "liquids";
        case editor::PieceType::Igroup: return "items";
        case editor::PieceType::Loot: return "";            // TODO: DOES NOT EXIST
        case editor::PieceType::Mgroup: return "monsters";
        case editor::PieceType::Monster: return "monster";
        case editor::PieceType::Vehicle: return "vehicles";
        case editor::PieceType::Item: return "item";
        case editor::PieceType::Trap: return "traps";
        case editor::PieceType::Furniture: return "";       // TODO: CONFLICT
        case editor::PieceType::Terrain: return "";         // TODO: CONFLICT
        case editor::PieceType::TerFurnTransform: return "ter_furn_transforms";
        case editor::PieceType::MakeRubble: return "rubble";
        case editor::PieceType::Computer: return "computers";
        case editor::PieceType::SealedItem: return "sealed_item";
        case editor::PieceType::Translate: return "translate";
        case editor::PieceType::Zone: return "zones";
        case editor::PieceType::Nested: return "nested";
        case editor::PieceType::AltTrap: return "";         // TODO: CONFLICT
        case editor::PieceType::AltFurniture: return "";    // TODO: CONFLICT
        case editor::PieceType::AltTerrain: return "";      // TODO: CONFLICT
        // *INDENT-ON*
        default:
            break;
    }
    debugmsg( "Invalid editor::PieceType" );
    abort();
}

std::string get_placing_category( editor::PieceType data )
{
    switch( data ) {
        // *INDENT-OFF*
        case editor::PieceType::Field: return "place_fields";
        case editor::PieceType::NPC: return "place_npcs";
        case editor::PieceType::Faction: return "faction_owner";
        case editor::PieceType::Sign: return "place_signs";
        case editor::PieceType::Graffiti: return "place_graffiti";
        case editor::PieceType::VendingMachine: return "place_vendingmachines";
        case editor::PieceType::Toilet: return "place_toilets";
        case editor::PieceType::GasPump: return "place_gaspumps";
        case editor::PieceType::Liquid: return "place_liquids";
        case editor::PieceType::Igroup: return "place_items";
        case editor::PieceType::Loot: return "place_loot";
        case editor::PieceType::Mgroup: return "place_monsters";
        case editor::PieceType::Monster: return "place_monster";
        case editor::PieceType::Vehicle: return "place_vehicles";
        case editor::PieceType::Item: return "place_item";
        case editor::PieceType::Trap: return "place_traps";
        case editor::PieceType::Furniture: return "place_furniture";
        case editor::PieceType::Terrain: return "place_terrain";
        case editor::PieceType::TerFurnTransform: return "place_ter_furn_transforms";
        case editor::PieceType::MakeRubble: return "place_rubble";
        case editor::PieceType::Computer: return "place_computers";
        case editor::PieceType::SealedItem: return "";      // TODO: DOES NOT EXIST
        case editor::PieceType::Translate: return "translate_ter";
        case editor::PieceType::Zone: return "place_zones";
        case editor::PieceType::Nested: return "place_nested";
        case editor::PieceType::AltTrap: return "";         // TODO: CONFLICT
        case editor::PieceType::AltFurniture: return "";    // TODO: CONFLICT
        case editor::PieceType::AltTerrain: return "";      // TODO: CONFLICT
        // *INDENT-ON*
        default:
            break;
    }
    debugmsg( "Invalid editor::PieceType" );
    abort();
}

/**
 * ============= HIGH-LEVEL FUNCTIONS =============
 */

void emit_file_contents( JsonOut &jo, const editor::me_file &file )
{
    emit( jo, "type", "mapgen" );
    emit( jo, "method", "json" );

    if( file.mtype == editor::MapgenType::Oter ) {
        emit( jo, "om_terrain", file.oter.om_terrain );
        emit( jo, "weight", file.oter.weight );
    } else if( file.mtype == editor::MapgenType::Nested ) {
        emit( jo, "nested_mapgen_id", file.nested.nested_mapgen_id );
    } else { // editor::MapgenType::Update
        emit( jo, "update_mapgen_id", file.update.update_mapgen_id );
    }

    emit_object( jo, "object", [&]() {

        if( file.mtype == editor::MapgenType::Oter ) {
            if( file.oter.mapgen_base == editor::OterMapgenBase::PredecessorMapgen ) {
                emit( jo, "predecessor_mapgen", file.oter.predecessor_mapgen );
            } else {
                emit( jo, "fill_ter", file.oter.fill_ter );
            }
            if( file.oter.rotation ) {
                emit( jo, "rotation", file.oter.rotation );
            }
        } else if( file.mtype == editor::MapgenType::Nested ) {
            emit_array( jo, "mapgensize", [&]() {
                emit_val( jo, file.nested.size.x );
                emit_val( jo, file.nested.size.y );
            } );
            if( file.nested.rotation ) {
                emit( jo, "rotation", file.nested.rotation );
            }
        } else { // editor::MapgenType::Update
            if( !file.update.fill_ter.is_null() ) {
                emit( jo, "fill_ter", file.update.fill_ter );
            }
        }

        if( file.uses_rows() ) {
            emit_array( jo, "rows", [&]() {
                for( int y = 0; y < file.mapgensize().y(); y++ ) {
                    std::string s;
                    for( int x = 0; x < file.mapgensize().x(); x++ ) {
                        const map_key &mk = file.base.get_key_at( point( x, y ) );
                        s += mk.str;
                    }
                    emit_val( jo, s );
                }
            } );

            emit_object( jo, "terrain", [&]() {
                for( const editor::me_palette_entry &it : file.base.inline_palette.entries ) {
                    emit( jo, it.key.str, it.ter );
                }
            } );

            emit_object( jo, "furniture", [&]() {
                for( const editor::me_palette_entry &it : file.base.inline_palette.entries ) {
                    if( !it.furn.is_null() ) {
                        emit( jo, it.key.str, it.furn );
                    }
                }
            } );

            for( const auto &it : editor::get_piece_templates() ) {
                editor::PieceType pt = it->get_type();

                std::string palette_cat = get_palette_category( pt );

                std::unordered_map<map_key, std::vector<const editor::me_piece *>> matching_pieces;

                for( const editor::me_palette_entry &it : file.base.inline_palette.entries ) {
                    for( const auto &pc : it.mapping.pieces ) {
                        if( pc->get_type() == pt ) {
                            matching_pieces[it.key].push_back( pc.get() );
                        }
                    }
                }

                if( matching_pieces.empty() ) {
                    // Nothing to do
                    continue;
                }

                if( palette_cat.empty() ) {
                    // TODO: export questionable pieces
                    debugmsg( "Not implemented: export of piece type %s", io::enum_to_string( pt ) );
                    continue;
                }

                emit_object( jo, palette_cat, [&]() {
                    for( const auto &it : matching_pieces ) {
                        emit_single_or_array( jo, it.first.str, it.second );
                    }
                } );
            }
        }
    } );
}

std::string to_string( const editor::me_file &file )
{
    return serialize_wrapper( [&]( JsonOut & jo ) {
        // Wrap it as 1-element array to adhere to modern BN data format
        emit_array( jo, [&]() {
            emit_object( jo, [&]() {
                emit_file_contents( jo, file );
            } );
        } );
    } );
}

std::string format_string( const std::string &js )
{
    std::stringstream in;
    std::stringstream out;

    in << js;

    JsonOut jsout( out, true );
    JsonIn jsin( in );

    format( jsin, jsout );

    out << std::endl;

    return out.str();
}

} // namespace editor_export

#include "all_enum_values.h"
#include "mapgen/piece_type.h"
#include "palette_making.h"

#include "mapgen/palette.h"
#include "mapgen_map_key.h"
#include "type_id.h"
#include "widget/editable_id.h"

// FIXME: conflicts in include path
#include "../../mapgen.h"

#include <memory>
#include <unordered_set>

namespace editor
{

void import_palette_data( Project &project, Palette &palette, const EID::Palette &source_id )
{
    palette_id id( source_id.data );
    const mapgen_palette &source = *id;

    std::unordered_set<map_key> all_keys;

    for( const auto &it : source.format_terrain ) {
        all_keys.insert( it.first );
    }
    for( const auto &it : source.format_furniture ) {
        all_keys.insert( it.first );
    }
    for( const auto &it : source.format_placings ) {
        all_keys.insert( it.first );
    }
    for( const map_key &key : all_keys ) {
        std::optional<EID::Ter> ter_eid;
        std::optional<EID::Furn> furn_eid;
        {
            auto it = source.format_terrain.find( key );
            if( it != source.format_terrain.end() ) {
                ter_str_id id = it->second->id;
                if( !id.is_null() ) {
                    ter_eid = id.str();
                }
            }
        }
        {
            auto it = source.format_furniture.find( key );
            if( it != source.format_furniture.end() ) {
                furn_str_id id = it->second->id;
                if( !id.is_null() ) {
                    furn_eid = id.str();
                }
            }
        }
        editor::Mapping mapping = make_mapping(
                                      ter_eid ? &*ter_eid : nullptr,
                                      furn_eid ? &*furn_eid : nullptr
                                  );
        {
            auto it = source.format_placings.find( key );
            if( it != source.format_placings.end() ) {
                for( const auto &piece_ptr : it->second ) {
                    const jmapgen_piece &piece = *piece_ptr;
                    std::unique_ptr<Piece> new_piece;
                    // TODO: this is inefficient O(n^2)
                    for( const PieceType &type : all_enum_values<PieceType>() ) {
                        new_piece = make_new_piece( type );
                        if( new_piece->try_import( piece ) ) {
                            break;
                        } else {
                            new_piece.reset();
                        }
                    }
                    if( new_piece ) {
                        mapping.pieces.emplace_back( std::move( new_piece ) );
                    } else {
                        // TODO: report import error
                    }
                }
            }
        }
        editor::PaletteEntry entry = make_simple_entry( project, palette, std::move( mapping ) );
        entry.key = key;
        palette.entries.emplace_back( std::move( entry ) );
    }
}

} // namespace editor

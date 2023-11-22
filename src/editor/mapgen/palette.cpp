#include "palette.h"

#include "mapgen/piece_impl.h"

namespace editor
{

void show_palette_entry_tooltip( const PaletteEntry &entry )
{
    std::string name = entry.display_name();
    if( !name.empty() ) {
        ImGui::Text( "%s", name.c_str() );
    }
    for( const auto &it : entry.mapping.pieces ) {
        ImGui::TextDisabled( "MAP" );
        ImGui::SameLine();
        ImGui::Text( "%s", it->fmt_summary().c_str() );
    }
}

Mapping::Mapping( const Mapping &rhs )
{
    *this = rhs;
}

Mapping &Mapping::operator=( const Mapping &rhs )
{
    pieces.reserve( rhs.pieces.size() );
    for( const auto &piece : rhs.pieces ) {
        pieces.emplace_back( piece->clone() );
    }
    return *this;
}

bool Mapping::has_piece_of_type( PieceType pt ) const
{
    for( const auto &piece : pieces ) {
        if( piece->get_type() == pt ) {
            return true;
        }
    }
    return false;
}

const map_key &Palette::key_from_uuid( const UUID &uuid ) const
{
    if( uuid == UUID_INVALID ) {
        return default_map_key;
    }
    const PaletteEntry *entry = find_entry( uuid );
    if( entry ) {
        return entry->key;
    }

    std::cerr << "Tried to find palette key, but uuid was not found " << uuid << std::endl;
    std::abort();
}

const ImVec4 &Palette::color_from_uuid( const UUID &uuid ) const
{
    if( uuid == UUID_INVALID ) {
        static ImVec4 default_color = ImVec4();
        return default_color;
    }
    const PaletteEntry *entry = find_entry( uuid );
    if( entry ) {
        return entry->color;
    }

    std::cerr << "Tried to find palette color, but uuid was not found " << uuid << std::endl;
    std::abort();
}

const SpriteRef *Palette::sprite_from_uuid( const UUID &uuid ) const
{
    if( uuid == UUID_INVALID ) {
        return nullptr;
    }
    const PaletteEntry *entry = find_entry( uuid );
    if( entry ) {
        if( !entry->sprite_cache_valid ) {
            entry->build_sprite_cache();
        }
        if( !entry->sprite_cache ) {
            return nullptr;
        } else {
            return &*entry->sprite_cache;
        }
    }

    std::cerr << "Tried to find sprite, but uuid was not found " << uuid << std::endl;
    std::abort();
}

PaletteEntry *Palette::find_entry( const UUID &uuid )
{
    if( uuid == UUID_INVALID ) {
        return nullptr;
    }
    for( auto &it : entries ) {
        if( it.uuid == uuid ) {
            return &it;
        }
    }
    return nullptr;
}

const PaletteEntry *Palette::find_entry( const UUID &uuid ) const
{
    if( uuid == UUID_INVALID ) {
        return nullptr;
    }
    for( const auto &it : entries ) {
        if( it.uuid == uuid ) {
            return &it;
        }
    }
    return nullptr;
}

std::string Palette::display_name() const
{
    if( !name.empty() ) {
        return name;
    } else if( !id.data.empty() ) {
        return id.data;
    } else {
        return string_format( "[uuid=%d]", uuid );
    }
}

void PaletteEntry::build_sprite_cache() const
{
    sprite_cache.reset();

    // Try furniture tile
    {
        const PieceAltFurniture *ptr = mapping.get_first_piece_of_type<PieceAltFurniture>();
        if( ptr ) {
            auto list = ptr->list;
            if( !list.entries.empty() ) {
                sprite_cache = SpriteRef( list.entries[0].val.data );
            }
        }
    }

    // Try terrain tile
    if( !sprite_cache ) {
        const PieceAltTerrain *ptr = mapping.get_first_piece_of_type<PieceAltTerrain>();
        if( ptr ) {
            auto list = ptr->list;
            if( !list.entries.empty() ) {
                sprite_cache = SpriteRef( list.entries[0].val.data );
            }
        }
    }

    sprite_cache_valid = true;
}

std::string PaletteEntry::display_name() const
{
    return name;
}

} // namespace editor

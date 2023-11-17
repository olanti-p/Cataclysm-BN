#include "palette.h"

#include "state/tools_state.h"
#include "common/color.h"
#include "mapgen.h"
#include "map_key_gen.h"
#include "piece_impl.h"
#include "project/project.h"
#include "state/state.h"
#include "state/ui_state.h"
#include "widget/widgets.h"

#include "translations.h"

#include <unordered_set>

namespace editor
{
map_key pick_available_key( const Palette &pal )
{
    MapKeyGenerator gen;
    for( const auto &it : pal.entries ) {
        gen.blacklist( it.key );
    }
    return gen();
}

static bool is_expanded( const State &state, const UUID &piece_id )
{
    return state.ui->expanded_mapping_pieces.count( piece_id ) != 0;
}

static void expand_piece( State &state, const UUID &piece_id )
{
    state.ui->expanded_mapping_pieces.insert( piece_id );
}

static void collapse_piece( State &state, const UUID &piece_id )
{
    state.ui->expanded_mapping_pieces.erase( piece_id );
}

void show_mapping( State &state, editor::Palette &p, editor::PaletteEntry &entry,
                   bool &show )
{
    std::string wnd_id = string_format( "Mappings##wnd-mappings-%d-%d", p.uuid, entry.uuid );
    ImGui::SetNextWindowSize( ImVec2( 450.0f, 300.0f ), ImGuiCond_FirstUseEver );
    ImGui::SetNextWindowPos( ImVec2( 50.0f, 50.0f ), ImGuiCond_FirstUseEver );
    if( !ImGui::Begin( wnd_id.c_str(), &show ) ) {
        ImGui::End();
        return;
    }
    ImGui::PushID( entry.uuid );

    auto &list = entry.mapping.pieces;

    bool changed = ImGui::VectorWidget()
    .with_add( [&]()->bool {
        std::vector<std::pair<std::string, PieceType>> piece_opts;
        for( const auto &it : editor::get_piece_templates() )
        {
            PieceType pt = it->get_type();
            if( !is_available_as_mapping( pt ) ) {
                continue;
            }
            if( is_piece_exclusive( pt ) && entry.mapping.has_piece_of_type( pt ) ) {
                continue;
            }
            piece_opts.emplace_back( io::enum_to_string<PieceType>( pt ), pt );
        }

        std::sort( piece_opts.begin(), piece_opts.end(), []( const auto & a, const auto & b ) -> bool {
            return localized_compare( a, b );
        } );

        std::string new_piece_str;
        new_piece_str += "Add mapping...";
        new_piece_str += '\0';
        for( const auto &it : piece_opts )
        {
            new_piece_str += it.first;
            new_piece_str += '\0';
        }
        bool ret = false;
        int new_piece_type = 0;
        if( ImGui::Combo( "##pick-new-mapping", &new_piece_type, new_piece_str.c_str() ) )
        {
            if( new_piece_type != 0 ) {
                auto ptr = editor::make_new_piece( piece_opts[new_piece_type - 1].second );
                UUID uuid = state.project().uuid_generator();
                ptr->uuid = uuid;
                ptr->init_new();
                list.push_back( std::move( ptr ) );
                expand_piece( state, uuid );
                ret = true;
            }
        }
        return ret;
    } )
    .with_for_each( [&]( size_t idx ) {
        const UUID &piece_id = list[idx]->uuid;
        if( is_expanded( state, piece_id ) ) {
            if( ImGui::ArrowButton( "##collapse", ImGuiDir_Down ) ) {
                collapse_piece( state, piece_id );
            }
            ImGui::HelpPopup( "Hide details." );
            ImGui::SameLine();
            ImGui::Text( "%d %s", static_cast<int>( idx ), list[idx]->fmt_summary().c_str() );
            list[idx]->show_ui( state );
            ImGui::Separator();
        } else {
            if( ImGui::ArrowButton( "##expand", ImGuiDir_Right ) ) {
                expand_piece( state, piece_id );
            }
            ImGui::HelpPopup( "Show details." );
            ImGui::SameLine();
            ImGui::Text( "%d %s", static_cast<int>( idx ), list[idx]->fmt_summary().c_str() );
        }
    } )
    .with_can_duplicate( [&]( size_t idx ) -> bool {
        return !editor::is_piece_exclusive( list[idx]->get_type() );
    } )
    .with_duplicate( [&]( size_t idx ) {
        list.insert( std::next( list.cbegin(), idx + 1 ), list[idx]->clone() );
    } )
    .run( list );

    if( changed ) {
        state.mark_changed();
    }

    if( state.is_changed() ) {
        entry.sprite_cache_valid = false;
    }

    ImGui::PopID();
    ImGui::End();
}

static void show_palette_entries( State &state, Palette &palette )
{
    std::vector<PaletteEntry> &list = palette.entries;
    std::unordered_set<map_key> checked;
    std::unordered_set<map_key> dupe_symbols;

    for( const editor::PaletteEntry &entry : list ) {
        if( checked.count( entry.key ) > 0 ) {
            dupe_symbols.insert( entry.key );
        } else {
            checked.insert( entry.key );
        }
    }

    Project &proj = state.project();
    ToolsState &tools = *state.ui->tools;

    bool changed = ImGui::VectorWidget()
    .with_add( [&]() -> bool {
        bool ret = false;
        if( ImGui::ImageButton( "add", "me_add" ) )
        {
            list.emplace_back( PaletteEntry{
                proj.uuid_generator(),
                pick_available_key( palette ),
                col_default_piece_color,
                Mapping(),
                false,
                std::nullopt
            } );
            ret = true;
        }
        ImGui::HelpPopup( "Add new entry." );
        return ret;
    } )
    .with_duplicate( [&]( size_t idx ) {
        const PaletteEntry &src = list[ idx ];
        list.insert( std::next( list.cbegin(), idx + 1 ), PaletteEntry{
            proj.uuid_generator(),
            pick_available_key( palette ),
            src.color,
            src.mapping,
            false,
            std::nullopt
        } );
    } )
    .with_delete( [&]( size_t idx ) {
        const UUID &uuid = list[ idx ].uuid;
        for( Mapgen &file : proj.mapgens ) {
            file.base.remove_usages( uuid );
        }
        if( tools.get_brush() == uuid ) {
            tools.set_brush( UUID_INVALID );
        }
        list.erase( std::next( list.cbegin(), idx ) );
    } )
    .with_for_each( [&]( size_t idx ) {
        if( list[idx].uuid == tools.get_brush() ) {
            if( ImGui::ImageButton( "unpick", "me_clear_rows_brush" ) ) {
                tools.set_brush( UUID_INVALID );
            }
            ImGui::HelpPopup( "Unselect (turns brush into eraser)." );
        } else {
            if( ImGui::ImageButton( "pick", "me_set_rows_brush" ) ) {
                tools.set_brush( list[idx].uuid );
            }
            ImGui::HelpPopup( "Select as active for brush." );
        }
        ImGui::SameLine();

        if( ImGui::ColorEdit4( "MyColor##3", ( float * )&list[idx].color,
                               ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel ) ) {
            state.mark_changed( "palette-entry-color" );
        }
        ImGui::SameLine();

        ImGui::SetNextItemWidth( ImGui::GetFrameHeight() );
        bool is_dupe_symbol = dupe_symbols.count( list[idx].key ) > 0;
        if( is_dupe_symbol ) {
            ImGui::BeginErrorArea();
        }
        if( ImGui::InputSymbol( "##key", list[idx].key.str, default_map_key.str.c_str() ) ) {
            state.mark_changed( "palette-entry-key" );
        }
        ImGui::HelpPopup( "Symbol to use on canvas." );
        if( is_dupe_symbol ) {
            ImGui::EndErrorArea();
        }
        {
            std::optional<std::string> text;
            PieceAltTerrain *ptr = list[idx].mapping.get_first_piece_of_type<PieceAltTerrain>();
            if( ptr ) {
                text = ptr->fmt_data_summary();
            }
            ImGui::SameLine();
            ImGui::BeginDisabled();
            ImGui::Button(
                text ? text->c_str() : "-",
                ImVec2( ImGui::GetFrameHeight() * 8.0f, 0.0f )
            );
            ImGui::EndDisabled();
        }
        {
            std::optional<std::string> text;
            PieceAltFurniture *ptr = list[idx].mapping.get_first_piece_of_type<PieceAltFurniture>();
            if( ptr ) {
                text = ptr->fmt_data_summary();
            }
            ImGui::SameLine();
            ImGui::BeginDisabled();
            ImGui::Button(
                text ? text->c_str() : "-",
                ImVec2( ImGui::GetFrameHeight() * 8.0f, 0.0f )
            );
            ImGui::EndDisabled();
        }
        ImGui::SameLine();
        if( ImGui::ArrowButton( "##mapping", ImGuiDir_Right ) ) {
            state.ui->toggle_show_mapping( palette.uuid, list[idx].uuid );
        }
        ImGui::HelpPopup( "Show/hide mappings\nassociated with this symbol." );

        int additional_pieces = 0;
        std::string additional_summary;
        for( const auto &it : list[idx].mapping.pieces ) {
            if( it->get_type() == PieceType::AltTerrain || it->get_type() == PieceType::AltFurniture ) {
                continue;
            }
            additional_pieces += 1;
            additional_summary += it->fmt_summary();
            additional_summary += "\n";
        }
        if( additional_pieces > 0 ) {
            ImGui::SameLine();
            ImGui::Text( "+ %d", additional_pieces );
            ImGui::HelpPopup( additional_summary.c_str() );
        }
    } )
    .run( list );

    if( changed ) {
        state.mark_changed();
    }
}

void show_palette( State &state, Palette &p, bool &show )
{
    ImGui::SetNextWindowSize( ImVec2( 670.0f, 120.0f ), ImGuiCond_FirstUseEver );
    ImGui::SetNextWindowPos( ImVec2( 50.0f, 50.0f ), ImGuiCond_FirstUseEver );

    std::string wnd_id = string_format( "Palette##palette-%d", p.uuid );
    if( !ImGui::Begin( wnd_id.c_str(), &show ) ) {
        ImGui::End();
        return;
    }
    ImGui::PushID( p.uuid );

    if( p.is_inline ) {
        ImGui::Text( "<inline palette>" );
    } else {
        ImGui::Text( "id: %s", p.id.data.c_str() );
    }

    show_palette_entries( state, p );

    ImGui::PopID();
    ImGui::End();
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

} // namespace editor

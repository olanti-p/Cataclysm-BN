#include "editor_me_palette.h"

#include "editor_me_canvas_tool.h"
#include "editor_me_color.h"
#include "editor_me_file.h"
#include "editor_me_state.h"
#include "editor_me_uistate.h"
#include "editor_widgets.h"

#include <unordered_set>

namespace editor
{
void show_palette_entry_extended( me_state &state, editor::me_palette &p,
                                  editor::me_palette_entry &entry )
{
    bool show = true;
    ImGui::Begin( "Extended Info", &show );

    if( ImGui::InputId( "ter", entry.ter ) ) {
        state.mark_changed();
        entry.sprite_cache_valid = false;
    }

    if( ImGui::InputId( "furn", entry.furn ) ) {
        state.mark_changed();
        entry.sprite_cache_valid = false;
    }

    ImGui::Text( "Pieces:" );

    auto &list = entry.mapping.pieces;

    static std::string new_piece_str;
    static std::vector<PieceType> piece_opts;
    if( piece_opts.empty() ) {
        new_piece_str += "Add piece...";
        new_piece_str += '\0';
        for( const auto &it : editor::get_piece_templates() ) {
            piece_opts.push_back( it->get_type() );
            new_piece_str += io::enum_to_string<PieceType>( it->get_type() );
            new_piece_str += '\0';
        }
    }

    bool changed = ImGui::VectorWidget()
    .with_add( [&]()->bool {
        bool ret = false;
        ImGui::Separator();
        int new_piece_type = 0;
        if( ImGui::Combo( "##pick-new-piece", &new_piece_type, new_piece_str.c_str() ) )
        {
            if( new_piece_type != 0 ) {
                list.push_back( editor::make_new_piece( piece_opts[new_piece_type - 1] ) );
                ret = true;
            }
        }
        return ret;
    } )
    .with_for_each( [&]( size_t idx ) {
        ImGui::Separator();
        ImGui::Text( "Piece %d: %s", static_cast<int>( idx ),
                     io::enum_to_string<PieceType>( list[idx]->get_type() ).c_str() );

        list[idx]->show_ui( state );
    } )
    .with_duplicate( [&]( size_t idx ) {
        list.insert( std::next( list.cbegin(), idx + 1 ), list[idx]->clone() );
    } )
    .run( list );

    if( changed ) {
        state.mark_changed();
    }

    ImGui::End();
    if( !show ) {
        state.uistate->view_mappings.reset();
    }
}

static void show_palette_entries( me_state &state, std::vector<me_palette_entry> &list )
{
    std::unordered_set<map_key> checked;
    std::unordered_set<map_key> dupe_symbols;

    for( const editor::me_palette_entry &entry : list ) {
        if( checked.count( entry.key ) > 0 ) {
            dupe_symbols.insert( entry.key );
        } else {
            checked.insert( entry.key );
        }
    }

    bool changed = ImGui::VectorWidget()
    .with_add( [&]() -> bool {
        bool ret = false;
        if( ImGui::ImageButton( "add", "me_add" ) )
        {
            list.emplace_back( me_palette_entry{
                state.file().uuid_gen(),
                state.file().base.pick_available_key(),
                col_default_piece_color,
                false,
                cata::nullopt,
                ter_eid::NULL_ID(),
                furn_eid::NULL_ID(),
                me_mapping()
            } );
            ret = true;
        }
        ImGui::HelpPopup( "Add new entry." );
        return ret;
    } )
    .with_duplicate( [&]( size_t idx ) {
        const me_palette_entry &src = list[ idx ];
        list.insert( std::next( list.cbegin(), idx + 1 ), me_palette_entry{
            state.file().uuid_gen(),
            state.file().base.pick_available_key(),
            src.color,
            false,
            cata::nullopt,
            src.ter,
            src.furn,
            src.mapping
        } );
    } )
    .with_delete( [&]( size_t idx ) {
        const uuid_t &uuid = list[ idx ].uuid;
        state.file().base.remove_usages( uuid );
        if( state.tools_state->brush == uuid ) {
            state.tools_state->brush = UUID_INVALID;
        }
        list.erase( std::next( list.cbegin(), idx ) );
    } )
    .with_for_each( [&]( size_t idx ) {
        uuid_t &brush = state.tools_state->brush;
        if( list[idx].uuid == brush ) {
            if( ImGui::ImageButton( "unpick", "me_clear_rows_brush" ) ) {
                brush = UUID_INVALID;
            }
            ImGui::HelpPopup( "Unselect (turns brush into eraser)." );
        } else {
            if( ImGui::ImageButton( "pick", "me_set_rows_brush" ) ) {
                brush = list[idx].uuid;
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
        ImGui::SameLine();
        ImGui::SetNextItemWidth( ImGui::GetFrameHeight() * 15.0f );
        if( ImGui::InputId( "##furn", list[idx].furn ) ) {
            state.mark_changed();
            list[idx].sprite_cache_valid = false;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth( ImGui::GetFrameHeight() * 15.0f );
        if( ImGui::InputId( "##ter", list[idx].ter ) ) {
            state.mark_changed();
            list[idx].sprite_cache_valid = false;
        }
        ImGui::SameLine();
        if( ImGui::ArrowButton( "##mapping", ImGuiDir_Right ) ) {
            state.uistate->view_mappings = list[idx].uuid;
        }
    } )
    .run( list );

    if( changed ) {
        state.mark_changed();
    }
}

void show_palette( me_state &state, me_palette &p, bool &show )
{
    ImGui::PushID( &p );

    if( !ImGui::Begin( "Palette", &show ) ) {
        ImGui::End();
        ImGui::PopID();
        return;
    }

    if( p.is_inline ) {
        ImGui::Text( "<inline palette>" );
    } else {
        ImGui::Text( "id: %s", p.id.data.c_str() );
    }

    show_palette_entries( state, p.entries );

    ImGui::End();
    ImGui::PopID();
}

me_mapping::me_mapping( const me_mapping &rhs )
{
    *this = rhs;
}

me_mapping &me_mapping::operator=( const me_mapping &rhs )
{
    pieces.reserve( rhs.pieces.size() );
    for( const auto &piece : rhs.pieces ) {
        pieces.emplace_back( piece->clone() );
    }
    return *this;
}

const map_key &me_palette::key_from_uuid( const uuid_t &uuid ) const
{
    if( uuid == UUID_INVALID ) {
        return default_map_key;
    }
    const me_palette_entry *entry = find_entry( uuid );
    if( entry ) {
        return entry->key;
    }

    std::cerr << "Tried to find palette key, but uuid was not found " << uuid << std::endl;
    std::abort();
}

const ImVec4 &me_palette::color_from_uuid( const uuid_t &uuid ) const
{
    if( uuid == UUID_INVALID ) {
        static ImVec4 default_color = ImVec4();
        return default_color;
    }
    const me_palette_entry *entry = find_entry( uuid );
    if( entry ) {
        return entry->color;
    }

    std::cerr << "Tried to find palette color, but uuid was not found " << uuid << std::endl;
    std::abort();
}

const SpriteRef *me_palette::sprite_from_uuid( const uuid_t &uuid ) const
{
    if( uuid == UUID_INVALID ) {
        return nullptr;
    }
    const me_palette_entry *entry = find_entry( uuid );
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

me_palette_entry *me_palette::find_entry( const uuid_t &uuid )
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

const me_palette_entry *me_palette::find_entry( const uuid_t &uuid ) const
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

void me_palette_entry::build_sprite_cache() const
{
    if( !furn.is_null() && furn.is_valid() ) {
        sprite_cache = SpriteRef( furn.data );
    } else if( !ter.is_null() && ter.is_valid() ) {
        sprite_cache = SpriteRef( ter.data );
    } else {
        sprite_cache.reset();
    }
    sprite_cache_valid = true;
}

} // namespace editor

#include "editor_me_state.h"
#include "editor_widgets.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include "../game_constants.h"
#include "../string_utils.h"
#include "../omdata.h"
#include "../text_snippets.h"

namespace editor
{
point_abs_epos me_camera::screen_to_world( const point_abs_screen &p ) const
{
    point disp_size = ImGui::GetIO().DisplaySize;
    return point_abs_epos( ( p.raw() - disp_size / 2 ) * ETILE_SIZE / scale + pos.raw() +
                           drag_delta.raw() );
}

point_abs_screen me_camera::world_to_screen( const point_abs_epos &p ) const
{
    point disp_size = ImGui::GetIO().DisplaySize;
    return point_abs_screen( ( p.raw() - pos.raw() - drag_delta.raw() ) * scale / ETILE_SIZE +
                             disp_size / 2 );
}

point_rel_epos me_camera::screen_to_world( const point_rel_screen &p ) const
{
    return point_rel_epos( p.raw() * ETILE_SIZE / scale );
}

point_rel_screen me_camera::world_to_screen( const point_rel_epos &p ) const
{
    return point_rel_screen( p.raw() * scale / ETILE_SIZE );
}

point_abs_screen get_mouse_pos()
{
    ImVec2 mouse_pos = ImGui::GetMousePos();
    return point_abs_screen( mouse_pos.x, mouse_pos.y );
}

point_abs_etile get_mouse_tile_pos( const me_camera &cam )
{
    point_abs_screen screen_pos = get_mouse_pos();
    point_abs_epos epos = cam.screen_to_world( screen_pos );

    point_abs_etile ret;
    point_etile_epos rem;
    std::tie( ret, rem ) = project_remain<coords::etile>( epos );

    return ret;
}

void draw_frame(
    ImDrawList *draw_list,
    const me_camera &cam,
    const point_abs_etile &p1,
    const point_abs_etile &p2,
    ImVec4 col,
    bool filled
)
{
    ImVec2 p_min = cam.world_to_screen( project_combine( p1, point_etile_epos() ) ).raw();
    ImVec2 p_max = cam.world_to_screen( project_combine( p2, point_etile_epos( ETILE_SIZE - 1,
                                        ETILE_SIZE - 1 ) ) ).raw();
    if( filled ) {
        draw_list->AddRectFilled( p_min, p_max, ImColor( col ), 0.0f, ImDrawFlags_None );
    } else {
        draw_list->AddRect( p_min, p_max, ImColor( col ), 0.0f, ImDrawFlags_None, 1.0f );
    }
}

void highlight_tile(
    ImDrawList *draw_list,
    const me_camera &cam,
    point_abs_etile tile,
    ImVec4 col
)
{
    draw_frame( draw_list, cam, tile, tile, col, false );
}

void fill_tile(
    ImDrawList *draw_list,
    const me_camera &cam,
    point_abs_etile tile,
    ImVec4 col
)
{
    draw_frame( draw_list, cam, tile, tile, col, true );
}

void highlight_region(
    ImDrawList *draw_list,
    const me_camera &cam,
    point_abs_etile p1,
    point_abs_etile p2,
    ImVec4 col_bg,
    ImVec4 col_border
)
{
    draw_frame( draw_list, cam, p1, p2, col_bg, true );
    draw_frame( draw_list, cam, p1, p2, col_border, false );
}

void fill_region(
    ImDrawList *draw_list,
    const me_camera &cam,
    point_abs_etile p1,
    point_abs_etile p2,
    ImVec4 col
)
{
    draw_frame( draw_list, cam, p1, p2, col, true );
}

void show_canvas( me_state &state )
{
    ImVec2 disp_size = ImGui::GetIO().DisplaySize;

    ImGui::SetNextWindowPos( ImVec2( 0, 0 ) );
    ImGui::SetNextWindowSize( disp_size );
    ImGui::Begin( "<canvas>", nullptr,
                  ImGuiWindowFlags_NoNav |
                  ImGuiWindowFlags_NoDecoration |
                  ImGuiWindowFlags_NoFocusOnAppearing |
                  ImGuiWindowFlags_NoBackground |
                  ImGuiWindowFlags_NoBringToFrontOnFocus |
                  ImGuiWindowFlags_NoScrollWithMouse
                );

    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    const ImVec4 col_cursor = ImVec4( 0.8f, 0.8f, 0.4f, 1.0f );
    const ImVec4 col_mapgensize_bg = ImVec4( 0.7f, 0.7f, 0.7f, 0.1f );
    const ImVec4 col_mapgensize_border = ImVec4( 0.7f, 0.7f, 0.7f, 1.0f );

    highlight_region(
        draw_list,
        state.camera,
        point_abs_etile( 0, 0 ),
        point_abs_etile( -1, -1 ) + state.file().mapgensize(),
        col_mapgensize_bg,
        col_mapgensize_border
    );

    ImGuiIO &io = ImGui::GetIO();
    bool canvas_hovered = ImGui::IsWindowHovered();
    bool brush_stroke_active = false;
    if( canvas_hovered ) {
        point_abs_etile tile_pos = get_mouse_tile_pos( state.camera );
        highlight_tile( draw_list, state.camera, tile_pos, col_cursor );

        if( ImGui::IsMouseDragging( ImGuiMouseButton_Right ) ) {
            point_rel_screen drag_delta( ImGui::GetMouseDragDelta( ImGuiMouseButton_Right ) );
            state.camera.drag_delta = -state.camera.screen_to_world( drag_delta );
        } else {
            state.camera.pos += state.camera.drag_delta;
            state.camera.drag_delta = point_rel_epos();
        }
        if( std::abs( io.MouseWheel ) > 0.5f ) {
            int zoom_speed;
            if( state.camera.scale >= 64 ) {
                zoom_speed = 16;
            } else if( state.camera.scale >= 32 ) {
                zoom_speed = 8;
            } else if( state.camera.scale >= 16 ) {
                zoom_speed = 4;
            } else {
                zoom_speed = 2;
            }
            int delta_wheel = static_cast<int>( std::round( io.MouseWheel ) );
            int delta = delta_wheel * zoom_speed;
            state.camera.scale = clamp( state.camera.scale + delta, MIN_SCALE, MAX_SCALE );
        }
        if( ImGui::IsMouseDown( ImGuiMouseButton_Left ) ) {
            brush_stroke_active = true;
            state.ongoing_brush_stroke = true;
            point_rel_etile mapgensize = state.file().mapgensize();
            if( tile_pos.x() >= 0 && tile_pos.y() >= 0 && tile_pos.x() < mapgensize.x() &&
                tile_pos.y() < mapgensize.y() ) {
                const uuid_t &uuid = state.file().base.get_uuid_at( tile_pos.raw() );
                if( state.rows_brush != UUID_INVALID && uuid != state.rows_brush ) {
                    state.file().base.set_uuid_at( tile_pos.raw(), state.rows_brush );
                    state.brush_stroke_changed_data = true;
                } else if( state.rows_brush == UUID_INVALID && uuid != UUID_INVALID ) {
                    state.file().base.set_uuid_at( tile_pos.raw(), state.rows_brush );
                    state.brush_stroke_changed_data = true;
                }
            }
        }
        if( ImGui::IsMouseClicked( ImGuiMouseButton_Middle ) ) {
            point_rel_etile mapgensize = state.file().mapgensize();
            if( tile_pos.x() >= 0 && tile_pos.y() >= 0 && tile_pos.x() < mapgensize.x() &&
                tile_pos.y() < mapgensize.y() ) {
                const uuid_t &uuid = state.file().base.get_uuid_at( tile_pos.raw() );
                state.rows_brush = uuid;
            } else {
                state.rows_brush = UUID_INVALID;
            }
        }
    }
    if( state.ongoing_brush_stroke && !brush_stroke_active ) {
        // Brush stroke ended, queue changes as a single operation
        if( state.brush_stroke_changed_data ) {
            state.mark_changed();
        }
        state.ongoing_brush_stroke = false;
        state.brush_stroke_changed_data = false;
    }

    for( int x = 0; x < state.file().mapgensize().x(); x++ ) {
        for( int y = 0; y < state.file().mapgensize().y(); y++ ) {
            point_abs_etile p( x, y );
            fill_tile( draw_list, state.camera, p, state.file().base.get_color_at( p.raw() ) ) ;
        }
    }

    for( int x = 0; x < state.file().mapgensize().x(); x++ ) {
        for( int y = 0; y < state.file().mapgensize().y(); y++ ) {
            point_abs_etile p( x, y );
            const map_key &mk = state.file().base.get_key_at( p.raw() );
            point_abs_epos center = coords::project_combine( p,
                                    point_etile_epos( ETILE_SIZE / 2, ETILE_SIZE / 2 ) );
            point_abs_screen text_center = state.camera.world_to_screen( center );
            point_rel_screen text_size( ImGui::CalcTextSize( mk.str.c_str() ) );
            point_abs_screen text_pos = text_center - text_size.raw() / 2;
            ImGui::SetCursorPos( text_pos.raw() );
            ImGui::Text( "%s", mk.str.c_str() );
        }
    }

    ImGui::End();
}

void show_control_window( me_state &state )
{
    ImGui::Begin( "Advanced Map Editor", &state.do_loop );
    ImGui::Text( "Close this window to exit the editor." );

    // Debugging
    if( ImGui::Button( "Toggle Demo Window" ) ) {
        state.show_demo_wnd = !state.show_demo_wnd;
    }

    // Controls
    if( ImGui::Button( "Toggle Asset Library" ) ) {
        state.show_asset_lib = !state.show_asset_lib;
    }
    if( ImGui::Button( "Toggle File Info" ) ) {
        state.show_file_info = !state.show_file_info;
    }
    ImGui::SameLine();
    if( ImGui::Button( "Toggle History" ) ) {
        state.show_file_history = !state.show_file_history;
    }

    // Camera
    {
        ImGui::DragInt( "Zoom", &state.camera.scale, 0.2f, MIN_SCALE, MAX_SCALE );
        ImGui::DragPoint( "Pos", &state.camera.pos, 1.0f, -10000, 10000 );
    }

    // Mouse position
    {
        point_abs_screen screen_pos = get_mouse_pos();
        point_abs_etile etile_pos = get_mouse_tile_pos( state.camera );
        ImGui::Text( "Mouse pos, px: %s", screen_pos.to_string().c_str() );
        ImGui::Text( "Mouse pos, tile: %s", etile_pos.to_string().c_str() );
    }

    ImGui::End();
}

void show_file_history( me_state &state, bool &show )
{
    if( !ImGui::Begin( "File history", &show ) ) {
        ImGui::End();
        return;
    }

    ImGui::SetNextItemWidth( ImGui::GetFrameHeight() * 4.0f );
    if( ImGui::InputInt( "History limit", &state.history_capacity, -1, -1,
                         ImGuiInputTextFlags_AutoSelectAll ) ) {
        state.history_capacity = clamp( state.history_capacity, 10, 10000 );
    }

    for( const me_file_revision &entry : state.file_history ) {
        std::string fname = string_format( "Version %d", entry.num );
        if( ImGui::Selectable( fname.c_str(), entry.num == state.current_revision.num ) ) {
            state.switch_to_revision = entry.num;
        }
    }

    ImGui::End();
}

static bool filter_matches( const std::string &s, const std::string &filter )
{
    return lcmatch( s, filter );
}

static void show_assetlib_tab( asset_library_cat &cat, asset_library &assets )
{
    const char *cat_name = get_asset_type_name( cat.atype );
    if( !ImGui::BeginTabItem( cat_name ) ) {
        cat.is_active_tab = false;
        return;
    }
    cat.is_active_tab = true;

    ImGui::Text( "%s - %d entries", cat_name, cat.get_num() );

    ImGui::InputText( "##filter", &cat.filter );
    std::string lb_name = string_format( "##lb-%s", cat_name );
    if( ImGui::BeginListBox( lb_name.c_str(), ImVec2( -1.0f, -1.0f ) ) ) {
        for( int i = 0; i < cat.get_num(); i++ ) {
            const asset_lib_entry &entry = cat.get( i );
            const char *id = entry.get_id();
            if( !filter_matches( id, cat.filter ) ) {
                continue;
            }
            const bool is_selected = i == cat.selected;
            if( ImGui::Selectable( id, is_selected ) ) {
                cat.selected = i;
            }
            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if( is_selected ) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndListBox();
    }

    ImGui::EndTabItem();
}

void show_asset_lib( asset_library &assets, bool &show )
{
    if( ImGui::Begin( "Asset Library", &show ) ) {
        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_FittingPolicyResizeDown;
        if( ImGui::BeginTabBar( "Asset Types", tab_bar_flags ) ) {
            for( asset_library_cat &cat : assets.categories ) {
                show_assetlib_tab( cat, assets );
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void show_file_info( me_state &state, me_file &file, bool &show )
{
    if( !ImGui::Begin( "File Info", &show ) ) {
        ImGui::End();
        return;
    }

    ImGui::Text( "Mapgen type:" );
    if( ImGui::RadioButton( "Oter", file.mtype == MapgenType::Oter ) ) {
        file.mtype = MapgenType::Oter;
        state.mark_changed();
    }
    ImGui::SameLine();
    if( ImGui::RadioButton( "Update", file.mtype == MapgenType::Update ) ) {
        file.mtype = MapgenType::Update;
        state.mark_changed();
    }
    ImGui::SameLine();
    if( ImGui::RadioButton( "Nested", file.mtype == MapgenType::Nested ) ) {
        file.mtype = MapgenType::Nested;
        state.mark_changed();
    }
    ImGui::Separator();

    if( file.mtype == MapgenType::Oter ) {
        if( ImGui::InputIntRange( "rotation", file.oter.rotation ) ) {
            state.mark_changed();
        }
        ImGui::Text( "Oter mapgen base:" );

        if( ImGui::RadioButton( "Fill terrain", file.oter.mapgen_base == OterMapgenBase::FillTer ) ) {
            file.oter.mapgen_base = OterMapgenBase::FillTer;
            state.mark_changed();
        }
        ImGui::SameLine();
        if( ImGui::RadioButton( "Predecessor mapgen",
                                file.oter.mapgen_base == OterMapgenBase::PredecessorMapgen ) ) {
            file.oter.mapgen_base = OterMapgenBase::PredecessorMapgen;
            state.mark_changed();
        }
        ImGui::SameLine();
        if( ImGui::RadioButton( "Rows", file.oter.mapgen_base == OterMapgenBase::Rows ) ) {
            file.oter.mapgen_base = OterMapgenBase::Rows;
            state.mark_changed();
        }

        if( file.oter.mapgen_base == OterMapgenBase::FillTer ) {
            if( ImGui::InputId( "fill_ter", file.oter.fill_ter ) ) {
                state.mark_changed();
            }
        }
        if( file.oter.mapgen_base == OterMapgenBase::PredecessorMapgen ) {
            if( ImGui::InputId( "predecessor_mapgen", file.oter.predecessor_mapgen ) ) {
                state.mark_changed();
            }
        }
        if( file.oter.mapgen_base == OterMapgenBase::Rows ) {
            ImGui::Text( "TODO: rows" );
        }
    } else if( file.mtype == MapgenType::Update ) {
        if( ImGui::InputId( "fill_ter", file.update.fill_ter ) ) {
            state.mark_changed();
        }
    } else { // MapgenType::Nested
        if( ImGui::InputIntRange( "rotation", file.nested.rotation ) ) {
            state.mark_changed();
        }
        // Only square nested mapgens are possible
        if( ImGui::InputInt( "size", &file.nested.size.x, -1, -1 ) ) {
            int size = clamp( file.nested.size.x, 1, SEEX * 2 );
            if( file.nested.size.x != size ) {
                file.nested.size.x = size;
                file.nested.size.y = size;
                file.base.set_size( file.mapgensize().raw() );
                state.mark_changed();
            }
        }
    }

    show_palette( state, file.base.inline_palette, state.show_base_inline_palette );

    ImGui::End();
}

static void show_palette_entries( me_state &state, std::vector<me_palette_entry> &list )
{
    cata::optional<size_t> del;
    cata::optional<size_t> move_up;
    cata::optional<size_t> move_dn;
    for( size_t i = 0; i < list.size(); i++ ) {
        ImGui::PushID( i );
        if( ImGui::ImageButton( "del", "me_delete" ) ) {
            del = i;
        }
        ImGui::SameLine();

        if( i == 0 ) {
            ImGui::BeginDisabled();
        }
        if( ImGui::ArrowButton( "up", ImGuiDir_Up ) ) {
            move_up = i;
        }
        if( i == 0 ) {
            ImGui::EndDisabled();
        }
        ImGui::SameLine();

        if( i == list.size() - 1 ) {
            ImGui::BeginDisabled();
        }
        if( ImGui::ArrowButton( "down", ImGuiDir_Down ) ) {
            move_dn = i;
        }
        if( i == list.size() - 1 ) {
            ImGui::EndDisabled();
        }
        ImGui::SameLine();

        if( list[i].uuid == state.rows_brush ) {
            if( ImGui::ImageButton( "unpick", "me_clear_rows_brush" ) ) {
                state.rows_brush = UUID_INVALID;
            }
        } else {
            if( ImGui::ImageButton( "pick", "me_set_rows_brush" ) ) {
                state.rows_brush = list[i].uuid;
            }
        }
        ImGui::SameLine();

        // TODO: undo/redo support for color selector
        ImGui::ColorEdit4( "MyColor##3", ( float * )&list[i].color,
                           ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel );
        ImGui::SameLine();

        ImGui::SetNextItemWidth( ImGui::GetFrameHeight() );
        if( ImGui::InputSymbol( "##key", list[i].key.str, default_map_key.str.c_str() ) ) {
            state.mark_changed();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth( ImGui::GetFrameHeight() * 15.0f );
        if( ImGui::InputId( "##furn", list[i].furn ) ) {
            state.mark_changed();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth( ImGui::GetFrameHeight() * 15.0f );
        if( ImGui::InputId( "##ter", list[i].ter ) ) {
            state.mark_changed();
        }
        ImGui::SameLine();
        if( ImGui::ArrowButton( "##placing", ImGuiDir_Right ) ) {
            // TODO: edit placing
        }
        ImGui::PopID();
    }
    if( del ) {
        const uuid_t &uuid = list[ *del ].uuid;
        state.file().base.remove_usages( uuid );
        if( state.rows_brush == uuid ) {
            state.rows_brush = UUID_INVALID;
        }
        list.erase( list.begin() + *del );
        state.mark_changed();
    }
    if( move_up ) {
        std::swap( list[*move_up], list[*move_up - 1] );
        state.mark_changed();
    }
    if( move_dn ) {
        std::swap( list[*move_dn], list[*move_dn + 1] );
        state.mark_changed();
    }
    if( ImGui::ImageButton( "add", "me_add" ) ) {
        list.emplace_back( me_palette_entry{
            state.file().uuid_gen(),
            state.file().base.pick_available_key(),
            ImVec4(),
            ter_eid::NULL_ID(),
            furn_eid::NULL_ID(),
            me_placing()
        } );
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

static void handle_revision_change( me_state &state )
{
    if( state.ongoing_brush_stroke ) {
        return;
    }
    if( ImGui::IsKeyDown( ImGuiKey_LeftCtrl ) && ImGui::IsKeyPressed( ImGuiKey_Z ) ) {
        if( ImGui::IsKeyDown( ImGuiKey_LeftShift ) ) {
            if( state.can_redo() ) {
                state.queue_redo();
            }
        } else {
            if( state.can_undo() ) {
                state.queue_undo();
            }
        }
    }
    if( state.switch_to_revision ) {
        auto it = std::find_if( state.file_history.cbegin(),
        state.file_history.cend(), [&]( const me_file_revision & rev ) {
            return rev.num == *state.switch_to_revision;
        } );
        assert( it != state.file_history.cend() );
        state.current_revision = it->make_copy();
        state.switch_to_revision.reset();
    } else if( state.file_has_changes ) {
        state.file_has_changes = false;

        // Erase alternative history
        while( state.file_history[0].num != state.current_revision.num ) {
            state.file_history.erase( state.file_history.cbegin() );
        }

        state.current_revision.num++;
        state.file_history.insert( state.file_history.cbegin(), state.current_revision.make_copy() );

        // Erase old entries
        if( static_cast<int>( state.file_history.size() ) > state.history_capacity ) {
            state.file_history.resize( state.history_capacity );
        }
    }
}

void show_me_ui( me_state &state )
{
    show_canvas( state );
    show_control_window( state );
    if( state.show_demo_wnd ) {
        ImGui::ShowDemoWindow( &state.show_demo_wnd );
    }
    if( state.show_asset_lib ) {
        show_asset_lib( state.assets, state.show_asset_lib );
    }
    if( state.show_file_info ) {
        show_file_info( state, state.file(), state.show_file_info );
    }
    if( state.show_file_history ) {
        show_file_history( state, state.show_file_history );
    }

    handle_revision_change( state );
}

point_rel_etile me_file::mapgensize()
{
    if( mtype == MapgenType::Nested ) {
        return point_rel_etile( nested.size );
    } else {
        return point_rel_etile( SEEX * 2, SEEY * 2 );
    }
}

me_map_key_generator::me_map_key_generator()
{
    const translation &trans = SNIPPET.get_snippet_ref_by_id( snippet_id( "me_auto_map_keys" ) );
    std::u32string s_u32 = utf8_to_utf32( trans.raw );
    // TODO: support combining characters
    opts.reserve( s_u32.size() );
    for( const char32_t &ch32 : s_u32 ) {
        opts.emplace_back( utf32_to_utf8( ch32 ) );
    }
}

void me_map_key_generator::blacklist( const map_key &opt )
{
    std::remove( opts.begin(), opts.end(), opt );
}

me_state::me_state()
{
    current_revision = me_file_revision();

    me_file &f = *current_revision.file;
    f.base.set_size( f.mapgensize().raw() );

    file_history.reserve( history_capacity + 1 );
    file_history.emplace_back( current_revision.make_copy() );

    init_assets( assets );
}

me_state::~me_state() = default;

const map_key &me_palette::key_from_uuid( const uuid_t &uuid ) const
{
    if( uuid == UUID_INVALID ) {
        return default_map_key;
    }
    for( const auto &it : entries ) {
        if( it.uuid == uuid ) {
            return it.key;
        }
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
    for( const auto &it : entries ) {
        if( it.uuid == uuid ) {
            return it.color;
        }
    }

    std::cerr << "Tried to find palette color, but uuid was not found " << uuid << std::endl;
    std::abort();
}

void me_mapgen_base::set_size( const point &s )
{
    if( size == s ) {
        return;
    }
    // TODO: graciously transfer entries from old size
    size = s;
    rows.clear();
    rows.resize( s.x * s.y, UUID_INVALID );
}

me_mapgen_base::~me_mapgen_base() = default;

map_key me_mapgen_base::pick_available_key() const
{
    me_map_key_generator gen;
    for( const auto &it : inline_palette.entries ) {
        gen.blacklist( it.key );
    }
    return gen();
}

void me_mapgen_base::remove_usages( const uuid_t &uuid )
{
    for( uuid_t &cell : rows ) {
        if( cell == uuid ) {
            cell = UUID_INVALID;
        }
    }
}

template<>
const std::vector<std::string> &editable_id<ter_t>::get_all_opts()
{
    if( all_opts.empty() ) {
        all_opts.reserve( ter_t::get_all().size() );
        for( const ter_t &it : ter_t::get_all() ) {
            all_opts.push_back( it.id.str() );
        }
    }
    return all_opts;
}

template<>
const std::vector<std::string> &editable_id<furn_t>::get_all_opts()
{
    if( all_opts.empty() ) {
        all_opts.reserve( furn_t::get_all().size() );
        for( const furn_t &it : furn_t::get_all() ) {
            all_opts.push_back( it.id.str() );
        }
    }
    return all_opts;
}

template<>
const std::vector<std::string> &editable_id<oter_t>::get_all_opts()
{
    if( all_opts.empty() ) {
        all_opts.reserve( overmap_terrains::get_all().size() );
        for( const oter_t &it : overmap_terrains::get_all() ) {
            all_opts.push_back( it.id.str() );
        }
    }
    return all_opts;
}

template<>
const std::vector<std::string> &editable_id<mapgen_palette>::get_all_opts()
{
    if( all_opts.empty() ) {
        all_opts.reserve( mapgen_palette::get_all().size() );
        for( const auto &it : mapgen_palette::get_all() ) {
            all_opts.push_back( it.first.str() );
        }
    }
    return all_opts;
}

} // namespace editor

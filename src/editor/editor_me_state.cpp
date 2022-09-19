#include "editor_me_state.h"
#include "editor_widgets.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include "../game_constants.h"
#include "../string_utils.h"
#include "../omdata.h"

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
                  ImGuiWindowFlags_NoBringToFrontOnFocus
                );

    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    const ImVec4 col_cursor = ImVec4( 0.8f, 0.8f, 0.4f, 1.0f );
    const ImVec4 col_mapgensize_bg = ImVec4( 0.7f, 0.7f, 0.7f, 0.1f );
    const ImVec4 col_mapgensize_border = ImVec4( 0.7f, 0.7f, 0.7f, 1.0f );

    highlight_region( draw_list, state.camera, point_abs_etile( 0, 0 ), point_abs_etile( -1,
                      -1 ) + state.file.mapgensize(), col_mapgensize_bg, col_mapgensize_border );

    ImGuiIO &io = ImGui::GetIO();
    bool canvas_hovered = ImGui::IsWindowHovered();
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

    // Contols
    if( ImGui::Button( "Toggle Asset Library" ) ) {
        state.show_asset_lib = !state.show_asset_lib;
    }
    if( ImGui::Button( "Toggle File Info" ) ) {
        state.show_file_info = !state.show_file_info;
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
    }
    ImGui::SameLine();
    if( ImGui::RadioButton( "Update", file.mtype == MapgenType::Update ) ) {
        file.mtype = MapgenType::Update;
    }
    ImGui::SameLine();
    if( ImGui::RadioButton( "Nested", file.mtype == MapgenType::Nested ) ) {
        file.mtype = MapgenType::Nested;
    }
    ImGui::Separator();

    if( file.mtype == MapgenType::Oter ) {
        ImGui::InputJmapgenInt( "rotation", file.oter.rotation );
        ImGui::Text( "Oter mapgen base:" );

        if( ImGui::RadioButton( "Fill terrain", file.oter.mapgen_base == OterMapgenBase::FillTer ) ) {
            file.oter.mapgen_base = OterMapgenBase::FillTer;
        }
        ImGui::SameLine();
        if( ImGui::RadioButton( "Predecessor mapgen",
                                file.oter.mapgen_base == OterMapgenBase::PredecessorMapgen ) ) {
            file.oter.mapgen_base = OterMapgenBase::PredecessorMapgen;
        }
        ImGui::SameLine();
        if( ImGui::RadioButton( "Rows", file.oter.mapgen_base == OterMapgenBase::Rows ) ) {
            file.oter.mapgen_base = OterMapgenBase::Rows;
        }

        if( file.oter.mapgen_base == OterMapgenBase::FillTer ) {
            ImGui::InputId( "fill_ter", file.oter.fill_ter );
        }
        if( file.oter.mapgen_base == OterMapgenBase::PredecessorMapgen ) {
            ImGui::InputId( "predecessor_mapgen", file.oter.predecessor_mapgen );
        }
        if( file.oter.mapgen_base == OterMapgenBase::Rows ) {
            ImGui::Text( "TODO: rows" );
        }
    } else if( file.mtype == MapgenType::Update ) {
        ImGui::InputId( "fill_ter", file.update.fill_ter );
    } else { // MapgenType::Nested
        ImGui::InputJmapgenInt( "rotation", file.nested.rotation );
        // Only square nested mapgens are possible
        if( ImGui::InputInt( "size", &file.nested.size.x, -1, -1 ) ) {
            int size = clamp( file.nested.size.x, 1, SEEX * 2 );
            file.nested.size.x = size;
            file.nested.size.y = size;
        }
    }

    show_palette( file.base.inline_palette, state.show_base_inline_palette );

    ImGui::End();
}

template<typename T, typename F>
void show_palette_map( const char *label, std::vector<std::pair<map_key, T>> &list, F payload_f )
{
    ImGui::PushID( label );
    ImGui::Text( "%s", label );
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
        if( ImGui::ImageButton( "up", "me_move_up" ) ) {
            move_up = i;
        }
        if( i == 0 ) {
            ImGui::EndDisabled();
        }
        ImGui::SameLine();

        if( i == list.size() - 1 ) {
            ImGui::BeginDisabled();
        }
        if( ImGui::ImageButton( "down", "me_move_down" ) ) {
            move_dn = i;
        }
        if( i == list.size() - 1 ) {
            ImGui::EndDisabled();
        }
        ImGui::SameLine();

        ImGui::SetNextItemWidth( ImGui::GetFrameHeight() );
        if( ImGui::InputText( "##key", &list[i].first.str, ImGuiInputTextFlags_AutoSelectAll ) ) {
            std::u32string s32 = utf8_to_utf32( list[i].first.str );
            list[i].first.str = utf32_to_utf8( s32[0] );
        }
        ImGui::SameLine();
        ImGui::PushID( "payload" );
        payload_f( list[i].second );
        ImGui::PopID();
        ImGui::PopID();
    }
    if( del ) {
        list.erase( list.begin() + *del );
    }
    if( move_up ) {
        std::swap( list[*move_up], list[*move_up - 1] );
    }
    if( move_dn ) {
        std::swap( list[*move_dn], list[*move_dn + 1] );
    }
    if( ImGui::ImageButton( "add", "me_add" ) ) {
        list.emplace_back();
    }
    ImGui::PopID();
}

void show_palette( me_palette &p, bool &show )
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
        ImGui::InputId( "id", p.id );
    }

    show_palette_map( "Terrains:", p.terrain, []( ter_eid & id ) {
        ImGui::InputId( "##", id );
    } );

    show_palette_map( "Furniture:", p.furniture, []( furn_eid & id ) {
        ImGui::InputId( "##", id );
    } );

    show_palette_map( "Placings:", p.placings, []( me_placing & pl ) {
        ImGui::InputText( "##", &pl.dummy );
    } );

    ImGui::End();
    ImGui::PopID();
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
        show_file_info( state, state.file, state.show_file_info );
    }
}

point_rel_etile me_file::mapgensize()
{
    if( mtype == MapgenType::Nested ) {
        return point_rel_etile( nested.size );
    } else {
        return point_rel_etile( SEEX * 2, SEEY * 2 );
    }
}

me_state::me_state()
{
    init_assets( assets );
}

me_state::~me_state() = default;

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

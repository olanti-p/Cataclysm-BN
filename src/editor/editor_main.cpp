#include "editor_main.h"
#include "editor_assets.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include "../avatar.h"
#include "../field.h"
#include "../game.h"
#include "../input.h"
#include "../map.h"
#include "../mapdata.h"
#include "../mapgen.h"
#include "../mongroup.h"
#include "../output.h"
#include "../sdl_utils.h"
#include "../sdltiles_editor.h"
#include "../string_formatter.h"
#include "../string_utils.h"
#include "../trap.h"
#include "../ui_manager.h"

namespace editor
{

struct editor_state {
    bool do_loop = true;
    bool show_demo_wnd = false;
    int loops = 0;
    int frames = 0;

    asset_library assets;

    cata::optional<tripoint> single_selection;
};

static int get_current_z( const editor_state & /*state*/ )
{
    return get_avatar().posz() + get_avatar().view_offset.z;
}

static void set_current_z( const editor_state &state, int z )
{
    int new_z = clamp( z, -10, 10 );
    if( get_current_z( state ) != new_z ) {
        get_avatar().view_offset.z = new_z - get_avatar().posz();
    }
}

static tripoint get_view_center( const editor_state & /*state*/ )
{
    return get_avatar().pos() + get_avatar().view_offset;
}

static void set_view_center( const editor_state & /*state*/, const tripoint &pos )
{
    get_avatar().view_offset = pos - get_avatar().pos();
}

static int get_zoom( const editor_state & /*state*/ )
{
    return g->get_zoom();
}

static void set_zoom( editor_state &state, int zoom )
{
    int new_zoom = clamp( zoom, 4, 64 );
    if( new_zoom != get_zoom( state ) ) {
        g->set_zoom( new_zoom );
        g->mark_main_ui_adaptor_resize();
    }
}

static point get_mouse_screen_pos( const editor_state & /*state*/ )
{
    ImVec2 mouse_pos = ImGui::GetMousePos();
    return point( mouse_pos.x, mouse_pos.y );
}

static cata::optional<tripoint> get_mouse_tile_pos( const editor_state &state )
{
    return editor::screen_to_tile( get_mouse_screen_pos( state ) );
}

static void show_control_window( editor_state &state )
{
    ImGui::Begin( "Advanced Map Editor", &state.do_loop );
    ImGui::Text( "Close this window to exit the editor. %d/%d", state.loops, state.frames );
    state.frames++;
    // Debugging
    if( ImGui::Button( "Toggle Demo Window" ) ) {
        state.show_demo_wnd = !state.show_demo_wnd;
    }
    if( ImGui::Button( "Toggle Submap Grid" ) ) {
        g->debug_submap_grid_overlay = !g->debug_submap_grid_overlay;
    }

    avatar &u = get_avatar();

    // Camera zoom
    {
        int zoom = get_zoom( state );
        ImGui::DragInt( "Zoom", &zoom, 0.2f, 4, 64 );
        set_zoom( state, zoom );
    }

    // Camera offset
    {
        std::vector<int> offs = {{
                u.view_offset.x,
                u.view_offset.y
            }
        };
        std::vector<int> offs_old = offs;
        ImGui::DragInt2( "Offset", &offs[0], 0.2f, -60, 60 );
        if( offs != offs_old ) {
            u.view_offset.x = offs[0];
            u.view_offset.y = offs[1];
        }

        int zlev = get_current_z( state );
        ImGui::DragInt( "Z-Level", &zlev, 0.05f, -10, 10 );
        set_current_z( state, zlev );
    }

    // Mouse position
    {
        point mouse_pos = get_mouse_screen_pos( state );
        cata::optional<tripoint> tile_pos = get_mouse_tile_pos( state );
        ImGui::Text( "Mouse pos, px: %s", mouse_pos.to_string().c_str() );
        if( tile_pos ) {
            ImGui::Text( "Mouse pos, tile: %s", tile_pos->to_string().c_str() );
        } else {
            ImGui::Text( "Mouse pos, tile: ???" );
        }
    }

    ImGui::End();
}

static void show_tile_properties_window( editor_state &state,
        const cata::optional<tripoint> &tile_pos )
{
    ImGui::Begin( "Tile Properties" );

    if( !tile_pos ) {
        ImGui::Text( "< ??? >" );
        ImGui::End();
        return;
    }

    tripoint p = *tile_pos;
    ImGui::Text( "pos: %s", p.to_string().c_str() );

    map &here = get_map();
    if( !here.inbounds( p ) ) {
        ImGui::Text( "< Out of bounds >" );
        ImGui::End();
        return;
    }

    // Terrain
    ter_id tid = here.ter( p );
    std::string tname = here.tername( p );
    ImGui::Text( "%s <%s>", tname.c_str(), tid->id.c_str() );

    // Furniture
    furn_id fid = here.furn( p );
    std::string fname = here.furnname( p );
    ImGui::Text( "%s <%s>", fname.c_str(), fid->id.c_str() );

    // Trap
    const trap &tr = here.tr_at( p );
    std::string trname = tr.name();
    ImGui::Text( "%s <%s>", trname.c_str(), tr.id.c_str() );

    // Creature
    const Creature *cr = g->critter_at( p );
    if( cr ) {
        std::string disp_name = cr->disp_name();
        ImGui::Text( "%s", disp_name.c_str() );
    } else {
        ImGui::Text( "< No creature here >" );
    }

    // Field
    const field &fields = here.field_at( p );
    for( const auto &fld : fields ) {
        std::string name = fld.second.name();
        const field_type_str_id &id = fld.first.id();
        int intensity = fld.second.get_field_intensity();
        time_duration dur = fld.second.get_field_age();
        ImGui::Text( "%s <%s> [%d] %d", name.c_str(), id.c_str(), intensity, to_turns<int>( dur ) );
    }

    // Items
    map_stack items = here.i_at( p );
    if( !items.empty() ) {
        ImGui::Text( "%d Item(s)", static_cast<int>( items.size() ) );
    } else {
        ImGui::Text( "< No items here >" );
    }

    // Radiation
    int rad = here.get_radiation( p );
    ImGui::Text( "Rad level: %d", rad );

    ImGui::End();
}

static void highlight_tile( ImDrawList *draw_list, point tile, ImU32 col )
{
    std::pair<point, point> rect = editor::tile_to_screen( tile );
    ImVec2 p_min( rect.first.x, rect.first.y );
    ImVec2 p_max( rect.second.x, rect.second.y );
    draw_list->AddRect( p_min, p_max, col, 0.0f, ImDrawFlags_None, 1.0f );
}

static void show_canvas_overlay_window( editor_state &state )
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

    const ImU32 col_cursor = ImColor( 0.8f, 0.8f, 0.4f, 1.0f );
    const ImU32 col_selected = ImColor( 1.0f, 1.0f, 0.0f, 1.0f );

    cata::optional<tripoint> tile_pos = get_mouse_tile_pos( state );
    if( tile_pos ) {
        highlight_tile( draw_list, tile_pos->xy(), col_cursor );
    }

    if( state.single_selection && state.single_selection->z == get_current_z( state ) ) {
        highlight_tile( draw_list, state.single_selection->xy(), col_selected );
    }

    ImGuiIO &io = ImGui::GetIO();
    if( ImGui::IsWindowHovered() ) {
        if( tile_pos ) {
            if( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
                if( tile_pos == state.single_selection ) {
                    state.single_selection = cata::nullopt;
                } else {
                    state.single_selection = tile_pos;
                }
            }
            if( ImGui::IsKeyDown( ImGuiKey_ModShift ) && ImGui::IsMouseClicked( ImGuiMouseButton_Right ) ) {
                set_view_center( state, *tile_pos );
            }
        }
        if( std::abs( io.MouseWheel ) > 0.5f ) {
            if( ImGui::IsKeyDown( ImGuiKey_ModCtrl ) ) {
                set_zoom( state, get_zoom( state ) + io.MouseWheel );
            } else if( ImGui::IsKeyDown( ImGuiKey_ModShift ) ) {
                set_current_z( state, get_current_z( state ) - io.MouseWheel );
            }
        }
    }

    ImGui::End();
}

static bool filter_matches( const std::string &s, const std::string &filter )
{
    return lcmatch( s, filter );
}

static void show_assetlib_tab( asset_library_cat &cat )
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
            const char *id = cat.get( i ).get_id();
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

static void show_asset_library_window( asset_library &assets )
{
    ImGui::Begin( "Asset Library" );

    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_FittingPolicyResizeDown;
    if( ImGui::BeginTabBar( "Asset Types", tab_bar_flags ) ) {
        for( asset_library_cat &cat : assets.categories ) {
            show_assetlib_tab( cat );
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

static void show_asset_details_window( editor_state &state )
{
    const asset_lib_entry &entry = state.assets.get_selected_asset();

    ImGui::Begin( "Asset Details" );

    ImGui::Text( "%s: %s", get_asset_type_name( entry.get_type() ), entry.get_id() );

    entry.show_details();

    ImGui::End();
}

static void show_editor_ui( editor_state &state )
{
    show_canvas_overlay_window( state );
    show_control_window( state );
    if( state.single_selection ) {
        show_tile_properties_window( state, state.single_selection );
    } else {
        show_tile_properties_window( state, get_mouse_tile_pos( state ) );
    }
    show_asset_library_window( state.assets );
    show_asset_details_window( state );
    if( state.show_demo_wnd ) {
        ImGui::ShowDemoWindow( &state.show_demo_wnd );
    }
}

static editor::editor_state *current_state = nullptr;

void advanced_editor_run()
{
    editor_state state;
    init_assets( state.assets );
    current_state = &state;

    bool old_submap_grid = g->debug_submap_grid_overlay;
    tripoint old_view = get_avatar().view_offset;
    int old_zoom = g->get_zoom();
    on_out_of_scope _close_ui( [&]() {
        current_state = nullptr;
        g->debug_submap_grid_overlay = old_submap_grid;
        get_avatar().view_offset = old_view;
        g->set_zoom( old_zoom );
        g->mark_main_ui_adaptor_resize();
    } );

    g->debug_submap_grid_overlay = true;

    g->invalidate_main_ui_adaptor();
    ui_manager::redraw();
    refresh_display();

    while( state.do_loop ) {
        state.loops++;

        inp_mngr.get_input_event();
        g->invalidate_main_ui_adaptor();
        ui_manager::redraw();
        refresh_display();
    }

    g->invalidate_main_ui_adaptor();
    ui_manager::redraw();
    refresh_display();
}

bool ui_exists()
{
    return current_state != nullptr;
}

void show_ui()
{
    show_editor_ui( *current_state );
}

ImVec4 curses_color_to_imgui( nc_color nc )
{
    SDL_Color col = curses_color_to_SDL( nc );
    return ImVec4( col.r, col.g, col.b, col.a );
}

} // namespace editor

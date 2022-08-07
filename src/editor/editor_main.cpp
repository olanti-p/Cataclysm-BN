#include "editor_main.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include "../avatar.h"
#include "../field.h"
#include "../game.h"
#include "../input.h"
#include "../item_factory.h"
#include "../item_group.h"
#include "../map.h"
#include "../mapdata.h"
#include "../mapgen.h"
#include "../mapgen_factory.h"
#include "../mongroup.h"
#include "../monstergenerator.h"
#include "../output.h"
#include "../sdltiles_editor.h"
#include "../string_formatter.h"
#include "../string_utils.h"
#include "../trap.h"
#include "../ui_manager.h"

// For some fun reason item_group_id does not actually refer to any item group.
// Not that item groups appear to be stored at all...!?
struct igroup_plug {
    item_group_id id;
};

struct nested_mapgen_plug {
    std::string id;
    std::shared_ptr<mapgen_function_json_nested> data;
};

struct update_mapgen_plug {
    std::string id;
    update_mapgen_function_json *data = nullptr;
};

struct oter_mapgen_plug {
    std::string id;
    std::shared_ptr<mapgen_function> data;
};

template<typename T>
struct asset_library_cat {
    std::vector<const T *> entries;
    std::string filter;
    int selected = 0;
};

struct asset_library {
    std::vector<igroup_plug> igroup_plugs;
    std::vector<nested_mapgen_plug> nested_mapgen_plugs;
    std::vector<update_mapgen_plug> update_mapgen_plugs;
    std::vector<oter_mapgen_plug> oter_mapgen_plugs;

    asset_library_cat<ter_t> terrains;
    asset_library_cat<furn_t> furnitures;
    asset_library_cat<trap> traps;
    asset_library_cat<field_type> fields;
    asset_library_cat<itype> itypes;
    asset_library_cat<igroup_plug> igroups;
    asset_library_cat<mtype> mtypes;
    asset_library_cat<MonsterGroup> mgroups;
    asset_library_cat<mapgen_palette> palettes;
    asset_library_cat<nested_mapgen_plug> nested_mapgens;
    asset_library_cat<update_mapgen_plug> update_mapgens;
    asset_library_cat<oter_mapgen_plug> oter_mapgens;
};

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

template<typename T>
void show_assetlib_tab( const char *name, asset_library_cat<T> &cat )
{
    if( !ImGui::BeginTabItem( name ) ) {
        return;
    }

    int num_all = static_cast<int>( cat.entries.size() );
    ImGui::Text( "%s - %d entries", name, num_all );

    ImGui::InputText( "##filter", &cat.filter );
    std::string lb_name = string_format( "##%s", name );
    if( ImGui::BeginListBox( lb_name.c_str(), ImVec2( -1.0f, -1.0f ) ) ) {
        for( int i = 0; i < num_all; i++ ) {
            if( !filter_matches( cat.entries[i]->id.c_str(), cat.filter ) ) {
                continue;
            }
            const bool is_selected = i == cat.selected;
            if( ImGui::Selectable( cat.entries[i]->id.c_str(), is_selected ) ) {
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

        show_assetlib_tab( "Terrain", assets.terrains );
        show_assetlib_tab( "Furniture", assets.furnitures );
        show_assetlib_tab( "Trap", assets.traps );
        show_assetlib_tab( "Field", assets.fields );
        show_assetlib_tab( "Item", assets.itypes );
        show_assetlib_tab( "IGroup", assets.igroups );
        show_assetlib_tab( "Monster", assets.mtypes );
        show_assetlib_tab( "MGroup", assets.mgroups );
        show_assetlib_tab( "Palette", assets.palettes );
        show_assetlib_tab( "N_Mapgen", assets.nested_mapgens );
        show_assetlib_tab( "U_Mapgen", assets.update_mapgens );
        show_assetlib_tab( "O_Mapgen", assets.oter_mapgens );

        ImGui::EndTabBar();
    }

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
    if( state.show_demo_wnd ) {
        ImGui::ShowDemoWindow( &state.show_demo_wnd );
    }
}

static void init_assets( asset_library &assets )
{
    for( const ter_t &elem : ter_t::get_all() ) {
        assets.terrains.entries.push_back( &elem );
    }
    for( const furn_t &elem : furn_t::get_all() ) {
        assets.furnitures.entries.push_back( &elem );
    }
    for( const trap &elem : trap::get_all() ) {
        assets.traps.entries.push_back( &elem );
    }
    for( const field_type &elem : field_types::get_all() ) {
        assets.fields.entries.push_back( &elem );
    }
    assets.itypes.entries = item_controller->all();
    for( const item_group_id &elem : item_controller->get_all_group_names() ) {
        igroup_plug plug;
        plug.id = elem;
        assets.igroup_plugs.push_back( std::move( plug ) );
    }
    for( const igroup_plug &elem : assets.igroup_plugs ) {
        assets.igroups.entries.push_back( &elem );
    }
    for( const mtype &elem : MonsterGenerator::generator().get_all_mtypes() ) {
        assets.mtypes.entries.push_back( &elem );
    }
    for( const auto &elem : MonsterGroupManager::get_all() ) {
        assets.mgroups.entries.push_back( &elem.second );
    }
    for( const auto &elem : mapgen_palette::get_all() ) {
        assets.palettes.entries.push_back( &elem.second );
    }

    const auto &all_nested = get_all_nested_mapgen();
    for( auto &it : all_nested ) {
        const std::string &id = it.first;
        int i = 0;
        for( auto &obj : it.second ) {
            nested_mapgen_plug plug;
            plug.id = string_format( "%s:w=%d:i=%d", id, obj.weight, i );
            plug.data = obj.obj;
            assets.nested_mapgen_plugs.push_back( std::move( plug ) );
            i++;
        }
    }
    for( const nested_mapgen_plug &elem : assets.nested_mapgen_plugs ) {
        assets.nested_mapgens.entries.push_back( &elem );
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
        assets.update_mapgens.entries.push_back( &elem );
    }

    const mapgen_factory &all_oter = get_all_oter_mapgen();
    for( const auto &it : all_oter.mapgens_ ) {
        const std::string &id = it.first;
        int i = 0;
        for( const auto &obj : it.second.weights_ ) {
            oter_mapgen_plug plug;
            plug.id = string_format( "%s:w=%d:i=%d", id, obj.weight, i );
            plug.data = obj.obj;
            assets.oter_mapgen_plugs.push_back( std::move( plug ) );
            i++;
        }
    }
    for( const oter_mapgen_plug &elem : assets.oter_mapgen_plugs ) {
        assets.oter_mapgens.entries.push_back( &elem );
    }
}

static editor_state *current_state = nullptr;

namespace editor
{
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
}

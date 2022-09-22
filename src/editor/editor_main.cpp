#include "editor_assets.h"
#include "editor_main.h"
#include "editor_me_state.h"
#include "editor_projects.h"
#include "editor_widgets.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include "../avatar.h"
#include "../creature_tracker.h"
#include "../field.h"
#include "../fstream_utils.h"
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

#include <thread>

namespace editor
{

enum class EditorTool : int {
    Examine,
    TerSet,
    TerPick,
    FurnSet,
    FurnPick,

    NumEditorTools
};

struct editor_state {
    bool do_loop = true;
    bool show_demo_wnd = false;
    bool show_asset_lib = true;
    bool show_cata_ui = true;
    int loops = 0;
    int frames = 0;
    const mapgen_function_json *selected_oter_mapgen = nullptr;

    EditorTool current_tool = EditorTool::Examine;
    ter_str_id brush_ter = ter_str_id::NULL_ID();
    furn_str_id brush_furn = furn_str_id::NULL_ID();

    asset_library assets;

    const asset_lib_entry *copied_entry = nullptr;

    cata::optional<tripoint> examine_selection;

    cata::optional<me_projects_state> projects_state;
    cata::optional<me_state> mapgenedit_state;
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
    ImGui::SameLine();
    if( ImGui::Button( "Toggle Asset Library" ) ) {
        state.show_asset_lib = !state.show_asset_lib;
    }
    ImGui::SameLine();
    if( ImGui::Button( "Toggle Submap Grid" ) ) {
        g->debug_submap_grid_overlay = !g->debug_submap_grid_overlay;
    }
    if( ImGui::Button( "Toggle Cata UI (Debug)" ) ) {
        state.show_cata_ui = !state.show_cata_ui;
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

    // Brush contents
    if( state.current_tool == EditorTool::TerPick || state.current_tool == EditorTool::TerSet ) {
        std::string buf = state.brush_ter.str();
        if( ImGui::InputAssetId( state, "Brush", buf, AssetType::Terrain ) ) {
            state.brush_ter = ter_str_id( buf );
        }
    }
    if( state.current_tool == EditorTool::FurnPick || state.current_tool == EditorTool::FurnSet ) {
        std::string buf = state.brush_furn.str();
        if( ImGui::InputAssetId( state, "Brush", buf, AssetType::Furniture ) ) {
            state.brush_furn = furn_str_id( buf );
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
        int num_items = static_cast<int>( items.size() );
        if( ImGui::TreeNode( "items_list", "%d Item(s)", num_items ) ) {
            int i = 0;
            for( const item &itm : items ) {
                std::string name = itm.display_name( 1 );
                const void *node_id = ( const void * )( intptr_t )i;
                if( ImGui::TreeNode( node_id, "[%d] %s", i, name.c_str() ) ) {
                    ImGui::Separator();
                    ImGui::Text( "TODO: more info" );
                    ImGui::Separator();

                    ImGui::TreePop();
                }
                i++;
            }

            ImGui::TreePop();
        }
    } else {
        ImGui::Text( "< No items here >" );
    }

    // Radiation
    int rad = here.get_radiation( p );
    ImGui::Text( "Rad level: %d", rad );

    ImGui::End();
}

static const char *get_tool_name( EditorTool tool )
{
    switch( tool ) {
        case EditorTool::Examine:
            return "Examine";
        case EditorTool::TerSet:
            return "Set Terrain";
        case EditorTool::TerPick:
            return "Pick Terrain";
        case EditorTool::FurnSet:
            return "Set Furniture";
        case EditorTool::FurnPick:
            return "Pick Furniture";
        default:
            std::abort();
    }
}

static void apply_tool_lmb( editor_state &state, const tripoint &p )
{
    map &here = get_map();
    switch( state.current_tool ) {
        case EditorTool::Examine: {
            if( state.examine_selection && p == *state.examine_selection ) {
                state.examine_selection = cata::nullopt;
            } else {
                state.examine_selection = p;
            }
            break;
        }
        case EditorTool::TerSet: {
            here.ter_set( p, state.brush_ter.id() );
            break;
        }
        case EditorTool::TerPick: {
            state.brush_ter = here.ter( p ).id();
            state.current_tool = EditorTool::TerSet;
            break;
        }
        case EditorTool::FurnSet: {
            here.furn_set( p, state.brush_furn.id() );
            break;
        }
        case EditorTool::FurnPick: {
            state.brush_furn = here.furn( p ).id();
            state.current_tool = EditorTool::FurnSet;
            break;
        }
        default: {
            break;
        }
    }
}

static void apply_tool_rmb( editor_state &state, const tripoint &p )
{
    map &here = get_map();
    switch( state.current_tool ) {
        case EditorTool::TerSet: {
            state.brush_ter = here.ter( p ).id();
            break;
        }
        case EditorTool::FurnSet: {
            state.brush_furn = here.furn( p ).id();
            break;
        }
        default: {
            break;
        }
    }
}

static void draw_frame( ImDrawList *draw_list, const point &p1, const point &p2, ImVec4 col,
                        bool filled )
{
    ImVec2 p_min;
    ImVec2 p_max;
    if( p1 == p2 ) {
        std::pair<point, point> rect = editor::tile_to_screen( p1 );
        p_min = ImVec2( rect.first.x, rect.first.y );
        p_max = ImVec2( rect.second.x, rect.second.y );
    } else {
        point r1 = editor::tile_to_screen( p1 ).first;
        p_min = ImVec2( r1.x, r1.y );
        point r2 = editor::tile_to_screen( p2 ).second;
        p_max = ImVec2( r2.x, r2.y );
    }
    if( filled ) {
        draw_list->AddRectFilled( p_min, p_max, ImColor( col ), 0.0f, ImDrawFlags_None );
    } else {
        draw_list->AddRect( p_min, p_max, ImColor( col ), 0.0f, ImDrawFlags_None, 1.0f );
    }
}

static void highlight_tile( ImDrawList *draw_list, point tile, ImVec4 col )
{
    draw_frame( draw_list, tile, tile, col, false );
}

static void highlight_region( ImDrawList *draw_list, point p1, point p2, ImVec4 col_bg,
                              ImVec4 col_border )
{
    draw_frame( draw_list, p1, p2, col_bg, true );
    draw_frame( draw_list, p1, p2, col_border, false );
}

static bool pos_in_rect( point p, point p1, point p2 )
{
    return p.x >= p1.x && p.y >= p1.y && p.x <= p2.x && p.y <= p2.y;
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

    const ImVec4 col_cursor = ImVec4( 0.8f, 0.8f, 0.4f, 1.0f );
    const ImVec4 col_selected = ImVec4( 1.0f, 1.0f, 0.0f, 1.0f );
    const ImVec4 col_obj_bg = ImVec4( 0.0f, 1.0f, 0.0f, 0.2f );
    const ImVec4 col_obj_border = ImVec4( 0.0f, 1.0f, 0.0f, 0.8f );
    const ImVec4 col_obj_hilite_bg = ImVec4( 0.4f, 1.0f, 0.4f, 0.3f );
    const ImVec4 col_obj_hilite_border = ImVec4( 0.4f, 1.0f, 0.4f, 0.8f );
    const ImVec4 col_mapgensize_bg = ImVec4( 0.7f, 0.7f, 0.7f, 0.1f );
    const ImVec4 col_mapgensize_border = ImVec4( 0.7f, 0.7f, 0.7f, 1.0f );

    cata::optional<tripoint> tile_pos = get_mouse_tile_pos( state );
    if( tile_pos ) {
        highlight_tile( draw_list, tile_pos->xy(), col_cursor );
    }

    if( state.current_tool == EditorTool::Examine &&
        state.examine_selection &&
        state.examine_selection->z == get_current_z( state ) ) {
        highlight_tile( draw_list, state.examine_selection->xy(), col_selected );
    }

    ImGuiIO &io = ImGui::GetIO();
    bool canvas_hovered = ImGui::IsWindowHovered();
    if( canvas_hovered ) {
        if( tile_pos ) {
            if( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) && tile_pos ) {
                apply_tool_lmb( state, *tile_pos );
            }
            if( ImGui::IsMouseClicked( ImGuiMouseButton_Right ) ) {
                if( ImGui::IsKeyDown( ImGuiKey_ModShift ) ) {
                    set_view_center( state, *tile_pos );
                } else {
                    apply_tool_rmb( state, *tile_pos );
                }
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

    if( state.selected_oter_mapgen ) {
        const mapgen_function_json &func = *state.selected_oter_mapgen;

        highlight_region( draw_list, point_zero, func.mapgensize - point( 1, 1 ), col_mapgensize_bg,
                          col_mapgensize_border );

        std::vector<std::pair<int, const jmapgen_piece *>> hovered;

        int i = 0;
        for( const jmapgen_objects::jmapgen_obj &obj : func.objects.objects ) {
            ImVec4 col_bg, col_border;
            point p1 = point( obj.first.x.val, obj.first.y.val );
            point p2 = point( obj.first.x.valmax, obj.first.y.valmax );
            if( canvas_hovered && tile_pos && pos_in_rect( tile_pos->xy(), p1, p2 ) ) {
                hovered.push_back( std::make_pair( i, obj.second.get() ) );
                col_bg = col_obj_hilite_bg;
                col_border = col_obj_hilite_border;
            } else {
                col_bg = col_obj_bg;
                col_border = col_obj_border;
            }
            highlight_region( draw_list, p1, p2, col_bg, col_border );

            i++;
        }
        if( !hovered.empty() ) {
            ImGui::BeginTooltip();
            ImGui::Text( "Pieces here:" );
            for( const auto &piece : hovered ) {
                ImGui::Text( "%d: %p", piece.first, piece.second );
            }
            ImGui::EndTooltip();
        }
    }

    ImGui::End();
}

static void on_asset_double_click( editor_state &state, const asset_lib_entry &entry,
                                   AssetType atype )
{
    if( atype == AssetType::Furniture &&
        ( state.current_tool == EditorTool::FurnPick || state.current_tool == EditorTool::FurnSet ) ) {
        state.brush_furn = furn_str_id( entry.get_id() );
    } else if( atype == AssetType::Terrain &&
               ( state.current_tool == EditorTool::TerPick || state.current_tool == EditorTool::TerSet ) ) {
        state.brush_ter = ter_str_id( entry.get_id() );
    } else if( atype == AssetType::OterMapgen ) {
        const mapgen_function *func = dynamic_cast<const asset_oter_mapgen &>( entry ).ref.data;
        const mapgen_function_json *jsfunc = dynamic_cast<const mapgen_function_json *>( func );
        if( jsfunc ) {
            set_as_active( jsfunc );
        }
    } else {
        state.copied_entry = &entry;
    }
}

static bool filter_matches( const std::string &s, const std::string &filter )
{
    return lcmatch( s, filter );
}

static void show_assetlib_tab( asset_library_cat &cat, editor_state &state )
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
            if( ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) ) {
                on_asset_double_click( state, entry, cat.atype );
            }
        }
        ImGui::EndListBox();
    }

    ImGui::EndTabItem();
}

static void show_asset_library_window( editor_state &state )
{
    ImGui::Begin( "Asset Library" );

    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_FittingPolicyResizeDown;
    if( ImGui::BeginTabBar( "Asset Types", tab_bar_flags ) ) {
        for( asset_library_cat &cat : state.assets.categories ) {
            show_assetlib_tab( cat, state );
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

static void set_current_tool( editor_state &state, EditorTool tool )
{
    state.examine_selection = cata::nullopt;
    state.current_tool = tool;
}

static void show_tool_bar( editor_state &state )
{
    ImGui::Begin( "Tools" );

    for( int i = 0; i < static_cast<int>( EditorTool::NumEditorTools ); i++ ) {
        const EditorTool ie = static_cast<EditorTool>( i );
        if( ImGui::Selectable( get_tool_name( ie ), ie == state.current_tool ) ) {
            state.current_tool = ie;
        }
    }

    ImGui::End();
}

static void show_editor_ui( editor_state &state )
{
    if( state.mapgenedit_state ) {
        show_me_ui( *state.mapgenedit_state );
        return;
    } else if( state.projects_state ) {
        show_projects_ui( *state.projects_state );
        return;
    }

    show_canvas_overlay_window( state );
    show_control_window( state );
    if( state.current_tool == EditorTool::Examine && state.examine_selection ) {
        show_tile_properties_window( state, state.examine_selection );
    } else {
        show_tile_properties_window( state, get_mouse_tile_pos( state ) );
    }
    if( state.show_asset_lib ) {
        show_asset_library_window( state );
        show_asset_details_window( state );
    }
    if( state.show_demo_wnd ) {
        ImGui::ShowDemoWindow( &state.show_demo_wnd );
    }
    if( state.selected_oter_mapgen ) {
        show_tool_bar( state );
    }
}

static editor::editor_state *current_state = nullptr;

void advanced_editor_run()
{
    {
        editor_state state;
        init_assets( state.assets );
        current_state = &state;

        state.projects_state = me_projects_state();
        state.show_cata_ui = false;

        bool old_submap_grid = g->debug_submap_grid_overlay;
        tripoint old_view = get_avatar().view_offset;
        int old_zoom = g->get_zoom();
        on_out_of_scope _close_ui( [&]() {
            current_state = nullptr;
            g->debug_submap_grid_overlay = old_submap_grid;
            get_avatar().view_offset = old_view;
            g->set_zoom( old_zoom );
            g->mark_main_ui_adaptor_resize();
            g->invalidate_main_ui_adaptor();
            editor::set_draw_view_center_mark( true );
        } );

        editor::set_draw_view_center_mark( false );
        g->debug_submap_grid_overlay = true;

        g->invalidate_main_ui_adaptor();
        invalidate_map_cache();
        ui_manager::redraw();
        refresh_display();

        while( state.do_loop ) {
            state.loops++;

            inp_mngr.get_input_event();
            if( state.show_cata_ui ) {
                g->invalidate_main_ui_adaptor();
                ui_manager::redraw();
            } else {
                std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
            }
            refresh_display();
            if( state.projects_state && state.projects_state->ret ) {
                const editor::projects_ui_retval &retval = *state.projects_state->ret;
                bool legacy_editor = false;
                if( retval.exit ) {
                    state.do_loop = false;
                } else if( retval.make_new ) {
                    state.mapgenedit_state = me_state();
                } else if( retval.load_existing ) {
                    std::unique_ptr<me_file> f = std::make_unique<me_file>();
                    auto reader = [&]( JsonIn & jsin ) {
                        f->deserialize( jsin );
                    };
                    if( read_from_file_json( retval.load_path, reader ) ) {
                        state.mapgenedit_state = me_state( std::move( f ), &retval.load_path );
                    } else {
                        state.projects_state->popup_prompt =
                            string_format( "Failed to load file:\n%s\nSee debug.log for details.", retval.load_path );
                    }
                } else if( retval.legacy_editor ) {
                    legacy_editor = true;
                }
                state.projects_state->ret.reset();
                if( legacy_editor ) {
                    state.projects_state.reset();
                }
            } else if( state.mapgenedit_state && !state.mapgenedit_state->do_loop ) {
                state.mapgenedit_state.reset();
            }
        }
    }

    ui_manager::redraw();
    refresh_display();
}

bool ui_exists()
{
    return current_state != nullptr;
}

bool show_cata_ui()
{
    return !current_state || current_state->show_cata_ui;
}

void show_ui()
{
    show_editor_ui( *current_state );
}

void invalidate_map_cache()
{
    for( int z = -OVERMAP_DEPTH; z < OVERMAP_HEIGHT; z++ ) {
        get_map().invalidate_map_cache( z );
    }
}

void set_as_active( const mapgen_function_json *mgfunc )
{
    current_state->selected_oter_mapgen = mgfunc;
    set_current_tool( *current_state, EditorTool::Examine );
    if( !mgfunc ) {
        return;
    }
    point size = mgfunc->mapgensize;
    map &here = get_map();

    // Set view center
    point new_view_center = size / 2;
    set_view_center( *current_state, tripoint( new_view_center, 0 ) );

    // TODO: move player away

    // Clear items & creatures
    for( int y = 0; y < size.y; y++ ) {
        for( int x = 0; x < size.x; x++ ) {
            tripoint p( x, y, 0 );
            here.i_clear( p );
            const Creature *cr = g->critter_at( p, true );
            if( cr && !cr->is_avatar() ) {
                g->erase_creature( *cr );
            }
        }
    }

    // Set terrain/furniture
    for( int y = 0; y < size.y; y++ ) {
        for( int x = 0; x < size.x; x++ ) {
            const ter_furn_id &ids = mgfunc->format[ y * size.x + x];
            here.ter_set( point( x, y ), ids.ter );
            here.furn_set( point( x, y ), ids.furn );
        }
    }

    invalidate_map_cache();
}

ImVec4 curses_color_to_imgui( nc_color nc )
{
    SDL_Color col = curses_color_to_SDL( nc );
    return ImVec4( col.r, col.g, col.b, col.a );
}

point get_visible_map_area()
{
    if( current_state && current_state->selected_oter_mapgen ) {
        return current_state->selected_oter_mapgen->mapgensize;
    } else {
        return point( MAPSIZE_X, MAPSIZE_Y );
    }
}

} // namespace editor

point::point( ImVec2 v )
{
    x = v.x;
    y = v.y;
}

point::operator ImVec2()
{
    return ImVec2( x, y );
}

namespace ImGui
{
bool InputAssetId( editor::editor_state &state, const std::string &label, std::string &buf,
                   editor::AssetType atype )
{
    ImGui::Text( "%s", label.c_str() );
    ImGui::SameLine();
    std::string widget_id = string_format( "##ass-id-in-%s", label );
    bool ret = ImGui::InputText( widget_id.c_str(), &buf );
    ImGui::SameLine();
    ImGui::BeginDisabled( !state.copied_entry || state.copied_entry->get_type() != atype );
    if( ImGui::Button( "Paste" ) ) {
        buf = state.copied_entry->get_id();
        ret = true;
    }
    ImGui::EndDisabled();
    return ret;
}

} // namespace ImGui

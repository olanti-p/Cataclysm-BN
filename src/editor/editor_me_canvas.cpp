#include "editor_me_canvas.h"
#include "editor_me_color.h"
#include "editor_widgets.h"

#include <set>

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

static void fill_tile_sprited(
    ImDrawList *draw_list,
    const me_camera &cam,
    point_abs_etile tile,
    const SpriteRef &img
)
{
    ImVec2 p_min = cam.world_to_screen( project_combine( tile, point_etile_epos() ) ).raw();
    ImVec2 p_max = cam.world_to_screen( project_combine( tile, point_etile_epos( ETILE_SIZE - 1,
                                        ETILE_SIZE - 1 ) ) ).raw();
    auto uvs = img.make_uvs();
    draw_list->AddImage( img.get_tex_id(), p_min, p_max, uvs.first, uvs.second );
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

/**
 * Find all tiles that match predicate.
*/
static std::vector<point> find_tiles_via_global( me_file &file,
        std::function<bool( const uuid_t & )> func )
{
    std::vector<point> ret;

    for( int x = 0; x < file.mapgensize().x(); x++ ) {
        for( int y = 0; y < file.mapgensize().y(); y++ ) {
            point p( x, y );
            const uuid_t &t = file.base.get_uuid_at( p );
            if( func( t ) ) {
                ret.push_back( p );
            }
        }
    }

    return ret;
}

/**
 * Find via floodfill all tiles that match predicate.
*/
static std::vector<point> find_tiles_via_floodfill( me_file &file, const point &initial_pos,
        std::function<bool( const uuid_t & )> func )
{
    std::vector<point> ret;

    if( !func( file.base.get_uuid_at( initial_pos ) ) ) {
        return ret;
    }

    std::set<point> open;
    std::set<point> closed;
    open.insert( initial_pos );
    point mgsize = file.mapgensize().raw();

    while( !open.empty() ) {
        auto it = open.cbegin();
        point p = *it;
        open.erase( it );
        closed.insert( p );
        ret.push_back( p );
        for( const point &d : neighborhood ) {
            point p2 = p + d;
            if( p2.x < 0 || p2.y < 0 || p2.x >= mgsize.x || p2.y >= mgsize.y ) {
                continue;
            }
            if( closed.count( p2 ) != 0 ) {
                continue;
            }
            closed.insert( p2 );
            if( func( file.base.get_uuid_at( p2 ) ) ) {
                open.insert( p2 );
            }
        }
    }

    return ret;
}

static void apply_bucket_tool( me_file &file, const uuid_t &brush, const point_abs_etile &tile_pos,
                               bool global )
{
    const uuid_t tgt = file.base.get_uuid_at( tile_pos.raw() );
    const auto predicate = [ = ]( const uuid_t &t ) {
        return t == tgt;
    };
    std::vector<point> tiles;
    if( global ) {
        tiles = find_tiles_via_global( file, predicate );
    } else {
        tiles = find_tiles_via_floodfill( file, tile_pos.raw(), predicate );
    }
    for( const point &p : tiles ) {
        file.base.set_uuid_at( p, brush );
    }
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
    me_canvas_tools_state &tools = state.tools_state;
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
        if( state.file().uses_rows() ) {
            if( tools.tool == CanvasTool::Brush && ImGui::IsMouseDown( ImGuiMouseButton_Left ) ) {
                brush_stroke_active = true;
                tools.ongoing_tool_operation = true;
                tools.ongoing_brush_stroke = true;
                point_rel_etile mapgensize = state.file().mapgensize();
                if( tile_pos.x() >= 0 && tile_pos.y() >= 0 && tile_pos.x() < mapgensize.x() &&
                    tile_pos.y() < mapgensize.y() ) {
                    const uuid_t &uuid = state.file().base.get_uuid_at( tile_pos.raw() );
                    if( uuid != tools.brush ) {
                        state.file().base.set_uuid_at( tile_pos.raw(), tools.brush );
                        tools.brush_stroke_changed_data = true;
                    }
                }
            }
            if( ( tools.tool == CanvasTool::Bucket || tools.tool == CanvasTool::BucketGlobal ) &&
                ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
                point_rel_etile mapgensize = state.file().mapgensize();
                if( tile_pos.x() >= 0 && tile_pos.y() >= 0 && tile_pos.x() < mapgensize.x() &&
                    tile_pos.y() < mapgensize.y() ) {
                    const uuid_t &uuid = state.file().base.get_uuid_at( tile_pos.raw() );
                    if( uuid != tools.brush ) {
                        apply_bucket_tool( state.file(), tools.brush, tile_pos, tools.tool == CanvasTool::BucketGlobal );
                        state.mark_changed();
                    }
                }
            }
            if( ImGui::IsMouseClicked( ImGuiMouseButton_Middle ) ) {
                point_rel_etile mapgensize = state.file().mapgensize();
                if( tile_pos.x() >= 0 && tile_pos.y() >= 0 && tile_pos.x() < mapgensize.x() &&
                    tile_pos.y() < mapgensize.y() ) {
                    const uuid_t &uuid = state.file().base.get_uuid_at( tile_pos.raw() );
                    tools.brush = uuid;
                } else {
                    tools.brush = UUID_INVALID;
                }
            }
        }
    }

    if( state.file().uses_rows() ) {
        if( tools.tool == CanvasTool::Brush && tools.ongoing_brush_stroke && !brush_stroke_active ) {
            // Brush stroke ended, queue changes as a single operation
            if( tools.brush_stroke_changed_data ) {
                state.mark_changed();
            }
            tools.ongoing_brush_stroke = false;
            tools.ongoing_tool_operation = false;
            tools.brush_stroke_changed_data = false;
        }

        for( int x = 0; x < state.file().mapgensize().x(); x++ ) {
            for( int y = 0; y < state.file().mapgensize().y(); y++ ) {
                point_abs_etile p( x, y );
                ImVec4 col = state.file().base.get_color_at( p.raw() );
                const SpriteRef *img = state.file().base.get_sprite_at( p.raw() );
                if( img ) {
                    col.w *= 0.6f;
                    fill_tile_sprited( draw_list, state.camera, p, *img );
                }
                fill_tile( draw_list, state.camera, p, col );
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
    }

    ImGui::End();
}

} // namespace editor

#include "editor_me_canvas.h"
#include "editor_me_color.h"

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
        if( state.file().uses_rows() ) {
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
    }

    if( state.file().uses_rows() ) {
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
    }

    ImGui::End();
}

} // namespace editor

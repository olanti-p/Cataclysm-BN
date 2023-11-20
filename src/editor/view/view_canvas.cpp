#include "view_canvas.h"

#include "camera.h"
#include "common/canvas_2d.h"
#include "common/color.h"
#include "common/math.h"
#include "common/sprite_ref.h"
#include "common/uuid.h"
#include "coordinates.h"
#include "drawing.h"
#include "imgui.h"
#include "mapgen/mapgen.h"
#include "mapgen/palette.h"
#include "mouse.h"
#include "project/project.h"
#include "state/control_state.h"
#include "state/state.h"
#include "state/tools_state.h"
#include "state/ui_state.h"
#include "tool/tool.h"
#include "view/ruler.h"
#include "widget/widgets.h"

#include <set>
#include <functional>

namespace editor
{

static void handle_view_change_hotkey( State &state )
{
    ImGuiIO &io = ImGui::GetIO();
    if( ImGui::IsKeyDown( ImGuiKey_ModAlt ) && std::abs( io.MouseWheel ) > 0.5f ) {
        int delta_wheel = static_cast<int>( std::round( io.MouseWheel ) );
        state.control->want_change_view = -delta_wheel;
    }
}

void show_editor_view( State &state, Mapgen *mapgen_ptr )
{
    ImVec2 disp_size = ImGui::GetIO().DisplaySize;

    ImGui::SetNextWindowPos( ImVec2( 0, 0 ) );
    ImGui::SetNextWindowSize( disp_size );
    ImGui::Begin( "<editor_view>", nullptr,
                  ImGuiWindowFlags_NoNav |
                  ImGuiWindowFlags_NoDecoration |
                  ImGuiWindowFlags_NoFocusOnAppearing |
                  ImGuiWindowFlags_NoBackground |
                  ImGuiWindowFlags_NoBringToFrontOnFocus |
                  ImGuiWindowFlags_NoScrollWithMouse
                );

    if( !mapgen_ptr ) {
        ImGui::BeginDisabled();
        ImGui::TextCenteredVH( "No active mapgen" );
        ImGui::EndDisabled();

        if( ImGui::IsWindowHovered() ) {
            handle_view_change_hotkey( state );
        }

        ImGui::End();
        return;
    }

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    Camera &cam = *state.ui->camera;
    editor::Mapgen &mapgen = *mapgen_ptr;
    ImGui::PushID( mapgen.uuid );

    highlight_region(
        draw_list,
        cam,
        point_abs_etile( 0, 0 ),
        point_abs_etile( -1, -1 ) + mapgen.mapgensize(),
        col_mapgensize_bg,
        col_mapgensize_border
    );

    ImGuiIO &io = ImGui::GetIO();
    bool view_hovered = ImGui::IsWindowHovered();
    ToolsState &tools = *state.ui->tools;

    const Palette &pal = *state.project().get_palette( mapgen.base.palette );
    if( tools.get_main_tile() != UUID_INVALID && !pal.find_entry( tools.get_main_tile() ) ) {
        tools.set_main_tile( UUID_INVALID );
    }

    point_abs_etile tile_pos = get_mouse_tile_pos( cam );
    point_rel_etile mapgensize = mapgen.mapgensize();
    bool is_mouse_in_bounds = tile_pos.x() >= 0 && tile_pos.y() >= 0 && tile_pos.x() < mapgensize.x() &&
                              tile_pos.y() < mapgensize.y();
    tools::ToolSettings *settings = &state.ui->tools->get_settings( tools.get_tool() );
    tools::ToolHighlight tool_highlight;

    tools::ToolTarget target {
        view_hovered,
        mapgen.uses_rows(),
        false,
        false,
        tile_pos,
        get_mouse_view_pos( cam ),
        mapgen,
        settings,
        tools.get_main_tile(),
        tool_highlight,
    };
    tools::ToolControl &tool_control = state.control->get_tool_control( tools.get_tool() );
    tool_control.handle_tool_operation( target );
    if( target.made_changes ) {
        state.mark_changed();
    }

    bool show_tooltip = false;
    const PaletteEntry *tooltip_entry = nullptr;
    point_abs_etile tooltip_pos;
    if( view_hovered ) {
        handle_view_change_hotkey( state );

        if( ImGui::IsKeyDown( ImGuiKey_ModCtrl ) ) {
            show_tooltip = true;
            tooltip_pos = tile_pos;
            if( is_mouse_in_bounds ) {
                const UUID &uuid = mapgen.base.canvas.get( tile_pos.raw() );
                tooltip_entry = state.project().get_palette(
                                    mapgen.base.palette )->find_entry( uuid );
            }
        }

        if( ImGui::IsMouseDragging( ImGuiMouseButton_Right ) ) {
            point_rel_screen drag_delta( ImGui::GetMouseDragDelta( ImGuiMouseButton_Right ) );
            cam.drag_delta = -cam.screen_to_world( drag_delta );
        } else {
            cam.pos += cam.drag_delta;
            cam.drag_delta = point_rel_epos();
        }
        if( !ImGui::IsKeyDown( ImGuiKey_ModAlt ) && std::abs( io.MouseWheel ) > 0.5f ) {
            int zoom_speed;
            if( cam.scale >= 64 ) {
                zoom_speed = 16;
            } else if( cam.scale >= 32 ) {
                zoom_speed = 8;
            } else if( cam.scale >= 16 ) {
                zoom_speed = 4;
            } else {
                zoom_speed = 2;
            }
            int delta_wheel = static_cast<int>( std::round( io.MouseWheel ) );
            int delta = delta_wheel * zoom_speed;
            cam.scale = clamp( cam.scale + delta, MIN_SCALE, MAX_SCALE );
        }
        if( mapgen.uses_rows() ) {
            if( ImGui::IsMouseClicked( ImGuiMouseButton_Middle ) ) {
                if( is_mouse_in_bounds ) {
                    const UUID &uuid = mapgen.base.canvas.get( tile_pos.raw() );
                    tools.set_main_tile( uuid );
                } else {
                    tools.set_main_tile( UUID_INVALID );
                }
            }
        }
    }

    if( mapgen.uses_rows() ) {
        for( int x = 0; x < mapgen.mapgensize().x(); x++ ) {
            for( int y = 0; y < mapgen.mapgensize().y(); y++ ) {
                point_abs_etile p( x, y );
                UUID uuid = mapgen.base.canvas.get( p.raw() );
                const SpriteRef *img = pal.sprite_from_uuid( uuid );
                if( img ) {
                    fill_tile_sprited( draw_list, cam, p, *img );
                }
            }
        }

        for( int x = 0; x < mapgen.mapgensize().x(); x++ ) {
            for( int y = 0; y < mapgen.mapgensize().y(); y++ ) {
                point_abs_etile p( x, y );
                UUID uuid = mapgen.base.canvas.get( p.raw() );
                ImVec4 col = pal.color_from_uuid( uuid );
                const SpriteRef *img = pal.sprite_from_uuid( uuid );
                if( img ) {
                    col.w *= 0.6f;
                }
                fill_tile( draw_list, cam, p, col );
            }
        }

        for( int x = 0; x < mapgen.mapgensize().x(); x++ ) {
            for( int y = 0; y < mapgen.mapgensize().y(); y++ ) {
                point_abs_etile p( x, y );
                const map_key &mk = pal.key_from_uuid( mapgen.base.canvas.get( p.raw() ) );
                point_abs_epos center = coords::project_combine( p,
                                        point_etile_epos( ETILE_SIZE / 2, ETILE_SIZE / 2 ) );
                point_abs_screen text_center = cam.world_to_screen( center );
                point_rel_screen text_size( ImGui::CalcTextSize( mk.str.c_str() ) );
                point_abs_screen text_pos = text_center - text_size.raw() / 2;
                ImGui::SetCursorPos( text_pos.raw() );
                ImGui::Text( "%s", mk.str.c_str() );
            }
        }
    }

    for( const MapObject &obj : mapgen.objects ) {
        if( !obj.visible ) {
            continue;
        }

        point_abs_etile p1( obj.x.min, obj.y.min );
        point_abs_etile p2( obj.x.max, obj.y.max );
        ImVec4 col_border = obj.color;
        ImVec4 col_text = obj.color;
        col_text.w = 1.0f;
        ImVec4 col_bg = obj.color;
        col_bg.w *= 0.4f;
        highlight_region( draw_list, cam, p1, p2, col_bg, col_border );

        std::string label = obj.piece->fmt_summary();
        point_abs_epos pos1 = coords::project_combine( p1, point_etile_epos( ETILE_SIZE / 2,
                              ETILE_SIZE / 2 ) );
        point_abs_epos pos2 = coords::project_combine( p2, point_etile_epos( ETILE_SIZE / 2,
                              ETILE_SIZE / 2 ) );
        point_abs_epos center( ( pos1.raw() + pos2.raw() ) / 2 );
        point_abs_screen text_center = cam.world_to_screen( center );
        point_rel_screen text_size( ImGui::CalcTextSize( label.c_str() ) );
        point_abs_screen text_pos = text_center - text_size.raw() / 2;
        ImGui::SetCursorPos( text_pos.raw() );
        ImGui::TextColored( col_text, "%s", label.c_str() );
    }

    if( view_hovered ) {
        point_abs_etile tile_pos = get_mouse_tile_pos( cam );
        highlight_tile( draw_list, cam, tile_pos, col_cursor );
    }

    if( target.highlight.active() ) {
        for( const point_abs_etile &p : target.highlight.tiles ) {
            ImVec4 col_bg = col_tool;
            col_bg.w *= 0.4f;
            fill_tile( draw_list, cam, p, col_bg );
            highlight_tile( draw_list, cam, p, col_tool );
        }
        for( const auto &p : target.highlight.areas ) {
            point_abs_etile p1( std::min( p.first.x(), p.second.x() ), std::min( p.first.y(), p.second.y() ) );
            point_abs_etile p2( std::max( p.first.x(), p.second.x() ), std::max( p.first.y(), p.second.y() ) );
            ImVec4 col_bg = col_tool;
            col_bg.w *= 0.4f;
            highlight_region( draw_list, cam, p1, p2, col_bg, col_tool );
        }
    }

    bool show_ruler = false;
    std::optional<point_abs_etile> &ruler = state.control->ruler.start;
    if( view_hovered && ImGui::IsKeyDown( ImGuiKey_ModAlt ) ) {
        if( !ruler ) {
            ruler = tile_pos;
        }
        if( *ruler != tile_pos ) {
            show_ruler = true;
        }
    } else {
        ruler.reset();
    }

    bool tooltip_needs_separator = false;

    if( show_ruler ) {
        ImVec4 col_border = col_ruler;
        ImVec4 col_bg = col_ruler;
        col_bg.w *= 0.4f;
        assert( ruler );
        point_abs_etile p1( std::min( tile_pos.x(), ruler->x() ), std::min( tile_pos.y(), ruler->y() ) );
        point_abs_etile p2( std::max( tile_pos.x(), ruler->x() ), std::max( tile_pos.y(), ruler->y() ) );

        highlight_region( draw_list, cam, p1, p2, col_bg, col_border );

        ImGui::BeginTooltip();
        if( tooltip_needs_separator ) {
            ImGui::SeparatorText( "Ruler" );
        }
        point delta = ( *ruler - tile_pos ).raw().abs();
        if( delta.x != 0 ) {
            ImGui::Text( "X %d", delta.x + 1 );
        }
        if( delta.y != 0 ) {
            ImGui::Text( "Y %d", delta.y + 1 );
        }
        ImGui::EndTooltip();
        tooltip_needs_separator = true;
    }

    if( target.want_tooltip ) {
        ImGui::BeginTooltip();
        if( tooltip_needs_separator ) {
            std::string label = tools::get_tool_definition( tools.get_tool() ).get_tool_display_name();
            ImGui::SeparatorText( label.c_str() );
        }
        tool_control.show_tooltip( target );
        ImGui::EndTooltip();
        tooltip_needs_separator = true;
    }

    if( show_tooltip ) {
        std::vector<const MapObject *> objects;
        for( const MapObject &obj : mapgen.objects ) {
            if( obj.x.max < tooltip_pos.x() ||
                obj.y.max < tooltip_pos.y() ||
                obj.x.min > tooltip_pos.x() ||
                obj.y.min > tooltip_pos.y()
              ) {
                continue;
            }
            objects.push_back( &obj );
        }

        if( tooltip_entry || !objects.empty() ) {
            ImGui::BeginTooltip();
            if( tooltip_needs_separator ) {
                ImGui::SeparatorText( "Info" );
            }
            if( tooltip_entry ) {
                const PaletteEntry &e = *tooltip_entry;
                for( const auto &it : e.mapping.pieces ) {
                    ImGui::TextDisabled( "MAP" );
                    ImGui::SameLine();
                    ImGui::Text( "%s", it->fmt_summary().c_str() );
                }
            }
            for( const MapObject *obj : objects ) {
                ImGui::TextDisabled( "OBJ" );
                ImGui::SameLine();
                ImGui::Text( "%s", obj->piece->fmt_summary().c_str() );
            }
            ImGui::EndTooltip();
            tooltip_needs_separator = true;
        }
    }

    ImGui::PopID();
    ImGui::End();
}

void handle_view_change( State &state )
{
    const std::vector<Mapgen> &mapgens = state.project().mapgens;
    if( mapgens.empty() ) {
        return;
    }
    if( state.control->want_change_view != 0 ) {
        if( !state.ui->active_mapgen_id ) {
            state.ui->active_mapgen_id = mapgens[0].uuid;
        } else {
            int cur_idx = 0;
            for( int i = 0; i < static_cast<int>( mapgens.size() ); i++ ) {
                if( mapgens[i].uuid == state.ui->active_mapgen_id ) {
                    cur_idx = i;
                    break;
                }
            }
            int new_idx = cur_idx + state.control->want_change_view;
            new_idx = wrap_index( new_idx, state.project().mapgens.size() );
            state.ui->active_mapgen_id = mapgens[new_idx].uuid;
        }
        state.control->want_change_view = 0;
    }
}

} // namespace editor

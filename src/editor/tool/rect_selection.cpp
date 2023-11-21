#include "rect_selection.h"

#include "common/algo.h"
#include "coordinates.h"
#include "imgui.h"
#include "line.h"
#include "mapgen/mapgen.h"
#include "point.h"
#include <vector>

namespace editor::tools
{

std::string RectSelection::get_tool_display_name() const
{
    return "Select (Rectangle)";
}

std::string RectSelection::get_tool_hint() const
{
    return "Drag LMB to select in a rectangular shape.\n\n"
           "Hold Shift to select in a square shape.\n"
           "Press Ctrl+A to select everything.\n"
           "Press Esc to dismiss selection.";
}

void RectSelectionSettings::show()
{
    ImGui::Checkbox( "Filled", &filled );
}

void RectSelectionControl::handle_tool_operation( ToolTarget &target )
{
    if( !target.has_canvas || !target.selection ) {
        start.reset();
        return;
    }
    RectSelectionSettings &settings = *dynamic_cast<RectSelectionSettings *>( target.settings );
    if( target.view_hovered ) {
        if( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
            // Stroke start
            start = target.cursor_tile_pos;
        }
        if( ImGui::IsMouseReleased( ImGuiMouseButton_Left ) && start ) {
            // Stroke end
            point_abs_etile p1 = *start;
            point_abs_etile p2 = get_rectangle_end( target );

            std::vector<point> rect = make_rectangle( p1, p2, settings.filled );
            apply( *target.selection, rect );
            start.reset();
        }
        if( ImGui::IsKeyDown( ImGuiKey_ModCtrl ) && ImGui::IsKeyPressed( ImGuiKey_A ) ) {
            // Abort & select all
            start.reset();
            target.selection->set_all();
        }
        if( ImGui::IsKeyPressed( ImGuiKey_Escape ) ) {
            // Abort & clear selection
            start.reset();
            target.selection->clear_all();
        }
        if( start ) {
            target.want_tooltip = true;
            point_abs_etile p1 = *start;
            point_abs_etile p2 = get_rectangle_end( target );
            std::vector<point> rect = make_rectangle( p1, p2, settings.filled );
            for( const point &p : rect ) {
                target.highlight.tiles.emplace_back( p );
            }
        }
    }
    if( !ImGui::IsMouseDown( ImGuiMouseButton_Left ) ) {
        start.reset();
    }
}

std::vector<point> RectSelectionControl::make_rectangle( point_abs_etile p1, point_abs_etile p2,
        bool filled ) const
{
    // TODO: deduplicate with Rectangle tool
    auto corners = editor::normalize_rect( p1, p2 );
    std::vector<point> ret;
    if( filled ) {
        for( int y = corners.first.y(); y <= corners.second.y(); y++ ) {
            for( int x = corners.first.x(); x <= corners.second.x(); x++ ) {
                ret.emplace_back( x, y );
            }
        }
    } else {
        int x1 = corners.first.x();
        int x2 = corners.second.x();
        int y1 = corners.first.y();
        int y2 = corners.second.y();

        for( int x = x1; x <= x2; x++ ) {
            ret.emplace_back( x, y1 );
            if( y1 != y2 ) {
                ret.emplace_back( x, y2 );
            }
        }
        for( int y = y1 + 1; y <= y2 - 1; y++ ) {
            ret.emplace_back( x1, y );
            if( x1 != x2 ) {
                ret.emplace_back( x2, y );
            }
        }
    }
    return ret;
}

void RectSelectionControl::apply( SelectionMask &selection, const std::vector<point> &rect )
{
    for( const auto &p : rect ) {
        if( selection.data.get_bounds().contains( p ) ) {
            selection.data.set( p, true );
        }
    }
}

point_abs_etile RectSelectionControl::get_rectangle_end( ToolTarget &target ) const
{
    // TODO: deduplicate with Rectangle tool
    if( start && ImGui::IsKeyDown( ImGuiKey_ModShift ) ) {
        point delta = target.cursor_tile_pos.raw() - start->raw();
        point delta_abs = delta.abs();
        int dist = std::min( delta_abs.x, delta_abs.y );
        point vec;
        if( delta.x > 0 ) {
            vec.x = 1;
        } else if( delta.x < 0 ) {
            vec.x = -1;
        }
        if( delta.y > 0 ) {
            vec.y = 1;
        } else if( delta.y < 0 ) {
            vec.y = -1;
        }
        return *start + ( vec * dist );
    } else {
        return target.cursor_tile_pos;
    }
}

void RectSelectionControl::show_tooltip( ToolTarget &target )
{
    // TODO: deduplicate with Rectangle tool
    if( !start ) {
        ImGui::Text( "<ERROR:null rectangle start>" );
        return;
    }
    point delta = ( *start - get_rectangle_end( target ) ).raw().abs();
    ImGui::Text( "X %d", delta.x + 1 );
    ImGui::Text( "Y %d", delta.y + 1 );
}

} // namespace editor::tools

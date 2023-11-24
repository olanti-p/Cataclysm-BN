#include "selection.h"

#include "common/algo.h"
#include "coordinates.h"
#include "imgui.h"
#include "line.h"
#include "mapgen/mapgen.h"
#include "point.h"
#include <vector>

namespace editor::tools
{

std::string Selection::get_tool_display_name() const
{
    return "Select";
}

std::string Selection::get_tool_hint() const
{
    return "Drag LMB to select in a rectangular shape.\n\n"
           "Hold Shift to add to existing selection.\n"
           "Press Ctrl+A to select everything.\n"
           "Press Esc while dragging to cancel selection.\n"
           "Press Esc or click without dragging to dismiss selection.";
}

void SelectionSettings::show()
{
    // TODO: selection modes
}

void SelectionControl::handle_tool_operation( ToolTarget &target )
{
    if( !target.has_canvas || !target.selection ) {
        start.reset();
        return;
    }
    //RectSelectionSettings &settings = *dynamic_cast<RectSelectionSettings *>( target.settings );
    if( target.view_hovered ) {
        if( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
            dismissing_selection = true;
            drag_start = target.cursor_tile_pos;
        }
        if( ImGui::IsMouseDragging( ImGuiMouseButton_Left ) && !start && !selection_aborted ) {
            // Stroke start
            start = drag_start;
            dismissing_selection = false;
        }
        if( ImGui::IsMouseReleased( ImGuiMouseButton_Left ) ) {
            if( start ) {
                // Stroke end
                point_abs_etile p1 = *start;
                point_abs_etile p2 = get_rectangle_end( target );

                std::vector<point> rect = make_rectangle( p1, p2 );
                if( !ImGui::IsKeyDown( ImGuiKey_ModShift ) ) {
                    target.selection->clear_all();
                }
                apply( *target.selection, rect );
                target.made_changes = true;
                start.reset();
            } else if( dismissing_selection ) {
                dismissing_selection = false;
                if( !ImGui::IsKeyDown( ImGuiKey_ModShift ) ) {
                    target.selection->clear_all();
                    target.made_changes = true;
                }
            }
        }
        if( ImGui::IsKeyDown( ImGuiKey_ModCtrl ) && ImGui::IsKeyPressed( ImGuiKey_A ) ) {
            // Abort & select all
            start.reset();
            selection_aborted = true;
            target.selection->set_all();
            target.made_changes = true;
        }
        if( ImGui::IsKeyPressed( ImGuiKey_Escape ) ) {
            // Abort / clear selection
            if( start ) {
                start.reset();
                selection_aborted = true;
            } else {
                target.selection->clear_all();
                target.made_changes = true;
            }
        }
        if( !ImGui::IsMouseDown( ImGuiMouseButton_Left ) ) {
            selection_aborted = false;
        }
        if( start ) {
            target.want_tooltip = true;
            point_abs_etile p1 = *start;
            point_abs_etile p2 = get_rectangle_end( target );
            std::vector<point> rect = make_rectangle( p1, p2 );
            for( const point &p : rect ) {
                target.highlight.tiles.emplace_back( p );
            }
        }
    }
    if( !ImGui::IsMouseDown( ImGuiMouseButton_Left ) ) {
        start.reset();
        drag_start.reset();
    }
}

std::vector<point> SelectionControl::make_rectangle( point_abs_etile p1,
        point_abs_etile p2 ) const
{
    // TODO: deduplicate with Rectangle tool
    auto corners = editor::normalize_rect( p1, p2 );
    std::vector<point> ret;
    for( int y = corners.first.y(); y <= corners.second.y(); y++ ) {
        for( int x = corners.first.x(); x <= corners.second.x(); x++ ) {
            ret.emplace_back( x, y );
        }
    }
    return ret;
}

void SelectionControl::apply( SelectionMask &selection, const std::vector<point> &rect )
{
    for( const auto &p : rect ) {
        if( selection.data.get_bounds().contains( p ) ) {
            selection.data.set( p, Bool( true ) );
        }
    }
}

point_abs_etile SelectionControl::get_rectangle_end( ToolTarget &target ) const
{
    return target.cursor_tile_pos;
}

void SelectionControl::show_tooltip( ToolTarget &target )
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

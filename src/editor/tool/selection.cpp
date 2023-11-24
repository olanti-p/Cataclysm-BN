#include "selection.h"

#include "common/algo.h"
#include "coordinates.h"
#include "imgui.h"
#include "line.h"
#include "mapgen/canvas_snippet.h"
#include "mapgen/mapgen.h"
#include "state/control_state.h"
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
    CanvasSnippet *snippet = target.snippets.get_snippet( target.mapgen.uuid );

    const auto apply_snippet = [&]() {
        if( snippet ) {
            snippet = nullptr;
            CanvasSnippet data = target.snippets.drop_snippet( target.mapgen.uuid );
            target.mapgen.apply_snippet( data );
            target.mapgen.select_from_snippet( data );
            target.made_changes = true;
        }
    };
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
                apply_snippet();
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
                apply_snippet();
                if( !ImGui::IsKeyDown( ImGuiKey_ModShift ) ) {
                    if( target.selection->has_selected() ) {
                        target.selection->clear_all();
                        target.made_changes = true;
                    }
                }
            }
        }
        if( ImGui::IsKeyPressed( ImGuiKey_Delete ) && target.selection->has_selected() ) {
            // Abort & delete
            start.reset();
            selection_aborted = true;

            target.mapgen.erase_selected( *target.selection );
            target.made_changes = true;
        }
        if( snippet ) {
            if( ImGui::IsKeyPressed( ImGuiKey_UpArrow ) ||
                ImGui::IsKeyPressed( ImGuiKey_DownArrow ) ||
                ImGui::IsKeyPressed( ImGuiKey_LeftArrow ) ||
                ImGui::IsKeyPressed( ImGuiKey_RightArrow )
              ) {
                // Abort & move snippet
                start.reset();
                selection_aborted = true;

                point delta;
                if( ImGui::IsKeyPressed( ImGuiKey_UpArrow ) ) {
                    delta = point_north;
                } else if( ImGui::IsKeyPressed( ImGuiKey_DownArrow ) ) {
                    delta = point_south;
                } else if( ImGui::IsKeyPressed( ImGuiKey_LeftArrow ) ) {
                    delta = point_west;
                } else {
                    delta = point_east;
                }

                snippet->set_pos( snippet->get_pos() + delta );
            }
            if( ImGui::IsKeyPressed( ImGuiKey_Enter ) ) {
                // Abort & apply snippet
                start.reset();
                selection_aborted = true;

                apply_snippet();
                target.mapgen.get_selection_mask()->clear_all();
                target.made_changes = true;
            }
        }
        if( ImGui::IsKeyDown( ImGuiKey_ModCtrl ) ) {
            if( ImGui::IsKeyPressed( ImGuiKey_A ) ) {
                // Abort & select all
                apply_snippet();
                start.reset();
                selection_aborted = true;

                target.selection->set_all();
                target.made_changes = true;
            }
            if( ImGui::IsKeyPressed( ImGuiKey_X ) && target.selection->has_selected() ) {
                // Abort & cut
                apply_snippet();
                start.reset();
                selection_aborted = true;

                CanvasSnippet new_snippet = make_snippet( target.mapgen.base.canvas, *target.selection );
                target.snippets.clipboard = std::move( new_snippet );

                target.mapgen.erase_selected( *target.selection );
                target.made_changes = true;
            }
            if( ImGui::IsKeyPressed( ImGuiKey_C ) && target.selection->has_selected() ) {
                // Abort & copy
                apply_snippet();
                start.reset();
                selection_aborted = true;

                CanvasSnippet new_snippet = make_snippet( target.mapgen.base.canvas, *target.selection );
                target.snippets.clipboard = std::move( new_snippet );
            }
            if( ImGui::IsKeyPressed( ImGuiKey_V ) && target.snippets.clipboard.has_value() ) {
                // Abort & paste
                apply_snippet();
                start.reset();
                selection_aborted = true;

                if( target.selection->has_selected() ) {
                    target.selection->clear_all();
                    target.made_changes = true;
                }

                // TODO: paste pos
                CanvasSnippet paste_data = *target.snippets.clipboard;
                target.snippets.add_snippet( target.mapgen.uuid, std::move( paste_data ) );
            }
        }
        if( ImGui::IsKeyPressed( ImGuiKey_Escape ) ) {
            // Abort / clear selection
            if( start ) {
                start.reset();
                selection_aborted = true;
            } else if( snippet ) {
                snippet = nullptr;
                target.snippets.drop_snippet( target.mapgen.uuid );
            } else if( target.selection->has_selected() ) {
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
        if( selection.get_bounds().contains( p ) ) {
            selection.set( p );
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

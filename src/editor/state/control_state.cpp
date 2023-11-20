#include "control_state.h"

#include "common/canvas_2d.h"
#include "tool/cursor.h"
#include "mapgen/mapgen.h"

#include <memory>

namespace editor
{
ControlState::ControlState() = default;
ControlState::ControlState( ControlState && ) = default;
ControlState::~ControlState() = default;
ControlState &ControlState::operator=( ControlState && ) = default;

bool ControlState::has_ongoing_tool_operation()
{
    if( !tool_control ) {
        return false;
    }
    return tool_control->operation_in_progress();
}

tools::ToolControl &ControlState::get_tool_control( tools::ToolKind t )
{
    set_tool_control( t );
    return *tool_control;
}

void ControlState::set_tool_control( tools::ToolKind t )
{
    if( t != tool_control_kind || !tool_control ) {
        tool_control_kind = t;
        tool_control = tools::get_tool_definition( t ).make_control();
    }
}

static SelectionMask make_selection_mask( const Mapgen &mapgen )
{
    return SelectionMask { Canvas2D<Bool>( mapgen.mapgensize().raw(), false ) };
}

SelectionMask *ControlState::get_canvas_selection_mask( const Mapgen &mapgen )
{
    if( !mapgen.uses_rows() ) {
        // Clean up any mapgens that could've lost their canvas
        canvas_selection_states.erase( mapgen.uuid );
        return nullptr;
    }
    if( canvas_selection_states.find( mapgen.uuid ) == canvas_selection_states.end() ) {
        canvas_selection_states[mapgen.uuid] = make_selection_mask( mapgen );
    }
    SelectionMask &mask = canvas_selection_states[mapgen.uuid];
    if( mask.data.get_size() != mapgen.mapgensize().raw() ) {
        mask = make_selection_mask( mapgen );
    }
    return &mask;
}

} // namespace editor

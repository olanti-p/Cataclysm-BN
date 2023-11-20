#include "control_state.h"

#include "tool/cursor.h"

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

} // namespace editor

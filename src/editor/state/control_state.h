#ifndef CATA_SRC_EDITOR_CONTROL_STATE_H
#define CATA_SRC_EDITOR_CONTROL_STATE_H

#include "tool/tool.h"

#include <memory>

namespace editor
{
/**
 * Editor control state.
 *
 * This handles control flow and window transitions.
 */
struct ControlState {
    public:
        ControlState();
        ControlState( const ControlState & ) = delete;
        ControlState( ControlState && );
        ~ControlState();

        ControlState &operator=( const ControlState & ) = delete;
        ControlState &operator=( ControlState && );

        bool is_editor_running = true;
        bool want_close = false;                // User wants to close the project
        bool want_save = false;                 // User wants to save
        bool want_save_as = false;              // User wants to save as
        bool want_exit_after_save = false;      // User wants to exit after save
        bool want_export = false;               // User wants to export
        bool want_export_as = false;            // User wants to export as
        int want_change_view = 0;               // User wants to change view

        bool has_ongoing_tool_operation();
        tools::ToolControl &get_tool_control( tools::ToolKind t );

    private:
        void set_tool_control( tools::ToolKind t );
        std::unique_ptr<tools::ToolControl> tool_control;
        tools::ToolKind tool_control_kind = tools::ToolKind::Cursor;
};

} // namespace editor

#endif // CATA_SRC_EDITOR_CONTROL_STATE_H

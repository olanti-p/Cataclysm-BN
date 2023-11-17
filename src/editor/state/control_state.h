#ifndef CATA_SRC_EDITOR_CONTROL_STATE_H
#define CATA_SRC_EDITOR_CONTROL_STATE_H

namespace editor
{
/**
 * Editor control state.
 *
 * This handles control flow and window transitions.
 */
struct ControlState {
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
};

} // namespace editor

#endif // CATA_SRC_EDITOR_CONTROL_STATE_H

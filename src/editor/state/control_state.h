#ifndef CATA_SRC_EDITOR_CONTROL_STATE_H
#define CATA_SRC_EDITOR_CONTROL_STATE_H

#include "common/uuid.h"
#include "state/selection_mask.h"
#include "tool/tool.h"
#include "view/ruler.h"
#include "widget/editable_id.h"

#include <memory>
#include <unordered_map>

namespace editor
{
enum class QuickAddMode {
    Ter,
    Furn,
    Both,
};

struct QuickPaletteAddState {
    UUID palette = UUID_INVALID;
    bool active = false;
    QuickAddMode mode = QuickAddMode::Ter;
    EID::Ter eid_ter;
    EID::Furn eid_furn;
};

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

        RulerState ruler;

        bool has_ongoing_tool_operation();
        tools::ToolControl &get_tool_control( tools::ToolKind t );
        SelectionMask *get_canvas_selection_mask( const Mapgen &mapgen );

        QuickPaletteAddState quick_add_state;

    private:
        void set_tool_control( tools::ToolKind t );
        std::unique_ptr<tools::ToolControl> tool_control;
        tools::ToolKind tool_control_kind = tools::ToolKind::Cursor;
        std::unordered_map<UUID, SelectionMask> canvas_selection_states;
};

} // namespace editor

#endif // CATA_SRC_EDITOR_CONTROL_STATE_H

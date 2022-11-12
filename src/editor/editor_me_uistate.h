#ifndef CATA_SRC_EDITOR_EDITOR_ME_UISTATE_H
#define CATA_SRC_EDITOR_EDITOR_ME_UISTATE_H

#include "../optional.h"

#include "editor_me_uuid.h"

namespace editor
{
struct me_state;

struct me_uistate {

    bool do_loop = true; // Setting this to false will quit the editor
    bool show_demo_wnd = false; // Whether to show ImGui Demo window
    bool show_asset_lib = false; // Whether to show asset library
    bool show_file_info = true; // Whether to show file info
    bool show_base_inline_palette = false; // Whether to show base mapgen's palette
    bool show_file_history = true; // Whether to show undo/redo history
    bool show_toolbar = true; // Whether to show canvas toolbar
    cata::optional<uuid_t> view_mappings; // Whether to show mappings for given palette entry
};

/**
 * =============== Windows ===============
 */
void show_ui_control_window( me_state &state );
void run_ui_for_state( me_state &state );

} // namespace editor

#endif // CATA_SRC_EDITOR_EDITOR_ME_UISTATE_H

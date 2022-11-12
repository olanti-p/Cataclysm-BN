#ifndef CATA_SRC_EDITOR_EDITOR_ME_STATE_H
#define CATA_SRC_EDITOR_EDITOR_ME_STATE_H

#include "../options.h"
#include "../coordinates.h"
#include "../type_id.h"
#include "../mapgen.h"

#include "editor_me_assetlib.h"
#include "editor_me_camera.h"
#include "editor_me_canvas_tool.h"
#include "editor_me_color.h"
#include "editor_me_editable_id.h"
#include "editor_me_file.h"
#include "editor_me_history.h"
#include "editor_me_save_export.h"
#include "editor_me_int_range.h"
#include "editor_me_map_key_gen.h"
#include "editor_me_palette.h"
#include "editor_me_piece.h"
#include "editor_me_uuid.h"
#include "editor_sprite_ref.h"
#include "imgui.h"

struct ImDrawList;
struct ImVec4;
class JsonOut;
class JsonIn;
template<typename T> struct enum_traits;

namespace editor
{

struct me_state {
    me_state();
    explicit me_state( std::unique_ptr<me_file> &&file );
    me_state( std::unique_ptr<me_file> &&file, const std::string *loaded_from_path );
    me_state( const me_state & ) = delete;
    me_state( me_state && ) = default;
    ~me_state();

    me_state &operator=( const me_state & ) = delete;
    me_state &operator=( me_state && ) = default;

    me_camera camera;
    bool do_loop = true; // Setting this to false will quit the editor
    bool show_demo_wnd = false; // Whether to show ImGui Demo window
    bool show_asset_lib = false; // Whether to show asset library
    bool show_file_info = true; // Whether to show file info
    bool show_base_inline_palette = false; // Whether to show base mapgen's palette
    bool show_file_history = true; // Whether to show undo/redo history
    bool show_toolbar = true; // Whether to show canvas toolbar
    asset_library assets;

    me_canvas_tools_state tools_state;

    cata::optional<uuid_t> view_mappings; // Whether to show mappings for given palette entry

    me_save_export_state sestate;
    me_history_state histate;

    inline me_file &file() {
        return histate.file();
    }

    /**
     * Mark state as changed.
     *
     * @param id (optional) If edit operation repeatedly generates change events that should be
     *           collapsed into a single undo/redo operation, pass id of the operation here.
     *           Respects current ImGui id stack.
     */
    inline void mark_changed( const char *id = nullptr ) {
        histate.mark_changed( id );
    }
};

/**
 * =============== Windows ===============
 */
void show_control_window( me_state &state );

/**
 * ============= Entry point =============
 */
void show_me_ui( me_state &state );

} // namespace editor

#endif // CATA_SRC_EDITOR_EDITOR_ME_STATE_H

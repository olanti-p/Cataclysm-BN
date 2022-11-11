#ifndef CATA_SRC_EDITOR_EDITOR_ME_STATE_H
#define CATA_SRC_EDITOR_EDITOR_ME_STATE_H

#include "../options.h"
#include "../coordinates.h"
#include "../type_id.h"
#include "../mapgen.h"

#include "editor_assets.h"
#include "editor_me_camera.h"
#include "editor_me_canvas_tool.h"
#include "editor_me_color.h"
#include "editor_me_editable_id.h"
#include "editor_me_file.h"
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

struct me_file_revision {
    std::unique_ptr<me_file> file;
    int num = 0;

    me_file_revision() {
        file = std::make_unique<me_file>();
    }
    me_file_revision( const me_file_revision & ) = delete;
    me_file_revision( me_file_revision && ) = default;
    ~me_file_revision() {};

    me_file_revision &operator=( const me_file_revision & ) = delete;
    me_file_revision &operator=( me_file_revision && ) = default;

    me_file_revision make_copy() const {
        me_file_revision ret;
        ret.file = std::make_unique<me_file>( *file );
        ret.num = num;
        return ret;
    }
};

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

    bool open_save_as = false;
    bool do_save = false;
    bool do_exit_after_save = false;
    cata::optional<std::string> file_save_path;

    bool open_export_as = false;
    bool do_export = false;
    cata::optional<std::string> file_export_path;

    inline me_file &file() {
        return *current_revision.file;
    }

    /**
     * Mark state as changed.
     *
     * @param id (optional) If edit operation repeatedly generates change events that should be
     *           collapsed into a single undo/redo operation, pass id of the operation here.
     *           Respects current ImGui id stack.
     */
    void mark_changed( const char *id = nullptr );

    inline bool can_undo() const {
        return current_revision.num != file_history[file_history.size() - 1].num;
    }

    inline void queue_undo() {
        switch_to_revision = current_revision.num - 1;
    }

    inline bool can_redo() const {
        return current_revision.num != file_history[0].num;
    }

    inline void queue_redo() {
        switch_to_revision = current_revision.num + 1;
    }

    bool has_unsaved_changes() const;
    bool has_unexported_changes() const;

    bool file_has_changes = false;
    cata::optional<ImGuiID> current_widget_changed = 0;
    std::string current_widget_changed_str;
    cata::optional<ImGuiID> last_widget_changed = 0;
    cata::optional<int> switch_to_revision;
    me_file_revision current_revision;
    std::vector<me_file_revision> file_history;
    int history_capacity = 200;
    cata::optional<int> last_saved_revision;
    cata::optional<int> last_exported_revision;
    int edit_counter = 0;
};

/**
 * =============== Windows ===============
 */
void show_control_window( me_state &state );
void show_file_history( me_state &state, bool &show );
void show_asset_lib( asset_library &assets, bool &show );

/**
 * ============= Entry point =============
 */
void show_me_ui( me_state &state );

} // namespace editor

#endif // CATA_SRC_EDITOR_EDITOR_ME_STATE_H

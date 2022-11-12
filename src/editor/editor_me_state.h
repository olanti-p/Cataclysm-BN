#ifndef CATA_SRC_EDITOR_EDITOR_ME_STATE_H
#define CATA_SRC_EDITOR_EDITOR_ME_STATE_H

#include "../pimpl.h"

namespace editor
{
struct me_file;
struct me_camera;
struct me_uistate;
struct asset_library;
struct me_canvas_tools_state;
struct me_save_export_state;
struct me_history_state;

struct me_state {
    me_state();
    explicit me_state( std::unique_ptr<me_file> &&file );
    me_state( std::unique_ptr<me_file> &&file, const std::string *loaded_from_path );
    me_state( const me_state & ) = delete;
    me_state( me_state && );
    ~me_state();

    me_state &operator=( const me_state & ) = delete;
    me_state &operator=( me_state && );

    pimpl<me_camera> camera;
    pimpl<me_uistate> uistate;
    pimpl<asset_library> assets;
    pimpl<me_canvas_tools_state> tools_state;
    pimpl<me_save_export_state> sestate;
    pimpl<me_history_state> histate;

    me_file &file();

    /**
     * Mark state as changed.
     *
     * @param id (optional) If edit operation repeatedly generates change events that should be
     *           collapsed into a single undo/redo operation, pass id of the operation here.
     *           Respects current ImGui id stack.
     */
    void mark_changed( const char *id = nullptr );
};

/**
 * ============= Entry point =============
 */
void show_me_ui( me_state &state );

} // namespace editor

#endif // CATA_SRC_EDITOR_EDITOR_ME_STATE_H

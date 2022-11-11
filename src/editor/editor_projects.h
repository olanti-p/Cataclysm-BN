#ifndef CATA_SRC_EDITOR_EDITOR_PROJECTS_H
#define CATA_SRC_EDITOR_EDITOR_PROJECTS_H

#include "../options.h"

#include "imgui.h"

namespace editor
{

struct projects_ui_retval {
    bool exit = false;
    bool exit_to_desktop = false;
    bool legacy_editor = false;
    bool make_new = false;
    bool load_existing = false;
    std::string load_path;
};

struct me_projects_state {
    me_projects_state() = default;
    me_projects_state( const me_projects_state & ) = delete;
    me_projects_state( me_projects_state && ) = default;
    ~me_projects_state() = default;

    me_projects_state &operator=( const me_projects_state & ) = delete;
    me_projects_state &operator=( me_projects_state && ) = default;

    bool open_file_dialog = false;
    cata::optional<projects_ui_retval> ret;
    cata::optional<std::string> popup_prompt;
};

/**
 * =============== Windows ===============
 */
void show_projects_window( me_projects_state &state );

/**
 * ============= Entry point =============
 */
void show_projects_ui( me_projects_state &state );

} // namespace editor

#endif // CATA_SRC_EDITOR_EDITOR_PROJECTS_H

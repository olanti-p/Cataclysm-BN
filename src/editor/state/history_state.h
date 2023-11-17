#ifndef CATA_SRC_EDITOR_HISTORY_STATE_H
#define CATA_SRC_EDITOR_HISTORY_STATE_H

#include <memory>
#include <vector>
#include <string>
#include <optional>

#include "imgui.h"

namespace editor
{
struct Project;
struct ToolsState;

struct FileRevision {
    std::unique_ptr<Project> project;
    int num = 0;

    FileRevision();
    FileRevision( const FileRevision & ) = delete;
    FileRevision( FileRevision && );
    ~FileRevision();

    FileRevision &operator=( const FileRevision & ) = delete;
    FileRevision &operator=( FileRevision && );

    FileRevision make_copy() const;
};

struct HistoryState {
    HistoryState() = default;
    ~HistoryState() = default;
    HistoryState( std::unique_ptr<Project> &&project, bool was_loaded );

    HistoryState( const HistoryState & ) = delete;
    HistoryState( HistoryState && ) = default;
    HistoryState &operator=( const HistoryState & ) = delete;
    HistoryState &operator=( HistoryState && ) = default;

    inline Project &project() {
        return *current_revision.project;
    }

    /**
     * Mark project as changed.
     *
     * @param id (optional) If edit operation repeatedly generates change events that should be
     *           collapsed into a single undo/redo operation, pass id of the operation here.
     *           Respects current ImGui id stack.
     */
    void mark_changed( const char *id = nullptr );

    /**
     * Check whether project has been marked as changed.
     */
    inline bool is_changed() const {
        return file_has_changes;
    }

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
    std::optional<ImGuiID> current_widget_changed = 0;
    std::string current_widget_changed_str;
    std::optional<ImGuiID> last_widget_changed = 0;
    std::optional<int> switch_to_revision;
    FileRevision current_revision;
    std::vector<FileRevision> file_history;
    int history_capacity = 200;
    std::optional<int> last_saved_revision;
    std::optional<int> last_exported_revision;
    int edit_counter = 0;
};

/**
 * =============== Windows ===============
 */
void show_file_history( HistoryState &state, bool &show );

void handle_revision_change( HistoryState &state, ToolsState &tools );

} // namespace editor

#endif // CATA_SRC_EDITOR_HISTORY_STATE_H

#include "history_state.h"

#include "state/tools_state.h"
#include "project/project.h"
#include "widget/widgets.h"

// Cata's DebugLog define conflicts with function in ImGui
#ifdef DebugLog
#  undef DebugLog
#endif

#include "imgui.h"
#include "imgui_internal.h"

namespace editor
{
ProjectSnapshot::ProjectSnapshot()
{
    project = std::make_unique<Project>();
}
ProjectSnapshot::ProjectSnapshot( ProjectSnapshot && ) = default;
ProjectSnapshot::~ProjectSnapshot() = default;
ProjectSnapshot &ProjectSnapshot::operator=( ProjectSnapshot && ) = default;

ProjectSnapshot ProjectSnapshot::make_copy() const
{
    ProjectSnapshot ret;
    ret.project = std::make_unique<Project>( *project );
    ret.num = num;
    return ret;
}

void show_file_history( HistoryState &state, bool &show )
{
    ImGui::SetNextWindowSize( ImVec2( 230.0f, 130.0f ), ImGuiCond_FirstUseEver );
    if( !ImGui::Begin( "File history", &show ) ) {
        ImGui::End();
        return;
    }

    ImGui::HelpMarkerInline(
        "Undo/redo support.\n\n"
        "In order to enable undo and redo, the editor has to keep track of the old versions (revisions) of the file.  "
        "This is done entirely in memory, so remembering too much revisions may exhaust available RAM at some point "
        "and trigger program termination by the OS.  You can manually control how much revisions will be kept alive "
        "using the widget below.\n"
        "\nHotkeys:\n"
        "  Ctrl+Z - Undo (advance to older revision)\n"
        "  Ctrl+Shift+Z - Redo (advance to newer revision)\n"
    );

    ImGui::SetNextItemWidth( ImGui::GetFrameHeight() * 4.0f );
    ImGui::InputIntClamped( "History limit", state.history_capacity, 10, 10000,
                            ImGuiInputTextFlags_AutoSelectAll );

    ImGui::HelpMarkerInline(
        "The list below keeps track of file revisions.\n\n"
        "Click on a revision to make it active.  "
        "Every edit automatically generates a new revision and places it at the top.\n"
        "\nMarkers use in the list:\n"
        "  [S] This revision is the one saved in the project file.\n"
        "  [E] This revision is the one that was used for export.\n"
    );
    ImGui::Text( "Edit counter (debug): %d", state.edit_counter );

    for( const ProjectSnapshot &entry : state.file_history ) {
        bool is_saved = state.last_saved_snapshot && *state.last_saved_snapshot == entry.num;
        bool is_exported = state.last_exported_snapshot && *state.last_exported_snapshot == entry.num;
        std::string fname = string_format(
                                "Version %d%s%s",
                                entry.num,
                                is_saved ? " [S]" : "",
                                is_exported ? " [E]" : ""
                            );
        if( ImGui::Selectable( fname.c_str(), entry.num == state.current_snapshot.num ) ) {
            state.switch_to_snapshot = entry.num;
        }
    }

    ImGui::End();
}

void handle_revision_change( HistoryState &state, ToolsState &tools )
{
    if( tools.has_ongoing_tool_operation() ) {
        return;
    }
    if( state.switch_to_snapshot ) {
        auto it = std::find_if( state.file_history.cbegin(),
        state.file_history.cend(), [&]( const ProjectSnapshot & rev ) {
            return rev.num == *state.switch_to_snapshot;
        } );
        assert( it != state.file_history.cend() );
        state.current_snapshot = it->make_copy();
        state.switch_to_snapshot.reset();
    } else if( state.file_has_changes ) {
        state.file_has_changes = false;

        const bool is_changing_same = state.last_widget_changed && state.current_widget_changed &&
                                      *state.last_widget_changed == *state.current_widget_changed;

        state.current_widget_changed_str.clear();
        state.last_widget_changed = state.current_widget_changed;
        state.current_widget_changed = std::nullopt;

        bool is_alt_history = false;

        // Erase alternative history
        while( state.file_history[0].num != state.current_snapshot.num ) {
            // TODO: optimize this to use dequeue
            state.file_history.erase( state.file_history.cbegin() );
            is_alt_history = true;
        }

        const bool is_rev_saved = state.last_saved_snapshot ? *state.last_saved_snapshot ==
                                  state.current_snapshot.num : false;
        const bool is_rev_exported = state.last_exported_snapshot ? *state.last_exported_snapshot ==
                                     state.current_snapshot.num : false;
        const bool collapse_change = is_changing_same && !is_alt_history && !is_rev_saved &&
                                     !is_rev_exported && !state.file_history.empty();

        if( collapse_change ) {
            state.file_history.erase( state.file_history.cbegin() );
        } else {
            state.current_snapshot.num++;
        }
        state.file_history.insert( state.file_history.cbegin(), state.current_snapshot.make_copy() );

        // Erase old entries
        if( static_cast<int>( state.file_history.size() ) > state.history_capacity ) {
            state.file_history.resize( state.history_capacity );
        }
    }
}

HistoryState::HistoryState( std::unique_ptr<Project> &&project, bool was_loaded )
{
    current_snapshot = ProjectSnapshot();

    if( was_loaded ) {
        last_saved_snapshot = current_snapshot.num;
    }

    if( project ) {
        current_snapshot.project = std::move( project );
    }

    file_history.reserve( history_capacity + 1 );
    file_history.emplace_back( current_snapshot.make_copy() );
}

void HistoryState::mark_changed( const char *id )
{
    std::string new_widget_changed_str = id ? id : "<nullptr>";
    if( file_has_changes ) {
        std::cerr << string_format(
                      R"(Tried to invoke mark_changed( "%s" ), but the file has already been marked as changed with id "%s".)",
                      new_widget_changed_str,
                      current_widget_changed_str
                  ) << std::endl;
        std::abort();
    }
    current_widget_changed_str = new_widget_changed_str;
    if( id ) {
        ImGuiWindow *wnd = ImGui::GetCurrentWindow();
        current_widget_changed = wnd->GetID( id );
    }
    file_has_changes = true;
    edit_counter++;
}

bool HistoryState::has_unsaved_changes() const
{
    return !last_saved_snapshot || current_snapshot.num != *last_saved_snapshot;
}

bool HistoryState::has_unexported_changes() const
{
    return !last_exported_snapshot || current_snapshot.num != *last_exported_snapshot;
}

} // namespace editor

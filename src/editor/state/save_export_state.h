#ifndef CATA_SRC_EDITOR_SAVE_EXPORT_STATE_H
#define CATA_SRC_EDITOR_SAVE_EXPORT_STATE_H

#include <optional>
#include <string>

namespace editor
{
struct State;

struct SaveExportState {
    SaveExportState() = default;
    ~SaveExportState() = default;

    SaveExportState( const SaveExportState & ) = delete;
    SaveExportState( SaveExportState && ) = default;
    SaveExportState &operator=( const SaveExportState & ) = delete;
    SaveExportState &operator=( SaveExportState && ) = default;

    std::optional<std::string> file_save_path;
    std::optional<std::string> file_export_path;
};

void handle_file_saving( State &state );
void handle_file_exporting( State &state );
void handle_project_exiting( State &state );

} // namespace editor

#endif // CATA_SRC_EDITOR_SAVE_EXPORT_STATE_H

#ifndef CATA_SRC_EDITOR_SAVE_AND_EXPORT_H
#define CATA_SRC_EDITOR_SAVE_AND_EXPORT_H

#include <optional>
#include <string>

namespace editor
{
struct me_state;

struct me_save_export_state {
    me_save_export_state() = default;
    ~me_save_export_state() = default;

    me_save_export_state( const me_save_export_state & ) = delete;
    me_save_export_state( me_save_export_state && ) = default;
    me_save_export_state &operator=( const me_save_export_state & ) = delete;
    me_save_export_state &operator=( me_save_export_state && ) = default;

    std::optional<std::string> file_save_path;
    std::optional<std::string> file_export_path;
};

void handle_file_saving( me_state &state );
void handle_file_exporting( me_state &state );
void handle_project_exiting( me_state &state );

} // namespace editor

#endif // CATA_SRC_EDITOR_SAVE_AND_EXPORT_H

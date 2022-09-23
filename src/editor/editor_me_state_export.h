#ifndef CATA_SRC_EDITOR_EDITOR_ME_STATE_EXPORT_H
#define CATA_SRC_EDITOR_EDITOR_ME_STATE_EXPORT_H

#include <string>

#include "editor_me_state.h"

namespace editor_export
{

std::string to_string( const editor::me_file &file );

std::string format_string( const std::string &js );

} // namespace editor_export

#endif // CATA_SRC_EDITOR_EDITOR_ME_STATE_EXPORT_H

#ifndef CATA_SRC_EDITOR_NEW_PALETTE_H
#define CATA_SRC_EDITOR_NEW_PALETTE_H

#include "common/uuid.h"

#include <string>

namespace editor
{
struct State;

struct NewPaletteState {
    std::string name;
    bool inherits = false;
    UUID inherits_from = UUID_INVALID;

    bool confirmed = false;
    bool cancelled = false;
};

bool show_new_palette_window( State &state, NewPaletteState &palette );
void add_palette( State &state, NewPaletteState &palette );

} // namespace editor

#endif // CATA_SRC_EDITOR_NEW_PALETTE_H

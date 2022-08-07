#pragma once
#ifndef CATA_SRC_SDLTILES_EDITOR_H
#define CATA_SRC_SDLTILES_EDITOR_H

#include "optional.h"
#include "point.h"

namespace editor
{

cata::optional<tripoint> screen_to_tile( point mouse_pos );

} // namespace editor

#endif // CATA_SRC_SDLTILES_EDITOR_H

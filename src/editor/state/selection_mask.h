#ifndef CATA_SRC_EDITOR_SELECTION_MASK_H
#define CATA_SRC_EDITOR_SELECTION_MASK_H

#include "common/bool.h"
#include "common/canvas_2d.h"
#include "point.h"

namespace editor
{

struct SelectionMask {
    Canvas2D<Bool> data = Canvas2D<Bool>( point_zero );

    void clear_all();
    void set_all();
};

} // namespace editor

#endif // CATA_SRC_EDITOR_SELECTION_MASK_H

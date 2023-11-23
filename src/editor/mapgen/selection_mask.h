#ifndef CATA_SRC_EDITOR_SELECTION_MASK_H
#define CATA_SRC_EDITOR_SELECTION_MASK_H

#include "common/bool.h"
#include "common/canvas_2d.h"
#include "point.h"

namespace editor
{

struct SelectionMask {
    SelectionMask() : data( point_zero ) {};
    SelectionMask( point size ) : data( size ) {}
    ~SelectionMask() = default;

    Canvas2D<Bool> data;

    void clear_all();
    void set_all();

    void serialize( JsonOut &jsout ) const;
    void deserialize( JsonIn &jsin );
};

} // namespace editor

#endif // CATA_SRC_EDITOR_SELECTION_MASK_H

#ifndef CATA_SRC_EDITOR_EDITOR_ME_CANVAS_TOOL_H
#define CATA_SRC_EDITOR_EDITOR_ME_CANVAS_TOOL_H

#include "editor_me_uuid.h"

#include "../enum_traits.h"


namespace editor
{
struct me_state;

enum class CanvasTool {
    Brush,
    Bucket,
    BucketGlobal,

    _Num,
};

struct me_canvas_tools_state {
    void serialize( JsonOut &jsout ) const;
    void deserialize( JsonIn &jsin );

    CanvasTool tool = CanvasTool::Brush;
    bool ongoing_tool_operation = false;
    bool ongoing_brush_stroke = false;
    bool brush_stroke_changed_data = false;
    uuid_t brush = UUID_INVALID;
};

/**
 * =============== Windows ===============
 */
void show_toolbar( me_canvas_tools_state &tools, bool &show );

} // namespace editor

template<>
struct enum_traits<editor::CanvasTool> {
    static constexpr editor::CanvasTool last = editor::CanvasTool::_Num;
};

#endif // CATA_SRC_EDITOR_EDITOR_ME_CANVAS_TOOL_H

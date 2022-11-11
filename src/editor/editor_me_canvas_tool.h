#ifndef CATA_SRC_EDITOR_EDITOR_ME_CANVAS_TOOL_H
#define CATA_SRC_EDITOR_EDITOR_ME_CANVAS_TOOL_H

#include "editor_me_uuid.h"


namespace editor
{
struct me_state;

enum class CanvasTool {
    Brush,
    Bucket,
    BucketGlobal,
};

struct me_canvas_tools_state {
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

#endif // CATA_SRC_EDITOR_EDITOR_ME_CANVAS_TOOL_H

#ifndef CATA_SRC_EDITOR_TOOLS_STATE_H
#define CATA_SRC_EDITOR_TOOLS_STATE_H

#include "common/uuid.h"
#include "enum_traits.h"

#include <cassert>


namespace editor
{
struct State;

enum class CanvasTool {
    Brush,
    Bucket,
    BucketGlobal,

    _Num,
};

struct ToolsState {
    public:
        void serialize( JsonOut &jsout ) const;
        void deserialize( JsonIn &jsin );

        inline const UUID &get_brush() const {
            return brush;
        }

        inline void set_brush( const UUID &uuid ) {
            assert( !ongoing_tool_operation );
            brush = uuid;
        }

        inline CanvasTool get_tool() const {
            return tool;
        }

        inline void set_tool( CanvasTool t ) {
            assert( !ongoing_tool_operation );
            tool = t;
        }

        inline void start_tool_operation() {
            assert( !ongoing_tool_operation );
            ongoing_tool_operation = true;
        }

        inline bool has_ongoing_tool_operation() {
            return ongoing_tool_operation;
        }

        inline bool end_tool_operation() {
            assert( ongoing_tool_operation );
            ongoing_tool_operation = false;
            bool ret = tool_op_changed_data;
            tool_op_changed_data = false;
            return ret;
        }

        inline void set_tool_operation_changed_data() {
            assert( ongoing_tool_operation );
            tool_op_changed_data = true;
        }

    private:
        bool ongoing_tool_operation = false;
        bool tool_op_changed_data = false;
        CanvasTool tool = CanvasTool::Brush;
        UUID brush = UUID_INVALID;
};

/**
 * =============== Windows ===============
 */
void show_toolbar( ToolsState &tools, bool &show );

} // namespace editor

template<>
struct enum_traits<editor::CanvasTool> {
    static constexpr editor::CanvasTool last = editor::CanvasTool::_Num;
};

#endif // CATA_SRC_EDITOR_TOOLS_STATE_H

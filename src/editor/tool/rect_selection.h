#ifndef CATA_SRC_EDITOR_TOOL_RECT_SELECTION_H
#define CATA_SRC_EDITOR_TOOL_RECT_SELECTION_H

#include "coordinates.h"
#include "tool.h"

namespace editor
{
struct SelectionMask;
} // namespace editor

namespace editor::tools
{

struct RectSelectionControl : public ToolControl {
    std::optional<point_abs_etile> start;
    bool dismissing_selection = false;

    void handle_tool_operation( ToolTarget &target ) override;
    inline bool operation_in_progress() const override {
        return start.has_value();
    }

    std::vector<point> make_rectangle( point_abs_etile p1, point_abs_etile p2 ) const;
    void apply( SelectionMask &selection, const std::vector<point> &rect );
    point_abs_etile get_rectangle_end( ToolTarget &target ) const;

    void show_tooltip( ToolTarget &target ) override;
};

struct RectSelectionSettings : public ToolSettings {
    void serialize( JsonOut &jsout ) const override;
    void deserialize( JsonIn &jsin ) override;

    void show() override;
};

struct RectSelection : public ToolDefinition {
    std::string get_tool_display_name() const override;
    std::string get_tool_hint() const override;

    std::unique_ptr<ToolControl> make_control() const override {
        return std::make_unique<RectSelectionControl>();
    }
    std::unique_ptr<ToolSettings> make_settings() const override {
        return std::make_unique<RectSelectionSettings>();
    }
};

} // namespace editor::tools

#endif // CATA_SRC_EDITOR_TOOL_RECT_SELECTION_H

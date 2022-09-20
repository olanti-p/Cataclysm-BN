#ifndef CATA_SRC_EDITOR_EDITOR_MAIN_H
#define CATA_SRC_EDITOR_EDITOR_MAIN_H

class mapgen_function_json;
class nc_color;
struct ImVec4;
struct point;

namespace editor
{
void advanced_editor_run();
void show_ui();
void invalidate_map_cache();

void set_as_active( const mapgen_function_json *mgfunc );

ImVec4 curses_color_to_imgui( nc_color nc );

/**
 * @brief Get the visible area of the map.
 */
point get_visible_map_area();

} // namespace editor

#endif // CATA_SRC_EDITOR_EDITOR_MAIN_H

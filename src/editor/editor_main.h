#ifndef CATA_SRC_EDITOR_EDITOR_MAIN_H
#define CATA_SRC_EDITOR_EDITOR_MAIN_H

struct ImVec4;
class nc_color;

namespace editor
{
void advanced_editor_run();
void show_ui();

ImVec4 curses_color_to_imgui( nc_color nc );

} // namespace editor

namespace ImGui
{
void SymbolColored( const std::string &sym, nc_color col );
void SymbolColored( int sym, nc_color col );

} // namespace ImGui

#endif // CATA_SRC_EDITOR_EDITOR_MAIN_H

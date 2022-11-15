#ifndef CATA_SRC_EDITOR_EDITOR_INIT_H
#define CATA_SRC_EDITOR_EDITOR_INIT_H

#include <SDL.h>

namespace editor
{
void set_default_ini_path();
void set_project_ini_path( const std::string &project_uuid );
bool init_ui( SDL_Window &window_ref, SDL_Renderer &renderer_ref );
void shutdown_ui();
void render_ui();
bool process_event( SDL_Event &event );
bool show_cata_ui();
bool ui_exists();
}

#endif // CATA_SRC_EDITOR_EDITOR_INIT_H

#ifndef CATA_SRC_EDITOR_EDITOR_INIT_H
#define CATA_SRC_EDITOR_EDITOR_INIT_H

#include <SDL.h>

namespace editor
{
bool init_ui( SDL_Window &window_ref, SDL_Renderer &renderer_ref );
void shutdown_ui();
void render_ui();
bool process_event( SDL_Event &event );
}

#endif // CATA_SRC_EDITOR_EDITOR_INIT_H

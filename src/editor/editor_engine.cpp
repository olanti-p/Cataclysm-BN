#include "editor_engine.h"
#include "editor_main.h"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"
#include <stdio.h>
#include <SDL.h>

#include "../path_info.h"

#if !SDL_VERSION_ATLEAST(2,0,17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

static SDL_Window *window = nullptr;
static SDL_Renderer *renderer = nullptr;
static std::string ini_file_path;

namespace editor
{
bool init_ui( SDL_Window &window_ref, SDL_Renderer &renderer_ref )
{
    window = &window_ref;
    renderer = &renderer_ref;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer( window, renderer );
    ImGui_ImplSDLRenderer_Init( renderer );

    // Specify ini file path
    ini_file_path = PATH_INFO::config_dir() + "imgui.ini";
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = ini_file_path.c_str();

    return true;
}

void shutdown_ui()
{
    ImGui_ImplSDLRenderer_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    window = nullptr;
    renderer = nullptr;
}

void render_ui()
{
    if( !editor::ui_exists() ) {
        return;
    }
    // Start the Dear ImGui frame
    ImGui_ImplSDLRenderer_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    editor::show_ui();

    // Rendering
    ImGui::Render();
    ImGui_ImplSDLRenderer_RenderDrawData( ImGui::GetDrawData() );
}

bool process_event( SDL_Event &event )
{
    if( !editor::ui_exists() ) {
        return false;
    }

    /*
    bool is_kb = false;
    bool is_mouse = false;
    switch( event.type ) {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
        case SDL_TEXTINPUT:
        case SDL_TEXTEDITING:
            is_kb = true;
            break;

        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEMOTION:
        case SDL_MOUSEWHEEL:
        case SDL_MOUSEWHEEL_NORMAL:
        case SDL_MOUSEWHEEL_FLIPPED:
            is_mouse = true;
            break;

        default:
            break;
    }

    ImGuiIO &io = ImGui::GetIO();
    if( is_mouse || ( is_kb && io.WantCaptureKeyboard ) ) {
        ImGui_ImplSDL2_ProcessEvent( &event );
        return true;
    }
    return false;
    */

    ImGui_ImplSDL2_ProcessEvent( &event );
    return true;
}
}

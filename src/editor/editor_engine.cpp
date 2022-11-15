#include "editor_engine.h"
#include "editor_main.h"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"
#include <stdio.h>
#include <SDL.h>

#include "../path_info.h"

#ifdef DebugLog
#  undef DebugLog
#endif

#include "imgui_internal.h"

#if !SDL_VERSION_ATLEAST(2,0,17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

static SDL_Window *window = nullptr;
static SDL_Renderer *renderer = nullptr;
static std::string ini_file_path;

namespace editor
{
void set_default_ini_path( bool flush )
{
    if( flush ) {
        flush_ini_to_disk();
    }

    ini_file_path = PATH_INFO::config_dir() + "imgui.ini";
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = ini_file_path.c_str();

    ImGui::ClearIniSettings();
    ImGui::LoadIniSettingsFromDisk( io.IniFilename );
}

void set_project_ini_path( const std::string &project_uuid, bool flush )
{
    if( flush ) {
        flush_ini_to_disk();
    }

    ini_file_path = PATH_INFO::config_dir() + "imgui/" + project_uuid + ".ini";
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = ini_file_path.c_str();

    ImGui::ClearIniSettings();
    ImGui::LoadIniSettingsFromDisk( io.IniFilename );
}

void flush_ini_to_disk()
{
    ImGuiIO &io = ImGui::GetIO();
    ImGui::SaveIniSettingsToDisk( io.IniFilename );
}

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
    set_default_ini_path( false );

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

    ImGui_ImplSDL2_ProcessEvent( &event );
    return true;
}
}

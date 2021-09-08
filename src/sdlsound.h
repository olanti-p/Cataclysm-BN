#pragma once
#ifndef CATA_SRC_SDLSOUND_H
#define CATA_SRC_SDLSOUND_H

#include <string>
#if defined(SDL_SOUND)

#include "options.h"

/**
 * Attempt to initialize an audio device.  Returns false if initialization fails.
 */
bool init_sound();
void shutdown_sound();
void play_music( const std::string &playlist );
void stop_music();
void update_music_volume();
void load_soundset();

std::vector<options_manager::id_and_option> build_sound_driver_list();
std::vector<options_manager::id_and_option> build_sound_device_list();

#else

inline bool init_sound()
{
    return false;
}
inline void shutdown_sound() { }
inline void play_music( const std::string &/*playlist*/ )
{
}
inline void update_music_volume() { }
inline void load_soundset() { }

#endif

#endif // CATA_SRC_SDLSOUND_H

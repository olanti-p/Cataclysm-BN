::
:: Override default audio driver used by SDL2.
::
:: Some possible values for Windows 10:
::   wasapi        (default, seems to interact weirdly with surround sound)
::   directsound   (used to be the default on SDL 2.0.5, should work with surround sound)
::   winmm         (no idea about this one, try it too I guess)
::   disk          (records output to disk)
::   dummy         (disables sound)
::
:: See settings -> audio driver for all supported values.

set SDL_AUDIODRIVER=wasapi

:: Release build
cataclysm-tiles.exe

:: Visual Studio build
:: Cataclysm-vcpkg-static-Release-x64.exe

project('cataclysm-bn', 'cpp', default_options: ['default_library=static', 'cpp_std=c++14'])

subdir('src/')

precompiled_hdr = 'src/pch.h'

# SDL build dependencies
sdl2_dep = dependency('sdl2', required: true)
sdl2_image_dep = dependency('SDL2_image', required: true)
sdl2_ttf_dep = dependency('SDL2_ttf', required: true)

lib_dependencies = [
    sdl2_dep,
    sdl2_image_dep,
    sdl2_ttf_dep,
]

libcata = static_library(
    'cata-lib',
    sources: lib_sources,
    dependencies : lib_dependencies,
    cpp_pch: precompiled_hdr,
)

libcata_dep = declare_dependency(
    link_with : libcata,
    include_directories: include_directories( 'src/' ),
    dependencies: lib_dependencies,
)

executable(
    'cataclysm-tiles',
    sources : files( 'src/main.cpp' ),
    dependencies : libcata_dep,
    cpp_pch: precompiled_hdr,
)

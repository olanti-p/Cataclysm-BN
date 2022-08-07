project('cataclysm-bn', 'cpp', default_options: ['default_library=static', 'cpp_std=c++14'])

subdir('src/')

precompiled_hdr = 'pch/main-pch.hpp'

# SDL build dependencies
thread_dep = dependency('threads')
sdl2_dep = subproject('sdl2').get_variable('sdl2_dep')
sdl2_image_dep = subproject('sdl2_image').get_variable('sdl2_image_dep')
sdl2_ttf_dep = subproject('sdl2_ttf').get_variable('sdl2_ttf_dep')
imgui_dep = subproject('imgui').get_variable('imgui_dep')

cpp_args = [ '-DTILES' ]

version_dep = declare_dependency(
    sources : [version_h],
)

lib_dependencies = [
    version_dep,
    sdl2_dep,
    sdl2_image_dep,
    sdl2_ttf_dep,
    thread_dep,
    imgui_dep,
]

libcata = static_library(
    'cata-lib',
    sources: lib_sources,
    dependencies : lib_dependencies,
    cpp_pch : precompiled_hdr,
    cpp_args : cpp_args,
)

libcata_dep = declare_dependency(
    link_with : libcata,
    include_directories : include_directories( 'src/' ),
    dependencies : lib_dependencies,
)

executable(
    'cataclysm-tiles',
    sources : files( 'src/main.cpp' ),
    dependencies : libcata_dep,
    cpp_pch : precompiled_hdr,
    cpp_args : cpp_args,
)

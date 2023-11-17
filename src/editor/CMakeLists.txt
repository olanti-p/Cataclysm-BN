file(GLOB EDITOR_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

file(GLOB EDITOR_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(
    editor OBJECT
    ${EDITOR_SOURCES} ${EDITOR_HEADERS}
)

target_link_libraries(editor PRIVATE cataclysm-tiles-common)

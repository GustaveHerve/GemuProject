cmake_minimum_required(VERSION 3.28)

project(GemuProject VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED 17)
set(CMAKE_C_EXTENSIONS FALSE)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ------------------------
# SDL3
# ------------------------

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)

# ------------------------
# Dear ImGui + dear_bindings
# ------------------------

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/dependencies/imgui)
set(DEAR_BINDINGS_DIR ${CMAKE_SOURCE_DIR}/dependencies/dear_bindings)

set(IMGUI_SOURCES
    ${IMGUI_DIR}/src/imgui.cpp
    ${IMGUI_DIR}/src/imgui_demo.cpp
    ${IMGUI_DIR}/src/imgui_draw.cpp
    ${IMGUI_DIR}/src/imgui_tables.cpp
    ${IMGUI_DIR}/src/imgui_widgets.cpp
    ${IMGUI_DIR}/src/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/src/imgui_impl_sdlrenderer3.cpp
)

set(DEAR_BINDINGS_SOURCES
    ${DEAR_BINDINGS_DIR}/src/dcimgui.cpp
    ${DEAR_BINDINGS_DIR}/src/dcimgui_internal.cpp
    ${DEAR_BINDINGS_DIR}/src/dcimgui_impl_sdl3.cpp
    ${DEAR_BINDINGS_DIR}/src/dcimgui_impl_sdlrenderer3.cpp
)

add_library(dear_imgui STATIC
    ${IMGUI_SOURCES}
    ${DEAR_BINDINGS_SOURCES}
)

set_target_properties(dear_imgui PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(dear_imgui PUBLIC
    ${IMGUI_DIR}/include
    ${DEAR_BINDINGS_DIR}/include
)

# ------------------------
# GemuProject
# ------------------------

add_executable(gemu)

set_target_properties(gemu PROPERTIES EXPORT_COMPILE_COMMANDS ON)

target_compile_options(gemu PUBLIC -Wall -Wextra -pedantic)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    target_compile_options(gemu PUBLIC -fsanitize=address)
    target_link_options(gemu PUBLIC -fsanitize=address)
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    target_compile_options(gemu PUBLIC -O3)
endif()

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
    target_compile_definitions(gemu PUBLIC "_MACOS")
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    target_compile_definitions(gemu PUBLIC "_WIN32")
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    target_compile_definitions(gemu PUBLIC "_LINUX")
endif()

add_subdirectory(src)

target_include_directories(gemu PUBLIC
    "include"
    "include/core"
    "include/core/mbc"
    "include/core/memory"
    "include/core/opcodes"
    "include/front"
    "include/front/sdl"
    ${DEAR_BINDINGS_DIR}/include
)

target_link_libraries(gemu PRIVATE SDL3::SDL3 dear_imgui)

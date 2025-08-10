cmake_minimum_required(VERSION 3.28)

project(GemuProject VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED 99)
set(CMAKE_C_EXTENSIONS FALSE)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
add_compile_options(-Wall -Wextra -pedantic -O3)

# Check if the build type is Debug
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif()

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
    add_compile_definitions("_MACOS")
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    add_compile_definitions("_WIN32")
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    add_compile_definitions("_LINUX")
endif()

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)

add_executable(gemu)

add_subdirectory(src)

target_include_directories(gemu PUBLIC
    "include"
    "include/core"
    "include/core/mbc"
    "include/core/memory"
    "include/core/opcodes"
    "include/front"
    "include/front/sdl"
)

target_link_libraries(gemu PRIVATE SDL3::SDL3)

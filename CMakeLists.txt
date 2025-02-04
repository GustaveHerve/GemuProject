cmake_minimum_required(VERSION 3.28)

project(GemuProject VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
add_compile_options(-Wall -Wextra -pedantic -std=c99)

# Check if the build type is Debug
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -fsanitize=address)
    add_link_options(-fsanitize=address)
endif()

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)

add_executable(gemu)

add_subdirectory(src)

target_include_directories(gemu PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_include_directories(gemu PUBLIC "${PROJECT_SOURCE_DIR}/include/mbc")

target_link_libraries(gemu PRIVATE SDL3::SDL3)

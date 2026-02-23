cmake_minimum_required(VERSION 3.20)
project(FURRY C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(FURRY_ENABLE_VULKAN "Enable Vulkan renderer integration (stub for now)" OFF)
option(FURRY_ENABLE_SDL3 "Enable SDL3 platform integration (stub for now)" OFF)
option(FURRY_ENABLE_MINIAUDIO "Enable miniaudio integration hooks" ON)

add_library(furry_lib STATIC
    src/furry.c
    src/furry_ui.c
)

target_include_directories(furry_lib PUBLIC include)

if(FURRY_ENABLE_VULKAN)
    target_compile_definitions(furry_lib PUBLIC FURRY_ENABLE_VULKAN=1)
endif()

if(FURRY_ENABLE_SDL3)
    target_compile_definitions(furry_lib PUBLIC FURRY_ENABLE_SDL3=1)
endif()

if(FURRY_ENABLE_MINIAUDIO)
    target_compile_definitions(furry_lib PUBLIC FURRY_ENABLE_MINIAUDIO=1)
endif()

add_executable(furry_app src/main.c)
target_link_libraries(furry_app PRIVATE furry_lib)

if(MSVC)
    target_compile_options(furry_lib PRIVATE /W4 /permissive-)
    target_compile_options(furry_app PRIVATE /W4 /permissive-)
else()
    target_compile_options(furry_lib PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(furry_app PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()
add_executable(test_furry tests/test_furry.c)
target_link_libraries(test_furry PRIVATE furry_lib)
add_test(NAME test_furry COMMAND test_furry)

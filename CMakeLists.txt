cmake_minimum_required(VERSION 3.1)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
project("olkiluoto_3-2-1")

enable_language(C)
enable_language(CXX)

include("defaults.cmake")

find_boost("filesystem" "program_options" "system" "wave")
find_opengl()
find_glew()
find_png()
find_sdl2()
find_threads()

output_flags("DEBUG" on)

add_definitions(-DUSE_LD)

include_directories("${PROJECT_SOURCE_DIR}/src")

set(EXECUTABLE_NAME "${CMAKE_PROJECT_NAME}")
add_executable("${EXECUTABLE_NAME}"
    "src/bsd_rand.c"
    "src/bsd_rand.h"
    "src/dnload.h"
    "src/fps_counter.cpp"
    "src/fps_counter.hpp"
    "src/forward.frag.glsl"
    "src/forward.frag.glsl.hpp"
    "src/forward.mesh.glsl"
    "src/forward.mesh.glsl.hpp"
    "src/glsl_pipeline.cpp"
    "src/glsl_pipeline.hpp"
    "src/glsl_program.cpp"
    "src/glsl_program.hpp"
    "src/glsl_shader.cpp"
    "src/glsl_shader.hpp"
    "src/glsl_shader_program.cpp"
    "src/glsl_shader_program.hpp"
    "src/glsl_shader_source.cpp"
    "src/glsl_shader_source.hpp"
    "src/glsl_wave.cpp"
    "src/glsl_wave.hpp"
    "src/gnu_rand.c"
    "src/gnu_rand.h"
    "src/image_png.cpp"
    "src/image_png.hpp"
    "src/intro.cpp")
if(MSVC)
    target_link_libraries("${EXECUTABLE_NAME}" "Imm32")
    target_link_libraries("${EXECUTABLE_NAME}" "Setupapi")
    target_link_libraries("${EXECUTABLE_NAME}" "version")
    target_link_libraries("${EXECUTABLE_NAME}" "WINMM")
    target_link_libraries("${EXECUTABLE_NAME}" "${OPENGL_gl_LIBRARY}")
    target_link_libraries("${EXECUTABLE_NAME}" debug "${GLEW_LIBRARY_DEBUG}" optimized "${GLEW_LIBRARY}")
    target_link_libraries("${EXECUTABLE_NAME}" debug "${PNG_LIBRARY_DEBUG}" optimized "${PNG_LIBRARY}")
    target_link_libraries("${EXECUTABLE_NAME}" debug "${SDL2_LIBRARY_DEBUG}" optimized "${SDL2_LIBRARY}")
    target_link_libraries("${EXECUTABLE_NAME}" debug "${ZLIB_LIBRARY_DEBUG}" optimized "${ZLIB_LIBRARY}")
else()
    target_link_libraries("${EXECUTABLE_NAME}" "${GLEW_LIBRARY}")
    target_link_libraries("${EXECUTABLE_NAME}" "${BOOST_FILESYSTEM_LIBRARY}")
    target_link_libraries("${EXECUTABLE_NAME}" "${BOOST_PROGRAM_OPTIONS_LIBRARY}")
    target_link_libraries("${EXECUTABLE_NAME}" "${BOOST_SYSTEM_LIBRARY}")
    target_link_libraries("${EXECUTABLE_NAME}" "${BOOST_THREAD_LIBRARY}")
    target_link_libraries("${EXECUTABLE_NAME}" "${BOOST_WAVE_LIBRARY}")
    target_link_libraries("${EXECUTABLE_NAME}" "${PNG_LIBRARY}")
    target_link_libraries("${EXECUTABLE_NAME}" "${SDL2_LIBRARY}")
endif()
target_link_libraries("${EXECUTABLE_NAME}" "${THREADS_LIBRARY}")

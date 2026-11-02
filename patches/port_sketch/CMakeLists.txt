# patches/port_sketch/CMakeLists.txt: SKETCH, UNTESTED, NOT WIRED INTO ANY BUILD. Ticket 004.
#
# One source tree, two build systems:
#   * the MSVC 7.1 byte-exact flow stays tools/matching + tools/relink/build.py (cl 13.10 under Wine); CMake never sees it;
#   * this file builds the PORTABLE configuration from the SAME src/*.cpp, with clang, no edits to src/ (a force-included
#     prelude + shim include directories do the work; docs/patches.md forbids #ifdefs in src/).
#
# Usage (macOS arm64 host, Linux x86-64 host):
#   cmake -S patches/port_sketch -B build/port -DMB_SRC=$PWD/src -DMB_TARGET=linux-x86_64
#   cmake --build build/port -j
cmake_minimum_required(VERSION 3.24)
project(mb_port CXX C)

set(MB_SRC "${CMAKE_SOURCE_DIR}/../../src" CACHE PATH "the reconstruction")
set(MB_TARGET "native" CACHE STRING "native | linux-i686 | linux-x86_64 | macos-arm64 | macos-x86_64")

# The reconstruction is C++03 and leans on MSVC extensions. Keep the dialect old and permissive.
set(CMAKE_CXX_STANDARD 98)            # clang: -std=gnu++98; no exceptions-spec surprises
set(CMAKE_CXX_EXTENSIONS ON)

file(GLOB_RECURSE MB_GAME CONFIGURE_DEPENDS
     "${MB_SRC}/game/*.cpp" "${MB_SRC}/rgl/*.cpp" "${MB_SRC}/tinyxml/*.cpp" "${MB_SRC}/dxframework/*.cpp")
# Two TUs are Windows-only in the portable build and replaced by port/ (the SDK framework window + the launcher dialog):
list(FILTER MB_GAME EXCLUDE REGEX ".*/dxframework/d3dapp\\.cpp$")
list(FILTER MB_GAME EXCLUDE REGEX ".*/game/config/options_dlg\\.cpp$")

file(GLOB PORT_IMPL CONFIGURE_DEPENDS "port/*.cpp")      # platform_sdl.cpp, audio_miniaudio.cpp, physics_*.cpp, d3dx_shim.cpp ...

add_executable(mb ${MB_GAME} ${PORT_IMPL})

target_include_directories(mb BEFORE PRIVATE
    port/include          # shim <windows.h> and the few D3D9 types still named by headers until ticket 008 removes them
    ${MB_SRC} ${MB_SRC}/include)

target_compile_options(mb PRIVATE
    -include ${CMAKE_SOURCE_DIR}/port_prelude.h
    -fms-extensions -fms-compatibility        # __declspec, i64 suffix, anonymous-struct members
    -fno-strict-aliasing                      # the code type-puns floats/ints and char*+offset everywhere
    -fno-delete-null-pointer-checks -fwrapv   # decompiled arithmetic assumes two's complement wrap
    -Wno-everything)

# Floating point: whatever the target does natively (SSE2 / NEON); bit-identity with the x87 original is not a goal.
if(MB_TARGET STREQUAL "linux-i686")
  target_compile_options(mb PRIVATE -m32 -msse2 -mfpmath=sse)
  target_link_options(mb PRIVATE -m32)
endif()

# 64-bit configurations are only valid after ticket 006 (layout-neutral fields); until then this define keeps the
# size asserts (src/include/troop.h mb_troop_size_chk, src/game/rgl_mesh.h mesh_size_check ...) honest and failing:
if(MB_TARGET MATCHES "x86_64|arm64")
  target_compile_definitions(mb PRIVATE MB_LP64=1)
endif()

find_package(SDL3 CONFIG REQUIRED)          # or SDL2; window, input, audio device (ticket 007)
target_link_libraries(mb PRIVATE SDL3::SDL3)
# Renderer (ticket 008): SDL3 GPU API; dxvk-native only as a Linux bring-up crutch
# find_package(dxvk_native ...)

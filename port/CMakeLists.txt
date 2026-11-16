# port/CMakeLists.txt: the PORTABLE build configuration (ticket 004, docs/tickets/004-portable-build-config-cmake-clang.md).
#
# One source tree, two build systems:
#   * the MSVC 7.1 byte-exact flow stays tools/matching + tools/relink/build.py (cl 13.10 under Wine / the laptop server);
#     CMake never sees it, and nothing here is visible to it;
#   * this file builds the SAME src/*.cpp with clang and no edits to src/: a force-included prelude (port_prelude.h) and
#     shim include directories (port/include for Windows/D3D9/D3DX/DirectInput, port/stl for the VC7.1-shaped standard
#     containers) do the adapting. docs/patches.md forbids #ifdefs in src/.
#
# Milestone 1 (now): every TU compiles to an object (`mb_objs`); nothing links yet. tools/port/burndown.py compiles each TU
# from compile_commands.json and records the count in docs/port_burndown.json.
#
#   cmake -S port -B build/port/linux-i686 -G Ninja -DCMAKE_TOOLCHAIN_FILE=port/cmake/linux-i686.cmake
#   cmake --build build/port/linux-i686            # or: tools/port/burndown.py
cmake_minimum_required(VERSION 3.24)
project(mb_port CXX C)

get_filename_component(MB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(MB_SRC "${MB_ROOT}/src" CACHE PATH "the reconstruction")
set(MB_TARGET "linux-i686" CACHE STRING "linux-i686 (now) | linux-x86_64 | macos-arm64 | macos-x86_64 (after ticket 006)")
set(PORT "${CMAKE_CURRENT_SOURCE_DIR}")

# Every TU of the MSVC build. d3dapp.cpp (the SDK framework's window/loop) and options_dlg.cpp (the launcher dialog) are
# compiled too: milestone 1 is "all of src/ compiles"; at link time port/ equivalents replace them (ticket 007).
file(GLOB_RECURSE MB_SOURCES CONFIGURE_DEPENDS "${MB_SRC}/*.cpp")
list(FILTER MB_SOURCES EXCLUDE REGEX "/_dataaudit_")

add_library(mb_objs OBJECT ${MB_SOURCES})

# Order matters: the shims come first so <windows.h>, <d3d9.h>, <vector> ... resolve to port/ and never to a system copy.
# match/include holds rgl.h, which the MSVC flow also puts on its INCLUDE path (tools/matching/cl.sh).
target_include_directories(mb_objs BEFORE PRIVATE
    ${PORT}/include
    ${PORT}/stl
    ${MB_SRC}/include
    ${MB_ROOT}/match/include)

target_compile_options(mb_objs PRIVATE
    -include ${PORT}/port_prelude.h
    $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++17>
    -fms-extensions                           # __declspec(...), __int64, i64 literals, __forceinline, anonymous structs
    -fno-strict-aliasing                      # the code type-puns floats/ints and char*+offset everywhere
    -fno-delete-null-pointer-checks -fwrapv   # decompiled arithmetic assumes two's complement wrap
    -Wno-everything)

# Floating point: whatever the target does natively (SSE2 / NEON); bit-identity with the x87 original is not a goal.
if(MB_TARGET STREQUAL "linux-i686")
  target_compile_options(mb_objs PRIVATE -msse2 -mfpmath=sse)
endif()

# 64-bit configurations are only valid after ticket 006 (layout-neutral fields); until then the size asserts already in the
# tree (src/include/troop.h mb_troop_size_chk, src/game/rgl_mesh.h mesh_size_check ...) fail there, as they should.
if(MB_TARGET MATCHES "x86_64|arm64")
  target_compile_definitions(mb_objs PRIVATE MB_LP64=1)
endif()

# Later milestones (ticket 004 "what's next"): an `mb` executable from mb_objs + port/*.cpp (SDL3 platform layer, ticket 007;
# renderer, 008; audio, 010; physics adapter, 011), linked against SDL3:
#   find_package(SDL3 CONFIG REQUIRED)

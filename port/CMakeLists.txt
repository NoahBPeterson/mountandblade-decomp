# port/CMakeLists.txt: the PORTABLE build configuration (ticket 004, docs/tickets/004-portable-build-config-cmake-clang.md).
#
# One source tree, two build systems:
#   * the MSVC 7.1 byte-exact flow stays tools/matching + tools/relink/build.py (cl 13.10 under Wine / the laptop server);
#     CMake never sees it, and nothing here is visible to it;
#   * this file builds the SAME src/*.cpp with clang and no edits to src/: a force-included prelude (port_prelude.h) and
#     shim include directories (port/include for Windows/D3D9/D3DX/DirectInput, port/stl for the VC7.1-shaped standard
#     containers) do the adapting. docs/patches.md forbids #ifdefs in src/.
#
# Milestone 1: every TU compiles to an object. Milestone 2: the `mb` executable links (port/link tables, port/runtime,
# port/stubs, port/main.cpp). tools/port/burndown.py compiles each TU from compile_commands.json, links `mb` and records
# both in docs/port_burndown.json.
#
#   cmake -S port -B build/port/linux-i686 -G Ninja -DCMAKE_TOOLCHAIN_FILE=port/cmake/linux-i686.cmake
#   cmake --build build/port/linux-i686            # or: tools/port/burndown.py
cmake_minimum_required(VERSION 3.24)
project(mb_port CXX C)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Release unless asked otherwise (an unoptimised game is no test)" FORCE)
endif()

get_filename_component(MB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(MB_SRC "${MB_ROOT}/src" CACHE PATH "the reconstruction")
set(MB_TARGET "linux-i686" CACHE STRING "linux-i686 (now) | linux-x86_64 | macos-arm64 | macos-x86_64 (after ticket 006)")
set(PORT "${CMAKE_CURRENT_SOURCE_DIR}")

# Every TU of the MSVC build. d3dapp.cpp (the SDK framework's window/loop) and options_dlg.cpp (the launcher dialog) are
# compiled too: milestone 1 is "all of src/ compiles"; at link time port/ equivalents replace them (ticket 007).
file(GLOB_RECURSE MB_SOURCES CONFIGURE_DEPENDS "${MB_SRC}/*.cpp")
list(FILTER MB_SOURCES EXCLUDE REGEX "/_dataaudit_")

# mb_src_flags: everything a src/ TU is compiled with, as usage requirements, so the per-TU object libraries of the link
# step (mb_weaken below) and the port TUs that share src/'s headers and layouts get exactly the same flags.
add_library(mb_src_flags INTERFACE)

# Order matters: the shims come first so <windows.h>, <d3d9.h>, <vector> ... resolve to port/ and never to a system copy.
# match/include holds rgl.h, which the MSVC flow also puts on its INCLUDE path (tools/matching/cl.sh).
target_include_directories(mb_src_flags BEFORE INTERFACE
    ${PORT}/include
    ${PORT}/stl
    ${MB_SRC}/include
    ${MB_ROOT}/match/include)

target_compile_options(mb_src_flags INTERFACE
    -include ${PORT}/port_prelude.h
    $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++17>
    $<$<COMPILE_LANGUAGE:CXX>:-nostdinc++>    # port/stl replaces the C++ library headers
    -fms-extensions                           # __declspec(...), __int64, i64 literals, __forceinline, anonymous structs
    -fms-compatibility                        # cl's leniencies the source relies on: goto past initialisations, lookup into
                                              # dependent bases (VecOps' iterator, shapes.h's _Buy); no _MSC_VER on this target
    -fexceptions -fcxx-exceptions             # (-fms-compatibility turns C++ exceptions off on a non-Windows target)
    # -fms-compatibility also drops the GNU identity macros, which glibc's headers test; restore the ones clang reports:
    -D__GNUC__=4 -D__GNUC_MINOR__=2 -D__GNUC_PATCHLEVEL__=1 -D__GNUG__=4 -D__GNUC_GNU_INLINE__=1 -D__GXX_ABI_VERSION=1002
    -D__GXX_RTTI=1 -D__GXX_WEAK__=1 -D__EXCEPTIONS=1
    -fno-strict-aliasing                      # the code type-puns floats/ints and char*+offset everywhere
    -fno-delete-null-pointer-checks -fwrapv   # decompiled arithmetic assumes two's complement wrap
    -Wno-everything)

# Per-TU preludes: port/tu/<path>.h is force-included (after port_prelude.h) into src/<path>.cpp only. For the rare TU
# that is valid for cl but not ISO C++ in a way no global flag relaxes; each file says why. Keeps src/ unchanged.
file(GLOB_RECURSE MB_TU_PRELUDES CONFIGURE_DEPENDS RELATIVE "${PORT}/tu" "${PORT}/tu/*.h")
foreach(tu ${MB_TU_PRELUDES})
  string(REGEX REPLACE "\\.h$" ".cpp" tu_src "${tu}")
  set_property(SOURCE "${MB_SRC}/${tu_src}" APPEND PROPERTY COMPILE_OPTIONS -include "${PORT}/tu/${tu}")
endforeach()

# Floating point: whatever the target does natively (SSE2 / NEON); bit-identity with the x87 original is not a goal.
if(MB_TARGET STREQUAL "linux-i686")
  # -malign-double: MSVC aligns double / __int64 struct members to 8; the i386 SysV ABI aligns them to 4. The source's
  # hand-laid records (offset comments, size asserts, char* + offset accesses) are MSVC's, so take MSVC's rule here.
  # glibc structs with 64-bit members (struct stat) then differ: src/ reaches those only through port/ shims.
  target_compile_options(mb_src_flags INTERFACE -msse2 -mfpmath=sse -malign-double)
endif()

# 64-bit configurations are only valid after ticket 006 (layout-neutral fields); until then the size asserts already in the
# tree (src/include/troop.h mb_troop_size_chk, src/game/rgl_mesh.h mesh_size_check ...) fail there, as they should.
if(MB_TARGET MATCHES "x86_64|arm64")
  target_compile_definitions(mb_src_flags INTERFACE MB_LP64=1)
endif()

# ---- milestone 2: the `mb` executable --------------------------------------------------------------------------------
# port/link/* is generated by tools/port/link_tables.py from the MSVC flow's ownership data (never edited by hand):
#   weaken.cmake      mb_weaken(<TU> <symbol>...): a strong symbol several TUs define keeps its row owner's copy (the
#                     copy tools/relink/build.py links first); the other TUs' objects go through llvm-objcopy
#                     --weaken-symbol. Each such TU gets an object library of its own so its object path is known.
#   emit_inline.cmake mb_emit_inline(<TU>): -fno-inline for a TU whose inline functions other TUs call out of line
#                     (cl emitted them as COMDATs; clang inlines every use and emits nothing)
#   aliases.ld        PROVIDE(alias = target): names src/ declares under one name and defines under another (glue.py's
#                     /alternatename table); linked as -T, which keeps lld's default layout
#   stl_instances.cpp port/stl members src/ declares as explicit specializations and defines nowhere
#   traps.cpp         weak placeholders for what has no definition for this ABI yet (they abort with the name)
find_program(MB_OBJCOPY NAMES llvm-objcopy HINTS /opt/homebrew/opt/llvm/bin /usr/local/opt/llvm/bin /usr/lib/llvm/bin REQUIRED)
set(MB_LINK "${PORT}/link")
set(MB_WEAK_OBJS "")
set(MB_WEAK_SOURCES "")
function(mb_weaken tu)
  string(MAKE_C_IDENTIFIER "${tu}" id)
  set(lib "mbw_${id}")
  add_library(${lib} OBJECT "${MB_ROOT}/${tu}")
  target_link_libraries(${lib} PRIVATE mb_src_flags)
  set(args "")
  list(LENGTH ARGN nsyms)
  foreach(sym ${ARGN})
    list(APPEND args "--weaken-symbol=${sym}")
  endforeach()
  set(out "${CMAKE_CURRENT_BINARY_DIR}/weak/${id}.o")
  add_custom_command(OUTPUT "${out}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/weak"
    COMMAND ${MB_OBJCOPY} ${args} "$<TARGET_OBJECTS:${lib}>" "${out}"
    DEPENDS ${lib} "$<TARGET_OBJECTS:${lib}>" "${MB_LINK}/weaken.cmake"
    COMMENT "weaken ${tu}: ${nsyms} symbols owned by another TU (port/link/owners.tsv)" VERBATIM)
  set(MB_WEAK_OBJS ${MB_WEAK_OBJS} "${out}" PARENT_SCOPE)
  set(MB_WEAK_SOURCES ${MB_WEAK_SOURCES} "${MB_ROOT}/${tu}" PARENT_SCOPE)
endfunction()
function(mb_emit_inline tu)
  set_property(SOURCE "${MB_ROOT}/${tu}" APPEND PROPERTY COMPILE_OPTIONS -fno-inline)
endfunction()
include("${MB_LINK}/weaken.cmake")
include("${MB_LINK}/emit_inline.cmake")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${MB_LINK}/weaken.cmake" "${MB_LINK}/emit_inline.cmake")

# every src/ TU not routed through mb_weaken
set(MB_PLAIN_SOURCES ${MB_SOURCES})
if(MB_WEAK_SOURCES)
  list(REMOVE_ITEM MB_PLAIN_SOURCES ${MB_WEAK_SOURCES})
endif()
add_library(mb_objs OBJECT ${MB_PLAIN_SOURCES})
target_link_libraries(mb_objs PRIVATE mb_src_flags)

# the port runtime: Microsoft CRT extras over POSIX. NOT src/'s flags: it calls glibc with glibc's own structs
# (struct stat, struct dirent), which -malign-double would lay out differently; it only borrows the shim declarations.
add_library(mb_runtime OBJECT ${PORT}/runtime/crt_extra.cpp ${PORT}/runtime/link_trap.cpp)
target_include_directories(mb_runtime PRIVATE ${PORT}/include)
target_compile_options(mb_runtime PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++17> -fno-strict-aliasing -Wall)
if(MB_TARGET STREQUAL "linux-i686")
  target_compile_options(mb_runtime PRIVATE -msse2 -mfpmath=sse)
endif()

set_source_files_properties(${MB_WEAK_OBJS} PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)
# (2026-11-19) the stub-linked `mb` (port/stubs: placeholders that return failure) is gone: it linked and could not run.
# The executable is mb_native (port/cmake/native_link.cmake) on the 64-bit targets; linux-i686 is a compile check.

# ---- the real port: one directory per subsystem, each with its own CMakeLists.txt defining library mb_<lane> and its
# own test programs (port/<lane>/tests). platform: Win32 USER/KERNEL/GDI + DirectInput8 over SDL3; gfx: Direct3D 9 +
# D3DX over SDL_GPU; audio: FMOD 3.75 over SDL3 audio; physics: Newton 1.x over Jolt. src/ keeps calling the same APIs.
foreach(lane platform gfx audio physics)
  if(NOT MB_TARGET STREQUAL "linux-i686" AND EXISTS "${PORT}/${lane}/CMakeLists.txt")   # 64-bit SDL3 / Jolt only
    add_subdirectory("${PORT}/${lane}" "${CMAKE_CURRENT_BINARY_DIR}/${lane}")
  endif()
endforeach()
include("${PORT}/cmake/native_link.cmake")

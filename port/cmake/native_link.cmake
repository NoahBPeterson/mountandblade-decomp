# port/cmake/native_link.cmake: the native `mb_native` executable (macOS arm64, Linux x86_64): src/ objects + the real
# subsystem libraries (port/platform, port/audio, port/physics, port/gfx). No placeholders: port/stubs is not linked.
# Included from port/CMakeLists.txt after the lane directories.
if(MB_TARGET STREQUAL "linux-i686")
  return()
endif()

set(MB_NATIVE_LIBS mb_platform mb_audio mb_physics mb_gfx)
set(MB_NATIVE_MISSING "")
foreach(l ${MB_NATIVE_LIBS})
  if(NOT TARGET ${l})
    list(APPEND MB_NATIVE_MISSING ${l})
  endif()
endforeach()
option(MB_NATIVE_PROBE "link mb_native without the missing subsystem libraries, to list what is still unresolved" OFF)
if(MB_NATIVE_MISSING AND NOT MB_NATIVE_PROBE)
  message(STATUS "mb_native: not configured, missing ${MB_NATIVE_MISSING}")
  return()
endif()
list(REMOVE_ITEM MB_NATIVE_LIBS ${MB_NATIVE_MISSING})

add_library(mb_native_src OBJECT ${PORT}/main.cpp ${MB_LINK}/stl_instances.cpp)
target_link_libraries(mb_native_src PRIVATE mb_src_flags)
add_executable(mb_native ${MB_WEAK_OBJS})
target_link_libraries(mb_native PRIVATE mb_objs mb_native_src ${MB_NATIVE_LIBS} mb_runtime)
set_target_properties(mb_native PROPERTIES LINKER_LANGUAGE CXX OUTPUT_NAME mb RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# aliases.ld's PROVIDE(alias = target): ld64 has no linker scripts; -alias makes `alias` another name of `target`
# (Mach-O C symbols carry a leading underscore). GNU ld (Linux x86_64) takes the script as is.
file(STRINGS "${MB_LINK}/aliases.ld" _mb_alias_lines REGEX "^PROVIDE\\(")
if(APPLE)
  # The subsystem libraries stand where the original's DLLs (d3d9, fmod, the CRT) stood: initialised before the game's own
  # static constructors, which already call into them (GameApp, world_app.cpp: registry, FSOUND_Init). ld64 runs the
  # initialisers in input-file order, so the libraries are loaded whole ahead of the src/ objects.
  foreach(l ${MB_NATIVE_LIBS})
    target_link_options(mb_native PRIVATE "LINKER:-force_load,$<TARGET_FILE:${l}>")
  endforeach()
  foreach(l ${_mb_alias_lines})
    string(REGEX MATCH "^PROVIDE\\(([^ =]+) = ([^ )]+)\\);" _m "${l}")
    if(_m)
      target_link_options(mb_native PRIVATE "LINKER:-alias,_${CMAKE_MATCH_2},_${CMAKE_MATCH_1}")
    endif()
  endforeach()
else()
  target_link_options(mb_native PRIVATE "LINKER:-T,${MB_LINK}/aliases.ld")
  target_link_libraries(mb_native PRIVATE m pthread dl)
endif()

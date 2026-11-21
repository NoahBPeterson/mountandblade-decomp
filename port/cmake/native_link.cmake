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

add_library(mb_native_src OBJECT ${PORT}/main.cpp ${PORT}/mb_start.cpp ${MB_LINK}/stl_instances.cpp)
target_link_libraries(mb_native_src PRIVATE mb_src_flags)
# The src/ objects in the original's link order (port/link/tu_order.txt, tools/port/tu_order.py): static constructors run
# in input-file order and the destructors in reverse, as the original's .CRT$XCU table and atexit list did, so a global
# destroyed at exit still finds the globals it uses (g_gui's widgets release meshes g_resmgr owns). A TU routed through
# mb_weaken contributes its weakened object, every other one its mb_objs object. All of them are passed as objects:
# CMake puts a target's external objects ahead of the objects it compiles itself, so a mix of the two (weakened objects
# plus sources compiled by mb_native) would not keep this order. It did not: world_app.cpp's weakened object ran
# GameApp::GameApp, which sets g_module_name, before trigger_vec.cpp constructed g_module_name and emptied it again
# ("Unable to find module info file", 2026-11-21).
file(STRINGS "${PORT}/link/tu_order.txt" _mb_order)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${PORT}/link/tu_order.txt")
function(mb_plain_obj src out)          # the object mb_objs compiles src to (CMake's path for a source outside the tree)
  string(REGEX REPLACE "^/" "" _rel "${src}")
  set(${out} "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/mb_objs.dir/${_rel}.o" PARENT_SCOPE)
endfunction()
set(MB_ORDERED "")
set(_mb_rest ${MB_PLAIN_SOURCES})
set(_mb_rest_weak ${MB_WEAK_OBJS})
foreach(tu ${_mb_order})
  string(MAKE_C_IDENTIFIER "${tu}" _id)
  set(_w "${CMAKE_CURRENT_BINARY_DIR}/weak/${_id}.o")
  list(FIND MB_WEAK_OBJS "${_w}" _wi)
  list(FIND MB_PLAIN_SOURCES "${MB_ROOT}/${tu}" _si)
  if(_wi GREATER -1)
    list(APPEND MB_ORDERED "${_w}")
    list(REMOVE_ITEM _mb_rest_weak "${_w}")
  elseif(_si GREATER -1)
    mb_plain_obj("${MB_ROOT}/${tu}" _o)
    list(APPEND MB_ORDERED "${_o}")
    list(REMOVE_ITEM _mb_rest "${MB_ROOT}/${tu}")
  endif()
endforeach()
list(APPEND MB_ORDERED ${_mb_rest_weak})                 # TUs newer than tu_order.txt: last
foreach(src ${_mb_rest})
  mb_plain_obj("${src}" _o)
  list(APPEND MB_ORDERED "${_o}")
endforeach()
set_source_files_properties(${MB_ORDERED} PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)
add_executable(mb_native ${MB_ORDERED})
add_dependencies(mb_native mb_objs)
target_link_libraries(mb_native PRIVATE mb_src_flags mb_native_src ${MB_NATIVE_LIBS} mb_runtime)
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

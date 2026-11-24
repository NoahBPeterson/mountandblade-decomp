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
# GameApp::GameApp, which sets g_module_name, before options_dialog_and_module_tables.cpp constructed g_module_name and emptied it again
# ("Unable to find module info file", 2026-11-21).
file(STRINGS "${PORT}/link/tu_order.txt" _mb_order)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${PORT}/link/tu_order.txt")
function(mb_plain_obj src out)          # the object mb_objs compiles src to (CMake's path for a source outside the tree)
  # CMake names an object after its source with CMAKE_CXX_OUTPUT_EXTENSION: .o, .obj on a Windows target (COFF).
  string(REGEX REPLACE "^/" "" _rel "${src}")
  set(${out} "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/mb_objs.dir/${_rel}${CMAKE_CXX_OUTPUT_EXTENSION}" PARENT_SCOPE)
endfunction()
set(MB_ORDERED "")
set(_mb_rest ${MB_PLAIN_SOURCES})
set(_mb_rest_weak ${MB_WEAK_OBJS})
foreach(tu ${_mb_order})
  string(MAKE_C_IDENTIFIER "${tu}" _id)
  set(_w "${CMAKE_CURRENT_BINARY_DIR}/weak/${_id}${CMAKE_CXX_OUTPUT_EXTENSION}")
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

# ELF's flat namespace binds a reference to the first definition in link order, and the src/ objects precede the
# dep archives. port/stl (the VC7.1-shaped STL, -nostdinc++) emits members under the same mangled names libstdc++
# uses; where both a src object and a dep emit one out of line, the dep's call binds to port/stl's implementation,
# which operates on the MSVC object layout, not libstdc++'s. Exactly one symbol collides on linux-x86_64 (the
# 2026-11-23 run-gate segfault): std::vector<int>::resize(unsigned long). port/stl's vector is _Alval + 3 pointers
# (32 bytes, pointers at 8/16/24); libstdc++'s is 3 pointers (24 bytes, at 0/8/16). glslang built against libstdc++
# emits resize out of line (libc++ inlines it, so macOS never hits this); its call bound to port/stl's resize, which
# left _M_start null, and memberRemapper[...][i] wrote to nil (GlslangToSpv.cpp:6499, via mbg::compile_shader in
# Device::init). Rename the symbol in every src object, definitions and references alike: src/ keeps port/stl's
# implementation under a private name, the deps bind to their own archives' libstdc++ definition. The originals are
# untouched (link_tables.py / burndown.py read them). Only the one-arg resizeEm: aliases.ld PROVIDEs the two-arg
# resizeEm<T> and vector<int>::operator=, whose manglings differ and must keep their names.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(_mb_isolated "")
  foreach(_o ${MB_ORDERED})
    string(MAKE_C_IDENTIFIER "${_o}" _oid)
    set(_iso "${CMAKE_CURRENT_BINARY_DIR}/isolated/${_oid}.o")
    get_filename_component(_oname "${_o}" NAME)
    add_custom_command(OUTPUT "${_iso}"
      COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/isolated"
      COMMAND ${MB_OBJCOPY} "--redefine-sym=_ZNSt6vectorIiSaIiEE6resizeEm=_ZNSt6vectorIiSaIiEE14mb_port_resizeEm" "${_o}" "${_iso}"
      DEPENDS "${_o}"
      COMMENT "isolate port/stl vector<int>::resize: ${_oname}" VERBATIM)
    list(APPEND _mb_isolated "${_iso}")
  endforeach()
  set(MB_ORDERED ${_mb_isolated})
endif()

set_source_files_properties(${MB_ORDERED} PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)
add_executable(mb_native ${MB_ORDERED})
add_dependencies(mb_native mb_objs)
# COFF: zig's lld-link driver rejects /WHOLEARCHIVE:<path> ("unsupported linker arg") but accepts -Wl,--whole-archive,
# which it applies to the archive that FOLLOWS it (probe work/winprobe2/waprobe: an unreferenced member's constructor
# ran for the flagged archive only). Link items starting with '-' pass through in place, so each subsystem archive gets
# its own flag right ahead of it; the dep archives and -l import libs after them stay selective — kernel32's thunks must
# not be loaded whole over the platform lane's own Win32 definitions.
set(MB_NATIVE_LINK_LIBS ${MB_NATIVE_LIBS})
if(WIN32)
  set(MB_NATIVE_LINK_LIBS "")
  foreach(l ${MB_NATIVE_LIBS})
    list(APPEND MB_NATIVE_LINK_LIBS "-Wl,--whole-archive" ${l})
  endforeach()
endif()
target_link_libraries(mb_native PRIVATE mb_src_flags mb_native_src ${MB_NATIVE_LINK_LIBS} mb_runtime)
set_target_properties(mb_native PROPERTIES LINKER_LANGUAGE CXX OUTPUT_NAME mb RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# aliases.ld's PROVIDE(alias = target): ld64 has no linker scripts; -alias makes `alias` another name of `target`
# (Mach-O C symbols carry a leading underscore). GNU ld (Linux x86_64) takes the script as is. COFF (Windows x86_64) has
# no linker scripts either: /alternatename gives the same PROVIDE semantics (define `alias` from `target` only while
# nothing defines it). The whole-archive loading is done above, with -Wl,--whole-archive link items.
file(STRINGS "${MB_LINK}/aliases.ld" _mb_alias_lines REGEX "^PROVIDE\\(")
if(WIN32)
  # traps.cpp's stubs, and the aliases that bind aliases.ld's PROVIDE entries to them (see below).
  target_sources(mb_native_src PRIVATE "${MB_LINK}/traps.cpp")
  # zig's lld-link driver rejects /alternatename on the command line in every spelling (-Wl,/... , -Wl,-... ,
  # -Xlinker, bare), but PARSES it inside a .drectve section with exactly PROVIDE semantics (probed
  # work/winprobe2/anprobe: an alias-only reference forwards to the target; a strong definition still wins).
  # Emit the PROVIDE entries as one file-scope asm .drectve object compiled into mb_native_src. clang's
  # #pragma comment(linker) does NOT emit .drectve on windows-gnu, and the IAS rejects single-quoted .ascii.
  set(_mb_drectve "${CMAKE_CURRENT_BINARY_DIR}/link/mb_aliases_drectve.cpp")
  set(_mb_drectve_body "__asm__(\n  \".section .drectve,\\\"n\\\"\\n\"\n")
  foreach(l ${_mb_alias_lines})
    string(REGEX MATCH "^PROVIDE\\(([^ =]+) = ([^ )]+)\\);" _m "${l}")
    if(_m)
      string(APPEND _mb_drectve_body "  \".ascii \\\"/alternatename:${CMAKE_MATCH_1}=${CMAKE_MATCH_2} \\\"\\n\"\n")
    endif()
  endforeach()
  string(APPEND _mb_drectve_body ");\n")
  file(WRITE "${_mb_drectve}" "${_mb_drectve_body}")
  target_sources(mb_native_src PRIVATE "${_mb_drectve}")
  add_dependencies(mb_native mb_winimports)   # the partial DATA-import archives mb_platform's interface links
elseif(APPLE)
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

# ---- install ---------------------------------------------------------------------------------------------------------
# `cmake --install <build> --prefix <dir>` (or `ninja install`, DESTDIR honoured). Unix: the executable goes to
# <prefix>/libexec/mountandblade/mb and <prefix>/bin/mountandblade is a launcher that cd's into the user's own game
# folder first (the game's paths are relative to it). Windows: mb.exe and mountandblade.bat go to <prefix>/bin.
# No game files are installed.
include(GNUInstallDirs)
if(WIN32)
  install(TARGETS mb_native RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(PROGRAMS ${PORT}/install/mountandblade.bat DESTINATION ${CMAKE_INSTALL_BINDIR})
else()
  install(TARGETS mb_native RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/mountandblade)
  file(RELATIVE_PATH MB_LAUNCHER_REL_EXE "${CMAKE_INSTALL_FULL_BINDIR}" "${CMAKE_INSTALL_FULL_LIBEXECDIR}/mountandblade/mb")
  configure_file(${PORT}/install/mountandblade.in "${CMAKE_BINARY_DIR}/mountandblade" @ONLY)
  install(PROGRAMS "${CMAKE_BINARY_DIR}/mountandblade" DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

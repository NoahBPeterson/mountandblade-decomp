# port/platform/CMakeLists.txt: mb_platform, the Win32 USER/KERNEL/GDI/shell/registry/Winsock subset + DirectInput 8 over SDL3
# and POSIX (docs/port_lanes.md, ticket 007). Tests: port/platform/tests.
find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_ttf CONFIG REQUIRED)

set(MB_PLATFORM_SOURCES core.cpp fs.cpp registry.cpp window.cpp dialog.cpp gdi.cpp clipboard.cpp com.cpp net.cpp keymap.cpp)
if(NOT WIN32)
  list(APPEND MB_PLATFORM_SOURCES dinput.cpp)   # WIN32: dinput.cpp is weakened separately below (GUID dups vs libSDL3.a)
endif()
add_library(mb_platform STATIC ${MB_PLATFORM_SOURCES})
# net_native.cpp talks to the native socket API, whose headers cannot meet the portable shims (port/include's windows.h,
# oleauto.h ... shadow the SDK's and the two declaration sets cannot coexist): its own object library, without port/include
# and without the force-included prelude, so <winsock2.h> is the SDK's own header.
add_library(mb_net_native OBJECT net_native.cpp)
target_compile_options(mb_net_native PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++17> -fno-strict-aliasing -Wall -Wno-unused-function -Wno-unused-parameter)
target_sources(mb_platform PRIVATE $<TARGET_OBJECTS:mb_net_native>)
target_include_directories(mb_platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(mb_platform PRIVATE ${PORT}/include)
target_compile_options(mb_platform PRIVATE
    -include ${CMAKE_CURRENT_SOURCE_DIR}/mbp_prelude.h
    $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++17> -fno-strict-aliasing -Wall -Wno-unused-function -Wno-unused-variable
    -Wno-unknown-pragmas -Wno-invalid-offsetof -Wno-address-of-temporary -Wno-null-dereference -Wno-unused-parameter)
target_link_libraries(mb_platform PRIVATE pthread)
if(WIN32)
  target_link_libraries(mb_platform PRIVATE ws2_32)     # net_native.cpp's sockets

  # dinput.cpp's GUID_SysKeyboard / GUID_SysMouse / IID_IDirectInput8A are also defined by libSDL3.a
  # (SDL_windowsjoystick.c.obj) -- identical standard constants. zig's COFF linker has no first-wins, and
  # __declspec(selectany) still duplicates against a strong definition (probed work/winprobe2/anprobe). coff.py
  # weaken gives owner-wins, the same mechanism mb_weaken uses for src/ TUs: dinput keeps a local copy, its
  # references bind to SDL3's. dinput.cpp is compiled as its own object library so the object path is known.
  add_library(mbp_dinput OBJECT dinput.cpp)
  target_include_directories(mbp_dinput PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${PORT}/include)
  target_link_libraries(mbp_dinput PRIVATE SDL3::SDL3)
  target_compile_options(mbp_dinput PRIVATE
      -include ${CMAKE_CURRENT_SOURCE_DIR}/mbp_prelude.h
      $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++17> -fno-strict-aliasing -Wall -Wno-unused-function -Wno-unused-variable
      -Wno-unknown-pragmas -Wno-invalid-offsetof -Wno-address-of-temporary -Wno-null-dereference -Wno-unused-parameter)
  set(_mbp_dinput_weak "${CMAKE_CURRENT_BINARY_DIR}/weak/dinput${CMAKE_CXX_OUTPUT_EXTENSION}")
  add_custom_command(OUTPUT "${_mbp_dinput_weak}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/weak"
    COMMAND ${MB_PYTHON} "${MB_ROOT}/tools/port/coff.py" weaken "$<TARGET_OBJECTS:mbp_dinput>" "${_mbp_dinput_weak}"
            GUID_SysKeyboard GUID_SysMouse IID_IDirectInput8A
    DEPENDS mbp_dinput "$<TARGET_OBJECTS:mbp_dinput>"
    COMMENT "weaken dinput.cpp: 3 DirectInput GUIDs owned by libSDL3.a (identical constants)" VERBATIM)
  add_custom_target(mbp_dinput_weak DEPENDS "${_mbp_dinput_weak}")
  add_dependencies(mb_platform mbp_dinput_weak)
  set_source_files_properties("${_mbp_dinput_weak}" PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)
  target_sources(mb_platform PRIVATE "${_mbp_dinput_weak}")

  # Partial DATA-import archives: SDL3's dllimport (__imp_X) references would otherwise pull zig's synthesized
  # import member whose bare X thunk collides with mb_platform's strong pseudo-Win32 X. winimports.py exports
  # exactly the platform-defined symbols as DATA (only __imp_X, no bare thunk); linked as explicit items ahead of
  # the -l flags so bare calls bind to the platform and __imp calls to the real DLL. See tools/port/winimports.py.
  set(MB_WINIMPORT_LIBS kernel32 user32 gdi32 winmm imm32 ole32 oleaut32 version advapi32 setupapi shell32
      dinput8 usp10 rpcrt4 ws2_32 winspool comdlg32)
  set(MB_WINIMPORT_DIR "${CMAKE_BINARY_DIR}/link/winimports")
  set(MB_WINIMPORT_ARCHIVES "")
  foreach(n ${MB_WINIMPORT_LIBS})
    list(APPEND MB_WINIMPORT_ARCHIVES "${MB_WINIMPORT_DIR}/${n}.a")
  endforeach()
  add_custom_command(OUTPUT ${MB_WINIMPORT_ARCHIVES}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${MB_WINIMPORT_DIR}"
    COMMAND ${MB_PYTHON} "${MB_ROOT}/tools/port/winimports.py"
            --platform "$<TARGET_FILE:mb_platform>" --outdir "${MB_WINIMPORT_DIR}" --ar "${CMAKE_AR}"
    DEPENDS mb_platform "$<TARGET_FILE:mb_platform>" "${MB_ROOT}/tools/port/winimports.py"
    COMMENT "winimports: partial DATA-import archives for the platform-owned Win32 symbols" VERBATIM)
  add_custom_target(mb_winimports DEPENDS ${MB_WINIMPORT_ARCHIVES})
  # the partial archives must precede SDL3 and the -l import flags in every consumer's link line
  target_link_libraries(mb_platform PUBLIC ${MB_WINIMPORT_ARCHIVES} SDL3::SDL3 SDL3_ttf::SDL3_ttf)
else()
  target_link_libraries(mb_platform PUBLIC SDL3::SDL3 SDL3_ttf::SDL3_ttf)
endif()

# tests: a WndProc app on the API, DirectInput, the path layer, threads and events
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
  add_subdirectory(tests)
endif()

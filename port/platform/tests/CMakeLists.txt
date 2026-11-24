# port/platform/tests: programs built on the platform API (windows + messages + DirectInput; files / threads / registry /
# GDI / dialogs; sockets). Run: ctest, or the executables directly (PASS / FAIL lines, exit status = failure count).
set(MBP_TEST_OPTS -include ${CMAKE_CURRENT_SOURCE_DIR}/../mbp_prelude.h -std=gnu++17 -Wno-everything)

add_executable(mbp_test_window test_window.cpp)
add_executable(mbp_test_kernel test_kernel.cpp $<TARGET_OBJECTS:mb_runtime>)
if(WIN32)
  # net_server.cpp talks to the native socket API, whose headers cannot meet the portable shims (the same reason
  # mb_net_native is its own object library in port/platform/CMakeLists.txt): without port/include and without the
  # force-included prelude, <winsock2.h> is the SDK's own header.
  add_library(mbp_net_server OBJECT net_server.cpp)
  target_compile_options(mbp_net_server PRIVATE -std=gnu++17 -Wno-everything)
  add_executable(mbp_test_net test_net.cpp $<TARGET_OBJECTS:mbp_net_server>)
  target_link_libraries(mbp_test_net PRIVATE ws2_32)
else()
  add_executable(mbp_test_net test_net.cpp net_server.cpp)
endif()
foreach(t mbp_test_window mbp_test_kernel mbp_test_net)
  target_link_libraries(${t} PRIVATE mb_platform)
  target_include_directories(${t} PRIVATE ${PORT}/include)
  target_compile_options(${t} PRIVATE ${MBP_TEST_OPTS})
  if(WIN32)
    add_dependencies(${t} mb_winimports)   # the partial DATA-import archives mb_platform's interface links
  endif()
endforeach()

enable_testing()
if(CMAKE_HOST_SYSTEM_NAME STREQUAL CMAKE_SYSTEM_NAME)
  add_test(NAME mbp_window COMMAND mbp_test_window)
  add_test(NAME mbp_kernel COMMAND mbp_test_kernel ${MB_ROOT}/game)
  add_test(NAME mbp_net COMMAND mbp_test_net)
else()
  # Cross-built from a Unix host (the port's macOS/Linux lanes): the executables are PE binaries, ctest cannot exec them
  # directly - through Wine, the binary director that runs them on macOS/Linux CI hosts (REGRESSION: matches the lane gate).
  # Wine for the cross-built PEs. NB a Wine .app ships the real loader at Contents/Resources/wine/bin/wine and a
  # launcher at Contents/MacOS/wine; the 11.21 tarball's launcher is a compiled AppKit "document opener" that launches
  # each argument separately, dropping the program's argv (the game directory never reaches the test) and spawning
  # explorer.exe windows for the stray items, so the loader (which forwards argv correctly) is hinted first and the
  # launcher path is the last resort. (The AGENT=1 / MANPATH=... strings once seen after the program name in process
  # listings were a pgrep artifact of wine's ps-title rewrite, not program arguments.)
  find_program(MB_WINE NAMES wine HINTS /opt/homebrew/bin /opt/local/bin /usr/local/bin
                             "$ENV{HOME}/wine1121/Wine Devel.app/Contents/Resources/wine/bin"
                             "$ENV{HOME}/wine1121/Wine Devel.app/Contents/MacOS" REQUIRED)
  # Loader split: the TESTS run on the cask's 11.22 loader (nothing here needs Vulkan, and the 11.21 loader dies
  # mid-mbp_kernel with a fatal Rosetta translation error after the file-layer checks — 16/121). The RUN gate pins
  # the 11.21 loader (tools/port/run_native.py): 11.22's vkCreateInstance faults (docs/c2scope_evaluation.md).
  # The tests run on their OWN Wine prefix: ctest must not touch the user's ~/.wine — stale or wrong-version
  # wineservers on /tmp/.wine-<uid> hang clients in wine's startup (mbp_kernel sat 32 idle minutes inside
  # __wine_main). A bare wineboot prefix is enough: the tests sandbox their data in MB_USER_DIR and the port's
  # registry is file-backed (registry.ini). Initialized once at configure; physics/CMakeLists.txt shares it.
  set(MB_TEST_WINEPREFIX "${CMAKE_BINARY_DIR}/wine-prefix" CACHE PATH "Wine prefix for the cross-built tests")
  if(NOT EXISTS "${MB_TEST_WINEPREFIX}/system.reg")
    message(STATUS "initializing the test Wine prefix ${MB_TEST_WINEPREFIX} ...")
    execute_process(COMMAND ${CMAKE_COMMAND} -E env WINEPREFIX=${MB_TEST_WINEPREFIX}
                    "${MB_WINE}" wineboot --init RESULT_VARIABLE _mb_boot TIMEOUT 300 OUTPUT_QUIET ERROR_QUIET)
    if(NOT _mb_boot EQUAL 0)
      message(FATAL_ERROR "wineboot --init failed for the test prefix (rc=${_mb_boot})")
    endif()
  endif()
  add_test(NAME mbp_window COMMAND ${CMAKE_COMMAND} -E env WINEPREFIX=${MB_TEST_WINEPREFIX} ${MB_WINE} $<TARGET_FILE:mbp_test_window>)
  add_test(NAME mbp_kernel COMMAND ${CMAKE_COMMAND} -E env WINEPREFIX=${MB_TEST_WINEPREFIX} ${MB_WINE} $<TARGET_FILE:mbp_test_kernel> ${MB_ROOT}/game)
  add_test(NAME mbp_net COMMAND ${CMAKE_COMMAND} -E env WINEPREFIX=${MB_TEST_WINEPREFIX} ${MB_WINE} $<TARGET_FILE:mbp_test_net>)
endif()

# port/platform/tests: programs built on the platform API (windows + messages + DirectInput; files / threads / registry /
# GDI / dialogs; sockets). Run: ctest, or the executables directly (PASS / FAIL lines, exit status = failure count).
set(MBP_TEST_OPTS -include ${CMAKE_CURRENT_SOURCE_DIR}/../mbp_prelude.h -std=gnu++17 -Wno-everything)

add_executable(mbp_test_window test_window.cpp)
add_executable(mbp_test_kernel test_kernel.cpp $<TARGET_OBJECTS:mb_runtime>)
add_executable(mbp_test_net test_net.cpp net_server.cpp)
foreach(t mbp_test_window mbp_test_kernel mbp_test_net)
  target_link_libraries(${t} PRIVATE mb_platform)
  target_include_directories(${t} PRIVATE ${PORT}/include)
  target_compile_options(${t} PRIVATE ${MBP_TEST_OPTS})
endforeach()

enable_testing()
add_test(NAME mbp_window COMMAND mbp_test_window)
add_test(NAME mbp_kernel COMMAND mbp_test_kernel ${MB_ROOT}/game)
add_test(NAME mbp_net COMMAND mbp_test_net)

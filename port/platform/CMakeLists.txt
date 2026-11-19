# port/platform/CMakeLists.txt: mb_platform, the Win32 USER/KERNEL/GDI/shell/registry/Winsock subset + DirectInput 8 over SDL3
# and POSIX (docs/port_lanes.md, ticket 007). Tests: port/platform/tests.
find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_ttf CONFIG REQUIRED)

add_library(mb_platform STATIC
    core.cpp fs.cpp registry.cpp window.cpp dialog.cpp gdi.cpp clipboard.cpp com.cpp net.cpp net_native.cpp dinput.cpp keymap.cpp)
target_include_directories(mb_platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(mb_platform PRIVATE ${PORT}/include)
target_compile_options(mb_platform PRIVATE
    -include ${CMAKE_CURRENT_SOURCE_DIR}/mbp_prelude.h
    $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++17> -fno-strict-aliasing -Wall -Wno-unused-function -Wno-unused-variable
    -Wno-unknown-pragmas -Wno-invalid-offsetof -Wno-address-of-temporary -Wno-null-dereference -Wno-unused-parameter)
target_link_libraries(mb_platform PUBLIC SDL3::SDL3 SDL3_ttf::SDL3_ttf)
target_link_libraries(mb_platform PRIVATE pthread)

# tests: a WndProc app on the API, DirectInput, the path layer, threads and events
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
  add_subdirectory(tests)
endif()

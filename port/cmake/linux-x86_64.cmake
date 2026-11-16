# port/cmake/linux-x86_64.cmake: the 64-bit Linux target (ticket 006). It configures and compiles today so the burn-down
# can count it; the reconstruction's 32-bit layout asserts fail here by design until 006 replaces the hand-laid records.
#   cmake -S port -B build/port/linux-x86_64 -G Ninja -DCMAKE_TOOLCHAIN_FILE=port/cmake/linux-x86_64.cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_C_COMPILER   "${CMAKE_CURRENT_LIST_DIR}/zig-cc")
set(CMAKE_CXX_COMPILER "${CMAKE_CURRENT_LIST_DIR}/zig-c++")
set(CMAKE_C_FLAGS_INIT "-target x86_64-linux-gnu")
set(CMAKE_CXX_FLAGS_INIT "-target x86_64-linux-gnu")
set(MB_TARGET "linux-x86_64" CACHE STRING "port target" FORCE)

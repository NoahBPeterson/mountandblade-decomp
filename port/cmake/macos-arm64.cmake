# port/cmake/macos-arm64.cmake: the native macOS target (Apple silicon), the one this repo's Mac can run. 64-bit, so
# the reconstruction's 32-bit layout asserts fail here until ticket 006 replaces the hand-laid records.
#   cmake -S port -B build/port/macos-arm64 -G Ninja -DCMAKE_TOOLCHAIN_FILE=port/cmake/macos-arm64.cmake
set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR arm64)
set(CMAKE_OSX_ARCHITECTURES arm64)
set(CMAKE_C_COMPILER   /usr/bin/clang)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)
set(MB_TARGET "macos-arm64" CACHE STRING "port target" FORCE)

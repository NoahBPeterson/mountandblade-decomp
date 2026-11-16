# port/cmake/linux-i686.cmake: toolchain for the stepping-stone target of ticket 004, i686 Linux (glibc), 32-bit pointers
# so the reconstruction's 32-bit layouts (size asserts, offset comments) still hold. Works on any host with zig on PATH
# (zig bundles clang plus the i686 glibc headers/libs); on a Linux host a plain `clang -m32` with multilib works too
# (pass -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS=-m32 instead of this file).
#   cmake -S port -B build/port/linux-i686 -G Ninja -DCMAKE_TOOLCHAIN_FILE=port/cmake/linux-i686.cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR i686)
set(CMAKE_C_COMPILER   "${CMAKE_CURRENT_LIST_DIR}/zig-cc")
set(CMAKE_CXX_COMPILER "${CMAKE_CURRENT_LIST_DIR}/zig-c++")
set(MB_TARGET "linux-i686" CACHE STRING "port target" FORCE)

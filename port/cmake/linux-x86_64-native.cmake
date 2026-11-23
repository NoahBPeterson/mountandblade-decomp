# port/cmake/linux-x86_64-native.cmake: the 64-bit Linux target built NATIVELY inside an amd64 Linux machine
# (OrbStack mb64, ubuntu 24.04, clang 18). zig's glibc sysroot has no X11 headers, so this lane compiles there,
# not cross from the Mac; the Mac home is mounted at identical paths, so tools/port/link_tables.py and the
# run gate read the same build dir from either side. Dependencies: tools/port/deps.py --target linux-x86_64 --native.
#   (inside mb64)
#   cmake -S port -B build/port/linux-x86_64 -G Ninja -DCMAKE_TOOLCHAIN_FILE=port/cmake/linux-x86_64-native.cmake \
#         -DCMAKE_PREFIX_PATH=$PWD/work/port_deps/linux-x86_64/install \
#         -DMB_JOLT_SOURCE_DIR=$PWD/work/port_deps/src/JoltPhysics-v5.5.0 \
#         -DZLIB_ROOT=$PWD/work/port_deps/linux-x86_64/install -DZLIB_USE_STATIC_LIBS=ON \
#         -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
# No CMAKE_SYSTEM_NAME: this is a native build, not a cross. lld because aliases.ld is linked as -T
# (port/cmake/native_link.cmake): GNU ld's -T REPLACES its default linker script, lld's augments it.
set(CMAKE_C_COMPILER   clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld")
set(MB_TARGET "linux-x86_64" CACHE STRING "port target" FORCE)
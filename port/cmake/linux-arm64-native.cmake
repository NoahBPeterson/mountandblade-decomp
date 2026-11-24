# port/cmake/linux-arm64-native.cmake: the 64-bit ARM Linux target, built NATIVELY on an aarch64 Linux machine (the
# OrbStack arm64 machine mbarm, ubuntu 24.04, clang 18; or any aarch64 box with the packages in docs/BUILDING_PORT.md).
# Same shape as linux-x86_64-native.cmake: zig's glibc sysroot has no X11 headers, so this lane does not cross-compile.
# Dependencies: tools/port/deps.py --target linux-arm64 --native.
#   (on the aarch64 machine)
#   cmake -S port -B build/port/linux-arm64 -G Ninja -DCMAKE_TOOLCHAIN_FILE=port/cmake/linux-arm64-native.cmake \
#         -DCMAKE_PREFIX_PATH=$PWD/work/port_deps/linux-arm64/install \
#         -DMB_JOLT_SOURCE_DIR=$PWD/work/port_deps/src/JoltPhysics-v5.5.0 \
#         -DZLIB_ROOT=$PWD/work/port_deps/linux-arm64/install -DZLIB_USE_STATIC_LIBS=ON \
#         -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
# No CMAKE_SYSTEM_NAME: a native build. lld because aliases.ld is linked as -T (port/cmake/native_link.cmake): GNU ld's
# -T REPLACES its default linker script, lld's augments it. Baseline armv8-a: no -mcpu, so the binary runs on any arm64.
set(CMAKE_C_COMPILER   clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld")
set(MB_TARGET "linux-arm64" CACHE STRING "port target" FORCE)

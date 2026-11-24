# port/cmake/windows-x86_64.cmake: the 64-bit Windows target, cross-built from the Mac with zig's bundled clang and
# mingw-w64 sysroot (x86_64-windows-gnu, Itanium ABI like the other ports — not MSVC). Runs under Wine/CrossOver.
# Dependencies: tools/port/deps.py --target windows-x86_64 (same toolchain). The port keeps its architecture on Windows:
# port/include's shims come BEFORE the sysroot's real windows.h, port/stl replaces the C++ library (-nostdinc++), and the
# subsystem lanes stay SDL3-based; only genuinely POSIX internals get _WIN32 branches.
#   cmake -S port -B build/port/windows-x86_64 -G Ninja \
#         -DCMAKE_TOOLCHAIN_FILE=$PWD/port/cmake/windows-x86_64.cmake \
#         -DCMAKE_PREFIX_PATH=$PWD/work/port_deps/windows-x86_64/install \
#         -DMB_JOLT_SOURCE_DIR=$PWD/work/port_deps/src/JoltPhysics-v5.5.0 \
#         -DZLIB_ROOT=$PWD/work/port_deps/windows-x86_64/install -DZLIB_USE_STATIC_LIBS=ON \
#         -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)
set(CMAKE_C_COMPILER   "${CMAKE_CURRENT_LIST_DIR}/zig-cc")
set(CMAKE_CXX_COMPILER "${CMAKE_CURRENT_LIST_DIR}/zig-c++")
set(CMAKE_RC_COMPILER  "${CMAKE_CURRENT_LIST_DIR}/zig-rc")
# x86-64-v3 (AVX2/BMI/FMA). zig's driver does not accept clang's -march=x86-64-v3 spelling; its equivalent is
# -mcpu=x86_64_v3. Only this lane gets it: the OrbStack linux-x86_64 VM has no AVX2, and Rosetta miscompiles VEX
# codegen on macos-x86_64 (see the ragdoll regression in 826de59e).
set(CMAKE_C_FLAGS_INIT   "-target x86_64-windows-gnu -mcpu=x86_64_v3")
set(CMAKE_CXX_FLAGS_INIT "-target x86_64-windows-gnu -mcpu=x86_64_v3")
# COFF archives: the host's (macOS) ar/ranlib cannot index them ("not a mach-o file") and silently write EMPTY archives;
# llvm's can. FORCE: a plain set() loses to a CMAKE_AR cache entry left by an earlier configure.
find_program(MB_LLVM_AR NAMES llvm-ar HINTS /opt/homebrew/opt/llvm/bin /usr/local/opt/llvm/bin REQUIRED)
find_program(MB_LLVM_RANLIB NAMES llvm-ranlib HINTS /opt/homebrew/opt/llvm/bin /usr/local/opt/llvm/bin REQUIRED)
set(CMAKE_AR "${MB_LLVM_AR}" CACHE FILEPATH "COFF archiver" FORCE)
set(CMAKE_RANLIB "${MB_LLVM_RANLIB}" CACHE FILEPATH "COFF archive indexer" FORCE)
set(CMAKE_C_COMPILER_AR "${MB_LLVM_AR}" CACHE FILEPATH "COFF archiver (C)" FORCE)
set(CMAKE_CXX_COMPILER_AR "${MB_LLVM_AR}" CACHE FILEPATH "COFF archiver (C++)" FORCE)
set(MB_TARGET "windows-x86_64" CACHE STRING "port target" FORCE)
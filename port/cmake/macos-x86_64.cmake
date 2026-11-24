# port/cmake/macos-x86_64.cmake: the Intel macOS target, cross-built from Apple silicon with the same Apple clang and
# run under Rosetta. Dependencies come from tools/port/deps.py (brew's are arm64-only):
#   .venv/bin/python tools/port/deps.py --target macos-x86_64
#   cmake -S port -B build/port/macos-x86_64 -G Ninja -DCMAKE_TOOLCHAIN_FILE=port/cmake/macos-x86_64.cmake \
#         -DCMAKE_PREFIX_PATH=$PWD/work/port_deps/macos-x86_64/install
set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_OSX_ARCHITECTURES x86_64)
# Deployment target: without it the binary is stamped with the build machine's SDK (minos 26) and refuses to start on
# anything older. Keep equal to the value in tools/port/deps.py (the static deps).
set(CMAKE_OSX_DEPLOYMENT_TARGET 12.0 CACHE STRING "minimum macOS" FORCE)
set(CMAKE_C_COMPILER   /usr/bin/clang)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)
set(MB_TARGET "macos-x86_64" CACHE STRING "port target" FORCE)
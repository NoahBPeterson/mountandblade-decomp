# Port lanes (2026-11-19): a native Linux / macOS build that runs the full game

Goal: `mb` built by CMake (port/) runs Mount & Blade 1.011 natively on macOS arm64 (this repo's Mac) and Linux x86_64,
on the user's game data (`game/` in the repo root: Data/, Textures/, Sounds/, music/, mb.fx, *.pp ...). No stubs: every
API src/ calls gets a REAL implementation. A placeholder that returns failure is not progress (2026-11-19: the i686
build "linked" on placeholders and could not open a window; that was rejected).

Architecture: src/ is unchanged and keeps calling the APIs the original called; port/ implements those APIs natively.
The MSVC byte-exact flow (5011 rows) must not move. The shim headers in port/include define the interfaces.

| lane | dir | implements | over |
|---|---|---|---|
| platform | port/platform | Win32 USER/KERNEL/GDI/shell/registry subset (window, message loop -> WM_*, cursor, clipboard, files, paths, time, threads, events, MessageBox), DirectInput8 keyboard + mouse, the CRT file layer (backslash paths, case-insensitive lookup) | SDL3, POSIX |
| gfx | port/gfx | IDirect3D9 / IDirect3DDevice9 and its resources (textures incl. DXT, VB/IB, surfaces, render targets, vertex declarations, shaders, states), D3DX subset (texture load/save, effects for mb.fx, shader assembly for *.pp, matrix helpers) | SDL_GPU (Metal on macOS, Vulkan on Linux); shaders via glslang (HLSL -> SPIR-V) + SPIRV-Cross (-> MSL) |
| audio | port/audio | FMOD 3.75 subset src/ calls | SDL3 audio streams, stb_vorbis (assets are Ogg + WAV) |
| physics | port/physics | Newton 1.x subset src/ calls (ragdolls, bodies, collision) | Jolt Physics |
| layout64 | src/ | ticket 006: 32-bit hand-laid records made correct at 64 bits | real C++ types |

The surface each lane must cover is exactly what src/ calls: see port/stubs/*.cpp (the placeholder list being replaced),
port/include/*.h, and `rg` over src/. Each lane: library `mb_<lane>` in port/<lane>/CMakeLists.txt (the top-level
port/CMakeLists.txt adds the directory), tests in port/<lane>/tests, built with
`cmake -S port -B build/port/macos-arm64 -G Ninja -DCMAKE_TOOLCHAIN_FILE=$PWD/port/cmake/macos-arm64.cmake`.
Building the result from source and installing it: [`BUILDING_PORT.md`](BUILDING_PORT.md).
Integration (linking `mb` on macOS, port/main.cpp, removing port/stubs) is the main session's.

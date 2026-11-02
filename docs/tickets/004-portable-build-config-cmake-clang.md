# 004 A second build configuration: CMake + clang + platform STL, one source tree

Area: port | Effort: M | Impact: very high | Depends on: 002; 003 for linking (compiling can start now)

## Summary

Add `CMakeLists.txt` that builds `src/` with clang using the **platform's** standard library. The MSVC 7.1 flow
(`tools/matching/cl.sh`, `tools/relink/build.py`) stays exactly as is and remains the only thing the harness and byte-exact
checks see. No `#ifdef` in `src/` (docs/patches.md: even an unused branch can change faithful codegen), so the portable config adapts
through a force-included prelude and shim include directories.

## Evidence / constraints

- 266 `__declspec(noinline)`, 54 `dllimport`, 931 calling-convention keywords (556 `__cdecl`, 204 `__stdcall`, 160 `__fastcall`, 11 `__thiscall`),
  712 `__int64`, 20 `__asm` (WL_MARKER in `src/include/wl_marker.h`, one `naked` in `options_panel/rec_slots_tr.cpp:107`),
  60 `#pragma comment(linker, "/alternatename:...")` (shapes.cpp:20-27, fx_anim.cpp:23-34 ...): see ticket 005.
- Layout hacks that hurt on a new STL: `((std::vector<int>*)((char*)g_game + 0x1a1b8))->resize(0, 0)` (mission_5.cpp), hand views using
  `_Myfirst/_Mylast/_Myend` (287 refs, 66 files). Per the maintainer these are not preserved: the fix is making the fields real `std::vector`
  members (ticket 006), which the memory note "idiomatic shared types" already prefers.
- Windows headers: 84 TUs `#include <windows.h>`, 49 `<d3d9.h>`.

## Detailed plan

1. `port/` directory (new, outside `src/`, `tools/`): `port/include/` shims (`windows.h` minimal types: `DWORD`, `HRESULT`, `RECT`, `POINT`, `CRITICAL_SECTION`
   mapped to `mbp::Mutex`; `d3d9.h` -> temporary shim of the types the renderer headers use until 008 removes them), `port_prelude.h`, and
   `port/*.cpp` implementations (ticket 007).
2. CMake targets: `mb_game` (src/game, src/rgl, src/tinyxml), `mb_port` (SDL3 platform, audio, physics adapter, renderer), `mb` exe.
   `d3dapp.cpp` and `options_dlg.cpp` are replaced by port equivalents (the SDK framework is the window/device owner).
3. Flags: `-std=gnu++17` (C++03 sources compile as 17 except removed features: `register`, `auto_ptr`, dynamic exception specs; fix in place if
   the MSVC build tolerates it), `-fms-extensions` (accepts `__declspec`, `i64`), `-fno-strict-aliasing` (the code type-puns constantly),
   `-fwrapv`, `-fno-delete-null-pointer-checks`. No `-ffp-contract` games: precision parity is out of scope.
4. Targets via toolchain files: `linux-x86_64`, `macos-arm64`, `macos-x86_64`, `windows-x64` (clang-cl or mingw), and the stepping-stone
   `linux-i686` (`-m32`) for bring-up before 006.
5. CI: GitHub Actions matrix; first goal is "compiles" counted by ticket 002's probe, then "links", then "boots".
6. The 4,585-line `script_interpreter.cpp` and similar giant TUs: split nothing; just check compile time (the MSVC flow already copes).

## Code sketch

Skeleton: `patches/port_sketch/CMakeLists.txt`, prelude: `patches/port_sketch/port_prelude.h` (both untested, not wired).

```cmake
target_compile_options(mb_game PRIVATE -include ${PORT}/port_prelude.h -fms-extensions -fno-strict-aliasing -fwrapv -Wno-everything)
target_include_directories(mb_game BEFORE PRIVATE ${PORT}/include ${SRC} ${SRC}/include)
```

## Risks

- `-fms-compatibility` changes name lookup in templates; start without it, add only if needed.
- The prelude must never be applied to the MSVC build (separate flow, separate flags) so byte-exact stays untouched.
- Giant functions (0x49fa10 interpreter, 70 KB) and `char pad[]` structs compile fine but `-O2` clang may expose UB the x87 build never did
  (uninitialised reads were already found: docs/equiv_failures.md `mesh_terrain 0x47ff60` vertex copy of uninitialised stack). Use ASan/UBSan/MSan builds early.

## Acceptance

- `cmake --build` compiles all 180 TUs on Linux x86-64 and macOS arm64 (ticket 002 probe reads 180/180).
- `tools/matching/run_all.py` output unchanged.

## Effort

M (3-5 wk for compile-clean; most time is ticket 005/006 fallout).

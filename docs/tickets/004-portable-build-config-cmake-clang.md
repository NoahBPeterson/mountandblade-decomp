# 004 A second build configuration: CMake + clang + platform STL, one source tree

Area: port | Effort: M | Impact: very high | Depends on: 002; 003 for linking (compiling can start now)

## Summary

Add `CMakeLists.txt` that builds `src/` with clang using the **platform's** standard library. The MSVC 7.1 flow
(`tools/matching/cl.sh`, `tools/relink/build.py`) stays exactly as is and remains the only thing the harness and byte-exact
checks see. No `#ifdef` in `src/` (docs/patches.md: even an unused branch can change faithful codegen), so the portable config adapts
through a force-included prelude and shim include directories.

## Status (2026-11-15): milestone 1 done, every TU compiles for i686, no src/ edits

**187 of 187 src/ TUs compile to i686 ELF objects with clang** (`cmake --build build/port/linux-i686`, ~12 s; baseline at the
start of the lane: 10/187). Recorded in `docs/port_burndown.json` by `tools/port/burndown.py` (history, per-target latest).
**src/ is unchanged** (`git diff main -- src` is empty), so the MSVC build and its byte-exact counts are untouched by design.

What is in the tree:

| path | what |
|---|---|
| `port/CMakeLists.txt` | `mb_objs` OBJECT library: every `src/**/*.cpp` (d3dapp.cpp / options_dlg.cpp included; they are replaced at link time), `-include port_prelude.h`, `-nostdinc++`, include order `port/include`, `port/stl`, `src/include`, `match/include` |
| `port/cmake/linux-i686.cmake`, `linux-x86_64.cmake`, `zig-c++`, `zig-cc` | toolchains. **zig** (Homebrew, bundles clang + glibc headers/libs for every target) is the compiler driver, so a macOS arm64 host cross-compiles for i686 Linux with no sysroot; on Linux `clang -m32` works the same |
| `port/port_prelude.h` | calling-convention keywords (real on i686, empty on 64-bit), WINAPI/CALLBACK/STDMETHODCALLTYPE, WL_MARKER pre-empted to `{}` (its include guard), `__vecDelDtor(flags)` as a template, `mb_crt_extra.h` |
| `port/include/` | shims declaring only what src/ uses: `windows.h` + `mb_win32_api.h` (USER/GDI/KERNEL/registry/clipboard), `d3d9.h` (full D3D9 interfaces in API vtable order, structs with their 32-bit sizes), `d3dx9.h` (math types, ID3DXEffect, the 12 imports), `dinput.h`, `tchar.h`, `io.h`, `direct.h`, `process.h`, `mmsystem.h`, `shlobj.h`, `shellapi.h`, `commctrl.h`, `windowsx.h`, `richedit.h`, `winsock2.h`, `oleauto.h`, `mbstring.h`, `malloc.h`, `mb_crt_extra.h` (_stricmp, _snprintf, _itoa, MSVC `struct _stat`, FILE `_iobuf`/`_flag`/`_IOEOF`) |
| `port/stl/` | **the port's standard library** (see the STL decision below): vector, list, deque, string/xstring, stdexcept, exception, new, utility, xutility, xmemory, memory, algorithm, functional, queue, iterator, hash_map |
| `port/tu/<path>.h` | per-TU preludes, force-included into `src/<path>.cpp` only, for code cl accepts and clang cannot be told to (one so far: `game/mission_ui2/slot_api.h` declares `Pool<Event>::get`'s explicit specialization before battle_rec.h's inline use instantiates it) |
| `tools/port/burndown.py` | configures port/, compiles each TU from compile_commands.json in parallel, records counts / error kinds / worst files plus a **link inventory** (llvm-nm) |

Decisions taken (they differ from the plan below; the plan is kept for the record):

1. **STL: a VC7.1-shaped library in `port/stl`, not the platform's.** The plan's route (platform STL, convert the
   `_Myfirst` views to real `std::vector` members in 006) cannot compile the tree as it is: src/ names the Dinkumware internals
   on ~750 non-comment lines (`_Insert_n` 156, `_Uninit_copy` 59, `_Uninit_fill_n` 47, `_Tidy` 45, `_Ucopy` 43, `_Xlen` 37,
   `_Buy` 25, `_Ptr_cat`, `_Nonscalar_ptr_iterator_tag`, `_Temp_iterator`, the sort/heap/stable_sort internals, `stdext::_Hash`),
   explicitly instantiates and specializes them, and casts hand-laid records to containers. `port/stl` implements that interface
   independently (written for the port; nothing copied from the MSVC headers) with VC7.1's layouts on i686: vector 16 bytes,
   list 12, deque 20, string 28, hash_map<int,T> 40 (linear hashing over a list + bucket vector, as src/game/glyph_hash.h needs).
   With it **every existing layout assert passes on i686** (troop, mesh, scene, script state, GUI manager...; the
   baseline run on libc++, whose vector is 12 bytes, already tripped them in the TUs that got that far), so the i686 port runs
   on the reconstruction's own layouts. 006 still converts the views; afterwards
   `port/stl` can shrink to wrappers or give way to the platform library.
2. **`-fms-compatibility`** (the plan said avoid it): needed for goto past initialisations (4 TUs), unqualified lookup into
   dependent bases (`VecOps`' `iterator`, shapes.h's `_Buy`) and `_ReadWriteBarrier` as a builtin. It also turns off C++
   exceptions and the GNU identity macros on a non-Windows target, so the CMake file re-enables `-fexceptions -fcxx-exceptions`
   and re-defines `__GNUC__` & co. (glibc's headers test them). No `_MSC_VER` is defined, so src/'s 6 `_MSC_VER` tests (tinyxml)
   take the portable branch.
3. **`-malign-double` on i686**: MSVC aligns `double` / `__int64` members to 8, the i386 SysV ABI to 4; src/'s offsets are MSVC's.
   glibc structs with 64-bit members then differ (`struct stat`), so src/ reaches those only through shims (`struct _stat` is
   the port's own; `mb_stat`).
4. **`-std=gnu++17`** works (no `register` / dynamic-exception-spec fallout); `auto_ptr` lives in port/stl.
5. `wchar_t` stays 4 bytes (only options_dlg.cpp's DxDiag COM path uses WCHAR; it is replaced, not ported).

64-bit baseline (`--target linux-x86_64`): 19/187 compile; 9052 of the 9065 errors are the existing layout asserts
(`scene_size_check`, `mesh_*_at_0x..` ...), i.e. exactly ticket 006's work list. The 13 others: `operator new(unsigned)`
definitions (size_t), `IntVec` member-pointer type, `tstring::read_from`, `operator+` declarations, one array size.

### Link inventory (i686, milestone 2's number; `burndown.py` prints it every run)

58 strong symbols are defined in more than one TU, and 375 externals are defined by no object:

| unresolved | provider | ticket |
|---:|---|---|
| 165 | Win32 / D3D9 / D3DX / DirectInput / COM / Winsock (declared in port/include) | 007 platform, 008/009 renderer + D3DX, launcher paths dropped |
| 47 + 18 + 13 | libc/libm, compiler runtime, C++ runtime (`__cxa_*`, operator new, `std::exception` typeinfo) | resolved by glibc + libc++abi at link |
| 41 | Newton 1.x | 011 (`phys::` adapter; stub first) |
| 26 | FMOD 3.75 | 010 (stub first) |
| 31 | src/: declared and called, defined in no TU (`Agent::reset`, `ItemKind::ItemKind`, `convv::vector_deleting_delete`, `E176::operator=` ...) | 004 milestone 2 |
| 20 | src/: STL instances specialized-and-declared in src/ but defined only by the MSVC link (`std::list<int>::_Buynode()`, `vector<entity*>::_Buy` ...) | 004 milestone 2 |
| 7 | port runtime (`mb_findfirst/next/close`, `mb_itoa`, `mb_strupr`, `mb_stat`, `_beginthread`) | 007 |
| 7 | other: `ceilf`/`floorf`/`sqrtf` (declared by src/ as C functions; libm provides them), SkinMagic | 007 |

The duplicates are the local copies the MSVC flow resolves by ownership in `tools/relink/build.py` (a TU that keeps its own
byte-matching copy of a helper); the 51 src/ externals are names the MSVC link resolves through the same address-keyed
machinery (forwarding stubs / symbol aliases, ticket 003). The port link needs the same table, generated from the MSVC
build's map rather than hand-kept.

## Next (milestone 2 onward)

1. **Link** (`mb` executable, i686): (a) an owner table for the 58 duplicates and an alias table for the 51 src/ externals,
   generated from `tools/relink/build.py`'s ownership data / the MSVC map into `port/link/` (weak aliases or `--defsym`), never
   edited by hand; (b) `port/runtime/*.cpp` for the 7 CRT extras; (c) stub libraries for FMOD and Newton (return failure /
   no-op) so the exe links before 010/011; (d) a `main` that calls the game's WinMain path through the platform layer.
   zig's linker driver filters `-z muldefs`-style flags, so the duplicates must really be resolved (good: it forces the table).
   Linking against libc++abi needs port/stl's `std::exception` hierarchy checked against the runtime's (logic_error /
   length_error are inline in port/stl; libc++abi defines its own with a different layout: keep ours out of its namespace
   symbols or make port/stl's exceptions derive from the runtime's).
2. **Platform layer** (ticket 007): implement the 165 platform names the game keeps (window, input via the DIK table, clipboard,
   cursor, timers, events/critical sections, directories) on SDL3 in `port/platform_sdl.cpp`; d3dapp.cpp / options_dlg.cpp and
   the launcher dialogs are replaced, not ported.
3. **Renderer** (ticket 008): first light through dxvk-native behind the unchanged `d3d9.h` interfaces (they are in API vtable
   order for that purpose), then the SDL_GPU backend; D3DX's 12 imports go to port/ code (ticket 009).
4. **Audio** (010) and **physics** (011): stubs at milestone 2, real SDL3 audio + stb_vorbis and the Jolt adapter after.
5. Turn warnings back on (`-Wno-everything` today) per category, and add ASan/UBSan configurations once it links (risks below).
6. 64-bit (006): `burndown.py --target linux-x86_64` is the counter; its error list is the layout-assert inventory.

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

Skeleton (2026-11-02, since promoted and wired): `port/CMakeLists.txt`, prelude: `port/port_prelude.h`.

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

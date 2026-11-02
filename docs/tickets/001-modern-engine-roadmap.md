# 001 Roadmap: from byte-exact 32-bit Windows exe to a modern, portable, moddable engine

Area: umbrella | Effort: XL (sum of 002-018) | Impact: very high | Depends on: 003 (hard gate)

## Goal and framing

The byte-exact cl 13.10 / x87 / 32-bit Windows build is a **stepping stone**: it proves the source is a faithful
reconstruction (equivalence harness, `docs/equiv_verdicts.json`) and gives a reference binary. The product is a second,
independent build configuration of the *same* `src/`:

- clang (or any modern compiler), **platform standard library** (`std::vector` etc. as shipped; no VC7.1 layout, no x87),
- **64-bit** first-class (Linux x86-64, macOS arm64/x86-64, Windows x64),
- **multithreaded** (a job system, not one physics thread),
- **SDL3** for window, input, audio, filesystem, threads, and the **SDL3 GPU API** as the renderer backend,
- a **moddable** core: extensible script operations, documented data formats, no hard-coded limits where the data allows more.

Per the maintainer's direction: byte-exactness, x87 precision, and VC7.1 container layouts are explicitly *not* goals of
the portable build. They matter only where something is **load-bearing for correctness**: file formats with pointer-sized
or layout-dependent fields (checked in ticket 006: the `.sav`/BRF/SCO paths go through typed streams, which is good news),
and code that reads structs through hand-computed offsets (ticket 006).

## Where the repo stands (measured on main @ 8b46ce3, 2026-11-02)

| | |
|---|---|
| Source | 180 TUs, 508 files, 146,671 lines in `src/` (`game`, `rgl`, `dxframework`, `tinyxml`) |
| Functions with compilable source | 3,352 of 3,352 (docs/STATUS.md) |
| Verified | byte-exact 2,256 (67.3% of functions, 27.4% of code size); equiv PASS 471, WEAK 386, FAIL 4 (docs/equiv_verdicts.json, 2026-11-01) |
| Runnable | `work/relink/standalone/mb_standalone.exe` (2.6 MB, 2026-11-01); README.md's "no runnable binary yet" is stale |
| What it still borrows | `glue.py` copies the original's .rdata/.data into `mb_data.obj` and links at the fixed base 0x400000: 787 data items borrowed + 790 reached only via pointer tables (docs/data_history.json, 2026-10-27), 248 `int3` traps, 8 rewritten vtable slots in 68 vtables, 512 vptr stores moved to the original's vtables, 1,032 forwarding stubs; image `.data` VirtualSize 0x893db94 = 143.9 MB (static pools) |

README.md should be refreshed (not done here: out of scope for a research pass).

### Platform surface (counts in `src/` unless noted)

| area | usage | count |
|---|---|---|
| Direct3D 9 | `IDirect3DDevice9` calls: SetRenderState 114, SetTextureStageState 37, SetSamplerState 17, SetTexture 15, SetTransform 11, ... ~50 methods | 314 sites |
| draws | `DrawIndexedPrimitive` terrain_a.cpp:3345 and terrain_b.cpp:598 (the whole 3D scene), `DrawPrimitiveUP` shapes.cpp x3 (debug lines), `DrawPrimitive` d3dfont.cpp x2 | 7 sites |
| D3DX | 12 imports: CreateTextureFromFileEx, CreateTexture, SaveSurfaceToFile, AssembleShaderFromFile, CreateEffectFromFile, 6 matrix helpers | 12 |
| shaders | `mb.fx` 3,084 lines (vs_2_0/ps_2_0), 3 tiny ps asm `.pp`, rest fixed-function; 5 vertex declarations (config.cpp:1086) | data |
| input | DirectInput8 keyboard + mouse (config.cpp:867), state arrays indexed by DIK codes | 2 devices |
| window | `d3dapp.cpp` (DX SDK sample): 14 `WM_*` cases, `Run` 0x402ee0, launcher dialog (res 0xaa) + SkinMagic | 1 loop |
| audio | FMOD 3.75, 26 imports, ~88 refs / 18 files, assets are **Ogg** (434 files) + 14 WAV | |
| physics | Newton 1.x, 41 imports, ~250 refs / 13 files, ragdoll API ~50 refs | |
| other Win32 | CreateDirectoryA 13, SHGetFolderPathA 12, FindFirstFileA 9, ShowCursor 9, registry 3, sockets 2, CreateProcess 2 (binkplay), ShellExecute 2, DxDiag COM 1, GDI font bake | ~150 |
| threads | 1 `_beginthread` (`loader_thread_body` 0x4c8900 -> `PhysSet::update`) | 1 worker |
| 3rd party inside the exe | MSVC CRT + Dinkumware STL, TinyXML 2.5, DX9 SDK framework, dinput8.lib tables; no zlib/png/jpeg | |

Compile-readiness (`patches/port_sketch/probe_syntax.py`, clang 21 on arm64, `-fsyntax-only`, MSVC keywords stubbed):
9 of 180 TUs compile; first errors: 84 `windows.h`, 49 `d3d9.h`, 14 size asserts that already exist in the tree
(`mb_troop_size_chk`, `mesh_size_check`: the 64-bit canaries), ~10 VC7.1 STL internals (`_Myfirst`, `std::_Uninit_copy`),
the rest one-offs.

## Route decisions

1. **Platform layer: SDL3.** Window, events, keyboard/mouse, text input, clipboard, cursor, audio streams, threads/mutex/
   condition, file system helpers, timers, message boxes. Covers every Win32 call the game makes except the launcher dialog,
   registry and DxDiag (ticket 007).
2. **Renderer: full port onto the SDL3 GPU API** (Vulkan / Metal / D3D12 backends from one API; shaders as SPIR-V/MSL/DXIL
   via SDL_shadercross). Fit assessed in ticket 008: the game needs vertex/index buffers, 2D/cube textures with DXT1/3/5, render
   targets + depth, MSAA, alpha test (in shader), fog and lighting (in shader), a shadow map, a 2-3 pass post path: all within
   SDL_GPU. Reasons it could still be a poor fit: it is young (SDL 3.2, 2025) and has no OpenGL/GLES fallback, so pre-2015 GPUs
   and some VMs are excluded. If a GL fallback ever matters, bgfx is the alternative; nothing in the tickets assumes it.
   A D3D9 translation layer (dxvk-native) is **only** a short-lived bring-up crutch on Linux (ticket 008, option B0).
3. **Audio: SDL3 audio streams + a small mixer + stb_vorbis** (assets are Ogg/WAV only; ticket 009).
4. **Physics: replace Newton 1.x** (closed, Windows-only binary) with an open engine behind a thin `phys::` interface; Jolt is
   the first candidate (MIT, ragdolls, 64-bit, own job system); ticket 010.
5. **Wine/CrossOver** stays the reference platform for the MSVC build and for A/B behaviour checks (ticket 017).

## Ticket map

| Phase | Tickets |
|---|---|
| P0 make the source self-contained | 003 data/literals/vtables from source (gate), 004 portable build config (CMake + clang + prelude), 002 burn-down probe |
| P1 portable and 64-bit | 005 MSVC-isms, 006 layout-neutral 64-bit structs, 007 SDL3 platform layer |
| P2 render, sound, physics | 008 SDL_GPU renderer, 009 shaders + D3DX replacement, 010 audio, 011 physics |
| P3 modern engine | 012 threading architecture, 013 performance foundation (loading, pacing, profiling) |
| Modding | 014 script/operation extensibility, 015 data formats + tooling, 016 hard-coded limits |
| Verification | 017 verifying a non-bit-identical build |
| Interim | 018 quick wins on the current 32-bit exe (brief) |

## Blockers (ordered)

1. Ticket 003: until the program no longer reads the original's data image, no other build can exist.
2. 64-bit: ~6,900 offset comments, ~2,400 pad arrays, ~650 raw offset accesses encode 32-bit layouts (006).
3. Physics replacement changes gameplay feel (011).
4. `mb.fx` is SM2 HLSL: SDL_shadercross's DXC path wants SM6; it must be ported by hand/by tool (009).
5. Evidence: the Unicorn-based harness cannot vouch for a clang binary (017).

## Acceptance criteria (whole programme)

- `cmake --build` yields `mb` on Linux x86-64 and macOS arm64; main menu, new game, field battle, save/load on the user's Steam data.
- At least two worker threads busy in a battle with 150+ agents; frame time lower than the Wine/CrossOver build on the same machine.
- A mod can add a script operation and a data table entry without editing the interpreter's giant switch (014, 016).
- The MSVC byte-exact flow still reports today's counts (STATUS.md).

## Effort (estimates, mine, +-2x; one engineer with agent help)

P0 8-16 wk (003 is already in flight), P1 8-14 wk, P2 14-24 wk (011 is the wild card), P3 6-10 wk, modding 8-12 wk,
overall 9-15 months to a daily-drivable port. First visible milestone (menu on Linux, stubbed audio/physics): about 3 months after 003.

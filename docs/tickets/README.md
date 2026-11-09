# Tickets: a modern, portable, moddable engine from the decompilation

Written 2026-11-02 (research pass; no code in `src/` or `tools/` changed). Direction (maintainer): the byte-exact 32-bit exe is a stepping stone; the goal is a
64-bit, multithreaded, SDL3-based, moddable engine on Linux/macOS/Windows. Byte-exactness, x87 behaviour and VC7.1 container layout are not goals of the portable build.
Effort: S < 1 wk, M 1-6 wk, L 6-12 wk, XL > 12 wk (estimates, one engineer with agent help, +-2x).

| id | title | area | effort | impact | depends on |
|---|---|---|---|---|---|
| [001](001-modern-engine-roadmap.md) | Roadmap: from byte-exact exe to a modern portable moddable engine | umbrella | XL | very high | 003 |
| [002](002-portability-burn-down-probe.md) | Portability burn-down: clang syntax probe + history | port | S | medium | - |
| [003](003-self-contained-source-data-vtables.md) | Self-contained `src/`: data, literals, vtables, traps (the gate) | port-gate | XL | very high | - |
| [004](004-portable-build-config-cmake-clang.md) | Second build configuration: CMake + clang + platform STL | port | M | very high | 002 (003 to link) |
| [005](005-msvc-isms-in-source.md) | Removing MSVC-only constructs (conv keywords, alternatename, asm, CRT internals) | port | M | high | 004 |
| [006](006-layout-neutral-structs-64-bit.md) | 64-bit: real C++ structs instead of hand-laid 32-bit records | port / modding | XL | very high | 003, 004 |
| [007](007-sdl3-platform-layer.md) | SDL3 platform layer: window, input, text, filesystem, time | port | M | high | 004 |
| [008](008-renderer-port-sdl-gpu.md) | Renderer: full port from D3D9 to the SDL3 GPU API | render | XL | very high | 004, 007, 009 |
| [009](009-shaders-and-d3dx-replacement.md) | Shaders, `mb.fx`, and the D3DX replacement | render | L | high | 004 (parallel to 008) |
| [010](010-audio-sdl3-replacing-fmod.md) | Audio: FMOD 3.75 -> SDL3 audio + stb_vorbis mixer | audio | M | high | 004, 007 |
| [011](011-physics-replace-newton.md) | Physics: replace Newton 1.x (Jolt) behind `phys::` | physics | L-XL | very high (risk) | 004 |
| [012](012-threading-architecture.md) | Multithreading: job system and parallel subsystems | engine | L | high | 004, 011 |
| [013](013-performance-foundation.md) | Performance foundation: profiling, BRF loading, frame pacing, memory | perf | M | high | 004 |
| [014](014-script-and-operation-extensibility.md) | Modding: extensible operation registry and plugin hooks | modding | L | very high | 004 |
| [015](015-data-formats-and-tooling.md) | Modding: documented formats and tooling (BRF/TRF/SCO/SAV) | modding | M | high | - |
| [016](016-raise-hard-coded-limits.md) | Modding: audit and raise hard-coded limits and static pools | modding | L | high | 003, 006 |
| [017](017-verifying-the-portable-build.md) | Verifying a build that is not bit-identical | verification | L | high | 004 |
| [018](018-interim-32bit-exe-quick-wins.md) | Interim quick wins on the 32-bit exe (LAA, SSE2, pacing) | perf | S | low | - |
| [019](019-idiomatic-source.md) | Idiomatic source: accessors, real names, no offset arithmetic (after byte-exactness) | cleanup | M | high | - |

Order of attack: 003 (in flight) and 004/002/005 -> 007 -> 009+008 with 010 and 011 stubbed first -> 006 in parallel per header -> 012/013 -> 014/015/016 -> 017 continuously.

Sketches (untested, not wired into any build): `patches/port_sketch/` (platform API header, force-include prelude, CMake skeleton, clang probe), `patches/laa/set_laa.py`.

# 018 Interim: quick wins on the current 32-bit Windows exe (brief, low priority)

Area: perf | Effort: S | Impact: low-medium, short-lived | Depends on: none

Per the maintainer the 32-bit exe is a stepping stone, so these are cheap, optional, and none should delay tickets 003-017.

## 1. Large Address Aware

- Fact: `IMAGE_FILE_LARGE_ADDRESS_AWARE` (0x0020) is **clear** in `mount&blade.exe` (Characteristics 0x010f), the unpacked analysis image, and the relinked `mb_standalone.exe` (0x010e). The static image alone is 143.9 MB (`.data` VirtualSize 0x893db94), SizeOfImage 0x8b4f000, stack reserve 0x3e8480 (~4 MB).
- Change: add `/LARGEADDRESSAWARE` to `link_extra` in tools/relink/build.py (~line 414, next to `/FIXED:NO`), or flip the bit post-link: `patches/laa/set_laa.py` (tested on a copy of `mb_standalone.exe`: exactly one byte changes at file offset 0x11e, 0x0e -> 0x2e; CheckSum field is 0; **not run in the game**).
- Why it is low value: it lifts the address-space ceiling (2 GB -> 3-4 GB) for large mods; it does not speed anything up. Risk: any code that treats a pointer as a *signed* int (`(int)ptr` 192 + `int x = (int)ptr` 168 sites) breaks above 0x80000000. The real fix is 006.
- Test: run a big-mod battle with the flag and watch for faults (crashrun.py in tools/relink).

## 2. `/arch:SSE2` build variant

- Exists: `tools/relink/build.py --cflags "/arch:SSE2"` (docs/patches.md). Measured microbenchmark under Rosetta 2: x87 18.4 s vs SSE2 10.4 s (1.77x on pure float code).
- Float differences are acceptable here. Note docs/codegen.md 9.1: the game's D3D9 device runs without `D3DCREATE_FPU_PRESERVE`, so the x87 already rounds every result to 24 bits on the main thread; the SSE2 variant is therefore close to the original except for `double` math.
- Action: make it a named patch set so it is one flag away; A/B with `patches/fps/fps_title.cpp`. Not part of the faithful gates.

## 3. Frame limiter spin-wait

- `Render3DEnvironment` 0x402430 (d3dapp.cpp:837) busy-waits to honour `max_framerate` (default 100); a `patches/pace` replacement that sleeps (`Sleep(rest-1)` then spin) cuts CPU burn. Needs `timeBeginPeriod(1)` (no use of it exists in src/). Replacement requires re-implementing the whole function in a patch (`// @ replaces 0x402430`, docs/patches.md); behaviour change declared in the header. The portable build gets the same fix for free in ticket 013.

## 4. Wine-layer options (unverified, outside the repo)

CrossOver bottle graphics backend choices (D3D9 -> DXVK/MoltenVK instead of wined3d/OpenGL) could change the opengl32/win32u cost visible in the profile; I have not tested it.

## Effort / acceptance

S each. Acceptance for 1: `Characteristics & 0x20`; for 2: variant builds and boots; for 3: CPU% at 100 fps cap drops without frame-time regression.

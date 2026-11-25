# Warband carry-over: how much of the 1.011 work transfers

Measured 2026-11-25. This is a read-only estimate; nothing in `src/` was changed. Scripts, objects and per-function results are in `work/warband/analysis/`. Planned follow-up: by the end of 2026.

## The binary

- Path: `work/warband/mb_warband.exe` (5.36 MB, PE32, linked 2018-09-11).
- Not packed. It has four normal sections, the entry point (0x62bbfc) is inside `.text`, and the import table is intact.
- `.text` is 0x3bcb6d (3.92 MB); the 1.011 binary's is 0x147578 (1.34 MB).
- Toolchain: linker 9.0 (VS2008). The Rich header shows cl 15.00.21022 objects (VS2008 RTM, 652 C++ objects) plus 15.00.30729 objects (SP1).
- The game code is mostly **RTM**. The cl 15.00 we have (`../spore-decomp/work/toolchain/vc9sp1/VC/bin`, also used by c2scope's 15.00 port, `../vs2005-cpp/docs/c2/port-1500.md`) is **SP1**. Byte matching Warband will need RTM's c2.dll, or a measurement of how far RTM and SP1 codegen differ.
- Extra imports compared with 1.011: WS2_32, WININET, WINHTTP, CRYPT32, steam_api, fmodex, SkinMagic.

### Flags inferred from prologues

- `/GS` off: only 38 security-cookie sites in about 12 477 functions, mostly CRT.
- `/O2 /Oy`: frame-pointer omission is on. 1977 of 12 477 functions open with `push ebp; mov ebp,esp`, close to 1.011's ratio.
- EH: 178 functions have SEH prologs, so some TUs use EH and some don't.
- Compiled with `/D_SECURE_SCL=0 /D_HAS_ITERATOR_DEBUGGING=0` (without it VC9's vector is 16 bytes and the layout asserts fail).

### How Warband's .text splits

| Range | Size | What |
|---|---:|---|
| 0x401000-0x62b000 | ~2.2 MB (56%) | game and engine; our pairs cover about half |
| 0x62b000-0x641000 | ~0.1 MB | CRT / libraries |
| 0x641000-0x7be000 | ~1.55 MB (40%) | Havok 6.6.0 physics (1.011 has no Havok) |

Networking is small in code terms. Only 48 functions (10 KB) call WININET or WS2_32 directly; ws2_32 has 63 importers and steam_api 42.

## Results

### Our 3342 game functions (1.261 MB)

| Class | Functions | Share | Bytes | Share of bytes |
|---|---:|---:|---:|---:|
| confident | 1255 | 37.6% | 705 KB | 55.9% |
| probable | 576 | 17.2% | 276 KB | 21.9% |
| no counterpart found | 1511 | 45.2% | 280 KB | 22.2% |

Of the confident pairs, 404 (437 KB) are solid: they share a rare string or have identical code hashes.

**Estimate: 30-55% of functions, 45-78% of code bytes, have a Warband counterpart.** The best single figure is about 38% of functions and 56% of bytes.

By function size, the matched share is:

| Function size | Matched |
|---|---:|
| < 32 B | 10% |
| 32-100 B | 45% |
| 100-400 B | 59% |
| 400-2000 B | 76% |
| > 2000 B | 88% |

The "none" class is an upper bound, because tiny functions are hard to pair and Warband's function boundaries are approximate.

The same classes split by our own tiers:

| Our tier | Confident | Probable | None |
|---|---|---|---|
| byte-exact (2990) | 36.4% (51.0% of bytes) | 15.7% (17.8%) | 47.9% (31.2%) |
| equivalent, not exact (352) | 47.2% (62.2%) | 30.4% (27.2%) | 22.4% (10.6%) |

Our byte-exactness says nothing about Warband's bytes (a different compiler). Every figure here is source carry-over.

Our matched pairs cover 1.12 MB of Warband's `.text` (28.6%); confident pairs alone cover 0.81 MB (20.7%).

### Compiling our source with cl 15.00

This is the most direct measure, but it only covers part of the code so far.

- 62 of our 188 TUs compile under cl 15.00, and 126 fail.
- That gives 575 game functions (126 KB, 10% of game bytes) that can be scored against Warband directly.
- Compile failures:
  - C2244 ×88: VC7.1-specific explicit specialisations and redefinitions of `std::vector` internals, e.g. `src/engine/core/vec_xlen.h`.
  - C2118 ×30: `MB_LAYOUT32_ASSERT` size checks, because VC9's std containers have other sizes.
  - C3190 ×4, C2977 ×2, C2912 ×2: other explicit-instantiation and STL-internals mismatches.

For the 238 matched pairs among those 575 functions:

| Tier | Pairs | Bytes |
|---|---:|---:|
| identical apart from relocations | 107 | 13.4 KB |
| identical apart from displacements / immediates | 25 | 2.7 KB |
| near (mnemonic sequence ratio ≥ 0.9) | 57 | 15.9 KB |
| similar (0.7-0.9) | 40 | 15.1 KB |
| weak (< 0.7) | 9 | |

So 55% of these pairs by count (31% by bytes) come out **byte-identical, or identical apart from offsets, straight from our source**. A further 24% are near. The sample is biased towards small functions, because the TUs that compile are small.

### Operation::execute

| | 1.011 | Warband |
|---|---|---|
| address | 0x49fa10 | 0x51c890 |
| size | 69 424 B | 68 666 B (16 366 insns) |
| jump table | 2356 entries (`add -0x1e`) | 2738 entries (opcodes 0x1e-0xacf) |
| distinct handlers | 595 | 1038 |

The pair matches by size and by string anchors. Warband seems to have split the interpreter: 0x52d4d7 (21 KB) and 0x536c8a (25 KB) are probably more of it. Our 590 decompiled cases are a large head start on its script engine.

### The 15 biggest matched pairs

| Ours | Warband | size ratio (Warband / ours) | class |
|---|---|---:|---|
| 0x49fa10 Operation::execute | 0x51c890 | 0.99 | confident |
| 0x484590 | 0x4f963d* | 1.04 | confident |
| 0x448eb0 | 0x4c5e90 | 0.46 | probable |
| 0x50a1c0 ItemD::build | 0x5f42d0 | 1.38 | confident |
| 0x4465e0 | 0x4d5864 | 1.13 | probable |
| 0x457b40 | 0x4c1726 | 0.64 | confident |
| 0x4fe860 GameApp::terrain_dialog | 0x6052e0 | 1.63 | confident |
| 0x45a170 | 0x4ee3f0 | 1.71 | confident |
| 0x4556d0 | 0x4cef90 | 0.92 | probable |
| 0x4e05c0 Recf0::init_screen | 0x5e9200 | 1.34 | confident |
| 0x507400 | 0x5ff9e9 | 0.60 | confident |
| 0x5213d0 | 0x5d1420 | 1.28 | confident |
| 0x46e590 | 0x42df50 | 0.56 | confident |
| 0x45d820 | 0x4cc410 | 0.72 | probable |
| 0x4f8c80 MissionBlocker::update_camera | 0x5d7f54 | 1.17 | probable |

\* The Warband start found by the scan is mid-function (a boundary error); the pair itself is right.

Ratios of 0.5-1.7 mean heavy changes: new fields and added features.

## Method

1. **String anchors.** Collect the string literals each function references in both binaries. A shared rare string pairs two functions.
2. **Call-graph propagation.** Starting from anchored pairs, pair their callees and callers in call order.
3. **Structural confirmation** for the rest: size ratio, number of calls, constants, and a normalised mnemonic n-gram signature.
4. **Compile and compare.** Compile our TUs with cl 15.00 and Warband's flags, then score our objects against Warband's code. Relocations, and where needed displacements and immediates, are masked.
5. **Hand checks.** 13 pairs from 32 B to 12 KB were compared by eye:
   - 10 are clearly right, e.g. 0x489400 (an offset moved from 0x1c to 0x9c) and 0x4be9f0 (identical).
   - 0x499830 is right: a constructor whose struct grew from 0x188 to 0x2e8 bytes.
   - 2 are plausible but changed (0x432c10, 0x431cb0).
   - 1 is doubtful (0x4704b0).
   - 1 has a boundary error (0x484590).

### Weaknesses and error bar

- The mnemonic-histogram similarity is loose: random pairs reach 0.79 at the 95th percentile. Hiding half the string anchors and re-propagating gave 82% precision at a 0.7 threshold and 92% at 0.8.
- Checked against the cl 15.00 compile, the VC7.1-based matching over-reports. It paired 397 of the 575 compiled functions, but only 225 (57%) were confirmed by cl 15.00 code; within that overlap 94% agreed. That is why the range runs down to 30%.
- Warband's function boundaries come from a call-target and prologue scan. Some start mid-function, some functions are split, and Warband has more SEH funclets.
- Pairing is one-to-one. Duplicated or inlined code in Warband can leave a true counterpart unclaimed.

## When we pick this up

1. **A VC9 build of our source.** Shim the VC7.1-specific `std::vector` internals (`vec_xlen.h` and the explicit specialisations) and the 32-bit layout asserts behind a toolchain switch, so the other 126 TUs compile under cl 15.00. Re-run `work/warband/analysis/build15.py` and `tiers.py` to turn the estimate into a measured per-function list.
2. **The exact compiler.** Find out whether Warband's game objects need cl 15.00 **RTM** (21022) for byte-exact work. If they do, obtain it and port c2scope to it, the same way as `port-1500.md`.
3. **Function boundaries.** Replace the prologue scan with a proper analysis (Ghidra, as for 1.011) before trusting per-function sizes.
4. **Start with what carries.** Tackle the 107 + 25 pairs that already come out identical or identical apart from offsets, then the big confident pairs (Operation::execute first).
5. **Plan around Havok.** About 40% of Warband's `.text` is Havok 6.6.0, which has no 1.011 counterpart.

Files: `work/warband/analysis/` holds:

| File | What |
|---|---|
| `pipeline.py` | pairing and classes |
| `objfeat.py` | features of cl 15.00 objects, and the masking |
| `tiers.py` | the identical / near / similar tiers |
| `build15.py`, `cl9.sh` | the cl 15.00 build driver and wrapper |
| `build15.out`, `log15/` | compile results and logs |
| `result.pkl`, `tiers.pkl` | per-function results |

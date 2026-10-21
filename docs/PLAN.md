# Plan

Goal 1: a drop-in `mount&blade.exe` built from source (cl 13.10, static CRT), using the game's own DLLs,
running under Wine/CrossOver. Goal 2 (later): native macOS/Linux (SDL3, OpenAL, a Newton/Jolt port, a
D3D9 translation layer or a new renderer).

## The binary (facts established 2026-10-20)

| | |
|---|---|
| Protection | SteamStub **v2.1** x86 (header 0xD9 dwords, XTEA-encrypted SteamDRMP.dll, AES-CBC `.text`). `tools/steamstub21_analysis_image.py` (port of Steamless Variant21). After unpacking there is no Steam check: the image boots under CrossOver without Steam. |
| Entry | OEP `0x5390BA` (VC7.1 `WinMainCRTStartup`, `__SEH_prolog`) |
| Toolchain | linker 7.10, **cl 13.10** (VC .NET 2003). The Rich header is gone (Valve's `VLV` block over the DOS stub). Confirmed by byte-exact matches with the Toolkit 2003 cl 13.10.3052 (x87 operand shapes are version-specific) and 415 byte-exact `libcmt`/`libcpmt` functions. |
| Flags | `/O2` (frame pointers omitted), x87 only (no `/arch:SSE`), `/EHsc` (EH magic 0x19930520), no `/GS` in game code (only the CRT uses cookies), no `/GR` (RTTI only for 6 std types), static CRT `/MT`, fixed base, **no `.reloc`** |
| Size | 4,395 Ghidra functions after fixups. Game code ≈3,500 functions / 1.21 MB / ≈360K instructions (~12× smaller than Spore). Static CRT ≈555 functions in `0x535440..0x547100`. Import thunks `0x5342a6..0x53438a`, `0x535440..0x53545e`. EH handler stubs + atexit thunks after the CRT (`0x547100..`), which are game code. |
| Biggest function | `0x49fa10`, the module-system script interpreter: one ~70 KB function, `switch (opcode - 30)` over 2,356 cases (table `0x4ae458`). Ghidra's 1024-entry jump-table limit truncated it to 366 bytes; `fixups.py` and `export_decomp.py` fix that. |
| Dynamic libs | Newton 1.x (41 imports), FMOD 3 `FSOUND_` (26), d3d9 + d3dx9_31 (12), dinput8, SkinMagic (by ordinal), winmm, ws2_32, kernel32/user32/gdi32/advapi32/shell32/shfolder/ole32/oleaut32 |
| Static libs | MSVC 7.1 CRT + Dinkumware C++ library, `dinput8.lib` data formats (`c_dfDIKeyboard`… at `0x534390`), and a 2004 hardware-locked activation library (`tlopatic` CVS ids) whose code was dropped by `/OPT:REF` (data only) |

## Fidelity tiers (same as spore-decomp)
- **byte-exact**: our source, compiled with cl 13.10, gives identical bytes (relocations masked).
- **equivalent**: complete source with the same behaviour (difftest harness), not yet byte-exact.
- **partial**: incomplete source; not counted.
Target: byte-exact wherever reachable. There is no sign of `/GL`/LTCG so far, so no ceiling is known yet.

## Stages
1. **Footing** (done): unpacker, runnable image, Ghidra project + fixups, decompile export, toolchain
   confirmed, verifier, tools ported from spore-decomp.
2. **Shared types first** (one small wave): there is no PDB and no RTTI, so class layouts must be
   recovered. Without a shared header, parallel agents invent conflicting layouts. Seed
   `match/include/mb/` with the core types: math (vec2/vec3/quat/matrix), strings, the main game
   object (global at `0x5e2930`), and the big record types (troops, items, parties, scenes,
   missions) whose layouts the Module System describes. Reference material: the official Module
   System for 1.011 (`header_operations.py` names every interpreter opcode) and the community's
   Warband RE (WSE headers: a later engine, so verify every offset).
3. **Matching waves**: contiguous slices (`tools/matching/plan_slices.py`), agents write
   `match/slices/<id>/`, verify with `chk.py`/`run_all.py`, record names in `symbols/slices/`.
   The interpreter gets its own track (it is one function, split by case ranges).
4. **Hybrid**: `tools/hybrid/build.py` splices equivalent (non-exact) functions into the original
   image and the game is played to test them (docs in spore-decomp `docs/hybrid.md`).
5. **Relink**: object boundaries inferred from link order, globals as source, CRT from the real
   `libcmt.lib`, linked with link 7.10 at the original addresses, then the drop-in exe.

## Layout
- `tools/` — ported from spore-decomp (`matching/`, `difftest/`, `hybrid/`, `status.py`), plus
  `steamstub21_analysis_image.py` and `ghidra/` (pyghidra, standalone: `index.py`, `fixups.py`,
  `coverage.py`, `export_decomp.py`).
- `match/` — slices and manifests (as spore-decomp). `match/include/` — shared headers.
- `symbols/` — recovered names. `work/` — git-ignored binary-derived artifacts.

## DeepSeek waves (opencode), ported from spore-decomp
- Config: `.opencode/opencode.json` (git-ignored; every tool "ask", web/external dirs denied; providers
  cheaperinference + OpenRouter for DeepSeek V4.1 Flash; `tools/opencode_cache_ns.py 16` writes per-session
  prompt-cache aliases). `.opencode/silent` mutes notification plugins.
- Batch + group: `work/batches/<batch>.json`, `work/opencode/groups/<group>.json` (`ranges` of slice indices,
  one per session). Prompt template `work/opencode/wave_prompt.txt` (tracked copy: `docs/opencode_wave_prompt.txt`).
- Gate: `tools/opencode_broker.py <batch> --sessions work/opencode/sessions_<group>.txt [--manual]`
  (kept alive by `tools/opencode_broker_watch.sh`); policy mode auto-approves the matching tools and edits
  inside the session's own slices, `--manual` holds everything for a supervisor.
- Run: `tools/opencode_sup.py <group> launch N | pending | reply | status | unhang | nudge`;
  `tools/opencode_drain.py <group>` nudges idle sessions and interrupts hung ones until done;
  `tools/opencode_verify_group.sh <batch>` re-verifies and gates bookkeeping afterwards.

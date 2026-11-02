# 017 Verifying behaviour of a build that is not bit-identical

Area: verification | Effort: L | Impact: high | Depends on: 004

## Summary

The equivalence harness (`tools/difftest/equiv.py`, Unicorn replay of original vs ours machine code) and byte-exact checks vouch for the *decompiled source as compiled by cl 13.10*. They cannot run a clang/arm64 binary. The maintainer does not require the portable build to
be harness-comparable; this ticket defines what *is* checked so decompilation mistakes (not compiler differences) are still caught.

## Evidence

- Standing verdicts (docs/equiv_verdicts.json, 2026-11-01): PASS 471, WEAK 386, FAIL 4, plus harness/comdat/input-limited rows; by code size WEAK is 587,821 B (47% of 1.26 MB), PASS 171,385 B, byte-exact 0.345 MB. Byte-exact + PASS cover ~41% of code bytes; the remaining ~59% is WEAK or otherwise
  weaker evidence. WEAK code has branches no input reached. Those are where decomp bugs live, and a portable build gets no extra protection there.
- Known indeterminate-original cases: docs/equiv_failures.md (4 FAILs): e.g. `mesh_terrain 0x47ff60` copies a vertex from an uninitialised stack temporary; `add_vertex 0x413c50` is original code in both builds. A clang build will behave differently there; ASan/MSan are the right instruments.
- `docs/snapshot_testing.md` and `tools/difftest/snapreplay.py`: snapshots of the real game (registers + memory) replayed through both builds: SAME 475, STOP 222, DIFF 3 (commit log 2026-11-01). These replay x86 machine code and cannot be used for the portable binary directly.
- Crash ledger `docs/crashes.md` (7 rows, 1 open): all found by running the real game, none by the harness.

## Detailed plan

1. **Keep the harness for what it is**: it gates changes to the faithful `src/` (MSVC config). Any source edit made for the portable build must pass `run_all.py` and the harness unchanged.
2. **Scripted playthroughs** (portable and Wine builds): a deterministic driver (inject input events through the platform layer, fixed `dt` and RNG seed via a `--replay` mode) that plays: boot -> menu -> new game -> map movement -> field battle -> save -> load -> town menus. Emit a per-frame state hash of selected subsystems (party positions, troop XP, inventory totals, mission agent counts) rather than rendering.
   Compare **tolerantly** (floats within an epsilon, event order) between the CrossOver build and the portable build.
3. **Module-level invariants**: after loading each module, compare counts and a hash of parsed tables (troops, items, scenes) between builds: this checks the parsing code (BRF/text) path-by-path, bypasses floating-point entirely.
4. **Sanitizers**: ASan/UBSan/MSan/TSan jobs on Linux for steps 2-3; each finding is triaged as original-behaviour (indeterminate) or decomp bug; the second gets a regression test.
5. **Record the original's oracle data** from the CrossOver build with the existing capture patches (capture/snapreplay infrastructure under `work/capture*`) as *function-level* golden I/O: for chosen leaf functions (AI scoring, damage computation, `store_*` ops) record (args, memory-in, memory-out) from the real game and replay them natively in a unit-test binary linking the same source. This is the portable analogue of snapreplay, restricted to functions with plain-data arguments (needs 006).
6. **Visual regression**: screenshot set from the CrossOver build (`D3DXSaveSurfaceToFileA` exists, config.cpp:566), perceptual diff in CI (SSIM threshold) for the renderer port (008).
7. WEAK-row burn-down continues in the MSVC flow: each WEAK->PASS is evidence for both builds.

## Code sketch

```cpp
// port/replay.cpp: --replay trace.bin --seed 1234 --fixed-dt 0.016
struct FrameHash { uint64_t parties, troops, inventory, agents; };
void replay_frame(FILE* out) { fwrite(&hash_state(), sizeof(FrameHash), 1, out); }
```
```python
# tools/port/compare_traces.py wine.bin native.bin --float-eps 1e-3 --report
```

## Risks

- Whole-game replays are chaotic: tiny float differences diverge the AI. Mitigate by comparing aggregate statistics over many seeds, and by function-level golden tests for the logic that matters.
- Building a deterministic driver needs a hook in the platform input layer (007) and the jobs system serial mode (012).

## Acceptance

CI job runs the scripted playthrough on both builds nightly, reports divergence per subsystem; sanitizer jobs green on Native; five golden-I/O function suites pass.

## Effort

L (6-10 wk). Estimates only.

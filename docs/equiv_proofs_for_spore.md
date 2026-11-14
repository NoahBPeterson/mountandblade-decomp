# How the M&B decomp proves equivalence (notes for the Spore session, 2026-11-13)

Mount & Blade 1.011 (cl 13.10, `/O2 /MT /Gy /TP`, **no /LTCG**, much x87 float code) is 86% byte-exact by function.
We use proofs for the remaining near misses. We started from spore-decomp's `tools/equiv` (commits aaaf5c79f,
82de4ab6c, f6bc57f27) and extended it.
Everything is in `tools/equiv/` (design and soundness arguments: `tools/equiv/LAYERS.md`).

## Results so far

- **39 recorded proofs** (`docs/equiv_proofs.json`, a bundle per row in `docs/equiv_proofs/<va>.json`):
  10 rename, 10 angr, 19 region. All are L1 except `Agent::update_controls` (L2).
- **About 20 newly counted as equivalent.** The rest were already a differential-test PASS; for those the proof
  upgrades evidence to proof.
- **One proof withdrawn.** `SkeletonBody::load` 0x50fc60: its lock-step walk relied on a callee pairing that no check
  had seen. That walk pairing is now checked.
- **Refutation run over the 91 differential-PASS rows** (angr, 15 s each): 0 confirmed differences, 6 "likely
  differs". All 6 traced to frame layout, a mispaired global, or source shape, not logic bugs.

## Layers (cheapest first; acceptance as in spore)

A function is accepted only when:

1. some layer proves it at L2, or at L1 when the facts (kill set, entry registers) are the same;
2. no layer refutes it;
3. the proving layer rejects every mutant of our build.

| layer | what it proves | M&B additions |
|---|---|---|
| rename (`rename_core.py`) | same code up to register renaming and independent reordering | **frame-slot renaming**: stack slots matched by a function-wide bijection, byte by byte, recognised at different esp depths; incoming arguments pinned. A delta mode handles frames whose address escapes (one constant shift). callee-saved register swaps. capstone access fixes (x87 stores reported as reads; `test r/m,r` missing a register) |
| angr (`angr_layer.py`) | whole-function symbolic execution, loop-free or checked-bound loops | literal constants paired **by content** (checked against the original's bytes); relocations paired **from the symbols file first**, then a contradiction check; float terms abstracted first, full float semantics capped at 20 s; laziness on forks (angr's per-fork sat check cost 26 s on x87); uninterpreted functions for VEX ops angr 10.0.6 lacks (sin/cos/atan/…); mutants only in code the proof executed |
| region (`regions.py`) | **cut-point decomposition**: identical stretches stepped in lock-step with no solver; angr only on stretches that differ; obligations chained by induction | see below |
| ddec (`ddec_layer.py`) | STOKE DDEC loop invariants (Docker) | **x87 lifter** (`x87lift.py`): x87 stack code rewritten to SSE2 scalar-double before STOKE; STOKE patched (`ddec/x87/4-x87-lifter.patch`): per-lane register equalities (Z3 4.4 hung on 128-bit ones), FP functions Ackermannised with commutativity / swapped-compare lemmas, unsound `x+0 → x` simplifications removed. **Proved nothing new on M&B**: the blockers were relocation pairing, frame layout and timeouts, not x87 |

## Region mode (the one that pays off)

A **cut** is `(retail addr, ours addr, π, d)`:

- **π** maps each of retail's registers and flags to the register of ours holding the same value. Partial
  relations are allowed (`ecx:8`, for cl's bools in `al` over a garbage upper part).
- **d** is the x87 stack depth.

The interface R at a cut is:

- the π pairs are equal;
- esp, DF and the d x87 values are equal;
- memory at and above esp is equal at the same addresses.

Each cut must reach the next cuts with R holding and equal events, or return with equal L1/L2 observables. Over
`update_controls` (2,074 instructions, 4.7×10²⁷ acyclic paths), 2,066 of 2,067 cuts are lock-step and one small
region needs angr.

- **Lock-step:** the same instruction under π and the relocation bijection, with no solver.
  - A call relates eax/ecx/edx afterwards.
  - A switch dispatch adds every (T_retail[k], T_ours[k]) as a cut: per-case proofs of 591-case switches.
- **Angr regions:** from a divergence to the next stop.
  - Stops are cuts already in S, loop heads, dispatches and rep instructions, so regions are acyclic and never
    need loop bounds.
  - At a pair of stops, π′ pairs every live register with an equal register on the other side.
- **Fine cuts:** a region may also stop at the **joins** of its stretch (two or more predecessors reachable from
  the start). That bounds path explosion by the paths between consecutive joins.
  - Variants are tried finest first: `joins`, then `pdom` (the first join every path passes), then `none`.
  - The two sides' joins are paired dynamically by the angr region itself.
  - **Backtracking:** when a fine cut splits code the compiler moved across a join, the earlier region is rerun
    coarser.
- **Soundness:** induction over execution length. A region's proved exit interface is exactly the next region's
  assumption. S is rebuilt from the entry each round, so abandoned proofs never stay in it. A region never
  refutes, because R is stronger than equivalence.

## Infrastructure that mattered

- **Pinned angr environment:** `tools/equiv/angr-requirements.txt` plus `setup_angr_venv.py`. Bundles record the
  lock's hash. Your note about a z3 version flipping a proof is why.
- **angr on a second machine:** `--remote` on prove/sweep/quick runs only the solver on our Windows laptop. The job
  prepared on the Mac is self-contained, so nothing else is shipped. Results were byte-identical to local on 3
  known proofs.
- **Sweep economics:** `--no-mutants` gives PROVED-UNTESTED; mutants run only on rows that prove. `--two-pass`
  gives a longer second pass for timeouts only.
- **Quick check:** `tools/matching/chk.py SRC VA --quick`, built after your note, about 15–25 s. It reports, in
  order:
  1. the call-count difference (missing and extra callees);
  2. rename;
  3. angr for 15 s without mutants, giving "very likely EQUIVALENT", or DIFFERS in words with the inputs, or
     LIKELY DIFFERS (unverified: why);
  4. the emulator for 10 s.

  Counterexamples are replayed concretely (`cexmodel.py`, `cexreplay.py`) and mapped to our .cpp line.
- **Verdicts:** `PROVEN` overlays the differential verdicts (`tools/difftest/verdicts.py --apply-proofs`). A FAIL
  beside a proof stays FAIL, and a withdrawn proof restores the earlier verdict. Progress counts PROVEN as
  equivalent.

## Lessons (what cost time, what fixed it)

1. **The mutant self-test dominated runtime.** Proofs took 1–3 s, but rechecks took 100–4,900 s, because a mutant
   region that fails is retried under every coarser cut variant with full timeouts. The fix in progress: a mutant
   stops at its first unproved region, with a short timeout; a mutant that times out instead of failing gets one
   full run, so acceptance is never weakened.
2. **Pairing is the biggest source of both false proofs and false refutations.** Found and fixed:
   - constants paired by position;
   - globals swapped by shape alignment (`g_game` and `g_items`);
   - a lock-step pairing never checked (the withdrawn proof);
   - a symbols-file check that only looked at addend-0 symbols.

   Exact pairing from symbols files and markers is the highest-yield fix, as you found.
3. **Address annotations in comments can mislead the resolver.** A comment `// wind_noise 0x463a80 …` on one
   `vector4` constructor was taken as the address of every constructor, including the default one. It produced a
   spurious FAIL on `update_ai`, because `__vec_new` ran `wind_noise` as the element constructor.
4. **Equivalent mutants exist:** dead code after a noreturn call, `fxch` of two equal values, a block skipped by
   `cmp eax,eax; je`. Restrict mutants to covered code.
5. **The remaining blocker is frame layout, and it affects every layer.** Different slot assignment, frame size,
   and push-vs-mov argument passing, especially once a frame address escapes to a callee. In `update_ai`,
   retail's `[esp+0x84]` is ours' `[esp+0x48]`, and frames are 0xa0 vs 0xb0. 7 of the 8 unproved regions of
   `Operation::execute` (the 591-case interpreter; 14 of 22 angr regions proved) fail this way.
   **In progress:** a per-object frame model in R:
   - a partial slot correspondence and pointer relations (`&ours_slot ~ &retail_slot`);
   - equality only for live corresponding slots;
   - escaped objects must have equal contents over their extent;
   - esp may differ at calls when argument values correspond.
6. **Two other open gaps:** register relations "ours = retail + c" (one row, a loop pointer offset that
   displacements compensate), and x87 depth conflicts in the static walk (3 rows).
7. **The differential harness can be fooled by state.** A FAIL appeared only inside a full sweep and not on replay.
   Distrust a FAIL that does not replay.

## Files

| file | purpose |
|---|---|
| `tools/equiv/prove.py` | layer stack, acceptance, `--record` / `--recheck` / `--recheck-all`, `--remote` |
| `rename_core.py`, `rename.py` | rename layer with frame slots |
| `angr_layer.py` | whole-function angr and the region worker |
| `regions.py` | region mode: cuts, lock-step, fine cuts, backtracking |
| `ddec_layer.py`, `ddec/` | DDEC (STOKE in Docker) |
| `x87lift.py` | the x87 lifter |
| `facts.py`, `callconv.py` | kill and entry sets; stack argument bytes from decorated names |
| `func.py` | loaders (original image without `.reloc`; relocated fields derived) |
| `mutate.py` | mutants, including x87 ones |
| `sweep.py`, `record_sweep.py` | sweeps and recording |
| `quick.py`, `cexmodel.py`, `cexreplay.py` | quick check and counterexample replay |
| `remote.py` | angr on the laptop |
| `angr-requirements.txt`, `setup_angr_venv.py` | pinned angr environment |
| `test_*.py` | about 130 tests |

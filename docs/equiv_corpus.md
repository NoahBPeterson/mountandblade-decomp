# The cross-compiler equivalence corpus: baseline

The corpus (tools/equiv/corpus/: build_pairs.py, index_pairs.py) pairs each function of a byte-exact decomp's /O2
units compiled from one source by cl 13.10 ("ours") and by cl 14.00 ("retail", /GS dropped). Each pair is equivalent
by construction, so a pair that does not prove shows where the prover is blocked. The decomp has no licence, so this
file gives only aggregate counts; the rows stay in work/equiv/corpus/.

Run: `tools/equiv/corpus/sweep_corpus.py --n 300 --ident 50 --seed 1 --jobs 5 --remote --remote-only`. That is
prove.py --objpair (tools/equiv/objpair.py) with rename, angr and region, no mutants and the frame model off.
Pass 1 allows angr 60 s and region 300 s; pass 2 reruns the 53 pairs that timed out with angr 120 s and region
1200 s. The sample is 50 differing pairs per size bucket where the bucket has enough (the 1000+ bucket has 32), with
frame-size and escaping pairs weighted 3:1. Total compute was 12,712 s summed over pairs, on the laptop's solvers.

## Baseline (2026-11-14, before the fixes below)

| | proved |
|---|---|
| identical (positive controls, layers run without the byte-exact shortcut) | 49 / 50 |
| same-frame (differing, equal frame size) | 39 / 209 |
| frame-size (differing frame size) | 1 / 91 |

Proof rate of the differing pairs by size (instructions, cl 14 side):

| bucket | proved | of them escaping | of them frame-size |
|---|---|---|---|
| 0-20 | 22 / 53 (42%) | 5 / 7 | 1 / 6 |
| 20-50 | 12 / 54 (22%) | 5 / 27 | 0 / 4 |
| 50-100 | 5 / 54 (9%) | 3 / 41 | 0 / 6 |
| 100-300 | 0 / 54 | 0 / 50 | 0 / 16 |
| 300-1000 | 0 / 53 | 0 / 49 | 0 / 37 |
| 1000+ | 1 / 32 (3%) | 1 / 32 | 0 / 22 |

The proving layers were rename 56, angr 25 and region 6. The one REFUTED pair was refuted at L2: its kill sets
differ, so the proof was attempted at L2, and edx differs at the return. That counterexample is genuine, not a
false refutation.

Blockers of the 261 differing pairs that did not prove. Each pair is counted under the first failing region's
reason (the region layer gets furthest), else the angr layer's:

| blocker | pairs |
|---|---|
| call count differs (inlining: one compiler inlines a callee the other calls; facts differ, so L2) | 60 |
| frame: esp differs at a cut (frame sizes, push vs mov) | 37 |
| fault exit: VEX's SIGSEGV exit of movaps (alignment), which the region gave up on | 35 + 8 at L2 |
| frame layout with an escaped frame address | 27 (+2 at L2) |
| call target (an indirect callee's pointer reloaded around argument pushes; aliasing of fresh argument slots) | 22 |
| register read only because an unknown callee's convention might read it (ecx/edx of virtual calls) | 18 |
| pairing check (a symbol cl 14 does not reference, such as `__real@00000000` loaded with fldz or an inlined `___rdtsc`) | 12 (+2 at L2) |
| solver undecided (float) | 6 (+2 at L2) |
| register relation | 5 (+2 at L2) |
| worker/remote | 5 |
| x87 float moves | 4 (+4 x87 values at L2) |
| switch / indirect, unsupported | 3 + 1, 3 |

The x87 relation of the 300 differing pairs was computed statically (sweep_corpus.x87_class):

| relation | pairs | proved |
|---|---|---|
| no x87 code | 161 | 39 |
| moves: only data movement differs (cl 14 copies floats with fld/fstp, stores 0.0/1.0 with fldz/fld1 and stores one value to several places with fst, where cl 13.10 uses integer movs) | 89 | 0 |
| ops: the operations differ (30 of them also differ in call count) | 40 | 0 |
| same x87 sequence | 8 | 1 |
| order: same operations, other order | 2 | 0 |
| literal: only how literals are loaded | 0 | 0 |

`__real@` references: 970 in cl 13.10 against 367 in cl 14, and cl 14 has fewer in 54 of the 300 pairs. So the
pilot's "x87 order" was data movement, not order or literal use. In the 89 moves pairs the first failure is a frame
blocker (esp delta 18, escaped frame layout 16) or a fault exit (17) more often than the x87 rule (4). Those pairs
need both the frame model and an x87 rule that accepts float copies (fld m32 / fstp m32 against an integer move: a
bit copy, except that a signalling NaN is quieted).

## Fixes since the baseline

- **Fault exits.** angr_layer turns a feasible fault exit into a trap event instead of giving up. The other
  version must fault on the same inputs. This removes the 43 fault-exit blockers as such.
- **Pairing.** pair_by_symbols leaves a field unpaired when no retail field holds its symbols-file address, instead
  of failing the whole pairing check. Rerunning the 41 pairing pairs: pairing is gone as a blocker, but none proved.
  Their next blockers are call count 27, esp delta 6, escaped frame layout 4 and others 4.

A second baseline with these fixes (work/equiv/corpus/baseline2.jsonl) is the reference for the frame model's delta.

## Second baseline and the frame model's delta (2026-11-14)

The second baseline (same sample and timeouts, the fixes above, frame model off) was finished with `--resume`. 67
pairs first ended in solver-slot errors (a second job was using the laptop's slots) and were rerun alone.

| | baseline | second baseline |
|---|---|---|
| identical | 49 / 50 | 49 / 50 |
| same-frame | 39 / 209 | 44 / 209 |
| frame-size | 1 / 91 | 1 / 91 |

The proving layers were rename 56, angr 29 and region 7. The blockers of the differing pairs that did not prove
were call count 59, esp delta 45, escaped frame layout 34, call target 26, assumed callee reads 18, register
relation 13, solver undecided (float) 13, x87 float moves 8, worker 5, and others.

**Frame model, stage 1** (`--frame-model`, the 256 pairs the second baseline did not prove): **no new proof**. The
blockers move further into the functions without clearing them. Escaped frame layout went from 34 to 46 and esp
delta stayed about the same: 24, plus 24 that sweep_corpus counts as "unsupported" because the frame model names
the missing word "below the cut's esp". Those pairs are frame-size pairs whose esp differs at the stop. Call count
stayed at 60. Every one of these needs stage 2 (the esp delta, pointer relations for escaped addresses) or work
outside the frame model.

## Same-compiler perturbation corpus (2026-11-14)

`tools/equiv/corpus/perturb.py` takes M&B byte-exact functions (the manifests) whose original allocates a frame of
8+ bytes and has 8 to 400 instructions (874 functions). It copies each source from git HEAD into work/, changes one
function in a way that keeps its behaviour, compiles the copy with cl 13.10 and proves the variant against the
original function (M&B mode: rename, angr and region, no mutants, angr 60 s and region 300 s). The perturbations
are: swap two top-level declarators (scalar, pointer or scalar-array types, initialisers without side effects);
an unused volatile int or a volatile char[8/16/32] with one store; and split a fully reassigned local into two.
Variants with the same code, or with the same frame size and the same esp-relative displacements, are dropped.

Generation stopped after 164 functions (32 had nothing to perturb or were not found in their source). Of the 576
compiled variants, 21 had the same code and 7 the same layout. That left 548 with a different frame layout,
of which 5 were the behaviour-changing controls, so 543 were measured. The pads dominate: cl 13.10 keeps most
scalar locals in registers, so declaration swaps changed the code in 15 cases only, and the two splits never did.
"Escaped" means either version computes a frame address (`lea r, [esp+k]`, `mov r, esp`).

| class | variants | flag off | flag on | new |
|---|---|---|---|---|
| same frame size | 59 | 34 | 35 | +1 |
| same frame size, escaped | 164 | 22 | 24 | +2 |
| frame size differs | 72 | 12 | 12 | 0 |
| frame size differs, escaped | 248 | 1 | 1 | 0 |
| all | 543 | 69 | 72 | +3 |

With the flag off, angr proved 40 and region 29. By perturbation (flag on): swap 11/15, pad-v 17/132, pad-a8 21/132,
pad-a16 19/132, pad-a32 4/132.

Blockers with the flag on:

| | same frame size (189 not proved) | frame size differs (307 not proved) |
|---|---|---|
| esp delta (esp differs at a cut, a stop or a call; includes the frame word "below the cut's esp") | 16 | 273 |
| a call after a frame address may have escaped (the callee may reach the frame: compared at the same address) | 66 | 15 |
| a call with no code evidence of its argument bytes (mostly virtual/indirect thiscall calls: unknown `ret N`) | 35 | 13 |
| a read of ours through a pointer that may point into the remapped frame | 13 | 1 |
| recursive call, x87, pairing and others | 59 | 5 |

**Negatives.** All 10 behaviour-changing controls (a swapped local's literal initialiser + 1) were not proved,
with the flag on and all three layers. Self-test with the flag on, run on the 3 new proofs plus 19 region proofs
made without the flag and reproved with it (fast mutants, 6 to 8 each):
- 12 proved and rejected all of their 76 mutants.
- 1 did not reprove.
- 9 had a "proved" mutant. I checked each by hand, and all are equivalent mutants. Eight are a store into the
  dead volatile pad slot that was dropped, moved or changed; cl often reuses that slot, and in one case a
  `rep movsd` into a temporary overwrites it. The ninth swaps `mov edx, [esp+0x18]` with the store of edx, and
  both values are the same constant.

prove.py rejects such proofs, because the self-test fails: this is the dead-store / self-test tension LAYERS.md
describes. No false proof was found.

**What stage 2 unlocks.** In this corpus the stage-1 machinery itself (F carried across cuts, frames kept across
non-escaping calls) was rarely the missing piece. 273 of the 307 unproved frame-size variants stop on the esp delta
g. Of the 189 unproved same-size variants, 66 stop at a call after an escape and 13 at an untrusted pointer read
of ours. Both need the pointer relation over whole objects. Another 35 + 13 stop at calls whose argument bytes have
no code evidence (virtual thiscall calls). Inferring their `ret N` from the caller's own stack balance would let F
survive those calls too.

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

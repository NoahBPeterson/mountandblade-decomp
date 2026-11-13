# Equivalence layers: the interface prove.py calls

Copied from spore-decomp tools/equiv/LAYERS.md at commit 82de4ab6c (the same owner) and adapted to M&B (no /LTCG,
the original is work/mb.analysis.bin, cl 13.10 /O2 with much x87 code). M&B changes are marked "(M&B)".

`tools/equiv/prove.py <src> <va>` decides whether our build of a function is behaviourally equivalent to the
original function, as a stack of independent layers ("Swiss cheese"). This file is the contract every layer
implements.

## Inputs (tools/equiv/func.py)

`Func`: one side of the comparison.
- `va` (int): where the code sits. The original: its VA. Ours (M&B): 0, the offset in its object section; a layer
  that executes ours places it at the original's VA itself (angr, ddec).
- `code` (bytes): the function's bytes, up to its switch tables (func.code_end; (M&B) for ours the table address is
  the target of the indexed operand's DIR32 relocation, func.obj_code_end).
- `relocs` (dict offset -> token): every 4-byte relocated operand, as a token naming what it refers to: the original
  `("abs", target)` ((M&B) recognised by value, the image has no .reloc: a displacement/immediate holding an image
  address, and every rel32 branch leaving the function); ours from the object `("sym", name, coff_type, addend)`.
  Tokens are compared through one consistent bijection (the same trust the byte-match check places in relocations),
  plus chk.py's constant check: a DIR32 of ours to a `__real@`/`__int64@`/`??_C@` literal must point, in the
  original, at the same bytes (func.const_ok; every layer applies it).
- `name` (str): for messages.

`meta` (ours): `src`, `flags`, `symbol`, `ret64` (returns edx:eax), `obj`, `consts`.
Register facts (kill set, entry registers, `ret N` of the original callees) come from tools/equiv/facts.py.

`retail_func(va)` and `our_func(src, va, symbol=None, flags=None)` build them (ours compiled through
tools/difftest/slice.compile_obj, i.e. tools/matching/cl.sh and the laptop compile server).

## The layer API

Each layer is a module in tools/equiv/ with

    def prove(retail: Func, ours: Func, level: str, hints: dict | None, timeout: float) -> dict

- `level`: `"L1"` (calling contract: eax (+edx for 64-bit returns; (M&B) no eax for a void function whose original
  writes eax), every memory write with its order relative to calls, every call with its target and the arguments it
  can read, ebx/esi/edi/ebp/esp restored, DF) or `"L2"` (L1 plus every register, including ecx/edx/xmm, at every
  call and at every return).
- `hints`: the layer's own bundle entry from an earlier run, or None. With hints the layer must not search for what
  the hints give, only check it; wrong hints must make it fail (never silently fall back to trusting them).
- Returns `{"status": "proved" | "refuted" | "unknown", "seconds": float, "detail": str, "hints": dict}`.
  - `proved`: sound proof at that level. Loops: either fully covered (a checked bound with a reason) or proved with
    an invariant. A bounded-only result is `unknown`.
  - `refuted`: a concrete counterexample exists at the requested level (describe it in `detail`).
  - `unknown`: anything else, including timeouts and unsupported instructions. Never guess.
  - `hints`: what makes a recheck cheap (see Bundle). Keep it small; no instructions, no original output values.
- Must enforce `timeout` itself and leave no processes behind.
- Must not print or store game bytes outside work/.

## Self-test (mutation) contract

`tools/equiv/mutate.py` makes `k` mutants of `ours` (changed immediate, dropped store, swapped dependent
instructions, changed register, (M&B) a frame slot moved by 4). prove.py accepts a layer's `proved` only if the
same layer fails to prove every mutant (a layer that proves a mutant is broken for this function; prove.py then
goes on to the next layer).

## Acceptance rule (prove.py)

A function is EQUIVALENT when, in order:
1. `facts.py`: our kill set and entry registers equal the original's (needed unless the proof is at L2).
2. Some layer returns `proved` at `L2`, or at `L1` with step 1 passing, and passes the mutation self-test.
3. No layer that ran before it returned `refuted` (a refutation stops the run).
Layers run cheapest first: rename (ms), angr (seconds to minutes, loop-free or bounded loops), ddec (loops, no x87).
Per-layer time limits: rename 30 s, angr 120 s, ddec 600 s (`--timeout` sets all).

## The layers

- **rename** (rename.py, rename_core.py): register renaming, frame-slot renaming and reordering of independent
  instructions; never refutes.
- **angr** (angr_layer.py): symbolic execution with Z3, in its own venv (`python3.12 -m venv work/angr-venv &&
  work/angr-venv/bin/pip install angr==10.0.6 pefile capstone`, or $EQUIV_ANGR_PYTHON). Fully automatic: call
  sites, returns, loop heads, callee `ret N` and argument registers come from the code and our decorated names.
  Loops are accepted only with a checked bound (every state reaching the bound provably diverges). Modelling
  assumption: an address computed only from the entry state never points into the 1 MB below the entry esp, nor
  (M&B) at the return address and the function's own stack parameters ([esp, esp + 4 + W), W from the decorated
  name or the original's `ret N`): they are the function's own objects (C++ lifetime), and cl keeps by-value
  arguments in registers across stores through `this` on that basis. (M&B) The same holds for addresses computed
  from callee results or memory read after calls as long as no frame address has escaped on the path, and both
  directions are used when reading memory: a read through such an address skips the function's own pushes and
  locals, and a read of its own stack skips stores through such addresses (else every saved register reloaded
  after a float store becomes an If on a symbolic offset). Further (M&B): an unknown callee's `ret N` (indirect
  calls) is fixed by "the original returns with esp = entry esp", and each comparison is first tried with float
  terms abstracted to free variables per structural hash (an over-approximation), before the full float query.
  Counterexamples that rest on a guessed relocation pairing, assumed callee register reads, differences inside the
  function's own frame at L1, differing x87 sequences or supplied hints are reported as unknown, never refuted;
  (M&B) also a different frame size (esp at a call differs) and x87 operations modelled as uninterpreted functions.
  angr 10 note: claripy's `AST.identical()` returns True for different symbols; compare by hash.
  x87 (M&B): VEX models the x87 registers as 64-bit doubles, which matches the 53-bit precision control the MSVC
  CRT sets except for the wider exponent range, so a proof is kept only when both versions run the same x87
  instruction sequence (mnemonic and memory size). angr 10.0.6 implements none of Iop_SqrtF64, SinF64, CosF64,
  TanF64, AtanF64, ScaleF64, Yl2xF64, PRemF64 (fsqrt, fsin, fcos, fptan, fpatan, fscale, fyl2x, fprem raise
  UnsupportedIROpError); the layer installs fpSqrt for the first and uninterpreted functions for the rest. angr's own
  satisfiability check at every fork is skipped (LAZY_SOLVES; on x87 code it cost 26 s per fork and Z3 does not
  honour its timeout while bit-blasting floats); forks are pruned by a 0.5 s Z3 check on paths without float terms.
  Literal constants are paired by content and checked (func.const_ok) before solving, and (M&B) a pairing that
  puts one of our functions or globals at another address than the symbols file's unique entry for it
  (tools/difftest/resolve.SymbolDB) is rejected (ddec applies both checks too); before that, our symbols the
  symbols file knows are paired with the retail fields holding their address (the shape alignment swaps pairs when
  the compilers order instructions differently). A counterexample on a path where a frame address escaped to a
  callee is not trusted (the model then lets any pointer alias the whole frame). The function's own stack
  parameters are private like its frame (protected from callees, not compared at L1 calls while nothing escaped):
  cl spills into dead parameter slots. The proof reports the
  instructions of ours it executed, and the self-test mutates only those (code after a call to a noreturn
  function such as mb_error is dead in the model, and mutating it gives an equivalent mutant). Loops whose trip count depends on the inputs are
  given up at bound 4 instead of 64. Unsupported (unknown): switch tables, nested loops, loops that call or write
  memory, segment registers, calls into the function itself.
- **region** (regions.py, (M&B) opt-in: `--layers rename,angr,region`): the angr layer's REGION mode, a cut-point
  decomposition for what one whole-function run cannot do (switch tables, loops with input-dependent trip
  counts, functions too big for 120 s). Described in its own section below.
- **ddec** (ddec_layer.py, ddec/): DDEC from STOKE (commit 98d8a0f) in Docker, optional: skipped when Docker is
  missing, unknown when the image is. Build once: `docker build --platform linux/amd64 -t mb-stoke-ddec:98d8a0f-2
  tools/equiv/ddec` (spore's Dockerfile and patches plus ddec/x87/4-x87-lifter.patch; builds on top of -1).
  Each 32-bit function is translated to x86-64 code with 32-bit addresses kept, split at calls; invariants are
  learned from unicorn traces (DDEC_EQ_ONLY) and proved inductive by Z3; hints carry the cut points and invariants
  so a recheck learns nothing. x87 code is supported through x87lift.py, which rewrites the x87 stack code of both
  sides into SSE2 scalar-double code under its model (a)-(f) (see its docstring: values representable as doubles,
  constant precision/rounding control, operations as uninterpreted functions with only commutativity of add/mul and
  the swapped-compare rule, sticky flags never read, empty slots unobservable, condition codes kept across
  fld/fstp/fxch before fnstsw); STOKE's FP functions are then Ackermannised (STOKE_ACKERMANN=1). Rejected: the
  transcendentals, frndint, fxam, fldcw, 80-bit loads/stores, fcmov, fisttp. Unsupported: rep string instructions
  without a constant count, segment overrides other than the SEH link, switch tables.

## The region mode (M&B; regions.py, angr_layer.py `run_region`)

**Cuts.** A cut is (x, y, pi, d): an address x of the original, an address y of ours (placed at the original's
VA), a register correspondence pi (for each of the original's eax, ecx, edx, ebx, ebp, esi, edi and the flags: the
register of ours holding the same value, or nothing) and the x87 stack depth d. Its interface R(x, y, pi) relates
a state of the original to a state of ours when: for every pair (r, pi(r)) the values are equal; esp, DF and the
d values on the x87 stack are equal; memory at and above esp is equal (all of it, the frame too, at the same
addresses). Nothing else: registers outside pi are independent.

**Obligations.** S starts with the entry cut (x = y = the entry, pi = identity, d = 0). Every cut of S carries one
obligation: from any two states related by R, both versions either return with the same observables of the
level (L1: the calling contract of the angr model; L2: every register), or reach cuts of S with states related
by their R, with the same events in between (calls in order with their targets, the registers their convention
reads, memory at and above esp). Two ways to discharge one:

1. *Lock-step* (no solver). The instructions at x and y are the same up to pi and the relocation bijection: the
   same mnemonic, prefixes and operands, with the original's register r written as pi(r) in ours (esp only as
   esp), immediates and displacements equal or relocated fields paired by one bijection (the angr layer's trust,
   with func.const_ok and the symbols-file check), implicit registers identical, branch targets paired instead
   of compared (short or near). Every register the instruction reads must be related by pi; its full writes
   create pairs, partial writes keep a pair only when it was already related. So from R(x, y, pi) it reads equal
   values, writes equal values, takes the same branch: R(x2, y2, pi') holds at the successor pair. A call needs
   pi(r) = r for eax, ecx, edx (any convention may read them, also cl's same-TU custom ones) and for what the
   original callee reads before writing (facts.retail_entry), every register at L2; afterwards eax, ecx, edx
   and the flags are related to themselves (the same callee, paired by the bijection, got equal inputs) and the
   callee-saved pairs are kept (the ABI). A return needs the identity on eax, ebx, esi, edi, ebp (edx for a
   64-bit or unknown return type; every register at L2). A switch dispatch `jmp [r*4 + T]` with the same index
   register adds (T_original[k], T_ours[k]) for every k: both tables have the same length and the range check
   before it is compared like any other code. cl's alignment filler (lea esp, [esp]; mov r, r; nop) is stepped
   over on one side (it changes no state). Not lock-stepped: rdtsc/cpuid/in/out and similar, loop/jecxz.
2. *Angr region* (Z3). Both versions start at (x, y) in one symbolic state built from R(x, y, pi): the
   original's registers are the input symbols, ours' pi(r) gets r's symbol, ours' other registers get symbols
   of their own; shared memory; the x87 stack holds d shared values. Each version runs to the first STOP of its
   side: a block leader of S's projection on that side, a loop head (of the CFG with the switch edges, so every
   cycle contains one and regions are acyclic: no loop bound), a switch dispatch, a rep instruction; or to a
   return. Paths are paired as in the whole-function layer (branch bits with equal conditions, else every pair
   whose conditions overlap); where the event sequences differ, either path may be cut at an earlier block
   leader (a candidate cut: the two compilers laid out blocks differently). At each pair of stops the new
   correspondence pi' pairs every register live at the stop on either side (a sound backward liveness over the
   CFG: a call reads eax/ecx/edx and the callee's entry registers, a return its contract) with a register of
   the other side holding the same value (the same term, else one solver query for the same register); the
   pair is compared with R(x', y', pi'), and (x', y', pi', d') joins S. The region also requires both versions
   to run the same x87 instruction sequence on each path pair (the 64-bit model's rule).

**Fine cuts** (where a region stops). When the versions diverge early (update_ai 0x4465e0 diverges at +0x90 and
never lines up again), S's leaders leave one region over most of the function and its paths multiply at every
branch (more than 3000 paths). A region may stop earlier, at the JOINS of its stretch: on each side, the
instructions reachable from the region's start before a stop that have two or more predecessors there (paths from
the start re-converge). Three variants, tried finest first per region: `joins` (every join is a stop), `pdom` (only
the first join every path passes: the nearest post-dominator of the start that is a join), `none` (S's stops
only, the region as before); a variant whose stop sets equal an earlier one's is skipped, and a region that fails
under one variant is rerun under the next (its failure under the finest is what the report names first). The two
sides' joins are not paired statically: the angr region pairs them itself, as it pairs any stops, by running both
versions to their first stop and comparing the path pairs whose conditions overlap; the stop pair a path pair
reaches gets pi' from the registers with equal values (match_exit) and joins S like any region exit. A chain of n
diamonds whose register allocation and block order differ becomes n regions of two paths each (test_regions.py
`test_diamond_chain_*`); the region from a join starts with lock-step from there, so stretches that line up again
cost no solver at all. Regions are found one per round along such a chain; the workers stay alive across rounds
(angr_layer.py `--serve`, regions._Pool).
Why it stays sound: the stops of a region are a choice of where its obligation ends, never something assumed. The
obligation of a cut (x, y, pi, d) is unchanged: from R(x, y, pi) both versions return with equal observables or
reach cuts with R of those cuts and equal events in between; the stop pairs a region actually reaches are added to
S with exactly the R the region established there (pi' only from proved equal values, memory at and above esp, d'),
and S's own obligations then cover them (the induction below does not depend on which addresses are cuts). A
coarser variant tried after a failed finer one is a separate, complete obligation of the same cut; the walk is
rebuilt from the entry each round from the regions that proved, so a failed variant's exits are never in S.
Region proofs keep naming their variant in the hints (`"cut"`), so a recheck rebuilds the same S.

**Soundness.** By induction over the number of steps: if both executions start related by R at a cut of S, each
obligation says they reach the next cuts of S (or return) related by R with equal events, so the whole event
sequences and the return observables are equal; an execution that never returns passes infinitely many cuts
with equal events. The interface is the same object at both ends: what a region (or a lock-step instruction)
establishes at its exit cut is exactly R of that cut, and R is all the next region assumes (shared symbols only
for pi's pairs, memory only at and above esp, d only from the cut). A region whose exit has a different pi or d
than an existing cut adds a new cut (a different interface), never reuses the old one; two depths for one
(x, y, pi) are a conflict and the run is unknown. The walk is rebuilt from the entry each round, closed only
under the exits of regions S still needs, so a cut is never discharged by an obligation that is no longer in
the proof.

**Assumptions** on top of the angr layer's: memory below esp is dead at every cut (a region that reads the stack
below its start esp at a known offset is refused); the stack is not inside the module image; a switch index stays
inside its table; the original's linker may have folded identical functions (identical COMDAT folding), so two
callees of ours may be paired with one original function when ours' object defines the function with the
original's bytes, or the symbols file names the original with the same member and signature (a call relocation
only, never data). At a cut the region symbols carry no provenance, so the whole-function
"entry pointers do not point into the frame" assumption is applied only where a static forward PROVENANCE pass
over each version's CFG proves it (regions.Side.provenance): the registers that cannot hold a frame address
(not computed from esp, not loaded from memory or returned by a callee after a frame address escaped: was stored
to memory or held in a register at a call; a frame address stored into the frame itself, `push ebp` after `mov ebp,
esp` in an aligned frame, escapes only at the next call, where it may be a stack argument, and until then loads
through esp or a frame pointer count as frame addresses), memory read at the cut when no frame address escaped
before it on any path, and the stack depth of the cut (known only when every callee's `ret N` before it is known
and the joins agree, a lower bound after `and esp, -2^k`; the frame zone then ends at or below the return address
and the function's own parameters: a smaller zone assumes less). With that zone, a read through such an address
skips the region's writes anywhere in [esp - 1 MB + 16, esp + 4 + W) (the whole frame above the cut's esp, not
only below it), and a read of a constant address inside the module image skips writes at esp-relative addresses
within 1 MB of esp (the stack-not-in-image assumption). Path conditions are compared first with float terms
abstracted (unsatisfiable there is unsatisfiable); with float terms the full query is skipped and the pair is
treated as possibly overlapping (more pairs compared, never fewer). Either
version's proof suffices (the related values are equal and the frame zones are the same addresses). The private
frame is not protected at calls (a frame address may have escaped): every callee may write all memory except the
callee-saved registers the region itself saved (the whole-function model's trust). That makes it weaker, never
stronger: frame temporaries that the two compilers put in different slots fail a region when a frame address
escaped (a callee might read them), and so do different frame sizes (esp differs at the cut). A region never
refutes (its interface is stronger than equivalence); a failure is `unknown` naming the region.

**Self-test.** The proof reports the instructions of ours its angr regions executed; prove.py's mutants are drawn
from them (with no angr region, from the whole function). A mutant changes ours' code: the walk reruns (a mutant in
lock-stepped code becomes a divergence and gets its own angr region) and the regions whose executed bytes changed
rerun (the others are reused from a per-process cache keyed by cut, bijection and executed bytes).

**Hints** (bundle): `{"mode": "region", "bij": [...], "folds": [...], "regions": {"<x>,<y>,<pi>": {"cut":
"joins"}}}` (`cut` absent: `none`, the bundles before fine cuts); a recheck re-walks (deterministic), requires every
angr region it needs to be named in the hints and runs each under its recorded variant only.

## Bundle and record (M&B)

`prove.py --record` writes the row of docs/equiv_proofs.json (`rows`: va -> {src, symbol, layer, level, detail,
build, flags, bundle, proved}; tools/difftest/verdicts.py --apply-proofs reads it) and one bundle per function,
docs/equiv_proofs/<va>.json when it is at most 2 KB + 1 KB per KB of function code, else work/equiv/proofs/<va>.json:

    {"va": "0051db70", "level": "L1", "retail_fp": "<sha256 of the original's code>", "build": "<func.build_hash>",
     "src": ..., "symbol": ..., "flags": ..., "facts": {"kill": [...], "entry": [...]}, "layer": "angr",
     "hints": {...layer-specific...}, "tools": {"prove": "2", "angr": "10.0.6"}}

Layer hints: rename: none; angr: relocation bijection, loop bounds with their reason, path pairing; ddec: segment
cut points and the learned invariants. `prove.py --recheck <va>` (or `--recheck-all`) re-verifies a row: when our
build hash still matches, the recorded layer checks the recorded hints; when it does not, the function is proved
again with every layer; `--record` writes the result back (or removes a row that no longer proves).
tools/equiv/record_sweep.py records the EQUIVALENT rows of a sweep (tools/equiv/sweep.py), re-proving any whose
build changed since.

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

**(M&B) Fast mutant rejection.** A mutant only has to be *not proved*; it does not need a full, decided failure.
So prove.py runs each mutant through the proving layer's `prove_mutant` (angr_layer, regions) instead of a full
`prove`: a short timeout (region: `FAST_PER_REGION` = 15 s per region, stop at the first region that does not
prove, no coarser cut variant, no backtracking; whole-function angr: `FAST_TIMEOUT` = 30 s), reusing the original
proof's hints — the relocation pairing, and in the region layer the per-region cut variant for every region the
mutant does not touch (those come straight back from the per-process cache keyed by cut/bijection/executed
bytes). Before: a failing mutant region was retried under every coarser variant with full timeouts and
backtracking, so rejecting a mutant cost 1,700–4,900 s where the proof took 1–3 s (0x46c4f0, 0x4556d0).
`EQUIV_FULL_MUTANTS=1` restores the old full-run behaviour.

Soundness. A proof is accepted iff *every* mutant is not proved, so shortening a mutant run is conservative only if
it never turns a mutant the full run *would prove* into "not proved" by accident — i.e. a mutant that the full run
would prove must still be caught. A fast run can end two ways. (i) It *proves* the mutant (reusing the same hints
and cut variants the real proof used): then the mutant is caught exactly as before — the self-test fails. (ii) It
does *not* prove the mutant: this is a weaker statement than the full run's, so prove.py may not reject on it
alone. It rejects on a fast failure only when that failure is *decided* — the model found a real difference (a
region's "difference in the model", or "different x87 instruction sequences"), with no resource limit or
unsupported construct in the reason (`regions._decided` / the `decided` flag on the result; `UNDECIDED` names
timeouts, path-count limits, solver-unknown, unsupported ops, a hint that does not fit the mutant, a conflict). A
decided difference is a genuine divergence the full run would also find, so rejecting on it is sound and can only
make the self-test *stricter*, never looser. An *undecided* fast failure (timeout, too many paths, …) proves
nothing, so prove.py reruns that one mutant with the full layer (`fast-undecided+full` in the self-test log) and
the old guarantee holds. Thus a mutant the full run would prove is always caught: either the fast run proves it
too (case i), or the fast run's failure is undecided and the full rerun proves it. The fast run's own hints can
only help it prove (they are checked, never trusted), so they cannot hide a provable mutant.

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
- **angr** (angr_layer.py): symbolic execution with Z3, in its own venv, pinned (M&B):
  tools/equiv/angr-requirements.txt lists every package of work/angr-venv with its exact version (angr, claripy's
  angr 10 copy, z3-solver, pyvex, archinfo, cle, capstone, pefile, ...; Python 3.12), and
  `python3.12 tools/equiv/setup_angr_venv.py` (Windows: `py -3.12 tools\equiv\setup_angr_venv.py`, or `--python`,
  `--dest`) builds the venv from it with `--no-deps`; `--check` compares an existing venv with it. Another
  interpreter goes in $EQUIV_ANGR_PYTHON. Fully automatic: call
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
  (tools/difftest/resolve.SymbolDB) is rejected (ddec applies both checks too). (M&B) The check covers ADDENDED
  references, not only addend-0 symbols: a DIR32 of ours to `sym+k`, when `sym` has one known address, must pair
  with the original's address `symbols-file(sym) + k` (`angr_layer.symbol_want`; 0x520270 paired
  `g_skill_tbl+0xbc` with the retail field at `g_skill_tbl+0x78` and passed before this). A call (REL32) names the
  symbol itself, no addend. The region walk applies the same check per field (`regions._field`): the lock-step
  walk diverges at a field it cannot pair this way instead of committing a pair that `prepare` would then reject.
  Before that, our symbols the
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
register of ours holding the same value, or nothing; or the same low 8 or 16 bits only, written `ecx:8`, `ecx:16`)
and the x87 stack depth d. Its interface R(x, y, pi) relates
a state of the original to a state of ours when: for every pair (r, pi(r)) the values (the low bits a partial pair
names) are equal; esp, DF and the
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
   of compared (short or near). Every register the instruction reads must be related by pi, as wide as it reads
   it (al: 8 bits, ah or ax: 16, otherwise and as an address 32); its full writes create pairs; a partial write
   (equal values: its inputs are related) relates the low bits that were related before or that it writes when
   they form a prefix: `mov al, m` makes an unrelated eax `eax:8`, `ah` extends `:8` to `:16` and leaves an
   unrelated register unrelated (cl keeps bools in al over a garbage upper part; without this a join between the
   load and the `test al, al` lost the relation, 0x44bc30). So from R(x, y, pi) it reads equal
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
   the other side holding the same value (the same term, else one solver query for the same register; failing
   that the same low 16 or 8 bits, a partial pair); the
   pair is compared with R(x', y', pi'), and (x', y', pi', d') joins S. A partial pair at a region start gives
   ours' register the original's symbol in its low bits and a symbol of its own above them. The region also requires both versions
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
*Backtracking*: a finer cut can split code the compilers moved across a join (ours loads edx after the diamond, the
original before it: at the join edx is unrelated and the next region fails). When a region fails under every
variant and its cut was reached (directly or through lock-step) from the exit of a region proved under a finer
variant, that earlier region is rerun under its next coarser variant whose stops differ (computed without its own
exits in S); with none different its proof is put back. At most 8 per proof, never with hints. This too only
changes which complete obligation discharges a cut: the next round's walk is rebuilt from the proofs in use, so
the failed region's cut and the abandoned proof's exits are no longer in S.

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

## The per-object frame model (M&B; design)

The dominant blocker across all layers is frame LAYOUT: the two compilers assign the same logical local to
different frame slots, use a different frame size, or pass an argument by `push` where the other uses
`mov [esp+k]`. The region interface R above requires `esp` equal and ALL memory at and above the cut's esp equal
(the frame too, at the same addresses), so any of these fails a region even when the functions are equivalent.
Examples: update_ai 0x4465e0 (retail stores edx at [esp+0x84], ours at [esp+0x48]; the ctor's vector is [esp+0x40]
retail / [esp+0x24] ours; frames 0xa0 vs 0xb0; slots reused differently), Operation::execute 0x49fa10 (8 regions
fail on frame memory at a call/exit), the split-test rows 0x4396b0 0x41ca60 0x41b960 0x4cda90 0x4362e0 0x4daa10,
and most of the last sweep's "exit memory (frame)" and "esp differs at exit" unknowns.

This section specifies a sound extension of the region interface with a FRAME CORRESPONDENCE. It is a design: the
synthetic battery is tools/equiv/test_frame.py (the exact pairs the implementation must prove and must not prove);
the implementation threads the correspondence through R, the entry state, the cut key and the hints, as set out
here. It is written down in full because the change is to R itself (not a local relaxation), and R's definition is
what every region's soundness rests on.

**The correspondence.** A cut is extended to (x, y, pi, d, F). F is a partial injective map between the two
versions' FRAME SLOTS — byte ranges of the function's own frame, i.e. below the entry esp and at or above the
cut's esp, named as offsets from each side's ENTRY esp (`fr_a = entry_esp - off_a`, likewise ours), one entry per
live object: `F = {(off_a, off_b, size)}`, each a maximal range the code accesses as a unit (size from the access
pattern or the object's type; conservatively up to the next slot either side accesses). F also carries the frame
DELTA `g = esp_b - esp_a` at the cut (0 when the frame sizes agree so far; a known constant after the prologues
differ), so the two cuts' esp need not be equal — only `esp_b = esp_a + g`. R(x, y, pi, d, F) relates a retail
state to a state of ours when: the pi pairs are equal (as before); DF and the d x87 values are equal; `esp_b =
esp_a + g`; memory AT AND ABOVE the entry esp is equal at the same addresses (the caller's frame, the arguments —
unchanged); and for every `(off_a, off_b, size)` in F the bytes `mem_a[entry_esp_a - off_a, +size)` equal
`mem_b[entry_esp_b - off_b, +size)` (corresponding live slots, at each side's own offset). NOTHING is required of
frame addresses not in F (dead slots are unconstrained) and nothing of the relation between a frame address and
esp beyond g.

**Pointer relations.** A register or slot of ours holds `&ours_slot` relates to a register/slot of retail holding
`&retail_slot` exactly when `(off_retail, off_ours, size)` is in F. This is recorded alongside pi (a register may
be marked "points at the F-slot k" instead of "equal to pi(r)"). A frame address that ESCAPES to a callee must, by
this relation, point at CORRESPONDING objects: at the call the two pointers are compared via the relation (equal
modulo the known slot offsets), not by value, and the objects they point at must have equal contents over the
object's extent (size from the access pattern/type, else conservatively up to the next live object). The callee's
possible writes through the pointer then apply to BOTH corresponding objects (the same callee, equal inputs, writes
equal bytes into each side's slot), so F still holds after the call. `esp` differences at a call (a different frame
size, `push` vs `mov [esp+k]`) are allowed when the outgoing ARGUMENT VALUES correspond: the existing per-call
argument comparison by value, with a stack-pointer argument compared through the pointer relation instead of by
address.

**Frame-slot liveness.** A frame slot is DEAD at a cut (and left out of F, unconstrained) only when a sound
liveness proves it: every path from the cut writes the slot before reading it, AND its address has not escaped (or
escaped only to callees that return before the cut and never stored the pointer). Be conservative: an escaped slot
stays live to the end unless proven otherwise; a slot whose address is taken but not proven non-escaping is live;
the default is live. The analysis is a backward pass over the CFG per side (regions.Side), dual to the register
liveness already there, over constant esp/frame-pointer offsets; an access through an unknown pointer kills the
analysis (everything live).

**How F is obtained.** Two sound sources, checked the same way. (1) The rename layer's frame-slot bijection
(rename_core) already pairs ours' slots with the original's for the whole-function renaming; its pairs seed F. (2)
At an angr region's stop pair, for each live slot of one side, the other side's slot whose CONTENTS are provably
equal (the same term, else one solver query — exactly match_exit's rule for registers) and whose liveness/size
agree. Either way F is only a PROPOSAL until checked: the region must establish R(x', y', pi', d', F') at its exit
with that F' (every live slot pair proved equal there), and the NEXT region assumes exactly F' at its start (ours'
frame symbols set to retail's only on F' slots, independent symbols elsewhere). An F that omits a live slot, or
pairs slots that are not equal, fails the region — never a false proof.

**Soundness.** The induction is unchanged (over the number of steps, R holds at every cut both executions pass in
step). What changes is the content of R, and the three obligations that touch the frame:
- *A region establishes F at its exit.* The exit memory obligation is: memory at/above entry esp equal, and every
  F'-slot pair equal. A live slot the region cannot prove equal is not in a valid F' (every live slot must be
  paired and proved), so the region fails; a dead slot is provably never read after the cut, so leaving it out
  changes no observable of any continuation. Hence R(x', y', pi', d', F') is exactly what was proved.
- *The next region assumes only F.* Its entry state shares memory at/above entry esp and, for each F slot, one
  symbol placed at retail's `entry_esp - off_a` and ours' `entry_esp - off_b`; every other frame address gets an
  independent symbol per side. So the next region assumes no more than R, and a region that reads a dead (unshared)
  slot before writing it reads an independent symbol and cannot prove its continuation equal unless it does not
  depend on that read — sound.
- *Escapes keep F.* When a frame address escapes, it is compared to retail's by the pointer relation (corresponding
  slots) rather than by value, and the callee's havoc writes the SAME fresh bytes into both corresponding slots
  (one callee, equal inputs). A frame address with no F partner that escapes fails the region (it may be read, and
  the model cannot relate it) — the current behaviour, never weaker. esp at the call may differ by g; the argument
  windows are compared by each side's own esp offset (as the whole-function model already compares argument memory
  by offset from each side's esp), and a pointer argument via the relation.
This is strictly MORE RESTRICTIVE than plain equivalence only where F demands a pairing — it can fail an
equivalent pair (a live slot it cannot match), never prove an inequivalent one: F only ever REMOVES byte-equality
requirements that liveness proved irrelevant, and ADDS the slot-pairing and pointer-relation requirements, all of
which the region must discharge. A region still never refutes. The cut key gains F (two F's for one (x, y, pi, d)
are distinct cuts, a different interface); a depth/F conflict for one (x, y, pi) is `unknown`. Hints record F per
cut (the slot pairs and g) so a recheck rebuilds the same S.

**Self-test under F.** A mutant that moves a value to a different slot, or changes a slot's contents, must still be
rejected: if the slot is LIVE, F requires it equal and the mutant fails that pair; if a mutant makes a live slot's
contents differ from its corresponding slot, the region fails. A mutant in a DEAD slot is an equivalent mutant
(the slot is never read), exactly as a mutant after a noreturn call is — correctly not counted (the mutant set is
already drawn only from executed instructions). The battery in test_frame.py pins this: two locals swapped between
slots proves; a slot reused for a different value proves when the slot is dead, not when live; an escaped object at
different offsets passed to the same callee proves with equal contents, not with one differing field; frame sizes
0x20 vs 0x24 prove; push vs mov argument passing proves; a pointer to the wrong object is not proved.

**Status.** Tasks 1 (fast mutant rejection) and 2 (the addended symbols-file check) are implemented and tested.
A SOUND SUBSET of the frame model above is implemented behind a flag (`EQUIV_FRAME_MODEL=1` /
`regions.prove(frame_model=True)`), OFF by default and NOT used by `--record`. What it does, at a call or exit
memory obligation of a region (angr_layer `_frame_region_diffs`, fed by `regions.Side.frame_liveness`):
- it splits the obligation into memory at/above the entry esp (caller frame and arguments: byte-equal as before)
  and the function's own frame below it;
- a frame word that is DEAD from this cut on (a sound backward frame-slot liveness, dual to the register one; a
  call conservatively reads every frame byte as a possible argument) is dropped;
- when the frame is DEAD on the far side of the cut on both sides (`dead_after`: nothing reads it later — the
  correspondence is not carried past a call's whole-frame havoc or into the next region), the remaining live words
  are matched to the other side's by equal content or by the POINTER RELATION (a frame pointer stored and read
  back, recovered through the solver, relating to the other side's pointer when their targets are matched
  objects); every live word must match, or the region fails;
- otherwise (the frame is read later) the live words are required byte-equal at the SAME address, which is always
  sound; only `dead_after` enables cross-offset matching.

This proves the escape-to-callee case — a frame object passed to a callee at different offsets with equal
contents, the headline blocker — and rejects a differing field, a wrong object, a wrong swap and the mutants
(test_frame.py, `FRAME_MODEL=1`). What is NOT yet implemented, so still falls back to byte-equality (sound, not
proved): a frame object that is READ AGAIN after the escaping call (the callee-writes-equal-bytes-through-
corresponding-pointers part needs F carried through the havoc and into the next region — the full cut-key F and
F-aware havoc of the design above), and a different FRAME SIZE live at a cut (the esp delta g; R still requires esp
equal at cuts). update_ai 0x4465e0 needs the first of these, so it is not yet proved. **Known false proof** of this subset
(found 2026-11-13, test_frame.py `test_escaped_object_fields_matched_elsewhere_not_proved`, xfail under
FRAME_MODEL=1): live words are matched one by one by content and an escaped pointer relates when only its target
word is matched, so an object whose second field matches a word elsewhere proves although the callee reads
different fields. Stage 2 replaces this by a pointer relation over the object's whole extent with one shift. The flag stays off for
`--record` until the design's forward-carried F and esp delta are implemented and hand-checked, because an unsound
frame correspondence would record false proofs that tools/difftest/verdicts.py --apply-proofs applies
automatically.

**Self-test.** The proof reports the instructions of ours its angr regions executed; prove.py's mutants are drawn
from them (with no angr region, from the whole function). A mutant changes ours' code: the walk reruns (a mutant in
lock-stepped code becomes a divergence and gets its own angr region) and the regions whose executed bytes changed
rerun (the others are reused from a per-process cache keyed by cut, bijection and executed bytes).

**Hints** (bundle): `{"mode": "region", "bij": [...], "folds": [...], "regions": {"<x>,<y>,<pi>": {"cut":
"joins"}}}` (`cut` absent: `none`, the bundles before fine cuts); a recheck re-walks (deterministic), requires every
angr region it needs to be named in the hints and runs each under its recorded variant only.

## Object pairs (tools/equiv/objpair.py)

`prove.py --objpair OURS.obj RETAIL.obj SYMBOL` (also regions.py and angr_layer.py) checks two COFF objects of one
source compiled by two compilers (the corpus of tools/equiv/corpus: ours = cl 13.10, "retail" = cl 14). Such proofs
are never recorded (`--record` and `--recheck` are refused). The retail object is linked into a synthetic image
(sections from 0x401000, undefined symbols in an external area, DIR32/DIR32NB/REL32 applied), its relocated fields
come from the relocation table, and func.image() and the symbols-file stand-in serve that image for the run.
- *Pairing* is exact by name: every plainly named symbol of ours (`?`, `_`, `@`) must pair with retail's symbol of
  the same name (after `canon`: U__m128@@ = T__m128@@, __ftol2_sse = __ftol2); a name retail does not reference gets
  a fresh address that pairs with nothing. Labels are not names: a switch dispatch's table pairs entry by entry
  through the walk, every other label read as data is checked by content like a literal (func.label_consts, the M&B
  mode too).
- *Callee facts*: a callee defined in the retail object is read from the linked image as in the M&B mode; an
  external has no code, so facts.py treats it as unknown (kills every volatile register, reads eax, ecx, edx) and
  its `ret N` comes from its decorated name (stdcall/thiscall/fastcall: the declared argument bytes; cdecl: 0).
- *Callee trust* (stronger than in the M&B mode, where both sides call the same original code): a call pairs by
  name, but the two sides really call two different compilations of that callee. The model gives both the same
  fresh outputs (eax, ecx, edx, flags, the x87 and xmm registers, memory outside the protected frame) once their
  inputs are proved equal. That is sound for the values the callee defines (its results and its writes are equal
  for equal inputs, the two compilations being equivalent), and for registers it only clobbers as long as neither
  caller reads a register its own callee clobbers (no correct caller does: cl reads a register after a call only
  where its own callee preserves it). A defined callee's kill set and entry registers are the cl 14 compilation's;
  ours' caller relies only on what its own compilation of the callee preserves, so a register cl 14's callee
  kills but cl 13.10's keeps is modelled as killed on both sides (a failure, never a false proof). Equal inputs
  include the convention: a same-TU custom convention that differs between the two compilations shows up as
  unequal argument registers or stack bytes at the call.

## Bundle and record (M&B)

`prove.py --record` writes the row of docs/equiv_proofs.json (`rows`: va -> {src, symbol, layer, level, detail,
build, flags, bundle, proved}; tools/difftest/verdicts.py --apply-proofs reads it) and one bundle per function,
docs/equiv_proofs/<va>.json when it is at most 2 KB + 1 KB per KB of function code, else work/equiv/proofs/<va>.json:

    {"va": "0051db70", "level": "L1", "retail_fp": "<sha256 of the original's code>", "build": "<func.build_hash>",
     "src": ..., "symbol": ..., "flags": ..., "facts": {"kill": [...], "entry": [...]}, "layer": "angr",
     "hints": {...layer-specific...}, "tools": {"prove": "2", "angr": "10.0.6", "angr_lock": "<sha256[:12]>"}}

(M&B) `tools` records the versions a proof was made with: angr's, and for the angr and region layers `angr_lock`,
the sha256 prefix of tools/equiv/angr-requirements.txt (the pinned venv: angr, z3-solver, pyvex, ... exact). A
proof made with a venv that does not match the lock file (`setup_angr_venv.py --check`) is not reproducible.

Layer hints: rename: none; angr: relocation bijection, loop bounds with their reason, path pairing; ddec: segment
cut points and the learned invariants. `prove.py --recheck <va>` (or `--recheck-all`) re-verifies a row: when our
build hash still matches, the recorded layer checks the recorded hints; when it does not, the function is proved
again with every layer; `--record` writes the result back (or removes a row that no longer proves).
tools/equiv/record_sweep.py records the EQUIVALENT rows of a sweep (tools/equiv/sweep.py), re-proving any whose
build changed since.

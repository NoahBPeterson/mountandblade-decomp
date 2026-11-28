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
  instructions, and (M&B) instructions that only move values (below); never refutes.

### rename: value flow (M&B, 2026-11-18)

The rule before paired instructions one to one, so a function whose register allocation differs by one copy, one
spill and reload, or a frame allocated by `push ecx` instead of `sub esp, 4` failed at "instruction count N vs M".
Now some instructions are TRANSPARENT (rename_core._transfer): their only effect is to move a value, exactly, from
one location to another, or to put a constant there:
`mov r32, r32`; `mov r32, imm32`; `xor/sub r32, r32` (a zero); and in slots mode (no frame address escapes in either
function) `mov [slot], r32`, `mov [slot], imm` (1, 2 or 4 bytes), `mov r32, [slot]`, `push r32`, `push imm`,
`pop r32`; `add/sub esp, imm` and `lea esp, [esp+d]` (the static depth already covers them); cl's in-block filler
(`lea r, [r]`, `mov r, r`). Never: a relocated field, a memory operand other than a frame slot of the slots mode, a
partial register, the `mov eax, N` of a `__chkstk` frame. Transparent instructions are not paired; each block must
have as many non-transparent instructions on both sides, and those are paired as before (same shape, memory order).

*The value pass* (rename_core.values) replaces the reaching-definition sets by VALUES, an SSA construction over the
(identical) CFG: a forward pass whose state maps each resource (register family, flag group, x87 state, frame byte)
to the value it holds: `entry r`, `undefined` (a frame byte nobody wrote), `def k` (an output of a non-transparent
instruction k), a constant, `byte(v, j)` / `cat(b0..b3)` (a 32-bit value moved through four frame bytes; `cat` of the
four bytes of v is v, `byte` of a constant is its byte), `opaque k` (the flags a transparent xor or esp adjustment
writes: equal to nothing), or `phi(n, r)` where the predecessors of block n disagree (its incoming values in
predecessor order; a phi whose incoming values are one value v, or itself, is v). A transparent instruction
transforms the state by its exact effect; a non-transparent one records the value each use slot reads and defines
`def k` for each output. A call makes every frame byte below its esp undefined again (the callee's own frame, at
least its return address, goes there; before, a slot freed before a call and re-allocated after it kept the old
value). A transparent instruction whose opaque flags a non-transparent instruction reads (through phis too) is
paired after all (the pass reruns).

*Comparison.* For every pair (a, pi(a)), each use slot's values correspond: `def k` with `def pi(k)` (same output
slot), entry with the same entry, constants by value, bytes and words structurally, and two MERGED values at the
same block (a phi; the word of four phis of consecutive frame bytes, whose incoming values are the words of the
incoming bytes; a byte of a phi) are recorded as a pair. At the end every recorded pair must have corresponding
incoming values along every predecessor edge (new pairs found there are checked too: the greatest fixpoint, as for
a loop's merge of its own back edge). Pins (ABI uses at calls and rets) are use slots like any other, so the
calling contract is compared on values: a callee-saved register restored through a transparent push/pop holds
`entry r` again.

*Why it is sound.* By induction over the joint execution (both functions take the same blocks, as before: the
CFGs are identical and every branch reads corresponding values). Claim: whenever two related values are both live,
their most recent instances are equal. The state after a transparent instruction is its exact effect, so a
location's value term is always the value that location holds (a copy, a spill, a reload are identities; a cat of
the bytes of v is v; the frame bytes are private: no frame address escapes in slots mode, so no store through a
pointer and no callee except through the argument bytes it is given, which the call defines, can change them, and
the call's undefining of the bytes below esp covers its own frame). A paired instruction computes the same
operation on related inputs, so its outputs are equal (the existing shape rule). A merged pair at block n: both
executions enter n along the same edge, whose incoming values are related, so the merged values are equal; that
the instances agree holds because the pass builds SSA (the fixpoint puts a phi wherever predecessors disagree, the
entry counting as a predecessor of block 0, and removes only trivial phis), so the node a use reads DOMINATES it: a
`def k` read in k's own block is from an instruction before the use, in another block from the block's latest visit,
which is the same visit in both executions (the paired k and pi(k) are in the same block, and pi(k) dominates pi(u)
likewise). The earlier reaching-set rule had no such node: a copy made before a loop's increment and the increment
itself gave the same set {entry value, increment}, which is why sets alone cannot carry copies
(test_loop_copy_placement). Thus at every call and ret the pinned uses read equal values, and memory operations
(never transparent) happen in the same order with equal operands: the observables of the level are equal. A
mismatch anywhere is `unknown`, never a refutation.

*Tests* (test_rename.py, value flow): accepted: an extra copy; a value kept in esi across a call vs spilled to a
slot allocated by `push ecx`; a spill kept apart from the pushed argument; a loop variable copied after its
increment; a reload done twice instead of once plus a copy; `push ecx; push ecx` vs `sub esp, 4`; a value merged in
a frame slot vs in a register. Rejected: the copy's source changed instead of its destination; the spill slot
overwritten before the reload; the reload without the spill; the register used across the call (clobbered); the
slot freed before the call and read after re-allocating it; reading back a pushed argument after the call (the
callee owns it); the copy made before the loop's increment (the merges differ); the slot overwritten between two
reloads; an extra move through an ESCAPED frame (delta mode: not transparent); xor whose flags are read vs a mov;
and every mutant of the accepted pairs (test_value_flow_mutants_rejected). Two mutants of other accepted pairs are
EQUIVALENT and proved (a reload swapped past an instruction that reads the register the reloaded value is still in):
the self-test then fails for such a function, which only costs the proof. `rename_core.TRANSPARENT = False`
restores the one-to-one rule.

### rename: CFG map (M&B, 2026-11-18)

The rule before required the same blocks in the same layout order. Now (rename_core.cfg_map) blocks are paired
along their successors from the entry: a block that is only an unconditional jump within the function is a
FORWARDER (an edge into it is an edge to its target; a block's own closing `jmp` is not an instruction to pair,
except one carrying a WinLicense marker); a conditional jump pairs its fall-through and taken successors with the
other's in order, or crossed when the other tests the OPPOSITE condition (je/jne, jb/jae, jbe/ja, jl/jge, jle/jg,
js/jns, jp/jnp, jo/jno; the paired jcc must then be exactly that opposite). The pairing must be one to one and cover
every non-forwarder block of both functions. The value pass runs on the forwarded CFGs, and a merge's incoming
values are paired by corresponding predecessor edges (the block map; the entry with the entry), not by position.
*Soundness*: an unconditional jump only transfers control, so the forwarded CFG has the same executions; `jcc c, T`
falling through to F and `jcc not-c, F` falling through to T take the same block for the same flags, and the paired
jumps read the same flags (both conditions read the same flag group), which the use comparison requires to be
related. So both executions pass through paired blocks in step, which is all the value-flow argument needs. Not
handled (unknown): tail duplication (one block of one function paired with two of the other), a different block
split, conditions that are not each other's opposite. Tests: test_cfg_inverted_condition_and_layout (an inverted
jcc with its bodies swapped, a fall-through through a jmp block, an arm moved after the join; rejected: inverted
without swapping, another condition, a changed constant in the moved arm, a one-instruction arm that is a
transparent move and not a forwarder) and test_cfg_map_mutants_rejected.

*Measured* (2026-11-18, value flow + CFG map): the 216 WEAK rows still compiled: 0 proved before and after (now
blocked by CFG shape 110 — mostly tail duplication and different block splits, call counts 39, frame model 24,
shapes 22; most "shape" and "moving values" failures are DELTA mode, an escaped frame address, where moves through
the frame stay paired); every DIFF row: 0 new. The same-compiler perturbation corpus (work/equiv/perturb, frame
layout changes only): 86 variants newly proved by rename before the self-test, 26 of them with it (the rest prove an
equivalent mutant: a dead pad store changed), 0 of the 10 behaviour-changing controls.
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
  x87_pi_mode (P1-C): when the two x87 instruction sequences differ, the proof counts only if every path pair is
  proved in the float abstraction alone (float terms free per structural hash, so it holds for every precision and
  rounding control; Z3's own float semantics is never used for pairing or proof, and a difference is "unknown", never a
  refutation). Value-preserving normalisation: fadd/fmul operands sorted by hash (commutation of one operation;
  never fsub/fdiv, never regrouping). Double stores (any 8-byte store) are an uninterpreted function of the value, so a
  spill to a double is not the identity (it rounds at 64-bit precision); float stores are the F64toF32 term;
  operations on two constants are uninterpreted (no 53-bit folding). Excluded: fldcw/fnstcw, tbyte, fprem, fscale,
  f2xm1, frndint, fyl2x, fldl2e etc., SSE/MMX code.
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
`regions.prove(frame_model=True)` / `prove.py --frame-model`), OFF by default. Its proofs are recorded as a tier of
their own, **PROVEN-O** (below). What it does, at a call or exit
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
different fields. Stage 2 replaced this by a pointer relation over the object's whole extent with one shift (below);
the test is a plain rejection test now.

**The PROVEN-O tier (user decision, 2026-11-19).** A proof made with the frame model COUNTS, but as its own
labelled tier, separate from the unconditional proofs, because it is CONDITIONAL: it holds only if assumption O
(stage 2 below: a callee reaches the caller's frame only inside the complete objects of the frame addresses it is
given or can find, and both versions pass complete objects) holds, together with the frame model's other two
assumptions, T (a callee's effect does not depend on esp's numeric value) and I (a callee does not depend on frame
words no path has written). The tier is named after O, the one with real exposure (an out-of-bounds callee breaks
it); T and I are recorded with it. How it is made and kept apart:
- `prove.py --frame-model` (or `EQUIV_FRAME_MODEL=1`) first runs every layer exactly as without it (the region
  layer with `frame_model=False`, passed explicitly: the environment never reaches a layer behind prove.py's back).
  Only when none of them proves the pair does the region layer run again with the frame model. A proof that needs
  no frame model is therefore always plain EQUIVALENT / PROVEN, never downgraded.
- A proof from that second run is the verdict **EQUIVALENT-O** (its layer status is under `region-O`). It must pass
  the same self-test, with every mutant (fast and full runs) proved or rejected under the frame model too
  (`prove.py` `layer_mutant`/`layer_prove` pass `frame_model`; test_frame.py `test_prove_frame_model_tier` runs
  prove.py's own self-test under the model).
- `--record` writes the row with `"tier": "PROVEN-O", "assumes": ["O", "T", "I"]` and a bundle with
  `"frame_model": true`; an unconditional row has no tier (PROVEN). `--recheck` of a PROVEN-O row rechecks its
  hints under the frame model; a re-proof from scratch (build changed) tries unconditionally first again, so a row
  can move up to PROVEN but a PROVEN row moves to PROVEN-O only when it no longer proves without the model (and
  `--frame-model` was given).
- tools/difftest/verdicts.py turns the tier into the verdict PROVEN-O, category `proven_o` (never merged into
  `proven`); tools/progress.py shows it as its own tier row, `--plot` band and `--treemap` colour ("Proven under
  assumption O"); sweep.py, record_sweep.py and the corpus tools count EQUIVALENT-O apart from EQUIVALENT.
Known limits that still fall back to byte-equality (not proved, never a false proof) are listed under stages 1 and
2 below; a soundness bug found in the frame model withdraws the tier: `prove.py --recheck-all --record` after the
fix removes every row that no longer proves.

**Stage 3 (lane P2, 2026-11-27; the frame-blocked rows).** Three additions, the first two always on under the frame
model, the third behind its own flag because it strengthens O:
- *Static stack depth through indirect calls* (regions.py `_pops`, `check_pops`; angr_layer `do_call`, `arg_bytes`,
  `deferred_cleanup`). The frame model needs the depth below the entry esp at every cut; it was lost after the first
  virtual call (unknown `ret N`) and at ebp frames. Now: (a) an INDIRECT call (register / `[reg+disp]` operand) removes
  exactly the bytes pushed for it (straight-line pushes back to the first other esp write, a block start or a
  prologue save `push r`/`pop r` pair), unless the code right after it cleans up itself (`add esp` / `pop ecx|edx`: a
  cdecl callee: no assumption); the worker models `ret N` as that constant (both versions must assume the same N:
  observable "assumed ret N"); (b) a direct callee whose own code does not tell (SEH epilogue elsewhere, import) and
  is followed by the compiler's cleanup `add esp,k` / `pop ecx` within six straight-line instructions pops 0; a `jmp`
  thunk takes its target's pop; (c) `mov esp, ebp` of an ebp frame restores the depth `mov ebp, esp` ran at (ebp
  written by that one mov only); (d) a cdecl call whose cleanup is merged into a later call's (`call f; pushes; call
  g; add esp,N`) has N minus the net pushes / callee pops in between as its argument bytes. SOUNDNESS: the compiler
  tracks esp statically, so every path of the function leaves esp at the entry value at its ret; `check_pops` verifies
  it under the assumed pops (every ret at depth 0 on both sides, else the frame-model run is `unknown`), which pins
  the sum of the assumed cleanups; an individual N could still be wrong only by errors that cancel on every path to
  every ret (two indirect calls on one path with opposite errors). Mutant: tests test_frame_depth.py.
  Also: a relocated call of ours that the walk rewrote to the original's callee address is no longer taken for a call
  into the function itself when that address merely falls inside a LONGER build of ours (only the entry, or the
  original's own range, counts).
- *EQUIV_FRAME_OUP=1: assumption O-up* ("an object extends upward from the lowest byte the function takes of it"):
  `Side.lowest_taken` (static: every `lea r,[esp+d]` / ebp-frame `lea`; any other use of esp/ebp as a value or an
  indexed lea: nothing known). A frame word entirely below the lowest taken address of its version is in no object
  given to a callee: at a call it is not compared (both sides below it: skipped; one side only: a difference in object
  structure) and the callee's havoc leaves it alone (protected). A frame whose objects all sit at ONE uniform shift d
  between the versions may be read after the call: ours' havoc then writes the original's bytes at the address
  shifted by a symbol D that the call obligation pins to d (`dsh<k>`), the correlated havoc of the design above;
  several shifts, or d from a different object than the pointers pair, stay refused. NOT covered (so such pairs stay
  unproved): a callee that returns a pointer into the frame which is then dereferenced or compared; objects at
  different shifts; a callee writing past the object it was given (assumption O itself; no model can test it). O-up
  is stronger than O (an interior pointer to an object that extends below it breaks it), so a proof that needs it is
  recorded as such: prove.py's tier text says O; the report of lane P2 names every row that needs the flag.
  Tests: test_frame_oup.py (spill below every object: proves only with the flag; wrong spilled value, spill reloaded
  from the object, object contents differing: not proved; the same under a uniform shift with the frame read after the
  call).

**Stage 1 (implemented, same flag).** F is carried in the cut key (x, y, pi, F) with g = 0 (esp equal at cuts):
- *F at a stop* (angr_layer `_frame_exit_diffs`): memory at and above E equal at the same addresses; the frame words
  [esp, E) live at the stop on either side (frame_liveness, "all" at a call) are compared at the same address
  first: all equal gives F' = None, the standard R (it also assumes the dead words equal: every path writes a
  dead byte before it reads it). Otherwise F' pairs words with proved-equal contents (identity first, then the
  first equal word of the other side, one to one); F' is exactly what the next region assumes, a live word with
  no partner is left unrelated. The pairs are obligations of the path pair.
- *A cut with an explicit F* is never lock-stepped. Its region remaps ours' base memory (`frame_base`): ours'
  byte in an F slot is the original's symbol at the paired address, any other frame byte of ours a symbol of its
  own (epoch `0o`); frame addresses get canonical keys on both sides. A read of ours through a pointer that is
  neither esp-relative nor noted outside the frame (dz) may hit the remapped frame and is refused. Every probe
  comparison is restricted to addresses at or above E (a return must leave esp above E + 4); a call whose callee
  may reach the frame compares every frame word at the same address (the havoc then models one callee).
- *Non-escaping calls* (regions `_noesc_calls`: the static provenance pass proves no frame address escaped on any
  path to the call, none in a register at it, none stored into the frame, depth known): the havoc protects
  [esp+4+N, E), so each version keeps its own frame, and the call compares only the return address, the N argument
  bytes and memory at and above E. N comes from evidence in the code, never from a name (a decomp's declaration
  may be wrong): a callee's own `ret N` (N > 0), or for a callee that pops nothing the cleanup right after the
  call (`add esp, N` / pops; it removes at least that call's arguments), or 0 for a callee-pops convention whose
  code pops nothing; the two versions' N must be equal, and both calls protected or neither.
- *frame_liveness fixes*: a byte of an access is labelled by its own offset (byte k of [esp+disp] is E - (dep -
  disp - k); it was labelled by the access base, so an overlapping store at a higher address killed bytes it does
  not write, `test_frame_liveness_byte_offsets`), and pop reads [esp] (it was not a use; leave/enter/popa: all).
Not in stage 1: the esp delta g, frame-address pointer relations across cuts, frame pointers (ebp frames refuse
remapped reads), escaping calls with an explicit F (byte-equal at the same address). The dead-after subset's
escaping-call matching (and its known false proof) is unchanged.

**Stage 2 (implemented, same flag; 2026-11-14).**
- *The esp delta g.* A cut whose two static depths are known and differ (regions `_cut_dz`: `depth` the original's,
  `depth_o` ours') has ours' esp = the original's + g, g = depth - depth_o (E is the same in both executions). Such a
  cut always has an explicit F (possibly empty): its frames are related only through F, offsets from E. The worker
  starts ours at in_esp + g, remaps ours' frame [in_esp + g, E), and checks reads below each version's own cut esp.
  esp at calls and stops is no longer a plain observable under the flag: the frame obligation compares it. At a
  stop each version's frame is its own [esp, E) (F' explicit whenever the depths differ; both static stop depths
  must be known and equal to the computed ones). A protected (non-escaping) call compares its argument bytes at each
  version's own esp. **Assumption T** (new): a callee's effect does not depend on the numeric value of esp, only on
  the bytes it reads relative to it and the pointers it is given.
- *Escaping calls with known argument bytes* (`do_call` "wframe", `_frame_window_diffs`). **Assumption O** (new): a
  callee reaches the caller's frame only inside the complete objects of frame addresses it is given or can find,
  and both versions pass complete objects (no out-of-bounds access through them); an object lies in the frame above
  the bytes pushed for the call (regions `Side.call_pushes`) and outside the prologue's saved-register words
  (`Side.saved_slots`: pushes of ebx/esi/edi/ebp the function pops back, before it writes them). The havoc protects
  the saved words; the callee's own area (its arguments and below) gets havoc bytes of its own per version. A frame
  address the callee receives (an argument word, a register it reads, a frame address stored in a reached word) is
  related by its offset shift d = o_ours - o_orig, and every frame word the callee can reach relative to it must
  correspond under d: the original's o and ours' o + d for all o in W_orig with o + d in W_ours (the callee's
  accesses lie in both objects). d = 0 (the same address on both sides, also required for addresses that escaped
  earlier: provenance before the call's pushes) may be read after the call: the havoc gives both versions the same
  bytes there, which were equal before. d != 0 needs the frame dead after the call on both sides (each version's
  object is havocked at its own addresses; a returned or stored address then differs by d, which only a dangling
  pointer can observe). A pointer into the bytes pushed for the call is refused (`push ecx` allocates locals).
  **Assumption I** (new): a word that no path from the entry writes before the call on either side
  (`Side.unwritten_at_calls`) holds an indeterminate value the callee does not depend on; written on one side only,
  it is a difference. The stage-1 dead-after content matching is gone, and with it the known false proof
  (`test_escaped_object_fields_matched_elsewhere_not_proved` is a plain rejection test now). Two stage-1 battery
  proofs (`_swap`, `_escape`) used a merged cleanup (`add esp, 0x10`/`0x44` after the call) whose N allows a callee
  that reads the swapped locals as argument words; they were proved by matching words by content regardless of
  position. Their fixtures now clean up separately; the merged versions are must-not-prove tests.
- *Frame pointers in registers across cuts.* A live register holding a frame address on both sides at different
  offsets is related as `v@ka,kb` in pi (the original's r = esp + ka, ours' v = esp + kb, the original's stop esp;
  exact, both are in_esp + k): the next region starts both at those addresses. Excluded from the dz no-frame set.
  A cut with such an entry is never lock-stepped (the walk relates registers by equal values only; it read
  `eax@4,8` as eax and lock-stepped a `push eax; call` whose pushed pointers differed: a false proof the corpus
  self-test found, fixed in 31ee262a, `test_frame_pointer_register_pushed_after_a_stop_not_proved`).
- *`__chkstk` frames.* The call's eax (each version's own allocation size) is not compared under the flag: the
  callee probes pages and moves esp by eax, and the esp delta that follows is the frame obligation's; its scratch
  eax afterwards is a symbol per version. The original's `__chkstk` is identified through the job's checked pairing
  (facts) for the static depth too (the walk diverges at the differing `mov eax, N` and never pairs the call).
- *Static depth through cl's alignment filler.* Under the flag the provenance pass steps over `lea esp, [esp]` (it
  lost the depth there, and with it every later stop's F and esp delta).
- *Region cache.* A region's frame obligations rest on whole-function static facts (frame liveness, non-escaping
  calls, saved words, never-written words, pushes, earlier escapes) and the cut's dz; under the flag the per-process
  region cache key covers them. Without this a mutant outside a region reused the original's region result (a
  self-test artifact, found on the perturbation corpus: a pad store moved into an object before its constructor
  looked proved). The flag-off cache key still covers only the cut, the pairing and the executed bytes (its regions
  read liveness and dz too; a stale hit can only make a mutant look proved, which fails the self-test).
- *Reads of ours through a symbolic pointer under an explicit F* are no longer refused: `frame_base_sym` builds the
  remapping as a term (inside an F slot of ours: the original's byte at the paired address; elsewhere in ours' frame
  a byte of its own; outside the frame the shared byte).
Not in stage 2: escaping calls whose argument bytes have no code evidence (virtual/indirect thiscall: unknown
`ret N`, no cleanup), a shifted escape whose frame is read after the call (the callee's outputs would need the
pointer relation in registers and memory), and frame pointers stored in memory at and above E with a shift.

**Self-test.** The proof reports the instructions of ours its angr regions executed; prove.py's mutants are drawn
from them (with no angr region, from the whole function). A mutant changes ours' code: the walk reruns (a mutant in
lock-stepped code becomes a divergence and gets its own angr region) and the regions whose executed bytes changed
rerun (the others are reused from a per-process cache keyed by cut, bijection and executed bytes).

**Hints** (bundle): `{"mode": "region", "bij": [...], "folds": [...], "regions": {"<x>,<y>,<pi>": {"cut":
"joins"}}}` (`cut` absent: `none`, the bundles before fine cuts); a recheck re-walks (deterministic), requires every
angr region it needs to be named in the hints and runs each under its recorded variant only.

## Judge hints (M&B, 2026-11-20; tools/equiv/judge_hints.py)

`prove.py --hints DIR` / `sweep.py --hints DIR` read an external static judge's pairings (vs2005-cpp
tools/judge.py, its docs/c2/judge.md section 4: one `<8-hex va>.json` per function with `regs`, `slots`, `globals`,
`assumed`, `at` per loop head of the original) and hand them to the region layer as CANDIDATES. Nothing is trusted;
each kind is used only where the layer's own machinery checks it:
- *Registers* (`regs`, `at[addr].regs`: ours' register holding the original's value). At a stop pair the worker's
  exit matching (angr_layer `_match_exit`) prefers a hinted partner among the registers with the same term (all
  equal: a choice, not an assumption), and for a live register still unpaired tries the hinted partner with one
  solver query under the path condition. The pair enters pi' only when proved equal there; pi' is the exit
  obligation of that region and exactly what the next region assumes (the induction of "Soundness" above is
  unchanged: a hint never adds an assumption, only a proved fact the layer did not find by itself).
- *Frame slots* (`slots`, offsets below the entry esp; frame model only). In `_frame_exit_diffs` a hinted partner
  of the original's live word is tried before the content search (one query, outside its budget); like every pair
  of F' it is an obligation of the path pair (diffs). Without the frame model frames are compared at the same
  address and slot hints do nothing.
- *Globals*. A hinted pair is kept only when the symbols file gives the symbol exactly that address, or the symbol
  is a literal whose bytes in the original are ours' (func.const_ok); it then fills a field of ours the pairing
  left unpaired, and prepare checks the pairing as always. Unanchored pairs (most of the judge's `assumed`) are
  dropped: nothing here can check them.
A hint that fails its check is dropped and logged (`info["judge"]`: used / rejected / dropped; prove.py prints each
line); it can at most cost the search a query. Mutants get the same candidates (they can only help a mutant prove,
so the self-test is never weaker). The region cache key covers the candidates, and a proof that used hints keeps
them in its bundle (`hints["judge"]`) so a recheck rebuilds the same cuts (and checks them again). Tests:
test_regions.py `test_judge_*` (a loop whose x+1 is `-(~x)` in eax against `inc edx`: unknown without the hint,
proved with it and through prove.py's self-test; a deliberately wrong hint rejected; the right hint on a version
that adds 2 rejected), test_frame.py `test_judge_slot_*`.

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
build, flags, bundle, proved}, plus `tier` "PROVEN-O" and `assumes` for a frame-model proof, "Status" above;
tools/difftest/verdicts.py --apply-proofs reads it) and one bundle per function,
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

A proof only counts for the build it proved. Every row's `build` is the fingerprint; `prove.current_proofs()` compiles
each row's function as it is today (the compile cache) and compares `func.build_hash` (code with relocated fields
zeroed, plus each relocation's offset and symbol). `verdicts.py --apply-proofs` and `sweep.py --skip-proven` use only
the proofs that match: a row whose build differs is "stale" and shows its emulation verdict with a `stale_proof` note
(`prove.py --recheck` / `record_sweep.py` re-prove it). A row that cannot be checked (no `build`, or the source no
longer compiles) is kept and reported on stderr.

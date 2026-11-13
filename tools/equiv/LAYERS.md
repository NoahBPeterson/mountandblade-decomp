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
- **ddec** (ddec_layer.py, ddec/): DDEC from STOKE (commit 98d8a0f) in Docker, optional: skipped when Docker is
  missing, unknown when the image is. Build once: `docker build --platform linux/amd64 -t mb-stoke-ddec:98d8a0f-1
  tools/equiv/ddec` (the same Dockerfile and byte-identical patches as spore's, so it reuses spore's build cache).
  Each 32-bit function is translated to x86-64 code with 32-bit addresses kept, split at calls; invariants are
  learned from unicorn traces (DDEC_EQ_ONLY) and proved inductive by Z3; hints carry the cut points and invariants
  so a recheck learns nothing. Unsupported: x87 (most of M&B's float code), string instructions, segment
  overrides, switch tables.

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

# Matching (byte-exact decompilation)

Procedure and rules for matching `mount&blade.exe`. The **mechanism reference** is spore-decomp's
[`docs/codegen.md`](../../spore-decomp/docs/codegen.md): read its untagged and **`[13]`** sections (cl 13.10,
the compiler that built *all* of this binary); ignore `[15]`, `[LTCG]` and `[DDK]`. Skim it during setup and
return to it the first time a function shows a diff. M&B-specific findings go in this file.

## Toolchain
- `tools/matching/cl.sh` runs **cl 13.10.3052** (Visual C++ Toolkit 2003) under wine. The game was built with
  VC .NET 2003 (cl 13.10, possibly a later build than 3052; no function has needed it so far).
- Default flags **`/O2 /MT /Gy /EHsc /TP`**. x87 only (no `/arch:SSE`), no `/GS`, no `/GR`.
  VC7.1 has no `/fp:` switch.
- Headers: `match/include/` (shared, verified layouts: `rgl.h`), the VC7.1 CRT/STL, Windows SDK 6.0A
  (`windows.h`, `d3d9.h`, `dinput.h`). No `d3dx9.h`: declare the few D3DX functions/interfaces you need.

## The binary, as it affects matching
- **No `.reloc`.** Relocated operands are masked at the offsets of *our* object's relocations. The verifier
  (`cmpobj.py`, `chk.py`, `run_all.py`, `try_variants.py`) additionally checks **constant values**: a
  `DIR32` to a `__real@…`/`__int64@…`/`??_C@` literal must point, in the original, at the same bytes.
  (Found on the pilot: `fcom [eps]` matched with eps 1e-10 where the original compares with 0.0.)
  Calls, globals and vtable targets are still masked, so their *identity* is checked only by the
  equivalence harness. Run it (below).
- **`/OPT:ICF`**: the linker folded identical functions. One VA can stand for several source functions
  (e.g. vtable 0x54a4b8 uses 0x408030 for two slots). Define the one function that lives at the VA; declare
  the others with the same `// 0x…` address.
- **No RTTI, no PDB.** Class names and layouts are ours to recover. Use the names the community uses
  (TaleWorlds' `rgl::` engine layer: `vector4`, `matrix`, `string`, …; the Module System's names for game
  records and script operations). Mark guesses as provisional in a comment.
- **Static CRT** at `0x535440..0x547100`: linked from the real `libcmt.lib`, never decompiled.
  `symbols/lib_names.txt` names 415 of them (`malloc`, `operator new`, `_purecall`, `_ftol`, …).
- **Module System 1.011 headers** (TaleWorlds, via the Star Wars Conquest repo; git-ignored):
  `work/ext/msys_1011/Headers/*.py`. `header_operations.py` names the script opcodes (589 of the
  interpreter's 591 real cases agree with it); `header_troops.py`, `header_items.py`, `header_parties.py`,
  `header_common.py`, … give flag bits, enums and constants. When a function tests a flag or switches on a
  constant that appears there, use that name (as an enum/`#define` in your slice).
- The script interpreter (`0x49fa10`, 70 KB, `switch (op - 30)` over 2356 cases) is a separate track,
  not part of normal waves.

## Slices and bookkeeping (same as spore-decomp)
Each slice `sXXXXXXXX` is a contiguous run of functions (`work/batches/*.json`). Output, all under the slice:
- `match/slices/<id>/<id>.cpp` (+ optional `<id>.h`): source for **every** function in the slice, each
  preceded by `// @ 0x<va>`.
- `match/slices/<id>/manifest.txt`: one row per **byte-exact** function, exactly as `chk.py` prints it:
  `slices/<id>/<id>.cpp  <symbol>  <va>  <flags>`.
- `match/slices/<id>/nonmatching.txt`: `<va>  <diff-bytes>  <reason>`: COMPLETE behaviourally-equivalent
  source (every path and call of the original) that is not byte-exact.
- `match/slices/<id>/partial.txt`: same format, for anything incomplete. Never list partial source as nonmatching.
- `symbols/slices/<id>.txt`: `<va> <name>  # evidence`, for names you are confident in, and for symbols the
  equivalence resolver cannot map from a declaration: deleting destructors (`??_G<Class>@@UAEPAXI@Z`) and
  vtables (`??_7<Class>@@6B@`).
- Scratch files only as `work/match/scratch_<id>_*`. Never edit shared files (`match/include`, `tools`,
  `docs`, other slices); never run git or Ghidra. If the shared header lacks something, declare it locally in
  your slice and say so in your report (the orchestrator merges it into `match/include`).

## Tools (from the repo root)
- `.venv/bin/python tools/matching/card.py <va> [...]`: annotated disassembly + Ghidra decompile.
- `.venv/bin/python tools/matching/chk.py <file.cpp> <slice-id | va,va,...> [-a]`: ONE compile, diff count for
  every VA. Prints finished manifest rows for matches. Pairs VAs to symbols by the `// @ 0x<va>` marker
  (`marker`), the manifest (`manif`), or size (`~size`, unreliable: add markers).
- `.venv/bin/python tools/matching/try_variants.py <scratch.cpp> <va>`: rank every function in a scratch file.
- `.venv/bin/python tools/matching/cmpdis.py <obj> <symbol-substring> <va>`: instruction-level diff (compile
  first: `tools/matching/cl.sh /nologo /c /O2 /MT /Gy /EHsc /TP <src> /Fo<obj>`).
- `.venv/bin/python tools/matching/disasm.py <va> [len]`: the original's instructions.
- `.venv/bin/python tools/matching/run_all.py --manifest match/slices/<id>/manifest.txt`: final check.
- `.venv/bin/python tools/difftest/equiv.py <id> <va> [...]`: **differential equivalence test**. Runs the
  original and ours in an emulator on generated inputs, with every callee/global mapped to the original's
  address. Must PASS for every nonmatching.txt row; run it on byte-exact rows that call or reference
  globals too. UNSUPPORTED "map that symbol" means a reference it cannot resolve: add a `// 0x…` comment on the
  declaration, or a `symbols/slices/<id>.txt` line for compiler-generated symbols.

## Writing matchable source
- Calling convention from the epilogue: `ret N` with ECX as `this` = member function (declare a stub class);
  `ret N` without ECX = `__stdcall`; ECX+EDX = `__fastcall`; plain `ret` = `__cdecl`.
- **Check every callee's convention at its call site**: `mov/lea ecx, …` right before a call = thiscall
  member; no `add esp, N` after = callee pops. Byte diffs barely notice a wrong declaration; the harness does.
- A function that leaves a value on the x87 stack (or in EAX) at `ret` returns it, even when no caller
  seems to use it (pilot: `vector4::normalize` returns the old length).
- Hidden return pointers: a `ret` that writes through `[esp+4]` is either a by-value struct return or an
  explicit out-parameter. They compile differently: sequential `out.x = …` stores are an out-param,
  `return vector4(a, b, c)` evaluates the ctor arguments right to left (z first).
- Inline helpers decide shape: the original reached `f` through a pointer in `matrix::orthonormalize`
  because it called the out-parameter `cross(r, a, b)` inline.
- Virtual calls need a stub class with the slot at the right index. Dump the vtable from the binary first.
- Budget: up to ~6 variant rounds per function, then keep the best complete version in nonmatching.txt.

**No assembly transcription.** `__declspec(naked)`, `_emit`, or long `__asm` blocks never count (run_all
prints `ASM`). A block of at most 8 instructions is allowed where the original used an asm helper.

**Papercuts:** before finishing, append real friction (tool bugs, unclear docs, tricks you had to rediscover)
to `docs/Papercuts.md`, following the format at its top. Read its Open section first.

## Lessons (M&B)
- Pilot `s00407c10` (25/25 byte-exact, all PASS): first-try source matches more often than on Spore; most
  misses were *semantic* (wrong constant, return value, out-param vs return), not codegen.

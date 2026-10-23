# Matching (byte-exact decompilation)

Procedure and rules for matching `mount&blade.exe`. The **mechanism reference** is spore-decomp's
[`docs/codegen.md`](../../spore-decomp/docs/codegen.md): read its untagged and **`[13]`** sections (cl 13.10,
the compiler that built *all* of this binary); ignore `[15]`, `[LTCG]` and `[DDK]`. Skim it during setup and
return to it the first time a function shows a diff. M&B-specific findings go in this file.

## Toolchain
- `tools/matching/cl.sh` runs **cl 13.10.3052** (Visual C++ Toolkit 2003) under wine. The game was built with
  VC .NET 2003 (cl 13.10, possibly a later build than 3052; no function has needed it so far).
- Default flags **`/O2 /MT /Gy /TP`**. Add `/EHsc` only where the original has EH frames (`push -1; push
  <handler>; mov eax, fs:[0]` prologues, `__ehhandler$` stubs): with it, a local with a destructor plus `new`
  emits a frame the original lacks (every wave-1 region was built without it). x87 only (no `/arch:SSE`),
  no `/GS`, no `/GR`. VC7.1 has no `/fp:` switch.
- Headers (all on `cl.sh`'s include path, so a hand-run `cl.sh` needs no `/I`): `match/include/` (shared,
  verified layouts: `rgl.h`), the VC7.1 CRT/STL, **Windows SDK 6.0A** (`#include <windows.h>`, `<d3d9.h>`,
  `<dinput.h>`, `<mmsystem.h>`, `<winsock2.h>` before `<windows.h>`), and the **DirectX SDK June 2010** for
  `<d3dx9.h>`. Use the real headers; do not hand-declare Win32/D3D9/D3DX.
- **Back-end (c2.dll) switches, for experiments only** (never in a manifest): `/d2<opt>` passes `-<opt>` to c2.
  Its option table (at 0x10866xxx in c2.dll, entries {name, global, type}) has inliner tuning knobs whose 0
  default means "built-in value" (set at 0x10708b4b): `inlT` 74 (inline threshold), `inlS` 9, `inlfcsw` 30,
  `inlocsa1` 45, `inlocsa4` 45, `inluserinl` 7, `inlnobr` 10, plus `inlld`, `inld`, `inlniln`, `inlnild`,
  `inlnoln`, `inlnold`, `inlcrmax`, `inlfcsa`, `isize` (4). Use them to MEASURE: the smallest `/d2inlT<N>` at
  which a call gets inlined is that callee's cost at that site. Example: ??_Eframe_set 0x41aee0 matches the
  original byte for byte with `/d2inlT88` and above (??_EBone's array-path cost is 87), but a global inlT88
  drops resmgr.cpp from 62 to 54 matches, so the original was not built with another threshold: our source
  presents ??_EBone (or the call site) as costlier than the original's did. Replacing Bone's std::vector with a
  minimal struct did not change the cost. The "INL:" inliner trace strings exist but print only in a PGO mode
  (global 0x108a87d0), not reachable from a normal compile; `-db#` prints nothing.

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
  first: `tools/matching/cl.sh /nologo /c /O2 /MT /Gy /TP <src> /Fo<obj>`).
- `.venv/bin/python tools/matching/disasm.py <va> [len]`: the original's instructions.
- `.venv/bin/python tools/matching/run_all.py --manifest match/slices/<id>/manifest.txt`: final check.
- `.venv/bin/python tools/difftest/equiv.py <id> <va> [...]`: **differential equivalence test**. Runs the
  original and ours in an emulator on generated inputs, with every callee/global mapped to the original's
  address. Must PASS for every nonmatching.txt row; run it on byte-exact rows that call or reference
  globals too. UNSUPPORTED "map that symbol" means a reference it cannot resolve: add a `// 0x…` comment on the
  declaration, or a `symbols/slices/<id>.txt` line for compiler-generated symbols (and for an `rgl.h`
  function the resolver missed).
  - **PASS** = no mismatch, >= 200 valid inputs, and **every branch edge covered on BOTH builds**: each
    conditional jump taken and not taken, each jump-table target, in the original AND in ours. The only
    exceptions are edges the harness lists as unreachable, each with its reason (printed, and in the JSON
    under `edges.<orig|ours>.unreachable`): an x87 branch no comparison outcome takes, a NaN-only branch
    (without `--nan`), or code only reachable past a call that never returned (unmodelled import, C++ throw,
    pure virtual call). Instruction coverage is still printed but no longer decides anything.
  - **WEAK** = no mismatch, but too few valid inputs or uncovered edges; the reason says which side and how
    many (`orig misses 3 edge(s), ours misses 1 edge(s)`), the JSON lists them (`edges.<side>.missed`).
    WEAK is not a PASS. Look at the missed edges: an edge only ours misses is a branch our code has and
    the original never needs (check it); a branch on state the inputs cannot reach is a harness limit
    (say so in the reason column).
  - Inputs are steered toward uncovered branches: compare constants, input-to-state replays (the compared
    argument/field/global/`new` result is set to the other operand), boundary probes (`==`, +-1, +-1 ulp)
    on covered compares, and fault repair that also maps memory at untraceable faulting addresses.
    `--no-steer` turns it off.
  - `--gate insns` is the old gate (>= 60% of the original's instructions); only for comparing with old
    results.
  - **A FAIL is not equivalent**, even by 1 ulp: fix the source (float rounding points, operand order,
    `!(a < b)` vs `a >= b` for NaN), or move the row to partial.txt. The first mismatch says which input
    triggered it (`origin`: fresh, mutant, steer; `alloc_fail` when a `new` returned NULL;
    `mapped_at_fault` when repair mapped memory); `--replay` re-runs it.
  - UNSUPPORTED because of the harness (no model yet for user32/gdi32/winmm imports, D3D device or stream
    vtables, faults on random pointers) is acceptable for a complete function: say so in the reason column.
  - **Resolution audit** (`tools/difftest/resaudit.py`, run by equiv.py before any input): every callee,
    global, string and vtable our object references was mapped to an original address; the audit pairs our
    instructions with the original's and checks each mapped operand against the original's operand at the
    paired instruction. `resolution audit: ... disagree (X resolved A, original operand B at C)` is
    UNSUPPORTED, because our build would call/read something else than the original there: either our source
    names the wrong thing (fix it), or the mapping is wrong (fix the `// 0x` annotation / symbols line). It
    also catches byte-exact rows that are wrong (byte compares mask relocations): run
    `tools/difftest/resaudit.py <slice> <va>...` on your manifest rows. Typical real findings: `new` where
    the original has `new[]` (0x535482), `delete` vs `delete[]` (0x53570d), `free` vs `operator delete`,
    `atol` vs `atoi` (0x53734d), two messages swapped, a struct field 4 bytes off, two slices using one
    name for different globals. `local copies` in the summary means our own copy of a callee ran where the
    original calls a real function (an ICF'd template helper): map it in your symbols file.
  - **`--fp-tol`** (default off; changes only how float mismatches are judged) and **`--fp-realistic`**
    (generate float fields as finite magnitudes in [1e-3, 1e5], both signs, plus exact 0/+-1, instead of
    random bit patterns). The game runs the x87 at 24-bit precision (D3D9 device thread; the default
    `--fpcw 007F`), so a recompile that sums in another order or spills a different value to a float differs
    from the original in rounding, and on absurd random magnitudes one build overflows float range where the
    other keeps the value in its 80-bit register. With `--fp-tol`, for each input where the builds differ the
    ORIGINAL is re-run at 64-bit precision (`037F`; the reference, about exact arithmetic) and the input is judged:
    - *real mismatch* (verdict FAIL, first one printed): any differing integer/pointer dword, call trace, call
      argument, status/register/esp/x87-depth difference, a different original path at 64-bit precision, or a
      float that is further from the reference in ours than `max(2 * err_orig, 4 ulp)` (ulp of the largest of
      reference/orig/ours), or NaN/Inf in ours only. A small (denormal) differing dword counts as a float only if
      the reference run stored it with an x87/SSE store (or copied such a stored value), so small integers are
      not mistaken for floats;
    - *out-of-range* (counted, neither way): a differing float whose original value or reference is
      Inf/NaN/denormal/above 1e30, or NaN/Inf in ours only while the run read an input dword that is
      Inf/NaN/above 1.27e30. It also shadows other differences on that input;
    - *fp-tolerable*: every differing observable is a float (memory dword, or st0 of the return) within that
      tolerance.
    **PASS-FP** (or **WEAK-FP** if the edge gate is short) = no real mismatch, at least one fp-tolerable or
    out-of-range input, and the usual coverage gate passed. The reason line and `--json` (`fp_tol`) give
    inputs compared, bit-identical, fp-tolerable (`max_ulp`, `max_err_ratio` = ours' error / the original's,
    where the original's was nonzero), out-of-range and real mismatches. **PASS-FP is not PASS**:
    the rows stay nonmatching/partial. It does let a row be recorded in nonmatching.txt as complete (instead of
    "equiv FAIL") when the byte diff is a float-rounding or association-order artefact: write "equiv PASS-FP"
    with the stats (max ulp, out-of-range count) in the reason column. A PASS-FP verdict does not excuse real
    mismatches (uninitialised stack lanes, integer or call differences stay FAIL), and run both modes: say what
    the default mode gives. Limits: a float stored as a double (two dwords) is judged as two float32 dwords and
    will usually FAIL; the "inputs feed it" test is any 4-byte read of a huge value (an integer 0xFFFFFFFF
    also qualifies); control flow of ours is not compared with the original's directly (call trace and
    outputs are).
  - **`--transparent VA,VA,...`** (image function addresses, hex): a depth-1 call to one of these is replaced
    in the call trace by the calls that function makes itself (recursively, for listed callees it calls).
    Use it when one build inlined a helper and the other calls it, and the call boundary is the only
    difference: e.g. ??_Eframe_set 0x41aee0, where the original inlined ??_EBone on one path as
    `??_I(.., ~Bone) + operator delete[]` (`--transparent 401830,41a510,41a550`). Arguments of every
    recorded call, memory, registers and imports are still compared; a real difference inside still FAILs
    (`test_transparent_keeps_real_differences`). Name the option and the addresses in the reason column,
    and say what the default mode gives.
  - Limit is 180 s per function by default. Run it with `run_in_background` and keep working, never with a
    shell `&`.

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
- **x87 code: read [`../spore-decomp/docs/codegen.md`](../../spore-decomp/docs/codegen.md) §9 first**, especially
  §9.6 (this project's own x87 lessons: what 24-bit precision means for a difference, aligned-instruction
  comparison instead of byte counts, `--fp-tol`, the `volatile`/aggregate/frame-struct levers, and the walls).
  It is the shared codegen guide for cl 13.10; add new x87 findings there, not here.
- **Judge a near-miss by aligned instructions, not the byte count:** `.venv/bin/python tools/matching/insn_align.py
  <src.cpp> <va> [--show]` reports identical / changed / missing / extra instructions (relocations masked; a
  second figure masks stack-slot numbers). chk.py's differing-byte count is positional and balloons when one
  instruction shifts the rest.

**No assembly transcription.** `__declspec(naked)`, `_emit`, or long `__asm` blocks never count (run_all
prints `ASM`). A block of at most 8 instructions is allowed where the original used an asm helper.

**Papercuts:** before finishing, append real friction (tool bugs, unclear docs, tricks you had to rediscover)
to `docs/Papercuts.md`, following the format at its top. Read its Open section first.

## Lessons (M&B)
- Pilot `s00407c10` (25/25 byte-exact, all PASS): first-try source matches more often than on Spore; most
  misses were *semantic* (wrong constant, return value, out-param vs return), not codegen.
- Wave 1 (98/120 byte-exact):
  - **Recognise library code first.** 0x401000-0x407c00 is the **DirectX 9 SDK sample framework**
    (`d3dapp.cpp` `CD3DApplication`, `d3denumeration.cpp` `CD3DEnumeration`/`CArrayList`, `d3dfont.cpp`
    `CD3DFont`, `d3dsettings.cpp` settings dialog). Writing the SDK's literal source (its comparison order,
    `bool` temporaries, one `case` per value) is what matched. Recall the original source before inventing.
  - **Register-argument callees.** A `static` function in the same TU gets cl 13.10's custom convention
    (argument in EAX; ECX kept across the call). Define such a callee as `static __declspec(noinline)` in
    your slice to reproduce a caller (0x4035d0, 0x404140). Conversely, a same-TU callee lets cl assume its
    register use; call an undefined declaration when the original does not.
  - `__declspec(noinline)` keeps a call that cl would otherwise inline.
  - A base class with an **inline ctor** puts the derived class's vptr store last.
  - `float pts[N]` locals set the exact `__chkstk`/frame size; whole-struct copies (`T m = *p`) reproduce
    4-dword spills that field-by-field reads do not.
  - Unsigned count tests: `> 0` gives `jbe`, `!= 0` gives `je`. `(unsigned short)x` gives `movzx`,
    `& 0xffff` gives `and`.
  - Nested `SendMessage(GetDlgItem(...), ...)` pushes constants first; a temp `HWND` assigned first
    reproduces the original order.
  - Shapes (vtables 0x54a4b8 sphere, 0x54a540 capsule, 0x54a588 face) share a 0x20-byte base: vptr + 7 ints
    (`m4=-1, m8=8, mc=-1`, int bbox ±1e8). See match/slices/s00407c10, s00408110, s00409e90.
  - Before writing a class, `rg` the other slices (`match/slices/*/*.h, *.cpp`) for its vtable address or ctor:
    a neighbour may already have the verified layout.

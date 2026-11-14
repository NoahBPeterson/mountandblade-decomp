# capstone: upstream tickets (checked 2026-11-13)

Our workarounds: `tools/equiv/rename_core.py` (the operand-access helper around line 114).
Repro script: run each byte string with detail on, `CS_MODE_32`, and print `op.access` and `regs_access()`.

| bug | 5.0.7 / 6.0.0 (pip) | `next` (823242a) | upstream |
|---|---|---|---|
| x87 stores (`fst`/`fstp`/`fist`/`fistp` m32/m64) report memory READ, not WRITE | wrong | fixed | #3107 (closed) |
| `test [mem], reg` (85 /r): both operands access 0, reg missing from regs_read | wrong | fixed | — |
| `shld`/`shrd` with `cl`: the cl operand's access is 0 | wrong | fixed | — |
| `cmpxchg [mem], reg`: the destination is READ only | wrong | fixed | — |
| **`test eax, imm32` (A9 id): eax reported READ\|WRITE, and eax in regs_write** | wrong | **still wrong** | **open: patch below** |

So only one bug is left to report upstream. Keep our workarounds until a capstone release that includes these fixes
is pinned in `tools/equiv/angr-requirements.txt`.

## The open one: `test eax, imm32`

- `a9 01 00 00 00` (`test eax, 1`): `operands[0].access` is `READ | WRITE`, and `regs_write` is `[eflags, eax]`.
  Expected: `READ`, and `[eflags]`. TEST never writes its operands.
- The sibling forms are correct: `f7 c0 imm32` (`test eax, imm` via F7 /0), `a8 ib` (`test al`), `66 a9 iw`
  (`test ax`), `48 a9 id` (`test rax`).
- Cause: `arch/X86/X86MappingInsnOp.inc` line 13388, the `X86_TEST32i32` entry, is
  `{ CS_AC_READ | CS_AC_WRITE, CS_AC_IGNORE, 0 }`. Every other `TESTxxix` entry, and the same entry in
  `X86MappingInsnOp_reduce.inc`, is `{ CS_AC_READ, CS_AC_IGNORE, 0 }`.
- Patch: `docs/upstream/capstone_test_eax_imm32.patch`, against `next` 823242a. It is the one-line table fix plus a
  case in `tests/details/x86.yaml`. Verified with a rebuilt `cstool` in 32- and 64-bit mode. cstest itself was not
  run locally: it would not build on this Mac (see below), so CI has to run it.

**capstone's CONTRIBUTING.md forbids AI-written PR descriptions and commit messages.** Whoever opens the PR writes
those by hand. The facts above are notes for that, not text to paste.

### Steps

```
gh repo fork capstone-engine/capstone --clone && cd capstone && git checkout -b fix-test-eax-imm32 origin/next
git apply ../mountandblade-decomp/docs/upstream/capstone_test_eax_imm32.patch
# build: cmake -S . -B build -DCAPSTONE_ARCHITECTURE_DEFAULT=OFF -DCAPSTONE_X86_SUPPORT=ON && cmake --build build
# check: build/cstool -d x32 "a9 01 00 00 00"  -> operands[0].access: READ, Registers modified: eflags
# commit and open the PR against `next`, writing the message and the template by hand
```

## Minor: cstest does not build on macOS

`suite/cstest/src/cstest.c:139` calls `snprintf` without `#include <stdio.h>`, so clang fails with
"call to undeclared library function". Also, the bundled libcyaml build does not find Homebrew's `yaml.h` without
`CPATH`. Low value; mention it only if the PR conversation gets there.

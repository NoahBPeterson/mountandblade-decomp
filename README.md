# mountandblade-decomp

A decompilation of `mount&blade.exe` (Mount & Blade 1.011, Steam build, linked 2008-11-04), aiming first
at a **drop-in Windows binary** built from source that runs under Wine/CrossOver with the game's
original DLLs (Newton, FMOD 3, D3D9/D3DX9, DirectInput 8, SkinMagic). A native macOS/Linux port comes later.

**This repository contains no game code or data.** Binary-derived artifacts live in `work/` (git-ignored)
and are produced locally from your own copy of the game.

Sister project: spore-decomp (Spore), whose tools this project's are ported from. **To contribute, start with
[`CONTRIBUTING.md`](CONTRIBUTING.md)**; `docs/codegen.md` is the cl 13.10 code-generation reference.

## Status: **unfinished** (2026-10-27)

This is a work in progress, not a working replacement for `mount&blade.exe`. **There is no runnable binary yet**: the
last trial link of our objects still had ~2,300 unresolved symbols. Do not expect `git clone && make` to produce a game.

| Tier | Functions | Share | How it is verified |
|---|---:|---:|---|
| Byte-exact | 2,254 | 67.2% | compiled object vs the original, byte for byte, relocations masked (`tools/matching/run_all.py`) |
| Equivalent only | 242 | 7.2% | both versions executed on 219-589 inputs, all reachable branch edges covered on **both** builds, exit state + call traces + written memory identical (`tools/difftest/equiv.py`) |
| Not equivalent yet | 856 | 25.5% | 425 weak (some edge never taken), 82 failing (a real mismatch), 210 untestable with random inputs, plus 149 symbol-less compiler fragments |

Denominator: 3,352 game functions, 1.26 MB of code (the static CRT/STL is linked from the real VC7.1 libraries and
excluded). "Equivalent" is **empirical evidence, not proof**: it says the two implementations were indistinguishable on the inputs tried, with every branch
edge of both versions exercised.

Current numbers, from the sources of truth (`docs/STATUS.md`, `docs/equiv_verdicts.json`):

```sh
.venv/bin/python tools/progress.py                  # tables: byte-exact / equivalent / evidence breakdown
.venv/bin/python tools/progress.py --plot            # treemap -> work/progress.png
```

The treemap is one box per module (area = instructions, colour = verdict) inside three groups: game, engine (`rgl`),
3rd-party. Orange is "weak", vermillion is "failing", purple is "every random input was rejected", grey/black are
symbol-less compiler fragments (byte-checked inside their COMDAT / not matching).

See [`docs/PLAN.md`](docs/PLAN.md) for the plan and [`docs/matching.md`](docs/matching.md) for the workflow.

## Setup
```sh
uv sync   # .venv from pyproject.toml + uv.lock (pinned: unicorn/capstone versions affect equiv results)
MB="$HOME/Library/Application Support/CrossOver/Bottles/Steam/drive_c/Program Files (x86)/Steam/steamapps/common/Mount and Blade"
.venv/bin/python tools/steamstub21_analysis_image.py "$MB/mount&blade.exe" work/mb.analysis.bin work/mb.analysis.json
# toolchain from your own installers (verified by hash): see CONTRIBUTING.md
.venv/bin/python tools/setup_toolchain.py vctoolkit VCToolkitSetup.exe   # + winsdk, dxsdk, wine, check
export GHIDRA_INSTALL_DIR=$(ls -d /opt/homebrew/Cellar/ghidra/*/libexec | tail -1) PY=/opt/homebrew/opt/python@3.11/bin/python3.11
$PY tools/ghidra/index.py new work/mb.analysis.bin work/ghidra mb 0x5390ba work/index/functions.csv  # ~5 min
$PY tools/ghidra/fixups.py work/mb.analysis.bin work/ghidra mb          # jump tables, missed functions
$PY tools/ghidra/create_functions.py work/mb.analysis.bin work/ghidra mb 401000 40c1d8  # callbacks referenced only by immediates
$PY tools/ghidra/index.py existing work/mb.analysis.bin work/ghidra mb work/index/functions.csv
$PY tools/ghidra/export_decomp.py work/mb.analysis.bin work/ghidra mb work/decomp
```
See `docs/PLAN.md` for the plan and `docs/matching.md` for the matching workflow.

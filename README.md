# mountandblade-decomp

A decompilation of `mount&blade.exe` (Mount & Blade 1.011, Steam build, linked 2008-11-04), aiming first
at a **drop-in Windows binary** built from source that runs under Wine/CrossOver with the game's
original DLLs (Newton, FMOD 3, D3D9/D3DX9, DirectInput 8, SkinMagic). A native macOS/Linux port comes later.

**This repository contains no game code or data.** Binary-derived artifacts live in `work/` (git-ignored)
and are produced locally from your own copy of the game.

Sister project: `../spore-decomp`. Most tools here are ported from it; its `docs/codegen.md` is the
code-generation reference (the `[13]` sections apply to us: same cl 13.10).

## Setup
```sh
uv sync   # .venv from pyproject.toml + uv.lock (pinned: unicorn/capstone versions affect equiv results)
MB="$HOME/Library/Application Support/CrossOver/Bottles/Steam/drive_c/Program Files (x86)/Steam/steamapps/common/Mount and Blade"
.venv/bin/python tools/steamstub21_analysis_image.py "$MB/mount&blade.exe" work/mb.analysis.bin work/mb.analysis.json
# toolchain: work/toolchain/{vc71tk,vc9sp1,wineprefix} -> symlinks to ../spore-decomp/work/toolchain/*
export GHIDRA_INSTALL_DIR=$(ls -d /opt/homebrew/Cellar/ghidra/*/libexec | tail -1) PY=/opt/homebrew/opt/python@3.11/bin/python3.11
$PY tools/ghidra/index.py new work/mb.analysis.bin work/ghidra mb 0x5390ba work/index/functions.csv  # ~5 min
$PY tools/ghidra/fixups.py work/mb.analysis.bin work/ghidra mb          # jump tables, missed functions
$PY tools/ghidra/create_functions.py work/mb.analysis.bin work/ghidra mb 401000 40c1d8  # callbacks referenced only by immediates
$PY tools/ghidra/index.py existing work/mb.analysis.bin work/ghidra mb work/index/functions.csv
$PY tools/ghidra/export_decomp.py work/mb.analysis.bin work/ghidra mb work/decomp
```
See `docs/PLAN.md` for the plan and `docs/matching.md` for the matching workflow.

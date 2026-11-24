# mountandblade-decomp

A decompilation of `mount&blade.exe` (Mount & Blade 1.011, Steam build, linked 2008-11-04), aiming first
at a **drop-in Windows binary** built from source that runs under Wine/CrossOver with the game's
original DLLs (Newton, FMOD 3, D3D9/D3DX9, DirectInput 8, SkinMagic). A native Linux / macOS / Windows x64 port also builds from source: see [`docs/BUILDING_PORT.md`](docs/BUILDING_PORT.md).

**This repository contains no game code or data.** Binary-derived artifacts live in `work/` (git-ignored)
and are produced locally from your own copy of the game.

Sister project: spore-decomp (Spore), whose tools this project's are ported from. **To contribute, start with
[`CONTRIBUTING.md`](CONTRIBUTING.md)**; `docs/codegen.md` is the cl 13.10 code-generation reference.

## Status: **playable** (2026-11-23)

This is a work in progress, and currently a semi-working replacement for `mount&blade.exe`.

| Tier | Functions | Share | Share by # of Instructions |  How it is verified |
|---|---:|---:|---|
| Byte-exact | 2,924 | 87.5% | 55.7% | compiled object vs the original, byte for byte, relocations masked (`tools/matching/run_all.py`) |
| Equivalent only | 122 | 3.7% | 6.9% | both versions executed on 219-589 inputs, all reachable branch edges covered on **both** builds, exit state + call traces + written memory identical (`tools/difftest/equiv.py`) |
| Not equivalent yet | 856 | 8.8% | 37.4% | 232 weak (some edge never taken), 10 failing (a known mismatch), 3 untestable with random inputs |

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

## Building without the original exe
```sh
.venv/bin/python tools/relink/rsrc_repo.py extract "$MB/mount&blade.exe"   # once, from your own install
.venv/bin/python tools/relink/build.py --no-original
```
The extract step writes the game's dialogs, menus and accelerators to `resources/`, which git ignores, because they aren't ours to
publish. After that a build needs only the toolchain. It sets the version strings to `1.0.11_Recomp` and uses placeholder icons from
`resources/icons/` (`tools/relink/make_placeholder_icons.py` writes them; they're to be replaced). `tools/relink/rsrc_repo.py` assembles
the `.rsrc` section. The build compiles `src/`, takes link order, aliases and import names from `symbols/link_inputs.json`, and holds no
code or data bytes of the original.
No gate runs in this mode. `build.py --faithful` still needs `work/mb.analysis.bin`: it checks the build against the original and
reports `link_inputs STALE` when a source change needs `--write-inputs`. The `--no-original` build is the faithful one (no patches).

## Source tree
`src/engine/` is the rgl layer (render, scene, physics, resources, math, streams), `src/game/<subsystem>/` the Mount & Blade code
(agent, campaign, mission, party, script, ui ...), `src/platform/dxframework/` the DirectX sample framework and
`src/third_party/tinyxml/` TinyXML. Every `#include` is rooted at `src/` (`"engine/math/mat4.h"`, `"game/data/troop.h"`); a `.cpp`'s
`.manifest.txt` / `.nonmatching.txt` / `.partial.txt` sidecars live in the mirror tree `match/manifests/<path under src/>/` (`tools/common/sidecars.py` is the one place that knows). The plan and the old-to-new table are `docs/source_tree.md` and
`docs/source_tree_moves.tsv`, the notes of the former directories `docs/migration/`, the tool that moved everything
`tools/migrate/restructure.py`.

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

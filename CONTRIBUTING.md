# Contributing

The most useful contribution is **turning an equivalent function into a byte-exact one**: change its source until
cl 13.10 emits exactly the original's bytes. About 1,100 functions are complete and tested for equivalence but not
yet byte-identical; `docs/equiv_verdicts.json` lists them, and each row's note in its module's `*.nonmatching.txt`
says what still differs.

This file covers setup and the workflow. The rules and mechanisms are in:
- [`docs/matching.md`](docs/matching.md): procedure, verifier rules, M&B-specific lessons;
- [`docs/codegen.md`](docs/codegen.md): how cl 13.10 turns source into the shapes in the binary, and which source
  choices move them. Read it once before you start, and again at your first diff.

## What you need

- **Your own copy of Mount & Blade 1.011** (the Steam build). `mount&blade.exe` must have SHA-1
  `43fd8eb4e44c30fd34493ec049583ce9c2e20cab`. The repository contains no game code or data, and nothing derived
  from the binary is ever committed: everything binary-derived is generated locally into `work/` (git-ignored).
- **macOS or Linux** with **wine** (tested with wine 11.16 from Homebrew), **msitools** (`msiextract`) and
  **p7zip** (`7z`). The compiler runs under wine through `tools/matching/cl.sh`.
- **[uv](https://docs.astral.sh/uv/)** and Python 3.14 for the tools; **Ghidra** with pyghidra (Python 3.11) for
  the one-time analysis that builds the function index and the decompile.
- **Three Microsoft installers, which you supply.** Microsoft no longer distributes the first two, and this project
  does not redistribute any of them. `tools/setup_toolchain.py` checks each against the hash below and refuses
  anything else, so your toolchain is byte-identical to the one every manifest was verified with:

  | Installer | Provides | SHA-1 |
  |---|---|---|
  | `VCToolkitSetup.exe` (Visual C++ Toolkit 2003) | **cl 13.10.3052**, the VC7.1 CRT/STL headers and libs | `956c81c3106b97042c4126b23c81885c4b5211f4` |
  | `VCForPython27.msi` (Visual C++ Compiler for Python 2.7) | Windows SDK 6.0A headers (`windows.h`, `d3d9.h`, `dinput.h`) and VC9's `sal.h` | `7800d037ba962f288f9b952001106d35ef57befe` |
  | `DXSDK_Jun10.exe` (DirectX SDK, June 2010) | `d3dx9*.h` (checked by the hashes of the extracted headers) | - |

## Setup

```sh
uv sync                                                   # .venv, pinned versions (they affect equivalence results)

# unpack the game's SteamStub wrapper into an analysis image (work/mb.analysis.bin, SHA-1 859362a6deb1b3e6998c9ad7b6e0079400f545d1)
MB="<your Mount and Blade folder>"
.venv/bin/python tools/steamstub21_analysis_image.py "$MB/mount&blade.exe" work/mb.analysis.bin work/mb.analysis.json

# toolchain, from your installers (each step verifies its input and its output)
.venv/bin/python tools/setup_toolchain.py vctoolkit /path/to/VCToolkitSetup.exe
.venv/bin/python tools/setup_toolchain.py winsdk /path/to/VCForPython27.msi
.venv/bin/python tools/setup_toolchain.py dxsdk /path/to/DXSDK_Jun10.exe
.venv/bin/python tools/setup_toolchain.py wine            # the project's own wine prefix
.venv/bin/python tools/setup_toolchain.py check           # compiles a module: expect "33 / 33 ... toolchain OK"
```

Then run the Ghidra steps in [`README.md`](README.md) (index, fixups, decompile export; about 10 minutes). They
produce `work/index/functions.csv` and `work/decomp/`, which the matching tools read.

The installer is never run: `setup_toolchain.py` decodes VCToolkitSetup.exe's InstallShield payload and extracts its
MSI directly (see the script's docstring). Compiles take about 2 s each once the wine session is warm; during long
sessions keep it warm with `tools/matching/keepalive.sh <hours>`.

## Workflow: matching an equivalent function

Source lives in `src/`, one `.cpp` per original translation unit (e.g. `src/game/strat/tstring.cpp`), each function
preceded by a `// @ 0x<va>` marker. Next to each `.cpp`:
- `<stem>.manifest.txt`: byte-exact functions, one row each (`../src/... <symbol> <va> <flags>`);
- `<stem>.nonmatching.txt`: complete and equivalent but not byte-exact, `<va> <diff> <note>`.

1. **Pick a row** from a `*.nonmatching.txt`. Its note says how far off it is and why (register allocation, frame
   size, block order, ...). Small, nearly matching functions are the best start.
2. **Look at the original:** `.venv/bin/python tools/matching/card.py 0x<va>` prints the annotated disassembly and
   Ghidra's decompile. `tools/matching/disasm.py <va>` prints just the instructions.
3. **Edit the function in its module `.cpp`** and compile-check it:
   ```sh
   .venv/bin/python tools/matching/chk.py src/game/strat/tstring.cpp 0042bac0
   ```
   One compile, a differing-byte count per VA, and a finished manifest row when a function matches. For an
   instruction-level view: `tools/matching/cmpdis.py`, and `tools/matching/insn_align.py`, which aligns
   instructions instead of counting positional bytes (a single inserted instruction inflates chk's count).
   To try many spellings at once, put them in a scratch file under `work/match/` and rank them with
   `tools/matching/try_variants.py`.
4. **When it matches:** move the row from `<stem>.nonmatching.txt` to `<stem>.manifest.txt` (the row chk prints),
   then verify the whole module and make sure nothing else in it moved:
   ```sh
   .venv/bin/python tools/matching/run_all.py --manifest src/game/strat/tstring.manifest.txt
   ```
   Every row of the module must still match. Header changes can move other functions; `run_all.py` is the
   arbiter.
5. **If it doesn't match yet** but you improved it, keep the function equivalent: run the equivalence harness and
   update the row's note with what still differs.
   ```sh
   .venv/bin/python tools/difftest/equiv.py tstring 0042bac0 --src src/game/strat/tstring.cpp
   ```
   It runs the original and your compiled function under emulation on the same inputs. A FAIL means your change
   altered behaviour: fix it before committing.

## Rules

- **Only what the verifier prints counts.** A function is byte-exact when `run_all.py` says so, nothing else.
- **No assembly transcription.** `__declspec(naked)`, `_emit` and long `__asm` blocks never count as matched;
  short helpers (`fnstcw`, a `cvtss2si`) are fine (see `docs/matching.md`).
- **One definition per class.** Shared layouts live in the canonical headers (`src/include/`, `src/game/*.h`,
  `src/game/include/`); extend those rather than declaring a local partial copy. Member offsets carry `// +0x..`
  comments citing the address that proves them.
- **Globals and functions keep their address annotation** (`// 0x5e2930`). Names are free to improve; the
  annotation is how the tools map a name to the original.
- **Behaviour first.** Never trade equivalence for bytes; a byte-exact function is also equivalent by
  construction, but a nonmatching one must stay equivalent at every step.
- Keep scratch files in `work/` (git-ignored), never in `src/` or the repo root.

## Submitting

Commit only the files you changed (the module's `.cpp`/`.h` and its manifest / nonmatching files), with a
message that names the function and what made it match. Note new codegen mechanisms you discovered in
`docs/codegen.md`, with the address that demonstrates them; that is how the next person saves an afternoon.

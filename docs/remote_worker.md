# Windows remote worker

A Windows laptop is a second worker for the equivalence harness (`tools/difftest/equiv.py`) and for native compiles
(`tools/matching/run_all.py`, `tools/relink/build.py`). There the tools run the VC7.1 `cl.exe` 13.10 / `link.exe` 7.10
directly (no wine, no Rosetta). `tools/matching/winnative.py` holds the switch: on `sys.platform == "win32"` paths are
native (no `Z:`), `INCLUDE` / `LIB` are set in the process environment in `cl.sh`'s order, and `flock` becomes
`msvcrt.locking` (the `run_all` lock, the `equiv.py` CPU slots). On the Mac nothing changes: the command lines are the
same bytes as before.

## Layout on the worker (everything under `D:\mb`; touch nothing else on the machine)

| path | what |
|---|---|
| `D:\mb\repo` | git clone; sync a commit with `tools/remote/worker.py sync` (detached checkout) |
| `D:\mb\repo\.venv` | `uv sync --frozen --python 3.14` (versions from `uv.lock`: capstone 5.0.9, numpy 2.5.4, unicorn 2.1.4, ...) |
| `D:\mb\uvtool`, `D:\mb\uvpy`, `D:\mb\uvcache` | uv (pip-installed into a venv) and the CPython 3.14 it downloaded |
| `D:\mb\repo\work\mb.analysis.bin`, `mb.analysis.json` | the unpacked image |
| `D:\mb\repo\work\toolchain\{vc71tk,sal,vc9sp1\WinSDK\{Include,Lib}}`, `work\ext\dxsdk_jun10\DXSDK\{Include,Lib\x86}` | toolchain (copied dereferenced from the Mac's work/) |
| `D:\mb\repo\work\{oss\crt_matches.csv,relink\resolve_map.json,index,decomp}` | the other files `equiv.py` opens |
| `D:\mb\repo\game\SkinMagic.dll` | read by `glue.py` (export names of the shipped DLL); the Mac's `game/` is the Steam install |
| `D:\mb\jobs` | `NAME.log` and `NAME.pid` of jobs started by `startjob.ps1` |

Not copied (copy when a workload needs them; the tools say which file is missing): `work/difftest/regress` and
`fail_inputs` (only the rows compared so far; see below), `work/difftest/equiv_cache`.

## Connection

Nothing secret is in the repo. `tools/remote/worker.py` reads `MB_WORKER_SSH` (the ssh command up to the command, e.g.
`ssh user@host`), `MB_WORKER_SCP` (e.g. `scp -q`) and `MB_WORKER_HOST` (`user@host`) from the environment. Keep them in an
untracked shell file. Windows messages are in Spanish (`Correcto` = success, `no se encuentra` = not found).

## Sync a commit

    tools/remote/worker.py sync [REV]      # bundle REV, copy it, `git checkout --detach` in D:\mb\repo, update startjob.ps1

Files under `work/` are not in git; changes to them (a new `mb.analysis.bin`, regress inputs) are copied by hand with
`tar` (set `COPYFILE_DISABLE=1` on the Mac, else `._*` files reach the compiler's include path).

## Run a shard there and fetch its JSON

    tools/remote/worker.py run fg "set DIFFTEST_NO_LEDGER=1 && .venv\Scripts\python tools\difftest\equiv.py facegen_win 004f1460 --src src/game/char_sheet/facegen_win.cpp --fp-tol --json D:\mb\out\fg.json"
    tools/remote/worker.py status
    tools/remote/worker.py tail fg 20
    tools/remote/worker.py get D:/mb/out/fg.json work/remote/fg.json

For bit-identical verdicts run with `PYTHONHASHSEED=0` (`startjob.ps1` sets it, and `PYTHONUTF8=1`), the same commit and
the same `work/difftest/regress/<row>` and `fail_inputs/<row>.pkl` on both machines. `DIFFTEST_REGRESS_DIR` /
`DIFFTEST_FAIL_DIR` point a run at copies of those, so a comparison run changes neither side's real directories;
`DIFFTEST_NO_CACHE=1` forces re-emulation. Shards of one row: `equiv.py --shard K/N`, then `merge_shards.py`.
`os.nice` does not exist on Windows; `startjob.ps1` starts every job at Idle priority instead.

## Shared-machine rules

The laptop also runs another project's game. Work only under `D:\mb`; never touch `D:\infamous` or anything else; never kill
a process you did not start (kill by a PID recorded in `D:\mb\jobs`, never by image name); run every CPU-heavy job at Idle
priority; at most 11 heavy jobs at once (`startjob.ps1` waits while 11 of its jobs are alive; running jobs are never
preempted). Open no windows; nothing needs a GUI.

## What differs from the Mac

* Objects: code, data, relocations (by symbol name) and symbols are identical; `.debug$S` carries host paths and the COFF
  header a timestamp, so whole files differ. `equiv.py`'s result-cache key already ignores those.
* An anonymous namespace's mangled names (`?A0x...`) hash the source path: a TU that has one compiles to different
  symbol names under `D:\mb\repo` than under another checkout path (the tools look such symbols up by name).
* Not ported: `case_sweep.py` (`fcntl`, `killpg`), `crashrun.py`, `loadprofile.py` and the wine-only `keepalive.sh`.
* No `MB_CL_SLOTS`-style compile limit: callers bound compile parallelism with `--jobs`.

## Verified equal to the Mac (commit a076d57 era)

* 14 `src/**/*.cpp` objects: identical code, data, relocations and symbols; 3 `run_all.py --manifest` runs (103/103, 23/23,
  30/30) print the same lines; `equiv.py` JSON (facegen_win 004f1460, facegen_misc 004ebb80) identical except `seconds`;
  `build.py` output: `.text`, `.rdata`, `.data`, `.reloc`, `.rsrc` byte-identical to the Mac's build (only the PE timestamp differs).
* Link order must not depend on the OS: `compile_all.sources()` sorts by the POSIX form of the path (`\\` sorts after
  digits, `/` before, which had reordered the objects and shifted 4725 symbols).
* Speed: the laptop at Idle priority is about 1.0 to 1.7x slower per emulated input than the Mac (facegen_win 919 s vs 553 s,
  facegen_misc 8 s vs 7 s) while another game runs; a cold first run is several times slower (disk and virus scan).

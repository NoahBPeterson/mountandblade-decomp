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

## Compile server (laptop as extra `cl.exe` slots for the Mac)

When all of the Mac's compile slots (`work/locks/cl.N`, `MB_CL_SLOTS`) are busy, `tools/matching/cl.sh` offers the compile to a
server on the laptop that runs the native `cl.exe` (no wine; a compile that takes 1 to 60 s under contention takes ~0.5 s). Every
caller of `cl.sh` benefits unchanged (`slice.compile_obj`, `run_all`, the `/Zs` probes, `trial_link`); the cache keys are untouched.

How it works:
* `tools/remote/compile_server.py` (copy at `D:\mb\rc\compile_server.py`, not the repo checkout) listens on the Tailscale IP, port
  47813, 36 slots. Auth: both ends hold the same token (`~/.config/mb-compile-token` on the Mac, `D:\mb\rc\token` on the laptop, both
  outside the repo); only HMACs of a fresh nonce cross the wire, in both directions (the client will not send source to a server that
  fails the check). The server runs `cl.exe` only, args starting with `@` are refused, job paths cannot leave the job directory.
* `tools/remote/rcl.py` (stdlib only) is the client. It reserves a slot first (a busy laptop answers in one round trip), then scans the
  source for `#include "..."` / `<...>` (includer's directory and its includers, then `/I` dirs, then `src/include`, `match/include`),
  ships exactly the repo files reached that the server lacks, content-addressed by SHA-1 (`D:\mb\rc\store`, 14-day prune at start), and
  gets the `.obj` (`/Fo`, `/Fa`) back. The job runs in a fresh `D:\mb\rc\j\<id>` stage tree holding only the shipped files, so the
  laptop's own `D:\mb\repo` sources can never leak in; the toolchain and DX SDK are the laptop's copies under `D:\mb\repo\work`.
  Paths in cl's messages are mapped back to the Mac's.
* Falls back silently: connect timeout 1 s, then a 30 s "down" marker (`work/locks/rcl.down`); a busy/unreachable laptop means cl.sh
  keeps waiting for a local slot. Commands it cannot ship (`@file`, `/Zi`, `/Fd`, `/Fp`, `/P`, no `/Fo`) and compiles that report C1083
  (a header the scan missed) run locally. `MB_RC=0` turns it off. It is also off when `~/.config/mb-compile-server` (one line,
  `100.84.111.92:47813`) does not exist (or `MB_RC_ENDPOINT` unset).

Start / stop / check (the server must be detached from the ssh session, so use WMI, not `startjob.ps1`'s `Start-Process`, which dies with
the session):

    # start: a ps1 run through ssh containing
    $cmd = 'cmd.exe /c "D:\mb\repo\.venv\Scripts\python D:\mb\rc\compile_server.py --bind 100.84.111.92 --slots 36 > D:\mb\jobs\rcserver.log 2>&1"'
    $r = Invoke-CimMethod -ClassName Win32_Process -MethodName Create -Arguments @{CommandLine=$cmd; CurrentDirectory='D:\mb\repo'}
    (Get-Process -Id $r.ProcessId).PriorityClass = 'Idle'; Set-Content D:\mb\jobs\rcserver.pid $r.ProcessId
    # check:  netstat -ano | findstr 47813      (LISTENING)   and   type D:\mb\jobs\rcserver.log
    #         from the Mac: python3 tools/remote/rcl.py with MB_RC_ENDPOINT set, or any compile with cl.sh while the Mac is saturated
    # stop:   taskkill /pid <PID in D:\mb\jobs\rcserver.pid> /t /f

**Traced compiles (the c2 tracer of `../vs2005-cpp`) run there too.** `D:\mb\rc\tc\c2-13.10.3077-trace` holds a copy of the tracer's
compiler directory (`../vs2005-cpp/work/toolchain/c2-13.10.3077-trace/VC/Bin`: cl.exe, the patched c2.dll, c2trace.dll; copy it again
after `c2build.py setup --c2 13.10` rebuilds them). With `C2TRACE_REMOTE=<this repo>/tools/remote/rcl.py` in the environment,
`c2trace.py`, `tsweep.py`, `regmap` and the rest send each traced compile through `rcl.py` (`MB_RC_TC` names the compiler directory,
the hook list is shipped, the trace log comes back as an output) and fall back to wine when the laptop declines (`/Zi`, an anonymous
namespace, a compiler it lacks). A traced compile waits for a slot instead of falling back. Measured 2026-11-11: one compile is no
faster than on an idle Mac (about 4 s against 6 s), but 12 in parallel finish in 14 s, and a 36-variant `tsweep` (72 traced compiles)
at `--jobs 16` takes 1 minute where `--jobs 3` under wine took over 10. `C2TRACE_REMOTE_DEBUG=1` and `MB_RC_DEBUG=1` say why a
compile was declined.

It must be restarted after a laptop reboot. Compile jobs run at Idle priority (the laptop's game wins).

Objects from the laptop vs the Mac: code, data, relocations, symbol names/values and the SEH table are identical; only the COFF
timestamp, the `.file` symbol, and the `.debug$S` record (which holds the `/Fo` path) differ, plus the symbol-table indices that follow
from the `.file` length. The Mac's own objects already differ from each other in exactly these fields (`compile_obj` compiles to a
per-pid `.part.obj` path), so nothing byte-compares whole objects. Anonymous-namespace mangled names hash the source path, so a TU
with `namespace {` mangles differently on the laptop (see "What differs"): `rcl.py` sends those to the local compiler.

Measured (30 real `src/game/*/*.cpp` modules, 11 compiles in flight, load average 13 to 22 from the other lanes, wine slots all busy):
Mac only (`MB_RC=0`) 760 s wall, 173 to 445 s per compile (the queue for a slot, not the compiler); with the laptop 3.9 s wall,
0.3 to 2 s per compile (a few seconds with a cold store; the connect + auth + reserve handshake is ~25 ms over Tailscale). The 30
objects were compared to the Mac-built ones with a COFF section/relocation/symbol comparison: 30 of 30 identical apart from the
timestamp, `.file` and `.debug$S` path fields (also agent.cpp, curve.cpp, item_kind.cpp, facegen_win.cpp).

## Smoke test on the laptop (`smoke.py --host win`)

A second place to run `tools/relink/smoke.py` while the Mac's one game slot is taken. The laptop runs the build natively (no wine).

    tools/relink/smoke.py --exe DIR/mb_standalone.exe --host win     # the laptop
    tools/relink/smoke.py --exe DIR/mb_standalone.exe --host auto    # the Mac if no game runs there, else the laptop if free
    ... --host win --save-ref                                        # store this run's menu as work/smoke/ref_menu_win.png

Needs `MB_WORKER_SSH`, `MB_WORKER_SCP`, `MB_WORKER_HOST` (see Connection). Exit codes as on the Mac: 0 OK, 1 differs / failed,
2 not configured, 3 busy (a game of ours is running on the laptop, or on both machines for `auto`; it is never killed).

How it works:
* The game is the Steam install `D:\SteamLibrary\steamapps\common\Mount and Blade`. The build is copied there as
  `mb_standalone.exe` next to the original (as on the Mac) and deleted afterwards; nothing else in the install is touched.
* A GUI process started from ssh lives in session 0 with no desktop. `smoke.py` creates a scheduled task `mb_smoke`
  (`schtasks /create ... /sc once /ru <user> /it`, then `/run`), which runs `D:\mb\smoke\run.cmd` -> `winsmoke.ps1` in the logged-in
  user's session (the user must be logged on, e.g. an RDP session). The task is ended and deleted at the end of every run, and
  `D:\mb\smoke` is removed, so nothing persists.
* `winsmoke.ps1` starts the exe (recording its PID), brings its window to the front and sends Return every 3 s until `rgl_log.txt`
  shows loading started (the launcher's Play button is the default), waits for `Finished All`, then captures the window's client area
  (`CopyFromScreen`) 8 s and 14 s later. Only that PID is ended.
* `smoke.py` polls `D:\mb\smoke\out\status.txt` / `done`, copies the two PNGs back and scores them with the same function
  (320x200 greyscale, threshold 12, better of two) against `work/smoke/ref_menu_win.png`. The Windows render differs from the Mac's
  (resolution, window frame), so it has its own reference, made from a known-good build.

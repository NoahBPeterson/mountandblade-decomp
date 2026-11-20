# Recording Newton 1.53 call traces from the original game

Differential testing of `newton_jolt.cpp` against the real Newton (the game's `Newton.dll`, version 153, built
2006-05-24): record every Newton call the ORIGINAL 32-bit game makes, with inputs and results, then replay the call
stream through the shim and diff (`tests/trace_replay.cpp`, target `physics_trace_replay`).

## Pieces

| file | what |
|---|---|
| `newton_trace.h` | trace format (records = op, thread, payload; per-op field spec shared by C, C++ and Python) |
| `proxy/newton_proxy.c` | logging `Newton.dll`: exports the 41 functions the game imports, forwards to the real DLL, logs; probes the real DLL after every `NewtonUpdate` (every body's matrix, velocity, omega, sleeping state), after `AddBone` (Newton's own bone state), after `CreateCapsule` (AABB, volume), once per body (damping, freeze threshold, AABB, inverse mass) |
| `build_proxy.sh` | `i686-w64-mingw32-gcc` (Homebrew mingw-w64) -> `work/physics_traces/bin/NewtLg.dll` |
| `record_win.py` | drives a recording on the Windows laptop: `setup`, `do CMD...`, `finish NAME` |
| `win/physdrive.ps1` | runs in the user's desktop session (scheduled task `mb_physlog`): starts the game, executes click/key/shot commands |
| `probe/newton_experiments.cpp`, `build_experiments.sh` | controlled scenes (damping, step clamp, inertia floor, sleep, joint limits) run on the real DLL on the laptop and natively on the shim (`physics_newton_experiments`); diff the outputs; `probe/newton153_output.txt` is the real DLL's output |
| `ntrace.py` | `summary`, `worlds`, `dump`, `extract` (cut a trace down to some worlds) |
| `ref/attract_2ragdolls.bin.gz` | reference trace (two attract-mode ragdolls, one a mounted rider with the mount capsule; worlds 15 and 42 of the 155 s recording) replayed by ctest |

## How the proxy is put in place (nothing in the install changes)

`mb_physlog.exe` is a copy of the unpacked original (`game/mb_original.exe`, the SteamStub-free image the hybrid tools
use) whose single import name `Newton.dll` is rewritten to `NewtLg.dll` (same length). `NewtLg.dll` (the proxy) loads
the install's own `Newton.dll` with `LoadLibrary`. Both files are copied next to the game and deleted by `finish`, which
also checks the install's `Newton.dll` SHA-1 (`6e6f5caa...`) and that nothing is left behind. The trace goes to
`D:\mb\phys\trace.bin` (`NEWTON_TRACE`; capped by `NEWTON_TRACE_MAX_MB`, default 3000), flushed at least once a second.

## Recording

    port/physics/trace/record_win.py setup
    port/physics/trace/record_win.py do "sleep 6" "key 13" "sleep 4" "key 13" "sleep 25" "key 27" "sleep 3" "key 27"
    port/physics/trace/record_win.py do "sleep 60" "shot s1"          # shots: work/physics_traces/shots/
    port/physics/trace/record_win.py finish attract                   # -> work/physics_traces/attract.bin

The laptop must have the user logged on (an active RDP session works; see `docs/remote_worker.md`). The game runs
full-screen in that session. Clicks are refused unless the point is on the game's window (the user often has an
always-on-top Task Manager open); the game window is raised to topmost before every click and shot.

Left at the main menu for ~1 minute, the game plays its attract-mode battle (a village raid: Swadian vs Vaegir infantry
and cavalry, the camera flying around). It uses the same mission code as a battle (`Agent::start_ragdoll`, one Newton
world per ragdolling agent, horse ragdolls, the mount capsule), so it is the cheapest source of many deaths: 48 ragdolls
in 155 s, up to 6 at once.

A Quick Battle works the same way: at the main menu click "Quick Battle" (`click 0.45 0.695` at 1720x967), then
"Skirmish 1" (`click 0.333 0.669`); the player can stand still, the AI fights it out ("Battle won!" after ~3 min,
59 ragdolls incl. a horse, up to 9 at once). A full-screen D3D9 game minimizes itself when it loses the focus;
physdrive.ps1 restores it before each command.

Recorded so far (work/physics_traces/): `t_attract.bin` (155 s attract-mode battle, 48 ragdolls, 62 MB),
`qb_skirmish1.bin` (Quick Battle Skirmish 1, 173 s, 59 ragdolls, 76 MB). Single deaths (no other ragdoll alive),
mounted riders (a mount capsule in the world), a horse (17 bones) and up to 9 concurrent ragdolls are all in them.

Traces stay out of git (`work/physics_traces/`, ~0.4 MB per second of battle); only `ref/` is committed.

## Replay

    cmake --build build/port/macos-arm64 --target physics_trace_replay
    build/port/macos-arm64/physics/physics_trace_replay work/physics_traces/attract.bin [--world N] [--sync] [--steps]

`--world N` replays one ragdoll (numbers from `ntrace.py worlds`), `--detail N` prints every bone's errors for the first N steps, `--sync` resets every body to Newton's recorded
state after each step (one-step model error instead of accumulated drift), `--steps` prints the root bone per step.

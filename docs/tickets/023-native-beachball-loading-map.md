# 023: Native macOS build shows the spinning wheel on "Loading Map File..."

Opened 2026-11-20 (maintainer, with a screenshot of the loading screen: "We seem to have some hang that causes the
rainbow spinning wheel on macos ... It does eventually pass this, but it is suspicious").
Effort: S to measure, S-M to fix. Related: 012 (threading architecture), 013 (performance foundation), 020 (native
profile run).

## What is seen

The native `mb` (build/port/macos-arm64, SDL3) sits on the loading screen with the text "Loading Map File..." long
enough for macOS to show the spinning wheel (the app has not taken events for ~2 s), then carries on to the map.
Not known yet: how long it stalls, whether it was the Release or the ASan build, new game or loaded save.

## Likely cause (unverified)

The loader runs in steps on the main thread (`src/game/party/game.cpp`, the step function around line 150): step 3
loads the save and sets the "ui_loading_map_file" text; step 4 runs `start_new_game` (0x51f1a0) and then
`build_map` (0x520b20) in one go before the next frame. Nothing pumps SDL events inside a step: the only
`SDL_PollEvent` loop is `port/platform/window.cpp:318`, reached from the game's PeekMessage between frames. So any
step longer than ~2 s shows the wheel. The original game on Windows blocks the same way, but Windows only marks a
window "Not responding" after 5 s, so a 2-5 s step went unnoticed there.

"Suspicious" because the step may also be much slower natively than in the original (a 64-bit port bug that does
extra work, a debug/ASan build, an O(n^2) path in a container replacement, or the load reading files byte by byte
through the CRT shim).

## What to do

1. Measure: time each loading step (MB_LOG timestamps around steps 3 and 4 and inside `build_map`) in the Release
   build, for a new game and for a loaded save; compare with the original exe under CrossOver and on the laptop.
2. If a step is much slower than the original: profile it (Instruments / `sample`) and remove the cause first
   (eliminate work before making it faster).
3. Whatever remains above ~1 s: keep the window responsive without changing src/ semantics, e.g. pump events
   (SDL_PumpEvents only, queueing input for the game's next PeekMessage) from a cheap hook the loader already calls
   often (file reads in the CRT shim, or the progress text update), rate-limited to ~every 100 ms. Moving loading to a
   worker thread belongs to ticket 012, not here.
4. Done when the wheel never appears during loading on the reference Mac and step times are within ~1.5x of the
   original's.

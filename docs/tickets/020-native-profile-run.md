# 020: Native profile run of the playable build

Opened 2026-11-15 (maintainer: "Performance is abysmal... we need to do a profile run eventually, once it's native").
Effort: S once a native build exists. Depends on: 004 (portable build), 007/008 (SDL3 platform + renderer), or a native
Windows run of the 32-bit exe as an interim baseline. Related: 013 (performance foundation), 018 (32-bit quick wins).

## Why

The play build (`./play.sh`: default patch sets + `gfx` + `play`) is playable but slow under CrossOver on Apple Silicon:
- a Quick Battle measured ~4.7 fps in one run (2026-11-15, render scale 1.25 + RCAS; not confirmed as steady state);
- the main menu runs at ~100 fps, so the cost is in 3D scenes;
- first play of a large sound stalls 2-5 s (FMOD 3.75's x87 Vorbis decode under Rosetta; ticket 010 replaces FMOD).

Under CrossOver every layer is suspect at once: Rosetta 2 translating x87 float code (~1.8x slower, docs/patches.md),
Wine's D3D9 -> Metal translation, the game's own CPU work. A profile there cannot separate them, so the real profile
waits for a native build.

## What to do

1. Baseline before any change: one scripted scene (a fixed save on the world map, a fixed Quick Battle with N troops),
   fixed resolution, render scale 1.0, gfx off. Record fps (mean, 1% low) and frame time over a fixed interval.
2. Profile that scene natively: CPU sampling (perf / Instruments / Superluminal), per-frame GPU timing, and the game's
   own phases (`patches/profile/sampler.cpp` and `tools/relink/profile_report.py` already symbolise per load phase;
   extend them per frame).
3. Split the cost: game logic (agents, AI, pathing, scripts), physics (Newton, ticket 011), animation/skinning,
   draw-call submission and state changes, shaders, audio.
4. Remove unnecessary work first, then make the rest faster (maintainer, 2026-11-15: "eliminate unnecessary work
   instead of doing unnecessary work faster").
5. Record before/after numbers here for each change.

## Interim data worth collecting now

- The same scene on the Windows laptop, natively (no Wine, no Rosetta), with `patches/profile`: separates "the game is
  slow" from "the translation layers are slow" without waiting for the port.
- gfx render scale 0.5 vs 1.0 in the same battle under CrossOver: if fps barely moves, the bottleneck is CPU, not GPU.

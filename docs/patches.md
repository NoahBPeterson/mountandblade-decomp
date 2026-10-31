# Patches: opt-in changes outside the reconstruction

`src/` is the faithful reconstruction. Everything the byte-exact, equivalence, layout and data checks measure lives
there, and so does every number in `docs/equiv_*.json` and `docs/progress_history.json`. Changes that deliberately
depart from the original (performance work, diagnostics, tooling hooks) live under `patches/<set>/` instead, and are
linked only when asked for:

    tools/relink/build.py --patches perf            # -> work/relink/patched-perf/mb_standalone.exe
    tools/relink/build.py --patches perf --patches profile

`compile_all.py` and `trial_link.py` only see `src/` (and `match/slices/`), so a patch never enters the difftest
pipeline, the progress counts or the gates' view of our source.

## Writing a patch

- One `.cpp` per patch in `patches/<set>/`. Optional `<name>.manifest.txt` for compiler flags (default `/O2 /MT /Gy /TP`,
  as for `src/`).
- A patch that **replaces** original functions says so, one line per function:

      // @ replaces 0x4ebfd0

  and defines the function under the same (mangled) name the reconstruction uses. The build makes the patch the owner
  of that symbol, so the link keeps the patch's definition (the same check that keeps a row's own definition fails
  the build if it does not).
- A patch that only **adds** code (a profiler thread started from a static constructor, a log hook) needs no marker.
- No `#ifdef PATCH` in `src/`: even an unused branch can change the faithful codegen and hide behaviour from the checks.

## Verifying a patch

- Replace only functions whose faithful version is byte-exact or equivalent, so correct behaviour is pinned down.
- A behaviour-preserving patch (a faster loader that must produce the same data) is checked against the original with
  the equivalence harness and recorded separately (`docs/patch_verdicts.json`), never in `docs/equiv_*.json`.
- A patch that changes behaviour on purpose (a frame cap, a log line) says so in its header comment.

## Sets

- `profile`: `sampler.cpp`, a 1 ms sampling profiler of the main thread (writes `profile_samples.bin` in the game folder;
  `tools/relink/profile_report.py` symbolises it per load phase, `tools/relink/logwatch.py` stamps the phases).
- `devfast`: shortcuts for test runs that **change behaviour** (never for play): `no_intro_videos.cpp` (binkplay
  launches fail, as with binkplay.exe missing) and `no_sound_samples.cpp` (every FSOUND_Sample_Load returns 0: no
  sound effects, ~55 s less start-up under Wine). Both swap one of our own import slots from a static constructor
  (`iat_hook.h`), so no function of the reconstruction is replaced.
- `diag`: runtime diagnostics. `rootwatch.cpp` logs the GUI root-widget stack (g_gui's vector) to rootwatch.txt
  whenever it changes (found the dropdown list that never closed).

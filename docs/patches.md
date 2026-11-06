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
- `fps`: `fps_title.cpp` puts the frame rate in the window title ("... - 59.9 fps (16.7 ms)"), read every 500 ms from
  the m_fFPS the faithful CD3DApplication::UpdateStats (0x402580) already measures once a second. Title text only.
- `diag`: runtime diagnostics. `rootwatch.cpp` logs the GUI root-widget stack (g_gui's vector) to rootwatch.txt
  whenever it changes (found the dropdown list that never closed).

- `perf`: behaviour-preserving load-time patches (record: `docs/patch_verdicts.json`). All replace byte-exact functions.
  - `stream_read_slots.cpp` replaces `mem_stream::r_int/r_i64/r_byte/r_tstr` (0x417b10/0x417b30/0x417af0/0x417a90): the
    binary reader's scalar read is one inline copy instead of forwarder -> slot -> `rgl::string::read` -> `memcpy`, and
    a string is one append instead of one `push_back` per character. A .brf is ~19.7 M scalars and 14.9 k strings.
  - `resmgr_add_index.cpp` replaces `resmgr::add` (0x41ba90): the "attempt to reregister mesh" check is a hash lookup
    instead of a scan of all registered meshes (11.2 M `equals` calls for 4740 meshes).
  - Where the time goes (static counts and a native model, see the header comments and the 2026-11-02 notes in
    docs/tickets/013): the stream layer is NOT the main cost of the 11 s on its own (modelled 0.115 s -> 0.043 s native
    for all 75 MB); measure with `--patches profile` before expecting more from this set.
- `perf_sound`: `lazy_samples.cpp`, deferred and background loading of the ~360 FMOD sound samples. **Changes timing and
  a few edge-case behaviours** (header comment lists them), so it is a separate set; do not combine with
  `devfast/no_sound_samples`. `MB_SOUND_LAZY=demand` loads only on first play (no background thread).
  **Linked by default** (user, 2026-11-02): every build.py run adds it (it replaces no function of the reconstruction,
  only our own fmod import slots), except with `--no-default-patches` (use that when testing sounds) or `--patches
  devfast` (same import slot). The output directory is still named after the sets you ask for, so a plain build still
  lands in work/relink/standalone.

- `guards`: checks the original doesn't have, kept out of `src/` so those rows can byte-match. **Changes behaviour on
  purpose**, only where the original reads out of bounds. **Linked by default** in the play build; `--faithful` and
  `--no-default-patches` leave it out.
  - `build_chain_guards.cpp` replaces `ChainGraph::build_chain` (0x5234e0): `cur < 0` stops the walk and a neighbour `v < 0`
    is skipped (they faulted at 0x49d47a). Not yet known whether the original can reach those reads; a `--faithful` build
    run through a world map answers it (see the file's header).

### Measuring the perf sets (runtime, under CrossOver)

    uv run python tools/relink/build.py --patches perf --out <dir A>                         # BRF/stream/add patches
    uv run python tools/relink/build.py --patches perf --patches perf_sound --out <dir B>   # plus lazy sound
    uv run python tools/relink/build.py --patches perf --patches profile --out <dir C>      # to see where step 2 goes

Copy the exe (and its .map for the profile build) into the game folder and compare with the unpatched standalone:
`step2_timing.txt` ("parse_module_info: N ms", reference 11-13 s), `load_timing.txt` (step 2 -> 3 etc.), `mod_timing.txt` and
`snd_timing.txt` ("sounds.txt N" / "part 1: N ms", reference 58 s), `lazy_sound.txt` (perf_sound: per real load, `BG`
lines are background, `GAME` lines are first-play stalls), and with the profile build `profile_samples.bin` ->
`tools/relink/profile_report.py --samples ... --exe <dir C>/mb_standalone.exe` (use a map of the SAME build).

## Variant builds

**Default since 2026-11-02:** a plain `build.py` is the play build: `/arch:SSE2` plus the default patch sets (perf_sound, guards).
Module loading under CrossOver went from 11.7 s to 4.0 s (x87 math is ~3x slower under Rosetta; mesh_x::compute_tangents
was 77% of it). `build.py --faithful` builds the x87 reconstruction with no patches, as the byte-exact and equivalence
checks compile it: use that for independence/runtime checks of the faithful code.

`build.py --cflags "/arch:SSE2"` compiles every src/ object with extra flags (output: work/relink/variant-archSSE2, or
patched-SET+archSSE2 with --patches). Under Rosetta 2 (Apple Silicon CrossOver) x87 float code runs ~1.8x slower than
SSE2 scalar code (microbenchmark in the game's bottle: 18.4 s vs 10.4 s); VC7.1 emits movss/mulss/addss for float math
with /arch:SSE2. Float results differ in the low bits (x87 keeps 80-bit intermediates), so a variant is for play, never
for the equivalence or byte-exact checks.

## Open tickets

- Battle-load time (2026-11-01, parked: reconstruction work first). Loading a battle is slow; reading the files is
  not the cause (4 GB on a >5 GB/s disk). Measure before patching:
  - build with `--patches profile` and load one field battle; `tools/relink/profile_report.py` per load phase
  - split the samples between the game's code and wine's D3D9 (texture/vertex-buffer creation, `Lock`, shader
    compiles): a wine-side cost points at the D3D translation layer, a game-side one at a `perf` patch
  - suspects in the game: the terrain builders (`Terrain::generate_heights` 0x4713d0, `make_river` 0x46bc70,
    `mesh_terrain` 0x481570, `create_water` 0x480db0), whose hot path is `noise_gradient` 0x414ae0 and `__ftol2`
    0x535a64; BRF parsing; per-agent setup
  - only then a `perf` patch (SSE noise, cheaper rounding, caching the height field per scene seed), if the
    numbers justify it

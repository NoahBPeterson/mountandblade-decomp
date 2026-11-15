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

- `gfx`: graphics settings the original lacks, **opt-in** (`build.py --patches gfx`, not a default set). **Changes
  behaviour on purpose**, but only when a key asks for it. The keys go in rgl_config.txt, and the launcher's Configure
  dialog and the in-game Video Options page both show them (below):

      gfx_anisotropy = 0        # 0 (off), 2, 4, 8 or 16; capped by D3DCAPS9.MaxAnisotropy
      gfx_render_scale = 1.00   # 3D scene size per axis, 0.25 .. 2.0 (above 1: supersampling)
      gfx_fsr = 0               # below 1: 0 bilinear upscale, 1 AMD FSR 1 (EASU + RCAS); at 1 and above: 1 adds RCAS
      gfx_fsr_sharpness = 0.20  # RCAS, in stops: 0 sharpest .. 2
      gfx_log = 1               # (diagnostics, optional) gfx_log.txt in the game folder

  The modes, by scale: below 1 upscale (bilinear, or EASU + RCAS); exactly 1 with gfx_fsr = 1 sharpen only (the scene
  renders at native size into our target, then RCAS); above 1 supersample and downsample (exact 2x: one bilinear tap,
  the 2x2 box; other ratios: a tent filter of one output pixel's radius, 4x4 point taps), then RCAS when gfx_fsr = 1.
  EASU never runs at 1 or above. With the defaults the hooks pass everything through and the frame is the original's.
  - `gfx_config.cpp` reads the keys itself from a static constructor, and replaces `rgl_config::save` (0x415fb0): the
    faithful 24 lines, then the gfx lines from `g_gfx_saved` (the values read at start-up, as a UI last set them). Every
    writer of the file (the launcher, Video Options, the shadow-map fallback) goes through that function, so saving
    options keeps the keys. The game's own reader (`Config::set` 0x4bbba0) ignores names it does not know, and its
    splitter stops a value at the '.'. `gfx_apply_saved` hands `g_gfx_saved` to the device: at once before it exists
    (the launcher), else at the next Present.
  - `gfx_device.cpp` swaps the Direct3DCreate9 import slot (as `devfast/iat_hook.h`), patches the IDirect3D9 vtable's
    CreateDevice and from it the device vtable's Reset, Present, EndScene and SetSamplerState, whatever the settings
    (a UI can turn a feature on later). Anisotropy: MAXANISOTROPY is forced to the level on all 16 samplers (after
    CreateDevice / Reset and whenever set), and a LINEAR MINFILTER becomes ANISOTROPIC only while that sampler's MIPFILTER
    is not NONE; POINT (CD3DFont's text) and un-mipped LINEAR (render targets, shadow maps) stay as asked. The game's own
    sampler requests are tracked, so a level change at run time re-sets every sampler. It drops D3DCREATE_PUREDEVICE
    (state blocks below; the game does not ask for it under CrossOver, flags 0x40). At a Present with a pending UI change
    it applies it: the new level, and the scene targets released to be remade at the new scale.
  - `gfx_scene_state.cpp` replaces `RenderScene::apply_state` (0x46ca50), the per-pass viewport / clear / camera setup
    of `render_batch::render` (0x47e520): the faithful body, after a call to `gfx_scale_scene_begin`.
  - `gfx_scale.cpp`: a pass is redirected into a scaled colour target and depth-stencil (multisampled like the
    backbuffer when it is) when render target 0 is the backbuffer, its camera is perspective and its viewport covers
    the whole backbuffer; consecutive such passes share it. It is resolved to the backbuffer (the modes above; EASU or the
    downsample write a backbuffer-sized texture that RCAS reads) when the next backbuffer pass does not qualify, and at
    EndScene and Present. Above 1 the scale is capped so the target fits D3DCAPS9.MaxTextureWidth/Height; when the
    targets cannot be created it tries 1.5, then (with FSR) 1, then leaves the scene native. The GUI's passes are orthographic, so the 2D UI and text draw after the resolve at native resolution;
    3D views inside the GUI that do not fill the screen stay native too. The passes save/restore the whole device state
    with a D3DSBT_ALL state block (the game caches its render states). Targets are D3DPOOL_DEFAULT: released before
    every Reset (alt-tab, resize, mode change) and recreated at the new size on the next pass. Without vs_3_0/ps_3_0 or
    when a target cannot be created, nothing is redirected.
  - FSR 1: `shaders/gfx_fsr.hlsl` ports ffx_fsr1.h (v1.20210629, FSR 1.0.2; licence and source commit in
    `shaders/LICENSE-FidelityFX-FSR.txt`) to ps_3_0: Gather4 becomes point taps (EASU keeps its 12), the integer-trick
    rcp/rsqrt approximations become plain ones with zero guards, ip comes from VPOS; FsrEasuCon / FsrRcasCon run on the
    CPU. `shaders/build_shaders.sh` compiles it into `shaders/gfx_shaders.h` (committed, so the build needs no shader
    compiler) with the game's own d3dx9_31.dll compiler (9.15.779) under wine: work/ext/dxsdk_jun10 has no fxc.exe.
    FSR 2/3 (temporal, motion vectors) are out of scope. The same file has `ps_down`, the supersampling tent filter.
  - `gfx_launcher.cpp`: the launcher's Configure dialog (template 0xc1, unchanged in resources/). A static constructor
    swaps the DialogBoxParamA import slot; for 0xc1 the dialog runs with our procedure, which calls the game's
    (`wndproc_thunk` -> `GameApp::config_proc` 0x4d0240) and then, at WM_INITDIALOG, grows the dialog and its tab
    control by four rows, moves OK / Auto Adjust / Cancel down and creates the controls with CreateWindowEx in dialog
    units (MapDialogRect) and the dialog's font: Anisotropic Filtering (Off / 2x / 4x / 8x / 16x), Render Scale (50 67 75
    85 100 125 150 200 %), an AMD FSR 1 checkbox and FSR Sharpness (0.00 .. 2.00, enabled with FSR). SkinMagic skins
    them like the template's own. They show on the Video tab only (WM_NOTIFY of the tab control, as
    `OptionsDlg::set_active_tab` 0x4b9790 does for the others). OK stores them in `g_gfx_saved` before the game's
    handler saves the file. A value that is not a preset gets its own entry.
  - `gfx_video_page.cpp` / `gfx_video_apply.cpp`: the in-game Video Options page. `Recf0::init_screen` (0x4e05c0) and
    `Recf0::frame_update` (0x4d63b0) are replaced by copies of their faithful bodies (src/game/cls_map/cls_map.cpp,
    src/game/options_panel/video_page.cpp) with four drop-down rows of the game's own kind (`make_dropdown` + a
    `make_hint` label): Anisotropy Level, Render Scale, AMD FSR 1 (Off / On) and FSR Sharpness. The page had no free
    row, so the four new rows take the top and every y of the original 18 goes through `GFX_Y` (0.032 apart instead
    of 0.04, under the new ones, ending where they did); at the bottom the new lists could not open (a drop-down opens
    downwards). The Done button and the performance caption keep their places. frame_update reads a
    changed row into `g_gfx_saved`; Done saves the file as before, then `gfx_apply_saved` applies the rows at the next
    Present, live (no restart and no "new area" message for them).
  - Tested (2026-11-15, Mac/CrossOver, 1280x800 windowed): the menu smoke passes with the defaults, anisotropy 16,
    scale 0.5 bilinear and scale 0.67 + FSR (work/smoke/gfx_*/). The main menu is a single orthographic GUI pass
    (gfx_log.txt: "persp 0 ... -> native"), so those runs prove the hooks, shader creation and the GUI staying native,
    not the redirect itself: the scaled path in a 3D scene (battle, world map) still needs a play-test.
    Later the same day (supersampling and the UIs): menu smokes pass at scale 1.5 bilinear and at 1.0 + FSR (RCAS only)
    (work/smoke/gfxui_a0f3_scale150/, gfxui_a0f3_rcas/). Driven by CGEvent clicks: the launcher dialog writes the keys
    on OK (150% + FSR gave `gfx_render_scale = 1.50`, `gfx_fsr = 1`), its rows show on the Video tab only; the Video
    Options page's Render Scale row set to 125% was saved on Done and applied at the next Present (gfx_log.txt
    "applied"), and a Quick Battle (Skirmish 1) then drew the 3D scene through a 1600x1000 target, tent downsample +
    RCAS, with the HUD native and nothing misplaced (work/smoke/gfxui_a0f3/). Not yet seen in 3D: upscaling, RCAS only,
    exact 2x, MSAA backbuffers; the page's final layout (new rows on top) was built but not looked at.

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

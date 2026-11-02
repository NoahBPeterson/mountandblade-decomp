# 013 Performance foundation: measurement, loading, frame pacing, memory

Area: perf | Effort: M | Impact: high (start-up and battle load are the visible pain) | Depends on: 004 for most items; the measuring part works today

## Summary

Make performance measurable in the portable build and fix the specific, measured hot spots: start-up resource parsing, battle loading, the spin-wait frame limiter, and static memory pools.
(Quick wins that only matter for the interim 32-bit exe are in 018.)

## Evidence

Start-up phases from `load_timing.txt` in the game folder (CrossOver, Apple Silicon, mb_standalone with timing patch, 5+ runs, 2026-10-30):

| phase | time | cause |
|---|---:|---|
| step 2 `parse_module_info` | **11.0 s** (11,002-11,012 ms, very stable; 13.1 s in the earlier 2026-10-26 runs) | `load_resource` lines -> `resmgr::load_pack` 0x41e150 for **148 `.brf` files, 76 MB** in `CommonRes/` (+ module resources); parsed token by token through the stream virtuals (`s->r_int`, `r_float`, `r_tstr`), `new body_part[n]`, `blocks.push_back`; src/rgl/core/resmgr.cpp:1100-1190 |
| step 6 `Loading Textures` | 1.84 s | 374 files, 156 MB DDS/TGA through `D3DXCreateTextureFromFileExA` (resource_io.cpp:350) |
| `Finished` | 0.7 s | |
| sounds (`mod_timing.txt` `sounds.txt`) | **57.8-58.2 s** with the real FMOD; 0 s with patches/devfast | ~360 `FSOUND_Sample_Load` (ticket 010) |
| `profile_samples.bin` (181.8 s, 134,928 main-thread samples) | 62.2% outside game code: fmod.dll 29.4, ntdll 19.5, win32u 19.4 | mostly Wine + FMOD |

So 76 MB of BRF takes 11 s: ~7 MB/s, i.e. CPU-bound parsing (disk reads are >5 GB/s: docs/patches.md "Battle-load time" ticket). Caveat: measured through Rosetta 2 + Wine; native will be several times faster, but the structure (one virtual call per scalar, one `new` per record) is the thing to fix.

Frame loop: Render3DEnvironment spins (`while (fElapsed < m_fMinFrameTime) fElapsed += DXUtil_Timer(...)`, d3dapp.cpp:837): at the default `max_framerate = 100` (Config::init_defaults, config.cpp:708) it burns a core whenever a frame takes < 10 ms. Present interval defaults to IMMEDIATE (`0x80000000`, config.cpp:374) unless `force_vsync`. No `timeBeginPeriod` anywhere in src/ (rg: 0 hits). `Sleep(100)` only when the device is lost.

Memory: `.data` VirtualSize 143.9 MB static (ticket 003/016); texture 156 MB on disk.

## Detailed plan

1. **Profiling harness** (portable): `perf_scope` macros + Tracy (or a small chrome-trace writer) around the frame phases; sampling via `perf`/Instruments externally. Port `patches/profile/sampler.cpp` idea to SIGPROF-based sampling for Linux/macOS. Redo the per-phase breakdown with *correct* symbol maps (the 2026-10-31 samples were symbolised against a different build and are unreliable below the module level).
2. **BRF/TRF loading**:
   a. read whole file into memory (`mem_stream` path exists: `bin_reader_b : mem_stream_inl`, resmgr.h:127);
   b. add bulk accessors `r_floats(float*, n)`, `r_ints(int*, n)` to `stream_base`, used by `read_mesh/read_body/read_skel_anim` for vertex/keyframe arrays (the data is little-endian packed; one `memcpy` per array on LE hosts);
   c. parse each `.brf` on a worker (012) into a private `blocks` list, then merge in file order (`add(&b[i])` order must be kept: names resolve in order);
   d. optional on-disk cache of parsed resource tables (mmap-able), invalidated by file mtime like the `.trf`-vs-`.brf` check in `load_pack`.
3. **Samples**: decode on a worker or lazily on first use (010).
4. **Textures**: upload on the loader pool, DDS parsed straight into transfer buffers (009); honour `D3DX_SKIP_DDS_MIP_LEVELS` by seeking.
5. **Frame pacing**: replace the spin with `SDL_DelayPrecise`/timed wait for the remaining time, leave a ~0.5 ms spin tail; default to vsync on (swapchain present mode), `max_framerate` as a cap only when vsync is off.
6. **Memory**: convert the static pools to heap/arena allocations sized by data (016); avoid 100+ MB of zero pages touched at start (BSS is lazily committed so this is mostly address-space and RSS accounting, but it also hides count limits).
7. Terrain generation (`generate_heights` 0x4713d0, `noise_gradient` 0x414ae0, `__ftol2`): parallel rows (012); replace `__ftol2` truncation with a plain cast.

## Code sketch

```cpp
// stream_base: new bulk virtuals (default implementations loop over r_float so existing readers keep working)
virtual stream_base& r_floats(float* v, int n) { for (int i = 0; i < n; ++i) r_float(&v[i]); return *this; }
// bin_reader/mem_stream override: memcpy(v, cur, n*4); cur += n*4;
```

```cpp
// frame limiter
double target = 1.0 / max_fps, t = now(); if (t < next) { double rest = next - t; if (rest > 0.0015) SDL_DelayNS((Uint64)((rest - 0.001) * 1e9)); while (now() < next) {} }
```

## Risks

- Bulk reads must match what `r_float` does for text (TRF) streams: keep the default loop for text.
- Parallel BRF parsing changes allocation order; name tables must stay deterministic.
- Cache invalidation bugs (stale parsed tables) -> corrupted resources; include a version + size + mtime key.

## Acceptance

Native build on a 2020-class laptop: module load < 3 s (from 11 s under CrossOver; target is an estimate), samples < 1 s, field-battle load not dominated by a single thread; flame graph shows no `__ftol2`.

## Effort

M (4-6 wk). Estimates only.

## Findings 2026-11-02 (static analysis; patches in patches/perf, patches/perf_sound)

- The .brf data: 146 files (CommonRes), 78.9 MB, 19.67 M scalars, 14.9 k strings, 4740 meshes (636 k points, 1.12 M vertices, 970 k faces), 408 materials, 337 textures, 82 shaders, 600 bodies, 118 animations, 2 skeletons. Of the scalars: meshes 17.1 M (vertices 10.1 M, faces 2.9 M, points 1.9 M, morph keyframes 2.0 M), animations 1.95 M, bodies 0.65 M.
- The file is already read whole (`text_stream::open_read` -> one fread); a scalar costs forwarder -> `mem_stream::r_int` -> `rgl::string::read` -> `memcpy` (patches/perf/stream_read_slots.cpp inlines it). A native model of that layer over all 146 files: 0.115 s faithful, 0.043 s patched (2.6x), identical hash of every value. That is ~1% of the 11 s, so **the stream layer alone cannot explain the 11 s**; the "per-scalar virtual call" hypothesis above is not the dominant cost.
- `resmgr::add` scans every registered mesh per new mesh (11.2 M `equals`): 0.03 s native (hashed 0.0002 s), results identical incl. injected duplicates and empty names. Also minor. `get_material`/`get_texture` are linear scans too (4740 x 408 and 408 x 5 x 337 compares): minor.
- The 2026-10-31 profile_samples.bin by module (5 s bins, no map needed): the step that follows the 53 s of fmod.dll (sounds) shows mb_standalone.exe 52% + **ntdll 37%** (heap/virtual memory in Wine) for ~30 s, and later 69/22. So the BRF phase is heavy in ntdll (allocation, page faults), not in the scalar reads. Suspects, unmeasured: ~200 MB of vector storage (`vertex60` 96 B x 1.12 M, faces, points) allocated and filled (page faults + `resize(n, value)` fill), `mesh::build_skin` (two 1 MB stack arrays, `__chkstk`, per skinned mesh), `mesh_x::compute_tangents` (x87, 970 k faces, under Rosetta), `new body_part[n]`/`new` per record. Next step is a profile with the matching map (`--patches perf --patches profile`).
- Sounds: 386 files, 9.4 MB (371 Ogg Vorbis, 14 WAV), flags HW3D (0x1000) or 2D (0x2000), no LOADMEMORY. The cost is FMOD decode + DirectSound buffer creation under Wine (profile: 53 s in fmod.dll, 98%), not file reading. The only consumers of a sample are FSOUND_PlaySoundEx (2 call sites) and the three Sample_* calls in `music_set::load` (length and default frequency), which is what makes a lazy scheme possible (patches/perf_sound).
- Textures: load_timing.txt shows 1.84 s then 0.7 s for the last two steps (by the code, step 6 is `load_module_files` and step 7 `load_all`, the texture load, so the 1.84 s label in the ticket table may belong to the module files; tex_timing.txt's per-file sums average ~0.85 s per run). D3DX decode of 156 MB inside Wine; no behaviour-preserving win identified without D3DX internals; not patched.

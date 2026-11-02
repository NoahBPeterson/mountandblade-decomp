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

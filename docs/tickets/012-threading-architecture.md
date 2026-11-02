# 012 Multithreading: from one physics worker to a job system

Area: engine | Effort: L | Impact: high (frame time in big battles) | Depends on: 004, 011; 006 helps

## Summary

Multithreading is a first-class goal. Today there is one hand-rolled worker with a critical section held across a whole tick. Replace it by a job system (SDL3 threads or a small
work-stealing pool) and parallelise the subsystems that are naturally data-parallel.

## Evidence: what exists

- Frame loop (single thread): `CD3DApplication::Run` 0x402ee0 -> `Render3DEnvironment` 0x402430 -> `FrameMove` (dt as float seconds from `DXUtil_Timer`, variable timestep) -> `Render` -> `Present`;
  frame limiter is a **spin-wait** `while (fElapsedAppTime < m_fMinFrameTime) fElapsedAppTime += DXUtil_Timer(...)` (d3dapp.cpp:837; `m_fMinFrameTime` is `g_res_scale = 1/max_framerate`, default 100 fps, cls_map.cpp:141).
- Worker: `_beginthread(loader_thread_body, 0x40000)` only if `force_single_threading == 0 && GetProcessAffinityMask > 1` (cls_map.cpp:130-137). Body (gui_edit.cpp:1699, 0x4c8900):
  `WaitForSingleObject(g_mt_event); EnterCriticalSection(&g_mt_cs); if (g_paused_time > 0) { g_phys.update(g_paused_time); g_paused_time = 0; } LeaveCriticalSection`.
- `Screen_s261e0::tick` 0x5261e0 (mission_5.cpp:455) **enters `g_mt_cs` for the entire tick** (input, `update`, `g_mission->frame(sdt,1)`, camera, GUI flush) and leaves it just before returning,
  then `SetEvent(g_mt_event)`. `MissionG::frame` 0x483f60 (mesh_game.cpp:607) skips `g_phys.update(dt)` when `g_mt_inited` and accumulates `g_paused_time += dt` instead.
  Net effect: physics for tick N runs *concurrently with rendering of tick N* (and can never overlap game logic). It is a 2-stage pipeline, not parallel simulation.
- Thread safety is by lock, not design: shared globals everywhere (`g_phys`, `g_mission`, `g_game`), 75 `volatile` uses, 4 `CRITICAL_SECTION` mentions.

## Evidence: where parallelism is (candidates found in src/)

| subsystem | function / location | shape | notes |
|---|---|---|---|
| physics step | `PhysSet::update` 0x472dc0: loops `recs[i].world` calling `NewtonUpdate` | per-world, independent worlds (up to 4+) | Jolt gives intra-world parallelism (011) |
| world update | `rgl::world::step` 0x427f40 (entity_types.cpp:72): `ents[i]->update(dt)` for all entities, then `release_links/insert_obj` into `world_grid`, then emitters | entity update loop; insert phase writes shared grid | split update (parallel) from grid insertion (serial or sharded by cell) |
| agents | `MissionG::frame` 0x483f60: three agent passes (non-horses, horses, ticks), `update_team_stats`, `update_team_counts` | per-agent AI/animation; reads shared team state | AI decisions in parallel against a snapshot, commit serially |
| skinning | `FxDef::skin` 0x4291c0 (fxdef_skin.cpp): per vertex 4-bone blend of position+normal, `mesh::ensure_skin` 0x412700; 5 vertex decls incl. `skinning`/`normalmap_skin` | embarrassingly parallel per mesh | CPU skinning today; GPU skinning (008) removes it |
| pathfinding | `NavMesh::reachable`, `ChainGraph::build_chain` (0x49d47a), `CellGrid::update_probes` 0x472ed0, `nearest_face@level` | per query | profile samples show `reachable@NavMesh` 0.4% of one load phase |
| terrain generation | `Terrain::generate_heights` 0x4713d0, `make_river` 0x46bc70, `mesh_terrain` 0x481570/0x47ff60, `create_water` 0x480db0; hot leaf `noise_gradient` 0x414ae0 + `__ftol2` | per-tile/row; mission load only | parallelise rows; docs/patches.md open ticket "battle-load time" lists the same suspects |
| particles | `psys`/`emitter::update` (world::step), `FxDef::render`, mesh_obj.cpp | per emitter | CPU particle meshes; candidate for GPU later |
| scene rendering | `render_scene` (terrain_b.cpp, rgl/scene/render_scene.cpp), frustum cull `frustum_grid.cpp` | cull per cell parallel; command recording serial or per-pass (SDL_GPU supports multiple command buffers) | after 008 |
| script interpreter | `Operation::execute` 0x49fa10 | inherently serial, global state | keep on the main thread |
| resource loading | `resmgr::load_pack` 0x41e150, texture loads | I/O + parse | per-file parallel (013) |

Measured context: a real CrossOver run (patches/profile, 181.8 s, 134,928 samples of the main thread): 62.2% of samples were outside the game's own code (`fmod.dll` 29.4, `ntdll` 19.5, `win32u` 19.4,
`opengl32` 2.0). Symbolisation of our own code was unreliable (map from another build), so no per-function ranking is claimed here; ticket 013 specifies the measurement to redo.

## Detailed plan

1. `core/jobs.h`: `parallel_for(n, grain, fn)`, task graph with explicit dependencies, worker count = `SDL_GetCPUCount()-1`. Main thread participates while waiting.
2. Remove `g_mt_cs`/`g_mt_event`/`g_paused_time`/`g_mt_inited`; delete the `force_single_threading` gate (keep the config key meaning "0 workers").
3. Establish a frame phase model: INPUT -> SIM (parallel jobs over agents/entities, serial commit) -> PHYSICS (Jolt jobs) -> ANIM/SKIN (parallel) -> CULL (parallel) -> RECORD/SUBMIT (serial or per pass).
4. Make shared state explicit: for each parallel loop, list the globals it touches (use the 6,856 offset-commented structs as the guide; start with `world::step` since it is already clean C++ with `std::vector<entity*>`).
5. Start with the easiest wins: terrain generation rows and skinning (pure functions of inputs), then the entity update loop, then agents.
6. Thread sanitizer (TSan) CI on Linux for the battle script.

## Code sketch

```cpp
// world::step, parallel update phase (src/rgl/scene/entity_types.cpp)
jobs::parallel_for(ents.size(), 64, [&](size_t lo, size_t hi) { for (size_t i = lo; i < hi; ++i) ents[i]->update(dt); });
for (entity* e : ents) if (e->awake()) { release_links(e); insert_obj(e); }       // serial until world_grid is sharded
```

## Risks

- Hidden shared state: entities call into global singletons from `update` (RNG `rand()` 244 sites is a global; make it thread-local or per-agent). Data races are likely in AI that reads other agents.
- Non-determinism: results depend on scheduling unless commits are ordered; acceptable (no multiplayer) but complicates bug reproduction: provide a `--jobs=0` serial mode for debugging.
- Equivalence harness validity: the harness verifies single-threaded function bodies against the original; parallelising calls leaves each body intact.

## Acceptance

Battle with 150+ agents: >= 2.5x frame-time reduction of the simulation phase on 8 cores versus `--jobs=0`; TSan clean on the battle script; identical agent counts and mission outcomes in scripted tests.

## Effort

L (8-12 wk incremental). Estimate only.

# src/game/terrain_b migration: the second half of the terrain object, then the mission/scene container (0x46e590..0x47aa40)

Region of the TU map (`work/migrate/tu_map.csv` TUs 61 (tail) and 62). The staging slices in `match/slices/`
(s0046e590 .. s0047a980) are unchanged and stay the reference. Every module builds with `/O2 /MT /Gy /TP` (one flag
set, no per-row flags; the `/EHa` rows of the slices, `reserve` and friends, also match without it).

## Summary

| module | range | rows | exact before (slices) | exact after | nonmatching | partial |
|---|---|---|---|---|---|---|
| terrain_b.cpp | 0x46e590..0x478830 | 110 (101 functions + 9 symbol-less catch funclets) | 78 | 78 | 32 (23 functions + 9 funclets) | 0 |
| mission.cpp | 0x478830..0x47aa40 | 19 | 9 | 10 | 5 | 4 |
| **total** | | **129** | **87** | **88** | **37** | **4** |

Regressions: none (the byte-exact VA sets of the slices are a subset of the manifests). Improved: 0x479930
`vector<MRow56>::erase` (missing in the slices, now exact); 0x470750 `compute_edge_creases` 315 -> 142 diff bytes;
0x472160, 0x4713d0 and others keep the slice numbers or move by a few bytes (see the nonmatching files).
The nonmatching/partial files list every non-exact row with its measured diff and equiv result.

## Where the region starts, and the boundary at 0x478830

- **Start: exactly 0x46e590.** The TU that contains it (61, `terrain_obj`) starts earlier (0x4645b0); the terrain_a
  agent owns everything before 0x46e590. There is no call evidence across 0x46e590 (no function in 0x464000..0x47b000 makes a
  forward call, so there is no hard TU-boundary evidence anywhere in this region); `render_scene` is a clean first
  function (class `render_batch`, first user of the shared render-state headers).
- **0x478830 (`CellGrid`-side `init`) starts module 2.** Evidence is a class change only: from here on every function
  is a member of the scene container (`CellGrid`, ctor 0x478ad0) or of `Mission`, while 0x46e590..0x478830 is Terrain /
  Mesh / Agent / vector-instance code. Weak (no forward call, no marker). Because both pieces are small and share
  most headers, a reader may merge the two files; they are kept apart so a wrong boundary costs one file move.
- **End: 0x47aa40.** `vector<MRow56>::resize` (0x47a980) is the last function; 0x47aa40 is where the next slice starts.
- Two functions the slices placed in "Mission" are `Terrain` members (`make_grid` 0x47a460, `spawn_bodies`
  0x479da0) and one is `Mission::post_spawn(MAgent*)` (0x47a0c0); the slices had a wrong `this` and wrong offsets
  there (strings are "grass" / "grass_no_shadow"), so `post_spawn`, `make_grid` and `build_interior` were rewritten
  from the disassembly rather than moved.

## Class unification (aliases the slices used for one object)

| here | slice names | evidence |
|---|---|---|
| `CellGrid` (0x344 bytes, ctor 0x478ad0, `rgl::world`-like scene container) | NavStore, Scene, Ctx, SceneW, SunCtx, LightCtx, Level | one `this` in 0x470c00, 0x4711e0, 0x478a10..0x479980 |
| `CellGridBase` / `Cell` / free list `PList` | RecSetBase, RecSet / RecP / PList | dtors 0x475c60, 0x475d50 |
| `Mesh` (polygon-mesh topology, `NavCore c` at +8) | Mesh, NavMap | 0x4705e0..0x470a00, 0x4785e0 |
| `MVert` (0x18) / `MEdge` (0x34) / `MPoly` (0x64) | Vtx18, NavVert / Edge, NavEdge / Face, NavFace | element sizes of the vector instances 0x476620, 0x476bb0, 0x4768f0 |
| `MRow36` (was `E36`), `E36x`, `E136`, `MRow56` (was `E56`) | E24/R24, rec36, rec136, R56 | sizes in the vector<T> instances |
| `Rec1c8` (0x1c8, vtable 0x590b84) | Agent, PropRec, PlacedObj in other slices | copy ctor 0x437bb0 |
| `MAgent` (0x5928, vtable 0x591848, `ViewBody` at +0x258, size 0x5374) | RecZ, AgentReset, "Agent" | ctor 0x477a30, `reset` 0x472f90 |
| `Pool<MAgent>` (`AgentTable`) | AgentTable | |
| `TroopMgr::apply_group` 0x470db0 | SkinOwner::build_map | its dst is an `ArrSet` |
| `GameData::blend` 0x471040 | Anchors::blend | |
| `Terrain` (the ~138 MB object) vs `Mission` | both called Mission | `make_grid`, `light_particle`, `generate_heights` use +0xc3b0dc as `CellGrid*` on a different `this` |

Renames made to keep audit names unique (the audit and equiv bind callees/vtables by mangled name, and other slices
reuse `E36`, `E56`, `Agent` for different objects): `E36` -> `MRow36`, `E56` -> `MRow56`, `Agent` -> `MAgent`.
`??_7MAgent@@6B@` is mapped to 0x591848 in `symbols/slices/terrain_b.txt`; the other slices use `??_7Agent@@6B@` for the
0x590b84 vtable and equiv gave a false FAIL on the `MAgent` constructor until the rename.

## Headers (one per class, offsets cite the proving function)

terrain_b.h (umbrella, vector helpers, globals), render_batch.h, frustum.h, poly_mesh.h, cell_grid.h, terrain.h,
records.h, game_data.h, face.h, troop_mgr.h, shadow_map.h, pool.h, agent_rec.h, mission.h, physics.h, missile.h,
face_gen.h, emit.h, scene_file.h, tmesh.h, entity_rec.h. Layout asserts (`static_assert`-style size/offset checks) are in
agent_rec.h (`ViewState` is 0x5374) and mission.h; each field comment names the function that reads or writes it.
Each cpp includes the headers it uses directly (`#include "./x.h"`): the resolution audit scans `// 0x...` annotations
only in headers the cpp includes itself, not transitively.

### Shared-header conflicts (not edited; worked around locally)

- `src/rgl/resource.h` and `src/rgl/shapes/shapes.h` both define a global `shape_base`, so they cannot be included
  together. `face` and `shape_calls` are declared stand-alone in `face.h`.
- `src/game/renderer.h` and `src/rgl/scene/*.h` hold different partial views of renderer / frustum / scene / material;
  `render_batch.h` carries the view this region needs.
- Other agents' headers describe Mission / Agent / Troop / PhysSet partially and differently; `mission.h` and
  `agent_rec.h` are the full view for 0x46e590..0x47aa40.
- `src/rgl/xmlcfg/records.h` has different `IfaceB`, `RecJ`, `RecZ`.
- `src/rgl/mesh.h`: the `texture` field named `owns_texture` (+0x28) is a frame count; the class lacks `get50_`, `get90_`,
  `flip_faces`, `clone`, so derived views are used (`mesh_calls`, `mesh_flip`, `mesh_clone`); `resource_manager` lacks
  `find_meshes` (view `resmgr_x`).

## Tricks kept, tricks removed

Kept (all needed for a byte-exact row, documented at the use):
- **Opacity casts** (`((MeshExt*)this)->build_adjacency()`, `TroopMgrExt`, `TerrainExt`): a function defined earlier in the
  TU lets cl assume its register use or inline it; the original compiled some callees as opaque. These "Ext" view
  classes keep the call out of line.
- `__declspec(noinline)` on helper wrappers whose original is a separate function (the allocator `construct`, the
  `uninit_fill_n_*` helpers).
- The `vecNN::ufill` stubs (`vector<T>::_Ufill` wrappers) and the hand-written `destroy_arrsets` using `~ArrSet`:
  the real STL instantiation of `_Destroy_range<ArrSet>` did not reproduce 0x4727a0.
- One `volatile float` spill (terrain_b.cpp, in the pick_spawn area) where the original goes through a stack slot.
- Anonymous-namespace `MissileG` (the `?A0x101b409b` scope in the mangled name of `ammo_scale` 0x472650).

Removed: no `/EHa` (rows that the slices built with it match without), no per-row flags, no inline asm, no
slice-local copies of the shared types.
The WinLicense "WL" markers inside the original (two inside one function, noted by comments in terrain_b.cpp) are not reproduced.

## Verification (what was run)

1. `tools/matching/chk.py` on both modules: the 78 + 10 exact rows MATCH; the other rows show the diff sizes in the
   nonmatching/partial files.
2. `run_all.py --manifest`: 78 / 78 and 10 / 10 byte-exact.
3. `resaudit_all.py --slices src:...terrain_b.manifest.txt,src:...mission.manifest.txt`: 88 rows, **87 agree**,
   1 unverified (0x476e60 `vector<ArrSet>::_Insert_n`: the only unpaired reference is the `~ArrSet` call inside an
   unwind funclet; the row is byte-exact), 0 disagree. 33 "local copy" lines are informational (inline
   `std` template instances our object defines where the original calls its own copy; they are byte-identical).
   The std instantiations that cannot be bound by annotation (the audit's annotations skip function templates because
   the scope of a mangled template function does not include `std`) are bound in `symbols/slices/terrain_b.txt`,
   together with the globals above the audit's 0x1900000 annotation limit (`g_emit_id`, `g_fog_id_*`) and a few
   exact-name callees (`unpack_color`, `mi_ops::setup_material`, `g_resmgr`).
   Fixed in this run: a scope-less `operator=` annotation on the `vector<Rec1c8>` instantiation captured every
   `operator=` callee (`tstring`, `vector<int>`); it is now a plain comment.
4. `equiv.py` on every nonmatching row: 23 + 9 + 5 + 4. No row left as a FAIL except the four rows listed in
   `mission.partial.txt` (reset: an uninitialised-stack read in the original; setup_ambient, rebuild_nav, make_grid:
   cause not found). PASS 10, WEAK 16 (no mismatch, some edges unreachable in both builds), UNSUPPORTED 2
   (`render_scene`: the audit disagrees on two operands, as in the slice; `build_interior`: 0 valid inputs because the
   harness hits a pure-virtual call).
5. `tools/status.py --no-write` run at the end: the tree total did not change on the byte-exact tier except +1
   (0x479930).

## Known problems and open work

- `render_scene` (0x46e590) is 7965 bytes in the original, 7926 here; 6512 differing bytes, frame layout and scheduling.
- `post_spawn`, `make_grid`, `build_interior`, `setup_ambient`, `rebuild_nav`, `build_faces`, `Mission::reset`, `CellGrid::insert`/`build`
  are complete in logic but differ in size; the four partial rows have an unexplained equiv mismatch.
- 0x478830 as a TU boundary is class evidence only.
- The `E36` / `E56` / `Agent` names still exist in other slices' headers for different objects; they cannot be
  co-included with these headers.

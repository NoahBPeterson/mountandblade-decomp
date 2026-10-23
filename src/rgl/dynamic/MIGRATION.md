# rgl/dynamic migration (0x421df0-0x427d80)

The staging slices `s00421aa0` (rows from 0x421df0), `s00422130`, `s00422db0`, `s00423540`, `s00423ed0`, `s00424830`,
`s004251c0`, `s004259a0`, `s004260a0`, `s00426a30`, `s00427000` and `s00427900` (rows below 0x427d80; its `world::step` 0x427f40
belongs to the next module) are merged into three modules. `match/slices/` is unchanged. Flags are the defaults
`/O2 /MT /Gy /TP` everywhere.

## Summary

| module | range | rows | byte-exact before (slices) | byte-exact after | improved | regressed | nonmatching | partial |
|---|---|---|---|---|---|---|---|---|
| fx_anim.cpp | 0x421df0..0x423c00 | 24 | 15 | 15 | 0 | 0 | 9 | 0 |
| world_grid.cpp | 0x423c00..0x425370 | 34 | 22 | 22 | 0 | 0 | 12 (6 functions + 6 symbol-less catch/COMDAT tails) | 0 |
| dynamic_entity.cpp | 0x425370..0x427d80 | 42 | 31 | 31 | 0 | 0 | 11 (8 functions + 3 catch/COMDAT tails) | 0 |
| **total** | | **100** | **68** | **68** | **0** | **0** | **32** | **0** |

Every VA of 0x421df0..0x427d20 that has a function in `work/index/functions.csv` is in exactly one of the three
`manifest` / `nonmatching` lists. `run_all.py --manifest` on each module: 15/15, 22/22, 31/31 (no regression).
The nonmatching rows were re-measured with scratch manifests (`work/migrate/scratch_rgl_dynamic_nm_*.manifest.txt`); the
differing-byte counts in the `*.nonmatching.txt` files are the new ones. Two spellings improved verifiability, not bytes:
- `FxDef::update` 0x421df0: `if (p64) update_poses(); else update_poses2();` (was `if (!p64) ...`). The old spelling had
  1 differing byte (branch polarity) but emitted the two calls in the opposite order, so the resolution audit saw
  `update_poses`/`update_poses2` disagree and equiv was UNSUPPORTED. Now 4 bytes differ and equiv runs (WEAK, no mismatch).
- `FxAnim::value` 0x423540: the `g_use_slerp` test is `!= 0 -> slerp, else nlerp` (same audit problem with
  slerp/nlerp, 808 -> 815 bytes differing); equiv is now PASS (320 valid inputs, 14/14 edges).
- `dynamic_entity::resolve` 0x4259a0: the second force loop forms the three `d * n` products in a temporary vector
  before updating `force`, as the original does (z product kept in a stack slot). Before, equiv FAILed on 1 input in 300
  (a contact pointing into the entity itself made the update order visible as -0.0 vs +0.0); no FAIL over 6 seeds now.

Resolution audit (`work/migrate/scratch_rgl_dynamic_audit.jsonl`): 68 rows, 68 agree, 0 disagree, 0 unverified.
Equivalence of the nonmatching rows (`equiv.py <module> <va> --src ... --flags "/O2 /MT /Gy /TP"`), no FAIL:
- fx_anim: PASS 0x422eb0, 0x422f60, 0x423390, 0x423440, 0x423540; WEAK (no mismatch) 0x421df0, 0x422130 (91% insns), 0x423a50;
  UNSUPPORTED 0x423b60 (`chain`: virtual/indirect state, every random input faults).
- world_grid: PASS 0x424220, 0x424650, 0x424cc0; WEAK 0x4240d0; UNSUPPORTED 0x423c80 and 0x4252c0 (every input faults writing
  the grid; the cell vectors need a real heap model); the six catch(...) tails are not callable functions.
- dynamic_entity: PASS 0x4257e0, 0x426500; WEAK 0x4259a0, 0x426750, 0x426a30, 0x427000, 0x427900; UNSUPPORTED 0x4260a0
  (`integrate`: fake-arity, the virtual calls inside defeat the harness); the three catch(...) tails are not callable.
The 0-byte rows are the symbol-less catch(...) funclets / cold tails: identical to the original by `cmpobj.py --len` against the
parent's COMDAT (recorded by the slices; the parent templates are byte-exact in the manifests).

## Boundaries

| edge | evidence |
|---|---|
| 0x421df0 start | hard: `StrategicEntity::update` 0x421b70 (s00421aa0, strategic entity TU) calls `FxDef::update` 0x421df0 (call at 0x421cdc) and `world::add_item` 0x423c00 (0x421d9c) forward, so its TU ends before 0x421df0 |
| 0x423c00 fx_anim -> world_grid | class change only (FxDef/quaternion/key tables -> the world's grid); the only forward call that reaches it is the one above. Kept separate: world.h/scene.h are needed on one side only |
| 0x425370 world_grid -> dynamic_entity | class change only (ground geometry vectors -> rigid body); no call crosses it. `dynamic_entity::dynamic_entity` 0x425370 has the vtable 0x54b668.. |
| 0x427d80 end | class change (`skeleton_entity` 0x427d80, vtables 0x54b730..); no function in 0x421df0..0x427d80 calls forward past it (scan of every call/jmp in the range) |
| module cuts vs `work/migrate/tu_map.csv` | the map splits the same ranges as three TUs (fx_def 0x421df0, curve 0x422130, dynamic_entity 0x425370); 0x422130 (the pose pass `update_poses2`, curve, quaternion) is the same object as FxDef::update: `update` calls `update_poses` 0x422080 and `update_poses2` 0x422130 and `FxAnim` is a view of FxDef |

Findings for the TU map (not applied elsewhere):
- 0x421df0..0x423c00 is one TU, not the map's fx_def + curve.

## Headers

New: `fx_anim.h` (FxAnim, AnimRec, Bone, KeyTable, quaternion, with their ext twins), `world_grid.h` (world_grid, grid_cell,
shape_vec, ground_store, vec36/vec136), `dynamic_entity.h` (rigid_base, dynamic_entity, Contact, world_q, cell_q, contact vectors),
`grid_shape.h` (the collision shape as the grid sees it). Existing headers used: `../rgl_math.h`, `../resource.h`,
`../scene/{scene_math,frustum,scene,emitter,scene_entity,world,fx}.h`. Nothing outside this directory was edited. Layouts cite
their proving addresses in the headers. The slices' names map as: Animator/Curve -> FxAnim, World -> world_grid, Store ->
ground_store, Cell -> grid_cell, rigid_base is the slice name kept (vtables via `symbols/slices/dynamic_entity.txt`,
`fx_anim.txt`).

Header conflicts and gaps (worked around locally, to be absorbed upstream):
1. `shapes/shapes.h` and `resource.h` both define `shape_base` (scene.h pulls resource.h), so the grid cannot include shapes.h:
   `grid_shape.h` re-states the vtable slots 5..13 and the two fields the grid reads as views of resource.h's `shape_base`.
2. `world.h` declares `rgl::world` / `rgl::cell` / `rgl::ground_geom` without the grid members; views `world_grid`, `grid_cell`,
   `ground_store`, `world_q`, `cell_q`, `entity_grid` add the member functions (no new fields).
3. `scene_entity.h` has a placeholder `rgl::dynamic_entity` (padding only); it is renamed while that header is included and the
   full class is defined in `dynamic_entity.h`.
4. `fx.h`: `FxElem` has quaternion fields as raw floats (views `Bone`), `EnvRec` = our `AnimRec`.
5. `rgl_math.h` declares `vector4` operators out of line; the effect code inlines them (local `vmul/vadd/vsub/vnormalize` in fx_anim.cpp).
6. The audit binds callers' by-value/out-parameter views through ext twins (`quaternion_x`, `FxAnim_x`, `shape_vec_x`, ...)
   bound to the real function with `/alternatename` (see `symbols/slices/fx_anim.txt`).

## Tricks kept
- Operand orders and temporaries written to reproduce the original's x87 sequences (`mul`/`mul_conj` explicit left-associated
  sums, `vscale` temporaries in `resolve`).
- Explicit VC7.1 `std::vector` layouts (`shape_vec`, `contact_vec`, `vec36/136`) so the member functions stay out of line.
- Every nonmatching row has equal control flow and all calls; remaining diffs are register allocation, commutative operand order
  and instruction scheduling (details per row in `<module>.nonmatching.txt`).

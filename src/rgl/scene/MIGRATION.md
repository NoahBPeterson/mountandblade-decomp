# src/rgl/scene migration (world, entities, effect definitions, render scene)

The regions 0x41ef30..0x41f1d0, 0x41fee0..0x421df0 and 0x427d80..0x42a090 of the TU map (TUs 23, 25-28, 32, 33
of docs/tu_map.md: "world", "entity_2", "aabb_2", "mesh_entity", "strategic_entity", "skeleton_entity",
"fx_def_2") are moved from the staging slices under `match/slices/` into 14 translation units, one `.cpp` each,
following the method of src/dxframework/MIGRATION.md, src/game/MIGRATION.md and src/rgl/MIGRATION.md. The
slices are unchanged and stay the reference. `src/game/entity.cpp` (0x41f1d0..0x41fee0) is not part of this work;
0x421df0..0x427d80 (curves, world update, dynamic entities) belongs to another region.

All modules build with the default flags `/O2 /MT /Gy /TP` (no row needs another flag; the one slice row that was
built with `/EHa`, in s0041f720, is entity.cpp's and not ours). Real headers only (`<vector>`, `<list>`,
`<math.h>`, `../rgl_math.h`, `../rgl_string.h`, `../stream.h`, `../resource.h`, `../mesh.h`); `match/include/rgl.h`
is not included (see "Header conflicts").

## Summary

| module | range | functions | byte-exact before (slices) | byte-exact after | improved | regressed | nonmatching | partial |
|---|---|---|---|---|---|---|---|---|
| world_items.cpp | 0x41ef30..0x41f1d0 | 5 | 5 | 5 | 0 | 0 | 0 | 0 |
| entity_recalc.cpp | 0x41fee0..0x4201b0 | 3 | 2 | 2 | 0 | 0 | 1 | 0 |
| entity_render.cpp | 0x4201b0..0x4205b0 | 2 | 1 | 1 | 0 | 0 | 1 | 0 |
| scene_objects.cpp | 0x4205b0..0x4218e0 | 27 | 23 | 23 | 0 | 0 | 4 | 0 |
| fxdef_set_color.cpp | 0x4218e0..0x421990 | 1 | 1 | 1 | 0 | 0 | 0 | 0 |
| mesh_list_color.cpp | 0x421990..0x4219c0 | 1 | 1 | 1 | 0 | 0 | 0 | 0 |
| strategic_emitters.cpp | 0x4219c0..0x421df0 | 5 | 4 | 4 | 0 | 0 | 1 | 0 |
| entity_types.cpp | 0x427d80..0x4285e0 | 16 | 15 | 15 | 0 | 0 | 1 | 0 |
| shape_load.cpp | 0x4285e0..0x428820 | 3 | 2 | 2 | 0 | 0 | 1 | 0 |
| fxdef_init.cpp | 0x428820..0x428b10 | 2 | 2 | 2 | 0 | 0 | 0 | 0 |
| fx_support.cpp | 0x428b10..0x428ff0 | 6 | 6 | 6 | 0 | 0 | 0 | 0 |
| fxdef_render.cpp | 0x428ff0..0x4291c0 | 1 | 1 | 1 | 0 | 0 | 0 | 0 |
| fxdef_skin.cpp | 0x4291c0..0x429670 | 1 | 0 | 0 | 0 | 0 | 1 | 0 |
| render_scene.cpp | 0x429670..0x42a090 | 5 | 3 | 3 | 0 | 0 | 2 | 0 |
| **total** | | **78** | **66** | **66** | **0** | **0** | **12** | **0** |

Function counts are manifest/nonmatching rows (one per address). The TU map's counts for the ranges (6 + 3 + 2 + 30 + 4 +
6 + 28 + the 0x4218e0 / 0x421990 singletons) are reproduced after the splits below. 0x4214e0, 0x427e80, 0x4280f0 and
0x428220 are compiler-generated scalar deleting destructors (`??_G...`): their rows are in the manifests under
their real mangled names (the size pairing of chk.py names them after the wrong class).

No `partial.txt` was needed. Improved: none, every function that was byte-exact in its slice is byte-exact in its module
(no regressions); the 12 nonmatching rows are the slices' 12 nonmatching rows, re-measured (same or smaller byte
counts, reasons in `<module>.nonmatching.txt`).

## Boundaries

The forward-call rule is applied as the task states it (cl 13.10 emits a TU callees-first; a forward call proves a
boundary in (caller, callee]; the `jmp +0x10` over a WinLicense marker is not a call). `work/migrate/facts.json` was
re-read for calls and tail jumps separately. Hard boundaries inside the three ranges, each the *only* gap between the two
functions named (the caller is immediately followed by the boundary address):

| boundary | evidence | kind |
|---|---|---|
| 0x41fee0 | entity.cpp's `set_local_scaled` 0x41fe90 calls `smat::scale_rows` 0x41fee0 | forward call (high, already used by entity.cpp) |
| 0x4201b0 | `entity::recalc` 0x41ffe0 calls `extend_box` 0x4201b0; recalc is the only function between | forward call (high) |
| 0x4205b0 | `entity::render` 0x420310 calls `lod_mesh2::select` 0x4205b0 | forward call (high) |
| 0x4218e0 | `strategic_entity::set_color` 0x4218a0 tail-jumps to `FxDef::set_color` 0x4218e0 | forward tail jump (high) |
| 0x421990 | `FxDef::set_color` 0x4218e0 calls `mesh_list2::set_color` 0x421990 | forward call (high) |
| 0x4219c0 | `FxElem::set_color` 0x4219c0 calls 0x421990 *out of line*; with the body visible in the same TU cl inlines it (153 differing bytes) | callee body invisible (medium) |
| 0x421df0 | `strategic_entity::update` 0x421b70 calls `FxDef::update` 0x421df0 and the world's `add_item` 0x423c00 | forward call (high), also the end of the assigned region |
| 0x4285e0 | `entity::load` 0x4284e0 calls `shape_resource::clone_shape` 0x4285e0 | forward call (high) |
| 0x428820 | `skeleton_entity::load` 0x4286e0 calls the `FxDef` constructor 0x428820 and `init` 0x4288c0 | forward call (high) |
| 0x428b10 | `FxDef::init` 0x4288c0 calls `quat_from_matrix` 0x428b10, `EnvRec::alloc` 0x428c60, `FxElem::FxElem` 0x428d10 | forward call (high) |
| 0x428ff0 | `skeleton_entity::render_fx` 0x428f80 calls `FxDef::render` 0x428ff0 | forward call (high) |
| 0x4291c0 | `FxDef::render` 0x428ff0 calls `FxDef::skin` 0x4291c0 and `skin_group` 0x4296f0 | forward call (high) |
| 0x429670 | `FxDef::skin` 0x4291c0 calls `mat4::rot_t` 0x429670 and `skin_iter::operator+` 0x4296d0 | forward call (high) |

Changes against the TU map:
- **TU map 25-28 are cut at 0x4201b0, 0x4205b0, 0x4218e0, 0x421990 and 0x4219c0**, not at the map's 0x4201b0 / 0x4205b0 /
  0x421a50 / 0x421df0 only. The map's "mesh_entity" (0x4205b0..0x421a50, 30 functions, "5 possible inner splits") contains
  the forward tail jump 0x4218a0 -> 0x4218e0 and the forward call 0x4218e0 -> 0x421990; both were missed because the
  map's inner splits are "free gaps", not call evidence. Result: `scene_objects` keeps 0x4205b0..0x4218e0 (27 functions,
  no forward call inside it), and `FxDef::set_color`, `mesh_list2::set_color` get objects of their own (one function each).
  The TU map's "strategic_entity" (0x421a50..0x421df0) now starts at 0x4219c0 (`FxElem::set_color`, class FxElem, was
  inside the map's TU 27/28 gap). These singletons are the ones most likely to be wrong in the other direction: an
  object that holds only a 164-byte method or a 38-byte loop is odd, and may be a header-inline function that the
  linker placed with its first user (COMDAT folding); the forward call is the only evidence for a split.
- **TU 32 start (0x427d80)** is the TU map's (class change collision_entity | skeleton_entity, low). It is only
  weakly supported: `~skeleton_entity` (0x427e10) inlines the body of `collision_entity::~collision_entity` (0x427cd0,
  34 bytes, in the previous object by address), which suggests that destructor is an inline member defined in the
  class (the module defines it inline in `scene_entity.h`, which makes the bytes match) rather than a function of the
  previous object. 0x427ca0..0x427d80 (collision_entity members) therefore may belong to this object.
- **TU 33 is split at 0x4285e0, 0x428820, 0x428b10, 0x428ff0, 0x4291c0, 0x429670** (all forward calls). The map had one
  object (0x427ea0..0x42a090) with "1 possible inner split"; it contains six forward calls (table above). Its start
  0x427ea0 (`world::alloc_emitter`) is merged with the skeleton entity members into `entity_types` (no call evidence
  separates them; both are small and the vtable 0x54b730 slots are the neighbours').
- **End 0x42a090** is the TU map's (class change world | strategic_entity::render). `world::render_scene` 0x429890 and
  `scene::scene` 0x429740 have no forward call tying them to the helpers before them, and none away from them, so they stay
  in `render_scene` (the object also holds `mat4::rot_t`, `skin_iter::operator+`, `FxDef::skin_group`, which by
  address precede them).
- **No WinLicense marker** (`EB 10 57 4C ...`) occurs in these ranges; no boundary was discarded for that reason.
- The TU map's TU 23 start "fwd-call 41ca60->41f970" is a call into `vector<mesh*>::resize`, i.e. into the previous
  object's range, not a boundary at 0x41ef30; the module keeps the TU map's start and the end where entity.cpp begins.

## Names

Class names in the slices were inconsistent (one class had up to five names). The canonical choices, with the evidence:

| slices | module |
|---|---|
| `world`, `World`, `objgrp`, `Ctx`, `grid`, `mesh_pool` (../psys.h), `world_b` | `rgl::world` (owner at entity+4 and at scene+0x1c4; ents at +0x7c, cell table at +0x28, emitters at +0x32c, meshes at +0xc8) |
| `Emitter`, `settings`, `item_t`, `Rec`, `Psys` (as a list element) | `rgl::psys` (the 0x1bc-byte particle emitter; `settings::init` 0x420b40 is `psys::init`; `rgl::psys` of ../psys.h is the same class seen through two functions) |
| `Psys`, `Stash`, `PsysGroup`, `PsysList`, `PsysListBody` | `rgl::mesh` (vtable 0x54a6f4), `rgl::scene` (Stash: ~scene 0x41f4e0), `rgl::lod_mesh2`, `rgl::mesh_list2` (`lists[i].set_color`) |
| `MeshEntity`, `MeshEnt`, `Ent`, `Ent3`, `Child`, `StratEnt`, `StrategicEntity`, `CollisionEntity`, `Stub` | `rgl::entity` (the base, vtable 0x54b460), `rgl::mesh_entity`, `rgl::collision_entity` (Stub = its three default slots 25..27), `rgl::strategic_entity`, `rgl::shape` view `coll_shape` (entity+0x98), `item_obj` (entity.items) |
| `NodeOwner`, `Slot`, `slotarr`, `owner_t` | `FxElem` (add_to_first = slot 0 of vtable 0x54b454, set_data, find_kid) and `FxDef::clear_defs` |
| `Pool<CellNode>`, `Pool<CellRef>`, `Cell12/16` | `rgl::pool<cellnode>`, `rgl::pool<cellref>` |
| `Plane`, `Frustum`, `View` | `rgl::plane`, `rgl::frustum`, `rgl::scene` |

Corrected structure facts (each verified against the binary): the vtable at 0x54b454 holds three virtual methods and
no destructor (`FxElem::~FxElem` 0x41f1d0 is not virtual; ../../game/entity.h declares `virtual ~FxElem()` as slot 0);
entity slot 7 is `update(float)` (the strategic and skeleton overrides, `world::step`), not `commit(int)`; slot 15 returns
`bool` (the `test al,al` in `strategic_entity::update`; it made that function 1 byte off until fixed);
`entity+0x98` is the collision shape (slots 8, 10, 11 are set_frame / the two extents), not a frame source;
`world+0xa0` is a `vec<int>` of ids to remove (cleared with a freeing `clear()`), not a pointer vector; slot 28 of the
skeleton entity is its fx draw.

## Headers (one per class, offsets cite a function)

| header | classes |
|---|---|
| `scene_math.h` | `mat3`, `mat4` (derived from `rgl::matrix`: `transform_point` 0x408460, `rot3` 0x40a920, `rot_t` 0x429670), `smat` (`scale_rows` 0x41fee0), `identity3` 0x40cd60, `identity_pos` 0x40cd00 |
| `frustum.h` | `plane`, `frustum` (0x120 bytes; ctor 0x41ef60) |
| `emitter.h` | `rgl::psys` (0x1bc bytes, `init` 0x420b40, both `update` overloads), `E100` (its 0x64-byte particle) |
| `scene.h` | `rgl::scene` (0x1d8 bytes), `render_info`, `mesh_ext` (methods of the mesh object the scene code calls), `lod_mesh2` (0xe0), `mesh_list2`, `vec<T>`, `resmgr_ext`, the scene registry |
| `scene_entity.h` | `entity` (0x15c, vtable 0x54b460), `mesh_entity`, `collision_entity`, `skeleton_entity`, `static_entity`, `strategic_entity` (0x320), `dynamic_entity` (stub for the factory), `coll_shape`, `box_ext` |
| `fx.h` | `FxElem` (0x1dc), `FxDef` (0x7c), `EnvRec` (0xd8), `quat4`, `SkinVert`, `skin_iter`, `FxRes`/`FxRec` (loaded skeleton) |
| `world.h` | `rgl::world`, `pool<T>`, `cell`, `cellnode`, `cellref` |
| `shape_clone.h` | `shape_x` (slots 5..14 of the collision shape), `sphere_shape`, `capsule_shape`, `face_shape`, `composite`, `manifold`, `shape_resource` (all sizes read from the `operator new` calls of 0x4285e0) |

FxElem and FxDef are global classes (like src/game/entity.cpp's FxElem, so the vtable `??_7FxElem@@6B@` and the
members entity.cpp defines resolve unchanged); everything else is in namespace `rgl`. Every module includes all
scene headers directly (`./x.h`): equiv.py reads the `// 0x...` annotations only from headers the `.cpp` includes by
relative path.

## Header conflicts with other src/ headers (nothing outside this directory was edited)

- `../../game/entity.h` (src/game/entity.cpp): defines `rgl::entity`, `rgl::mesh`, `rgl::scene`, `rgl::aabb`, global
  `world`, `FxElem`, `pool<T>`, `vec<T>`, `E100`. Conflicts: (1) `rgl::mesh` there is a 5-field stub without a
  vptr (the real object is ../mesh.h's) and `rgl::scene` a 7-field stub; (2) its `entity` has slot 7 `commit(int)`,
  slot 15 `int is_dirty()`, `frame_src* parent` at 0x98 and no slot 24+; (3) `FxElem` has `virtual ~FxElem()` as slot 0
  (wrong, see above) and `psys`/`groups` element types of its own; (4) global `world` is a different class from
  `rgl::world`. `scene_entity.h` / `fx.h` / `world.h` supersede it with the same layout and the corrections above;
  they cannot be included together with it. The two should be merged when entity.cpp is revisited; entity.cpp's
  `FxElem` members keep working against `fx.h`'s class (same offsets).
- `../mesh.h`: its `scene` is a partial stub (pad + 4 fields); `scene.h` includes the header with `scene` renamed to
  `mesh_h_scene` by the preprocessor and defines the full class, so `mesh::render` is reached through the
  `mesh_render()` cast helper. Methods it lacks and `mesh_ext` adds (no fields): `get70_` 0x40eba0, `get60_` 0x40ec60,
  `ensure_skin` 0x412700, `update_emitters` 0x4126a0, `clear_geometry` 0x428e80, `clone` 0x41c6d0, and the bone fields
  (+0x220 matrices, +0x224 source, +0x228 count, ints `f220/f224/f228` there). `mesh.h`'s `aabb` has no
  `extend_box` 0x4201b0 (`box_ext`). `mesh_pool` (forward declared there, defined in ../psys.h) is `rgl::world`.
- `../psys.h`: its `psys` is the same class with only a few fields (no `init`, no `update(dt, frame)`); `emitter.h` has
  the complete 0x1bc-byte layout and both `update` overloads. Defining both in one program is an ODR violation: replace
  psys.h's class by emitter.h's. Its `particle` (0x64) is `E100` here (the name the neighbouring objects registered
  for the `std::vector<E100>` instances).
- `../resource.h`: `resource_manager` lacks `release` 0x4175f0, `get_object` 0x419540, `get_render_target` 0x41b7d0
  (`resmgr_ext`, a cast of `g_resmgr`); `shape_base` stops at slot 4 (`shape_x` adds 5..14); `get_object60` returns
  `named_object*` (cast to `shape_resource`).
- `../../game/mat4.h` / `tstring.h` are not used: they include `match/include/rgl.h`, whose `rgl::vector4` and
  `rgl::string` clash with `../rgl_math.h` / `../rgl_string.h` (the src/rgl family). `mat4` is therefore a
  one-line extension of `rgl::matrix` (`scene_math.h`), not `game/mat4.h`'s struct.
- `../rgl_string.h`: used for `rgl::string`; `mb_error` 0x42cc80 comes from there.

## Removed slice-local tricks

- Per-slice class copies (`entity.h` x4, `mesh.h` x6, `world.h` x3, `fx.h`, `psys.h`): one header per class (above).
- `-DOPAQUE_SCALE_ROWS` (s0041f720): `scale_rows` is in this directory's `entity_recalc` and its caller 0x41fe90 in
  entity.cpp sees only the declaration.
- Stand-in classes `Stub` (get_vec / nop / zero with `(float*, int)`), `PsysListBody` (a copy of `PsysList::set_color`
  under another name), `Stash`, `MeshEnt`/`MeshEntity` twins, `Psys : mesh`, `PtrVec`-as-vector copies: real members
  (`collision_entity::point_velocity` ... , `mesh_list2::set_color`, `~scene`, `mesh`, `ptr_list`).
- `ptr_vec<T>` (hand-written pointer vector) in s00427900 and `vec<T>`-typed `Obj*` lists: `std::vector<entity*>`,
  `std::vector<psys*>` and the game's `vec<int>` for the removal list. Their out-of-line instances are the ICF-folded
  pointer-vector code of other objects (0x41f720, 0x420650, 0x41f970, 0x41fa00, 0x41ad20), mapped in
  `symbols/slices/scene.txt` (see Verification).
- `(int)this` for the entity owner (`o->owner = (int)this`): `world*`.
- `bool wants_items()` / `Ent3::notify(View*, bool*)` stand-ins: the real slots (`is_dirty`, `render(scene*, render_info*)`
  with a `render_info` temporary instead of a 8-byte `Flag` struct).
- Type-punned field names (`f19c`, `f1a8`, `f1ac`, `v32c`, `f33c`, `vb8`, `vc8`, ...): named after the function that reads
  them (`spawn_left`, `tag`, `radius`, `emitters`, `live`, `lights`, `meshes`).

Kept, with reason:
- Declaration-only callees where the original calls an out-of-line function that cl would otherwise inline from the
  same file (`mesh_list2::set_color` for `FxElem::set_color`, `mat4::rot_t` for `FxDef::skin`, `shape_resource::clone_shape`
  for `entity::load`, the `FxDef` constructor and `init` for `skeleton_entity::load`): each is a hard boundary above.
- `mesh_ptr_vec` (fx.h): `FxElem::psys` is a `std::vector<mesh*>` whose `push_back` is the mesh object's instance
  (0x4119d0), reached by `skeleton_entity::load`; `particle_mesh : mesh` (shape_load.cpp) reproduces the pointer
  temporary the original builds for that call (the slice's `Psys : mesh`).
- `*(mat3*)&B[k] = ...rot3(...)` and the `(base + i).cur` / `skin_iter` out-of-line `operator+` in `FxDef::skin`; the
  `vscale` helper there is the original's inline `vector4(s*v.x, ...)`.
- `rawvec` + `remove_from` in `world::remove_entity` (the original's inlined erase loop through raw vector fields)
  and the explicit `std::remove` instantiation 0x41f080.
- `p->mf4.o` / `p->radius` casts replaced by fields; `(unsigned)live` in `alloc_emitter` is needed because the
  field is a signed int (partition_items compares it with a signed index) and the comparison with `size()` must stay
  unsigned.
- `*(const mat4&)m_cc` casts in `entity::render`: `mesh::m_cc` is a `matrix`, `transform_point` is on `mat4`.

## Verification

- `chk.py` on every address of every module, then `run_all.py --manifest src/rgl/scene/<m>.manifest.txt`: all rows
  byte-exact (13 manifests with rows, 66 rows; `fxdef_skin.manifest.txt` is empty because its only function is
  nonmatching).
- Resolution audit (`resaudit_all.py --slices src:...`, log `work/migrate/scratch_scene_audit.jsonl`): see
  "Audit and equivalence results" below.
- Every nonmatching row: `equiv.py <module> <va> --src src/rgl/scene/<module>.cpp --flags "/O2 /MT /Gy /TP"`
  (log `work/migrate/scratch_scene_equiv.log`).

### Audit and equivalence results

Resolution audit over the 14 manifests (66 rows): **65 agree, 1 unverified, 0 disagree**; one local copy remains
(0x420b40 `vector<E100>::resize`, the original calls 0x41fb00; the row agrees, and the name is also in `symbols/slices/scene.txt`).
The unverified row is 0x4286d0 `strategic_entity::load` (a 5-byte `jmp` thunk to `entity::load`, below the audit's
3-instruction pairing run; the same row is unverified in its slice). The first audit run found four real
disagreements (the `~entity` calls of 0x427e10, 0x4280f0, 0x428120, 0x428240 resolved to the deleting destructor
0x4214e0 because the declaration's comment named it first; fixed by annotating the body 0x4213f0) and unresolved
references (headers not included directly, ambiguous `pool<T>::link_chunks`), all fixed.

Equivalence (`equiv.py`, flags `/O2 /MT /Gy /TP`, log `work/migrate/scratch_scene_equiv.log`), the 12 nonmatching
rows: **no FAIL**.
- PASS: 0x41fee0 `smat::scale_rows` (400 valid, 100% insns), 0x4201b0 `box_ext::extend_box` (400 valid, all 4+4 edges),
  0x421720 `strategic_entity` ctor (400 valid), 0x429740 `scene::scene` (400 valid).
- WEAK, no mismatch (both builds miss the same number of edges; harness limits):
  0x420ec0 `FxDef::clear_defs` (546 valid, 50/52 edges both: the loops over virtual deleting destructors of random objects),
  0x421110 `world::render_part` (716 valid, 25/28 edges both, 95.7% insns),
  0x4219c0 `FxElem::set_color` (393 valid, 11/12 edges both),
  0x427f40 `world::step` (525 valid, 21/28 (+2 unreachable) edges both; random entities rarely reach the emitter and removal loops),
  0x4285e0 `shape_resource::clone_shape` (158 valid of 500: the random shape's `shape_type()` is a fake-arity virtual call;
  6/18 edges both), 0x4291c0 `FxDef::skin` (589 valid, 15/26 edges both, 36% insns: random pointers fault before the
  vertex loops), 0x429890 `world::render_scene` (609 valid, 38/78 edges both, 43.7% insns; the reflection pass needs a valid
  render target, so it is checked by reading only).
- UNSUPPORTED (harness): 0x421600 `world::insert_obj` (0/400 inputs valid: the virtual calls on the random entity fault, as in the slice).
- `FxDef::skin` first FAILed on a missing symbol line: the `__vec_new` constructor argument of `new mat4[32]` is the
  ICF-folded empty constructor 0x407c20 in the original and our local copy in ours. `symbols/slices/scene.txt` maps
  `??0mat4@rgl@@QAE@XZ` to 0x407c20 (and `mesh::get60` to 0x40ec60, which is also called out of line); it passes after that.

`tools/status.py --no-write` counts the 66 manifest rows as byte-exact and the 12 nonmatching rows as equivalent
(migrated into src/: 1007 functions in total at the end of this run).

## Problems / flags

- `tools/matching/chk.py` pairs the compiler-generated deleting destructors by size, so 0x4214e0, 0x427e80,
  0x4280f0 and 0x428220 (all 30-36 bytes) got the name of whichever `??_G` of the object has that size; the manifests
  were written with the correct names (`??_Gentity`, `??_Gskeleton_entity`, `??_Gstatic_entity`, `??_Gstrategic_entity`) and
  the audit agrees on all four.
- The resolver needs a symbol line for every ICF-folded template instance a module calls through a differently named
  element type (`vector<entity*>::resize` for the shared `vector<Obj*>::resize` 0x41fa00, etc.); they are in
  `symbols/slices/scene.txt` together with the vtables and deleting destructors of the entity family.
- `world::step` (0x427f40): copying `dt` to a local reproduces the original's hoist of the load but adds a stack slot
  (372 vs 371 bytes), so the plain form (98 differing bytes, same as in the slice) is kept.
- Possible hidden single-function TUs: `fxdef_set_color` and `mesh_list_color` (see Boundaries).
- The assigned region excludes 0x421df0..0x427d80; `FxDef::update` (0x421df0), `update_envelopes`, `update_poses`
  and the world's `add_item` (0x423c00) are declared in `fx.h` / `world.h` with their addresses for that region's
  module to define; `dynamic_entity` is declared as a stub (size 0x1d8, constructor 0x425370) for `world::instantiate`.

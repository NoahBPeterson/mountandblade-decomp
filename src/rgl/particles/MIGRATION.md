# rgl migration, part 2: the mesh object and its particle emitters (0x40e940-0x413b5f)

One range of the TU map (TU 12, "psys", 0x40e940..0x413b60, 137 rows) is merged from eleven slices (`s0040e060`
(its row 0x40e940 only), `s0040ea00`, `s0040f300`, `s0040f9a0`, `s00410080`, `s004108b0`, `s00411150`,
`s00411a90`, `s00412150`, `s00412a90`, `s00413320`) into one module, `mesh_obj.cpp`. The slices under
`match/slices/` are unchanged and stay the reference.

**The map's "psys" is the mesh object, not the particle system.** The slices' class `Psys` is `rgl::mesh`
(vtable 0x54a6f4, ctor 0x412a90, dtor 0x4127f0; the rgl part-1 notes already say so), and the class they call
`Emitter` (flags +0xe8, particle vector +0x0c, `update` 0x412150, `spawn` 0x40ec90) is the particle-system
object that `mesh::kids` (+0xb0) holds, the same object as `psys` in `src/rgl/psys.h` (see "Header conflicts").

**Is part of the range the same TU as `src/rgl/psys.cpp`? No.** `mesh::render` (0x413750, here) calls
`psys::update` (0x414960, defined in `psys.cpp`) forward; cl 13.10 emits an object callee-first, so the
object holding `mesh::render` ends below 0x414960 and `psys.cpp` is another object. The class `psys` is not
duplicated: nothing in this module defines it (`mesh_obj.h` has the simulation-side layout under the name
`emitter`, see below). Where exactly the two objects meet (somewhere in 0x413ac0..0x413d30, the map's window) is
not provable: `vector<vertex60>::erase/resize` (0x413b60/0x413bb0) and `mesh::add_vertex` (0x413c50) could be
the tail of this object just as well as the head of `psys.cpp`; they stay in `psys.cpp` (its decision, kept).

All 114 byte-exact rows build with the default flags `/O2 /MT /Gy /TP` (the slices' `/EHa` for the STL
helpers is not needed, as in part 1: cl 13.10 emits `try`/`catch(...)` code without `/EH`). No function of the
range has an unwind frame. Only real headers are used (`<windows.h>`, `<d3d9.h>`, the VC7.1 STL); neither
`match/include/rgl.h` nor `src/game/renderer.h` is included (see "Header conflicts").

## Summary

| module | range | rows | real functions | byte-exact before (slices) | byte-exact after | improved | regressed | nonmatching | partial |
|---|---|---|---|---|---|---|---|---|---|
| mesh_obj.cpp | 0x40e940..0x413b5f | 137 | 119 | 113 | 114 | 1 | 0 | 5 | 0 |

The 137 map rows are 119 functions and 18 symbol-less tails (the `catch(...)` funclets after the `_Uninit_*`
and `_Insert_n` helpers, the vector copy constructor's and `_Uninit_copy<bone_influence>`'s). The slices
listed six of the tails as byte-exact `$L...` rows (an artefact of their `/EHa` builds) and twelve as
0-byte nonmatching rows; here all 18 are 0-byte rows in `mesh_obj.nonmatching.txt`, each verified identical
against the parent function's COMDAT tail in the compiled object (`work/migrate/scratch_particles_funclet.py`:
18/18 identical, 4..40 relocation bytes masked per tail). They are the "nonmatching" part of the count that
`status.py` shows (`src_equiv`), not missing code.

Per slice (rows in the range): s0040e060 1, s0040ea00 25, s0040f300 25 (6 tails), s0040f9a0 25 (4 tails),
s00410080 12 (3 tails), s004108b0 8 (3 tails), s00411150 13, s00411a90 9, s00412150 9, s00412a90 6,
s00413320 4 (of which `vector<vertex60>::erase/resize` are `psys.cpp`'s).

**Improved**
- 0x413320 `mesh::update_morph`: nonmatching (40 bytes, "stack slots: original overlays the dead frame
  index with the loop counter") -> MATCH. With the real `get_kf()` and the one-level expansion written out
  (`local_a1 ? kf : parent->get_kf()`, `get_kf` an out-of-line, non-inlined loop) the frame matches.

**Regressions:** none. Every function that was byte-exact in its slice is byte-exact in `mesh_obj`
(`run_all.py --manifest src/rgl/particles/mesh_obj.manifest.txt`: 114 / 114).

**Closer, still nonmatching** (diff bytes, slice -> module):
0x413750 `mesh::render` 16 -> 8, 0x412fe0 `mesh::append` 561 -> 360 (their remaining diffs below).
0x413d30 (`psys.cpp`) is unchanged.

Nonmatching rows (`mesh_obj.nonmatching.txt`, the 5 real ones; equivalence logs in
`work/migrate/scratch_particles_eq/`):

| va | function | diff | equiv |
|---|---|---|---|
| 0x411d20 | `mesh::build_skin` | 204 | UNSUPPORTED: 0/400 inputs valid (random indices address the 2 MB stack arrays out of range) |
| 0x412150 | `emitter::update` | 24 | UNSUPPORTED: the resolution audit disagrees on `__CIpow` only (see "Problems"); 6 other references agree |
| 0x412c30 | `scene::get_mesh` | 197 | PASS (254 valid, all 18 edges on both builds) |
| 0x412fe0 | `mesh::append` | 360 | WEAK, no mismatch (374 valid, 97.7% insns, 22/24 edges on both builds) |
| 0x413750 | `mesh::render` | 8 | WEAK, no mismatch (341 valid; harness reaches 1/50 edges, faults in nested vectors) |

No partial rows, so no `mesh_obj.partial.txt`.

## Boundaries

| module | start evidence | end evidence |
|---|---|---|
| mesh_obj | class change `pos3` -> `renderer` at 0x40e940 (low); the preceding functions (0x40e3d0..0x40e870: clamp, frand, pack/unpack_color, the three axis rotations, `matrix::operator*`) have callers all over the game, so they form a utility object of their own | `mesh::render` 0x413750 -> `psys::update` 0x414960 **forward** (hard: boundary in (0x413750, 0x414960]); `psys.cpp` starts at 0x413b60 (its decision, window 0x413ac0..0x413d30) |

- **No hard evidence inside the range.** The map's 34 "possible inner splits" are free gaps; every internal
  call goes to a lower address (checked over all 137 rows with `facts.json`): the range is consistent with one
  object compiled callee-first. The three forward calls out of it are the object's exits:
  `emitter::update` 0x412150 -> `noise_gradient` 0x414ae0, `~mesh` 0x4127f0 -> `~named_object` 0x4150f0,
  `mesh::render` -> `psys::update`. Three more forward calls (0x410610 -> 0x41ef30, 0x412040 -> 0x41f970,
  0x411139 -> 0x41f4b0) go to template instances (`vector<int>::_Ucopy`, `vector<scene*>::resize`, `clear`)
  that /OPT:ICF kept from a later object although this object has an identical copy: ICF artefacts, not
  boundaries (the audit shows the same for `fill<idx_weight*>` -> 0x48ad70 and `_Xlen` -> 0x43e2a0). The
  map's forward-call rule is unreliable for template COMDATs for that reason.
- **Why one object, not several** ("renderer", "rnode", "mesh_inst", "emitter", "Psys" are one class family):
  `rnode`/`mesh_inst`/`Psys`/`Scene`/`Obj` in the slices are all `rgl::mesh` or `rgl::scene` (see below); the
  renderer-state setters (0x40e940, 0x40ea00) and `material::get_flags` (0x40eab0), `aabb::aabb` (0x40eaf0)
  are inline functions of other headers that cl emitted here as COMDATs before their first caller in this
  object (`mesh::setup_material` 0x40f3f0, `mesh::mesh` 0x412a90): exactly what a callee-first object that
  includes those headers looks like. The vector instantiations all have mesh data as element types.
- **Overlap with the part-before agent:** `s0040e060` (0x40e060..0x40e940) also lists the row 0x40e940
  (`set_blend_mode`); this module owns it. The module before must end at `matrix::operator*` (0x40e870..0x40e935).

## Shared headers

| header | contents |
|---|---|
| `mesh_obj.h` | **drop-in replacement for `src/rgl/mesh.h`** (see below): `rgl::mesh`, `scene`, `scene_registry`, `mesh_list`, `lod_mesh`, `emitter` (+ `emitter_particle`), `keyframes`, `frame_t`, `aabb`, `slot`, the element types of the mesh's vectors |
| `render_cache.h` | the part of the render-state cache (global 0xa17910) this object uses: `set_blend_mode` 0x40e940, `set_texcoord_gen` 0x40ea00, fields 0x574..0x618 |

Included from `src/rgl/`: `rgl_string.h`, `rgl_math.h`, `stream.h`, `resource.h`, `noise.h`. Every include is
direct and relative (`"../resource.h"`, `"./mesh_obj.h"`).

Offsets of the header, with the function that proves each (addresses in the field comments of `mesh_obj.h`;
the less obvious ones):

| class | offset | proved by |
|---|---|---|
| `mesh` (named_object +0x04 name, +0x14 f14) | parent +0x18, name2 +0x1c, `res` +0x2c (skin*, `->f64` is the use count `~mesh` decrements), flags +0x30, f38 +0x38 (force alpha blend), color +0x3c | 0x4127f0, 0x40f3f0, 0x412980 |
| | `v50` vertex60 +0x50, `v60` skin_vert +0x60, `v70` point20 +0x70, `v80` bone_influence +0x80, `v90` face20 +0x90, local flags +0xa0 (faces), +0xa1 (points/vertices) | 0x40eba0.., 0x412ee0, 0x411920, 0x4127f0 (inline vector teardown in this order) |
| | `kf` keyframes +0xa4, `kids` emitter* +0xb0, `p_c0` +0xc0, c4/c8 +0xc4/0xc8, `m_cc` +0xcc, mask +0x10c | 0x4125e0, 0x413320, 0x413750, 0x4126a0 |
| | `slots[3]` +0x110 (0x2c each), `m194` +0x194, `m1d4` +0x1d4 (texture matrix), f214 +0x214, sortkey +0x218, p220 +0x220, `bounds` +0x22c | 0x412a90 (ctor), 0x412980, 0x40f3f0, 0x4127f0 |
| `scene` | flags +0x18c, list +0x194, registered +0x1a4, mask +0x1a8, active +0x1ac, `queue` +0x1b4, view +0x1c4, cache +0x1c8 | 0x412040, 0x412100, 0x413750, 0x412c30 |
| `scene_registry` (global 0x5fe898) | `scenes` +0x3a4 | 0x412040 |
| `emitter` | particles +0x0c, noise +0x1c/0x20, drag +0x24, lifetime +0xa0, accel +0xc4/0xd4, flags +0xe8, water_z +0xec, disabled +0xf0, world +0xf4, local +0x134, box +0x174, vel_base +0x184, vel_scale +0x194, spawn +0x198..0x1a4, radius +0x1ac, spin_scale +0x1b0, spin_drag +0x1b4, noise_index +0x1b8 | 0x412150, 0x40ec90 |

`mesh::render` queues the *scene* (the slices' `Obj`) in the global registry the first time something is added:
`scene_registry::add_mesh(sc, this)` is `add_obj_param(Obj* e = scene, int v = mesh)` of the slices
(`mov ecx, 0x5fe898` at 0x413933, args pushed `mesh`, `scene`); the (mesh, 0.0f) entries are `idx_weight`s
(an 8-byte (int, float) record shared with the skinning lists), and `register_scene` (0x412040) is
`register_obj`.

## Header conflicts

1. **`src/rgl/mesh.h` is too weak for this object; `mesh_obj.h` replaces it.** The destructor 0x4127f0,
   the copy-assignments and the particle code cannot be byte-exact against the old layout. Differences, all
   verified (module and the unchanged `psys.cpp`, `mem_stream.cpp`, `resource_manager.cpp`, `text_stream.cpp`
   compile and match against `mesh_obj.h` renamed to `mesh.h`, see "Verification"):
   - `elem30` -> `skin_vert` {w[4], bone[4], bonef[4]} (0x411d20 `build_skin` writes them).
   - `elem14` {int, vector<int>} -> `bone_influence` {int bone, vector<idx_weight>}: the items are 8-byte (index,
     weight) records (0x412d50 calls `vector<idx_weight>::operator=` 0x411150, 0x4118b0 etc.).
   - `frame_t` gets its out-of-line ctor 0x411780 and `copy_from` 0x411c10; `keyframes` gets an inline
     destructor (`delete[] frames`, `delete[] times`: the `??_E` call 0x40fe10 in `~mesh`), its `times` are
     `int*` (0x40f0d0 compares `(float)times[i]`), `operator=` returns nothing (0x4125e0 leaves eax alone).
   - `slot` gets its inline ctor (the loop in the mesh ctor is its member-init), `aabb` its ctor 0x40eaf0
     (called after the slots loop in 0x412a90, i.e. as a member).
   - `mesh`: `kids` is `vector<emitter*>`, `p_c0` is deleted in `~mesh`, `f220` is `p220` (delete[]), +0x218 is
     the 64-bit sort key (`compute_sortkey` 0x412980; was `slot2`), the five chain accessors are the loops
     (see "Tricks"), and the members of this object are declared (`append`, `update_morph`, `should_batch`,
     `setup_material`, `ensure_skin`, `build_skin`, `update_emitters`).
   - `scene`: flags, registration and queue fields; `scene_registry`; `mesh_list`, `lod_mesh` added.
   `vertex60`, `point20`, `face20`, `vec2`, `obj_c0`, `vpush`, `vat` are as in `mesh.h`. **Orchestrator:** replace
   `src/rgl/mesh.h` by `mesh_obj.h` (change the include guard back to `RGL_MESH_H`, `"../x.h"` to `"./x.h"`);
   `mesh_obj.h` cannot be included together with the old `mesh.h`.
2. **`psys.h`'s `psys` is this module's `emitter`.** Same object (flags +0xe8; `mesh::kids` holds it; `psys::update`
   0x414960 is called from `mesh::render`): `emitter` has the simulation fields and the particle *vector*
   (+0x0c) that `psys.h` holds as raw `begin`/`end` at +0x10/+0x14 (`~mesh` inlines the vector teardown for
   each emitter: 0x412845..0x41285f). To keep `psys.cpp` untouched the class is not redefined here; merge by
   giving `psys` the `emitter` fields (`psys.cpp` uses `begin`/`end` only in `psys::update`, 0x414960). The
   100-byte record is `rgl::particle` in `psys.h` (s,f,u,pos,dir,age=t,f54,rot=angle,f5c=angvel,scale) and
   `emitter_particle` here.
3. **`src/game/renderer.h` and this module's `render_cache.h`** are the same object (global 0xa17910) under
   two names; `game/renderer.h` cannot be included next to `rgl/resource.h` (it defines global `material`,
   `mesh_part` (= `skin`) and `mesh_inst` (= `mesh`) again). Merge `render_cache` into `renderer`.
4. **`src/game/entity.h`** has a third `rgl::mesh` (fields at +0xb0/+0xc0/+0x1a8 only), a global-namespace `scene`
   (list +0x194, `v1b4` +0x1b4 = this module's `queue`, cache +0x1c8, handle +0x154) and `aabb` (ctor 0x40eaf0 =
   `rgl::aabb::aabb` here). The three headers should be one.
5. **`rgl_math.h`**: `matrix` lacks `transform_point` (0x408460) and `rot` (0x40a9c0); `emitter::spawn` calls them
   through `rgl::frame_ops : matrix` in `mesh_obj.h` (`src/game/mat4.h` has the first). Also
   `unpack_color(out, argb)` has the by-value form `unpack_color_ret(argb)` (same function 0x40e4e0; the
   copy out of the temporary is visible in `setup_material`). `rotate_about_s/f/u` are the slices'
   `rotate_fu/su/sf` (0x40e570/0x40e670/0x40e770), called on the first three rows of the particle.
6. **`resource.h`**: `material::get_flags` (0x40eab0) is defined here (declared in `resource.h`); it carries
   `inline` at the definition so that `mesh::render` does not get interprocedural register information (see
   Tricks), which belongs in `resource.h`'s declaration when the headers merge. `stage_t` has unnamed ints; the
   word at +0xc (`d`) of a shader stage is the texcoord generation mode in its low byte (`set_texcoord_gen`) and
   the texture-transform enable in bit 8 (0x40f3f0). `skin::f64` is the use count.
7. `named_object::~named_object` is out of line (0x4150f0, tail-called by `~mesh`): this file defines
   `RGL_NAMED_OBJECT_DTOR_OUT_OF_LINE` like `text_stream.cpp` and does not define the body.
8. `symbols/lib_names.txt` puts `__CIpow`, `__CIatan`, `__CIlog`, `__CIlog10` on one address (0x536c80);
   `emitter::update` calls `pow` at 0x536a6a (0x412240). The equivalence harness resolves the name first, so
   every function that calls `pow` is UNSUPPORTED on a "resolution conflict" until that line is fixed.

## Removed slice-local tricks / stand-ins

- All the slice class stand-ins: `renderer` copies, `mesh_info`, `rot_state` (is `aabb::aabb`), `rnode` (is
  `mesh`), `key_table` (is `keyframes`), `vec_int`/`vec96`/`vec48`/`vec32`/`vec20`/`vec100` (the size/capacity
  rows are real `std::vector<T>::size/capacity` of the mesh's element types, instantiated with
  `template class`), `E8`..`E100` (the element types are real), `MorphSet`/`VecPair`/`OwnedPtr` (are
  `keyframes`/`frame_t`/the inline `delete p_c0`), `PsysBase`/`PName`/`PsysDef` (are `named_object`, `rgl::string`,
  `skin`), `Scene`/`Obj` (`scene_registry`/`scene`), `Particle` (`emitter_particle`).
- `/EHa` (all STL rows) and the `$L...` funclet rows.
- Wrapper functions `copy96`, `fill96`, `copy_backward96`, `uninit_copy96`, `uninit_fill_n96`, `fill_n4`, the
  `alloc96_def` twins, `iter32`, `destroy_vecpair_array`: replaced by explicit instantiations of the real
  templates (`std::_Copy_opt`, `std::fill`, `std::_Copy_backward_opt`, `std::_Uninit_copy`, `std::_Uninit_fill_n`,
  `std::_Destroy_range`, `std::allocator<T>::construct`, `std::vector<T>::_Insert_n/insert/erase/resize/_Buy/
  _Ufill/_Ucopy/operator=/push_back` through `template class std::vector<T>`).
- `get_verts_inl()` (the hand-inlined first level of `should_batch`) is the explicit `local_a1 ? v70 :
  parent->get70()` in `render`, `append`, `update_morph`; `render_child` is `emitter::render_update`
  (0x414960, `psys.cpp`).

## Kept tricks (flagged)

- **Chain accessors** `get70/get50/get_kf/get90/get60` (0x40eba0..0x40ec60): `__declspec(noinline)` in-class
  loops. In-class (inline linkage) because a non-inline definition in the `.cpp` lets cl compile callers with
  the loop's register footprint (`should_batch` 0x40f7f0 lost its match); `noinline` because cl otherwise
  expands the loop into `update_morph`. Callers that inlined "the first level" in the original write the flag
  test themselves (`render`, `append`, `should_batch`, `update_morph`), as `psys.cpp` already does.
- **`emitter::update`**: the induction pointer `pv` (+0x48 of the current particle, stepped by 25 floats) and
  the reference-taking in-place helpers `add_to`/`scale_f` reproduce the original's addressing and its
  store/reload of vel/pos/angvel; kept from the slice.
- `mesh::setup_material`'s texture-stage loop indexes `material::stages[i].d` (the slice used `+0x74 + 0x14*i`
  pointer arithmetic: the plain form matches too).
- `mesh::build_skin`: the two 64K-entry stack tables (`float tw[0x10000][4]; int tb[0x10000][4]`) set the 2 MB
  `__chkstk` frame; `skin_vert tmp` is deliberately uninitialised (the original's `resize(n, tmp)` copies it).
- `unpack_color_ret` (by-value form of 0x40e4e0), `frame_ops` (the two matrix members `rgl_math.h` lacks),
  `*(rot3*)out = *(const rot3*)&world` and `((matrix*)out)->rotate_about_*` in `emitter::spawn` (the first 0x30
  bytes of a particle are three matrix rows), `(int)m` in `scene_registry::add_mesh` (a mesh pointer in an
  `idx_weight.index`).
- Local copies (the audit lists them, all `agree`): `vector<vector4>::resize` -> 0x40db80 (`frame_t::copy_from`),
  `vector<scene*>::resize` -> 0x41f970, `std::vector::_Xlen` -> 0x43e2a0, `allocator<emitter_particle>::construct`
  -> 0x468170, `std::fill<idx_weight*>` -> 0x48ad70, `vector<int>::_Ucopy`/`_Ufill` -> 0x41ef30/0x438c30, and the
  same-address ICF twins inside this module (`copy`/`_Copy_opt`, `resize<face20>` -> `resize<point20>`...).

## Resolution audit

`resaudit_all.py --slices src:src/rgl/particles/mesh_obj.manifest.txt` (114 rows, `scratch_particles_audit.jsonl`):
**109 agree**, 5 unverified, 0 not-audited, 0 disagree.
- The 5 unverified are the large `_Insert_n` instances (0x40fed0, 0x4101a0, 0x4108b0, 0x410b60, 0x410dc0): the
  audit decodes only up to the next function symbol, and the calls they make (`_Uninit_copy`, `fill`,
  `copy_backward`, `_Xlen`) are in the 230..270-byte cold tail of the same COMDAT (the symbol-less `catch`
  funclet and cold branches at 0x410080 ... 0x410f6a). Re-run with the original's range extended over the
  tail (`work/migrate/scratch_particles_audit_ext.py`, which also does 0x410610, 0x411080, 0x411b80):
  **0 unverified, 0 disagree**, e.g. 0x40fed0 agree=8.
- 0x412a90 was first not-audited (`identity` had no `// 0x` annotation the tool reads); the declaration moved into
  `mesh_obj.h` and the row agrees now.

## Equivalence harness

`equiv.py mesh_obj <va> --src src/rgl/particles/mesh_obj.cpp --flags "/O2 /MT /Gy /TP"` on the five
nonmatching rows (table above): no FAIL. Sampled on 19 byte-exact rows (`ex1.log`): PASS 0x412a90, 0x4127f0,
0x412100, 0x411c10, 0x412ee0, 0x412f50, 0x411920, 0x40f0d0, 0x413b00; WEAK with no mismatch (missed edges are the
same on both builds) 0x413320, 0x40f7f0, 0x412980, 0x412040, 0x4125e0, 0x40ec90, 0x40f3f0, 0x412700, 0x4126a0,
0x413ac0. No FAIL anywhere.

## Verification

`chk.py` on all 137 VAs: 114 MATCH (the rest: 18 tails, 5 nonmatching). `run_all.py --manifest`: 114 / 114.
The old `src/rgl` modules against `mesh_obj.h` renamed to `mesh.h`/psys.h unchanged
(`work/migrate/scratch_particles_dropin/`): `psys.cpp` 3/3, `mem_stream.cpp` 30/30, `resource_manager.cpp`
34/34, `text_stream.cpp` 24/24 byte-exact (the three `psys.cpp` nonmatching rows keep their diffs, `psys::emit` is
2727 instead of 2775 bytes with the new header).

## Problems / for the orchestrator

- Replace `src/rgl/mesh.h` by `mesh_obj.h` (item 1), merge `psys` and `emitter` (2), `renderer` and
  `render_cache` (3), and the `entity.h` copies of `mesh`/`scene`/`aabb` (4).
- Fix `lib_names.txt`'s `__CIpow` (8): then `emitter::update` can be run by the harness.
- `resaudit.py` stops a function at the next symbol: 5 `_Insert_n` rows (and every STL function with a cold tail)
  show "unverified"; the extended-range script in `work/migrate/` shows the references agree.
- The three remaining operand-order diffs (`render`, `append`, `emitter::update`) look like one cl effect:
  the order of the two memory operands of a commutative x87 multiply does not follow the source order (tested
  swapped source, 144 expression shapes for `render`, const/non-const pointers for `update`) and differs from
  the original in exactly the positions listed.

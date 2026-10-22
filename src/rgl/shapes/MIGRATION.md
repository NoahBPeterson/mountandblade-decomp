# rgl/shapes migration (0x407c10-0x40e940)

The physics collision shapes (sphere, capsule, face, manifold, composite), their contact generation and the
vector/matrix helpers that sit in the same stretch are merged from fourteen slices (`s00407c10`, `s00408110`,
`s00408a50`, `s00408e60`, `s00409500`, `s00409e90`, `s0040a640`, `s0040aff0`, `s0040b0f0`, `s0040bcb0`,
`s0040c670`, `s0040cfe0`, `s0040d7d0`, and `s0040e060` without its last function 0x40e940) into one module,
`shapes.cpp` + `shapes.h`. The slices under `match/slices/` are unchanged and stay the reference.

All 154 rows build with the default flags `/O2 /MT /Gy /TP`. The slices used `/EHsc` (s00407c10, s00408110) and
`/EHa` (s0040e060) for some rows; none is needed: cl 13.10 emits `try`/`catch(...)` code without `/EH` (warning
C4530), exactly as in the rgl part 1 module. Real headers only (`<windows.h>`, `<d3d9.h>`, the VC7.1 STL), plus
`rgl_string.h`, `rgl_math.h`, `stream.h` of `src/rgl`. `match/include/rgl.h` is not included.

## Summary

| module | range | rows | byte-exact before (slices) | byte-exact after | improved | regressed | nonmatching | partial |
|---|---|---|---|---|---|---|---|---|
| shapes.cpp | 0x407c10..0x40e93f | 154 (146 functions + 8 interior VAs) | 135 rows (133 functions + 2 funclet rows) | 134 | 1 | 0 | 8 functions + 8 interior VAs | 4 |

Every function that was byte-exact in its slice is byte-exact in the module. The two slice rows 0x40e200
(`$L73928`) and 0x40e3bb (`$L72380`) were catch funclets that only had a symbol under `/EHa`; with the default
flags they are inside the parent COMDATs (`vec<mface>::insert_n` 0x40e060, 745 of 745 bytes identical, and
`uninit_fill_n<mface>` 0x40e350, 117 of 117), which are exact, so the bytes are still verified. They are listed
in `shapes.nonmatching.txt` with 0 differing bytes, like the other funclets (0x40c1b1, 0x40c257, 0x40d5cc,
0x40d7bd, 0x40dfda: parents 0x40c140, 0x40d550, 0x40d750, 0x40df60, all whole-COMDAT identical).

Improved (slice status -> module status):
- 0x40cb60 `manifold::build_composite`: nonmatching (3 bytes, base/index operand swap of `[eax+ebx+0x1c]`) -> MATCH.
  The swap was a consequence of the slice calling `shape_ext::shape_ext()` as an opaque function; with the real
  out-of-line constructor and the three inline construction sites written as in the original, cl schedules it
  like the original.

Nonmatching (complete source, equiv not FAIL): 0x408bd0 face::transform (34 bytes, PASS), 0x408e60
renderer::draw_polygon (1426, WEAK: the same 3 loop-guard edges are missed by both builds, also with 2000
inputs), 0x409980 (118, PASS), 0x409d20 (153, PASS), 0x409e90 (144, PASS), 0x40a640 capsule::draw (248, PASS),
0x40ab10 renderer::tessellate (25, PASS), 0x40dc50 `vec<vector4>::insert_n` (30 + 70 bytes, WEAK: the harness
cannot return NULL from malloc, 90% of the instructions are reached). Partial (source complete, equiv FAIL only
through undefined behaviour or 1-ulp x87 rounding, as in the slices): 0x409500, 0x409ac0, 0x40a000, 0x40b0f0.
The differences of all of them are the slices' differences (same byte counts); the module did not make them
worse. 0x409980 gets to 104 bytes with `vector4::operator-`/`length()` instead of the component sums, not exact.

## Verification

- `chk.py` on all 154 VAs: 134 MATCH, the rest are the rows above (interior VAs are checked with COMDAT-wide
  comparisons of the parents).
- `run_all.py --manifest src/rgl/shapes/shapes.manifest.txt`: 134 / 134 byte-exact.
- Resolution audit (`resaudit_all.py --slices src:src/rgl/shapes/shapes.manifest.txt`): 132 agree, 2 unverified
  (0x40c140 `_Copy`: its only reference is `operator delete`, 0x40cb50 `manifold::clone`: a 5-byte tail jump),
  0 disagree, no local copies.
- Equivalence (`equiv.py shapes <va> --src ... --flags "/O2 /MT /Gy /TP"`): PASS 0x408bd0, 0x409980, 0x409d20,
  0x409e90, 0x40a640, 0x40ab10; WEAK 0x408e60, 0x40dc50; FAIL (partial rows) 0x409500, 0x409ac0, 0x40a000,
  0x40b0f0. No nonmatching row fails.
- `symbols/slices/shapes.txt` names the vtables and the folded deleting destructors, and the nonmatching
  `insert_n<vector4>` (the resolver cannot find a nonmatching explicit instantiation by its marker).

## Boundaries

The map has seven TUs in this stretch (`sphere` 0x407a70..0x40ad80, `capsule` ..0x40ae40, `face` ..0x40c010,
`manifold` ..0x40c940, `manifold_2` ..0x40ce10, `composite` ..0x40d320, `manifold_3` ..0x40e940); the start of the
first is dxutil's `CArrayList` (src/dxframework, 0x407a70..0x407c10, not mine). **All seven are one module.**

Evidence, per boundary: every map boundary here is a class-name change of confidence "low" (the map itself says
class changes are right about half the time, and "possible inner splits" are listed: 25 + 12 + 10), and this is
a "T" region (the map's caller-first emission regime, 0x408000..0x40d000), where forward calls prove nothing.
What the module shows:

- **One compile unit reproduces every slice row, with default flags, and improves one.** A split into several
  objects would have needed a different symbol layout for the shared pieces; the module needs none.
- **The shape classes interleave with their helpers in one emission order.** `sphere::draw` (0x408110) is followed
  by the renderer functions it calls (0x408170 translate, 0x4081b0 pop, 0x4081f0 draw_sphere), `sphere::transform`
  by `pos::transform_point` (0x408460), the save/load slots by the stream helpers (0x408510, 0x408570),
  `face::draw` by `renderer::draw_polygon` (0x408e60), `capsule::draw` by `renderer::apply`, `pos::rot3`, `pos::rot`,
  `draw_capsule`, `tessellate` (0x40a870..0x40ab10), `face::get_min` by `vmin` (0x40af80): helper placed right
  after the first caller, in a single object's emission order (the T regime). About two dozen such forward calls (callers 0x408110, 0x408420, 0x4084c0, 0x408540, 0x408e40,
  0x40a560, 0x40a640, 0x40a870, 0x40a920, 0x40aa20, 0x40aef0, 0x40aff0, 0x40cb50, 0x40cb60, 0x40cd00, 0x40d140,
  0x40d320, 0x40d3a0, 0x40d3d0, 0x40d5e0, 0x40d9d0, 0x40db80, 0x40dc50, 0x40dff0 ...) would each be a "hard"
  boundary under the map's B-regime rule; here they would cut the shape classes into one-function objects.
  They are not boundaries.
- **The vector/matrix functions at both ends belong to the same object.** The head (0x407c10..0x407f70: `vector4`
  members, `normalize`, `cross`, `orthonormalize`, `transform_point_to_local`) and the tail (0x40e3d0..0x40e870:
  `clamp`, `frand`, `pack_color`, `unpack_color`, `rotate_about_*`, `matrix::operator*`) are called backwards
  and forwards by the shape code (0x409500 -> 0x407ef0/0x407f70, 0x40e870 -> 0x40a920), and 0x40e870 calls the
  `rot3` helper that sits in the middle (0x40a920).
- **The `std::vector` instantiations at 0x40d3d0..0x40e3bb** (`vec<vector4>`, `vec<mface>`: operator=, resize,
  erase, `_Insert_n`, `_Uninit_copy`, `_Uninit_fill_n`) are COMDATs of the manifold/composite code that uses them;
  `vec<mface>::erase` (0x40fd40) and `_Buy` (0x4105c0) are not in this range and are declared only: they are
  emitted later in the same object or in a later one (not decided here).
- **The `std::string` members and `std::logic_error`/`length_error` (0x40c0c0..0x40c630)** are template and
  exception-class COMDATs the linker placed at the first object that instantiates them (here, between the
  contact dispatcher and the manifold). They are explicit instantiations in the module.
- **The end is the next agent's start, not a boundary.** `renderer::set_blend_mode` (0x40e940, in the same
  slice s0040e060) and the following region continue the same emission order (`renderer` members again). The
  TU may well continue past 0x40e940; `vec<mface>::erase` (0x40fd40) and the call from 0x40dff0 to 0x40fd40
  suggest that the object's emission order goes on at least that far. Whoever migrates 0x40e940 onward should
  decide whether to extend `shapes.cpp` or to cut a second module there.

Not resolved: whether the object starts exactly at 0x407c10 (the first function, `rgl::string::c_str` as an
out-of-line copy, and 0x407c20 `return this`, have no caller in the range) and whether the head (vector4 math)
is a different object: `normalize` and the matrix members are opaque to the shape code (see "ext declarations"),
which is what a cross-object call looks like, but so is every forward-called helper of the T region.

## Header (shapes.h): one layout per class

| class | vtable | size | fields (offset: function that proves it) |
|---|---|---|---|
| `shape_base` | 0x54a4b8 | 0x20 | m4 +4 (copy_from 0x4080b0), m8 +8 (0x408070/0x408080), mc +0xc (0x408040), bbox ints +0x10..0x1c (1e8/-1e8: 0x407ff0) |
| `sphere` | 0x54a4f0 | 0x44 | local_center +0x20 (load 0x4084c0, scale 0x4083f0), center +0x30 (transform 0x408420, bounds 0x4085a0, contact code), radius +0x40 (draw 0x408110) |
| `capsule` | 0x54a540 | 0x64 | p0 +0x20, p1 +0x30 world ends (0x408770, 0x40a640, 0x409ac0), l0 +0x40, l1 +0x50 local ends (0x408870, 0x408720), radius +0x60 |
| `face` | 0x54a588 | 0xb4 | w[4] +0x20 world vertices (0x408bd0, contact code), v[4] +0x60 local (0x408d60), normal +0xa0, count +0xb0 |
| `manifold` | 0x54a5f4 | 0x50 | verts +0x20 (first at +0x24), wverts +0x30, faces +0x40 (`mface` 0x20 bytes: a, flags, n, idx[4], m4) |
| `composite` | 0x54a63c | 0x28 | bodies +0x20, count +0x24 (0x40cfb0..0x40d260; the dispatcher 0x40bcb0 reads them as "kids") |

Findings that change the slices' reading:

- **Load and save were swapped for sphere, capsule and face** (s00408110, s00408a50 had a stream with slot 3 =
  read, 14 = write). The stream is `stream_base` (stream.h): slot 3 `w_float`, 8 `w_int`, 10 `w_cstr`, 14
  `r_float`, 19 `r_int`, 21 `r_tstr`. 0x4084c0 calls slots 14/19 (`[eax+0x38]`, `[edx+0x4c]`): it is `load`
  (vtable slot 3), 0x408540 is `save` (slot 4); likewise capsule 0x408870 = load / 0x408910 = save, face
  0x408d60 = load / 0x408dd0 = save. The slices' `save`/`load` names for them are wrong.
- **Slot 6 is `draw`, not `update`.** sphere 0x408110, capsule 0x40a640, face 0x408e40 draw the shape through the
  render state; the base's slot 6 is an empty function (0x477b70), manifold keeps it, composite's 0x40cfb0 draws
  each body. The slices' "update" for composite is `draw`.
- **Slot 1 is `shape_type`**: 0 sphere (0x408020, folded with the base's `clone` returning 0), 1 capsule
  (0x48b1f0), 2 face (0x48b590), 3 composite (0x48b880), 4 manifold (0x48c460); the last four are tiny functions of
  other objects. The dispatcher 0x40bcb0 and `composite::save` (0x40ced0, names "sphere", "capsule", "face",
  "manifold") use them. The type name strings are "sphere", "capsule", "face", "manifold", "composite_body".
- **`get_min`/`get_max` return their `out` pointer** (eax = the argument) for every shape; the base fills zeros
  (0x408050). The face version in the slice, `vector4 get_min()` by value, is the same ABI.
- `contact` is 0x24 bytes (normal, point, depth). The slices' `coll_shape`/`coll_poly` views are gone: the
  contact functions take the real classes (`sphere*`, `capsule*`, `face*`, `shape_base*`).
- The unknown 0x407c10 is `rgl::string::c_str()` out of line (`string_view_::get`), 0x407c20 returns `this`.

## Header conflicts (not edited, worked around locally)

- `src/rgl/resource.h` `shape_base` (vtable 0x54ac58's `shape`): has slots 0..4 only and calls slot 1
  `shape_type`, 2 `slot2`, 3 `slot3`, 4 `save`: wrong (the real base has 14 slots, 3 = load, 4 = save, 2 =
  type_name) and it has no constructor. `shapes.h` defines the real class under the same name; the two cannot
  be included together. Replace resource.h's by `#include "./shapes/shapes.h"` (body_part only needs a pointer
  and the virtual destructor).
- `src/rgl/scene/shape_clone.h` (another agent): `shape_x`/`shape_ext`/`composite`/`sphere_shape`/`capsule_shape`/
  `face_shape` are partial copies of these classes (`sphere_shape` has v20/v30, `capsule_shape` p0..p3). Same
  layouts, other names; to be replaced by shapes.h. It calls the base constructor as `shape_ext::shape_ext()`;
  shapes.cpp aliases `??0shape_ext@@QAE@XZ` to the real 0x407ff0 symbol with `/alternatename`.
- `src/game/agent/phys_set.h`, `src/game/strat/strat_ext.h`: `shape_base`, `capsule_shape`/`capsule`, `face_shape`,
  `mat3`, again partial copies.
- `src/game/renderer.h` `renderer` (the D3D9 state cache of the same object 0xa17910): shapes.h declares a
  second `renderer` with the three fields the shapes use (stack at 0, `sp` +0x500, `dev` +0x618, `cur` +0x794)
  and its members 0x408170, 0x4081b0, 0x4081f0, 0x40a870, 0x40aa20, 0x40ab10, 0x408e60. renderer.h cannot be
  included: it includes `match/include/rgl.h`, whose `vector4`/`string` collide with `rgl_math.h`/`rgl_string.h`.
  Its `view2` (+0x794) is my `cur`. One `renderer` has to absorb both.
- `src/game/mat4.h`: `rgl::identity()` (by value) for 0x40cd00; shapes.cpp defines 0x40cd00 as
  `pos* identity_pos(pos*)` and 0x40cd60 as `mat3* identity3(mat3*)` (both `__cdecl`, out pointer returned): same
  ABI as the by-value form, other mangled name. `mat3` = mat4.h's `rot3`.
- `src/rgl/rgl_math.h`: `void unpack_color(vector4* out, const unsigned*)` at 0x40e4e0 cannot reproduce the code
  (the function returns the colour by value: hidden return pointer, eax = it; local `v` then copied). shapes.cpp
  defines `vector4 unpack_color(const unsigned*)` (same call ABI). The header should be changed. Everything else
  declared there (`vector4` members, `cross`, `normalize`, `dot`, `length`, `matrix::orthonormalize`,
  `transform_point_to_local`, `operator*` 0x40e870, `rotate_about_*`, `pack_color` 0x40e440) is defined here as declared.
- `src/rgl/mem_stream.h`'s `w_body_part` names the shape kinds; they are `shape_base::type_name()` results.

## Tricks kept, tricks removed

Kept (each is needed for the byte-exact rows):

- **"ext" declarations** (`pos_ext::transform_point`, `pos_ext::rot_ext`, `matrix_ext`, `vec4_ext`,
  `normalize_ext`, `vmin_ext`, `vmax_ext` in shapes.h): the original's callers of these functions were compiled
  without cl's knowledge of the callee. When cl compiles the callee first in the same object it keeps its
  register use and side effects: the callers then choose other registers (`mov edx, ecx` instead of
  `push esi`), or do not reload `count` after the call (face::get_min/get_max, 114 bytes instead of 129). The
  slices got this by declaring the callee without a body. The module still defines the real functions;
  `shapes.cpp` binds each ext name to its function with `#pragma comment(linker, "/alternatename:...")`, so the
  link has one function per address. `// 0x...` annotations on the ext declarations give the resolver the
  original address. (Why the original's compile had no callee information, though the callee is in the same
  stretch, is open; `inline` + `noinline` on the callee, definition order and moving the definition did not
  change anything.)
- **`shape_base(inline_init)`** next to the out-of-line `shape_base()` (0x407ff0): the constructors of sphere,
  capsule, face and manifold (and `composite::load`'s `new`s) have the base stores expanded inline; the creation
  sites of composite (0x40cb60, 0x40ce10) call 0x407ff0. Both forms exist in the object; the slices got the call
  with a body-less `shape_ext` constructor.
- `vec<T>`: a hand-written copy of the VC7.1 `std::vector` layout (empty allocator, first, last, end) with
  `clear`, `buy` and `xlen` declared only: they live in other objects (0x41f4b0, 0x42d570/0x4105c0, 0x43e2a0), and
  a real `std::vector` would instantiate its own. `<vector>` is included for the allocator and the iterator tags.
  The same pattern as `vec<T>` in src/game/entity.h (not included here).
- `volatile` in `segment_closest_points` (partial) and `polygon_capsule_contact` (partial): register-pressure
  levers from the slices, only in rows that are not exact anyway.
- `__declspec(noinline)` on the renderer members, `pos::transform_point`/`rot`, `vmin`/`vmax`, the stream
  helpers, `identity_pos`/`identity3`, `uninit_copy`, `uninit_fill_n`, `vec::operator=`/`resize`/`erase`/
  `insert_n`; `__forceinline` on `transform_point_inl` (the body capsule::transform and `matrix::operator*`
  have inlined) and on the vec helpers.
- The two small helpers `string_view_::get` (0x407c10) and `self_ref::self` (0x407c20), by their bodies.

Removed:

- Per-row `/EHsc` and `/EHa` flags and the slices' separate compile of the STL rows.
- `shape_ext` (a class whose constructor was defined elsewhere), `coll_shape`, `coll_poly`, the slices' four
  local `shape_base`/`sphere`/`capsule` stubs, the `stream` stubs (`stream_base` is used), `d3d_device` /
  `IDirect3DDevice9` stubs (real `<d3d9.h>` compiles: `D3DRS_*`, `D3DTS_WORLD`, `D3DPT_LINELIST`, `D3DCMP_*` are
  used instead of `0x8b`, `0x100`, `2`, `4`), `name_string` (`rgl::string`), `log_error` (`mb_error`).
- `pos_ext` for `sphere::transform` stays (above); `write_vec3`/`read_vec3` are now `read_vec3` at 0x408510
  and `write_vec3` at 0x408570 (the slice had them reversed together with save/load).
- `vec32`/`elem32` of s0040e060 (same template as `vec<mface>`, different name): one instantiation,
  `vec<mface>::insert_n`/`uninit_fill_n<mface>`, which is now byte-exact over the whole COMDAT.
- `force_vtables` is kept (std::logic_error / length_error scalar deleting destructors and vtables forced into
  the object, as in s0040bcb0), because they are not referenced by any function of this stretch.

## Files

`shapes.cpp`, `shapes.h`, `shapes.manifest.txt` (134 rows), `shapes.nonmatching.txt` (16 rows),
`shapes.partial.txt` (4 rows), `MIGRATION.md`; `symbols/slices/shapes.txt`.

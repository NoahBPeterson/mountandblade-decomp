# rgl migration, part 1 (0x413b60-0x41918f)

Three ranges of the TU map (`text_stream` 0x413b60..0x415900, `mem_stream` 0x417010..0x417bb0,
`resource_manager` 0x417bb0..0x419190) are merged from nine slices (`s00413320` (2 of its functions),
`s00413c50`, `s00413e50`, `s00414960`, `s00415320` (its first 13 functions), `s00416ef0` (its last 18),
`s00417850`, `s00418150`, `s00418a70` (its first 15)) into five modules of the engine library. The slices
under `match/slices/` are unchanged and stay the reference.

All modules build with the default flags `/O2 /MT /Gy /TP`, like the framework. No function of the
ranges has an unwind frame. The four STL helpers with `try`/`catch(...)` (0x418050, 0x418150,
0x4184b0, 0x4187b0) were built with `/EHa` in their slices: they are byte-exact with the default
flags too (cl 13.10 emits `try`/`catch` code without `/EH`, it only warns C4530), so `/EHa` and the
`_eh.cpp` side files are gone. Only real headers are used (`<windows.h>`, `<shlobj.h>`, `<d3d9.h>`, `<d3dx9.h>`,
the VC7.1 STL). `match/include/rgl.h` is not included (see "Header conflicts").

Verification: `chk.py` on every module VA, `run_all.py --manifest` on each `<module>.manifest.txt`,
the resolution audit, and the equivalence harness on every nonmatching row (results below).

## Summary

| module | range | functions | byte-exact before (slices) | byte-exact after | improved | regressed | nonmatching | partial |
|---|---|---|---|---|---|---|---|---|
| psys.cpp | 0x413b60..0x414adf | 6 | 3 | 3 | 0 | 0 | 2 | 1 |
| noise.cpp | 0x414ae0..0x4150ef | 2 | 0 | 0 | 0 | 0 | 2 | 0 |
| text_stream.cpp | 0x4150f0..0x4158ff | 24 | 24 | 24 | 0 | 0 | 0 | 0 |
| mem_stream.cpp | 0x417010..0x417baf | 30 | 29 | 30 | 1 | 0 | 0 | 0 |
| resource_manager.cpp | 0x417bb0..0x41918f | 39 | 34 | 34 | 0 | 0 | 5 | 0 |
| **total** | | **101** | **90** | **91** | **1** | **0** | **9** | **1** |

The TU map's counts per range are reproduced: `text_stream` 32 = psys + noise + text_stream (27 exact,
4 nonmatching, 1 partial), `mem_stream` 30, `resource_manager` 39 (35 functions + 4 catch funclets).

Improved:
- 0x4172d0 `tstream3::tstream3`: nonmatching (6 bytes, vptr store order) -> MATCH. The ctor has the
  same shape as `bin_reader`'s (0x417c50); with the reader fields in an intermediate class whose inline
  ctor does the init (`file_reader_base`, shared with `bin_reader`), cl stores the final vptr last.

Regressions: none. Every function that was byte-exact in its slice is byte-exact in its module.

Nonmatching rows (`<module>.nonmatching.txt`) are the slices' rows, re-measured in the module:
0x413d30 (26 bytes), 0x414960 (115), 0x414ae0 (808), 0x414fc0 (137), 0x418e90 (159), and the four
catch(...) funclets 0x4180b9, 0x4181b4, 0x418516, 0x41881f (0 bytes: verified against the parent's
COMDAT tail, see below). The partial row 0x413e50 `psys::emit` is 1614 bytes off (1627 in its slice).
Variants tried in the module for 0x413d30 (12: index/iterator/while loops, `push_back`, a `this` copy, a
temporary string, the `found` declaration) do not move its this/&meshes register swap.

## Boundaries

| module | start evidence | end evidence |
|---|---|---|
| psys | `mesh::render` 0x413750 calls `psys::update` 0x414960 **forward** (hard: boundary in (0x413750, 0x414960]); start kept at 0x413b60 (below) | content only |
| noise | content only | content only |
| text_stream | content only | `text_stream::r_tstr` 0x415880 calls 0x415900 **forward** (hard, but see flag 3) |
| mem_stream | content only (TU map: class change, kept) | content only (TU map: class change, kept) |
| resource_manager | content only (kept) | content only (TU map: class change, kept) |

Changes to the TU map:
- **The `text_stream` range (0x413b60..0x415900) holds three objects, not one.** It is split into
  `psys` (0x413b60..0x414ae0: `std::vector<vertex60>` erase/resize, `mesh::add_vertex`, the particle
  mesh pool, `psys::emit`, `psys::update`), `noise` (0x414ae0..0x4150f0: `noise_gradient`,
  `noise_fractal`) and `text_stream` (0x4150f0..0x415900: `~named_object`, file helpers, `stream_base`'s
  slots, the text stream's open/close/write slots). The psys functions form one bottom-up call chain
  (0x414960 -> 0x413d30, 0x413e50 -> 0x413c50 -> 0x413bb0 -> 0x413b60) and none calls into the rest
  of the range or is called by it. The two splits at 0x414ae0 and 0x4150f0 rest on content only
  (particle system vs. gradient noise vs. file/stream code); no call crosses either gap. `noise` could
  also be the tail of `psys.cpp` (nothing proves a boundary at 0x414ae0); it is a module of its own because
  the noise functions are general utilities (callers all over the game: 0x412150, 0x43bdb0, 0x461570, ...)
  and own their tables (`.data` 0x5a8160/0x5a8260, `.bss` 0x5d6f98, which follows dxutil's last static
  0x5d6f94). Their `.rdata` (0x54a710, 0x54a714) sits between psys's and text_stream's, as expected.
- The start of psys stays at 0x413b60 (window 0x413750..0x414960). `vector<vertex60>::erase`/`resize`
  are template COMDATs that the linker places with the first object that instantiates them; their callers
  are 0x413c50 (here) and 0x41d6f0, 0x428e80 (later objects), none in the mesh object before 0x413b60,
  so they belong to this object. `mesh_list::render_all`/`lod_mesh::render` (0x413ac0/0x413b00) call
  back into `mesh::render` and stay with it.
- The other boundaries are unchanged. Evidence checked for 0x417010..0x419190 (no forward call in or
  out of it except to/from distant code):
  - `options::save` (0x4181c0) inlines the `bin_writer` ctor (0x417c10), which other objects call out of
    line (0x41e150): a non-inline ctor that cl /Ob2 inlined inside its own object, so 0x417c10 and
    0x4181c0 are one object. This supports keeping 0x417bb0..0x417cb0 (`bin_writer::flush`, the two
    file-stream ctors, `~mem_stream`) in `resource_manager` rather than moving them to `mem_stream`, as
    their class names would suggest.
  - Vector deleting destructors come in two groups: 0x417600..0x4177b0 (skin, material, mesh,
    body_part) and 0x417f20/0x417fb0 (texture, effect_set). cl only emits a real `??_E` in an object that
    contains an array new of the class (elsewhere `??_E` is a weak alias of `??_G`), so each group comes
    from a different object's array news, or from two places in one object. Consistent with the
    0x417bb0 boundary, not proof of it.
  - `.rdata` is monotone across the whole stretch (vtables 0x54ac08 effect_set, 0x54ac58 body_part,
    0x54ac88 tstream3, 0x54ad68 mem_stream, 0x54add8 bin_writer, 0x54ae48 bin_reader, 0x54aeb4
    frame_set, then resource_manager's strings 0x54aebc..0x54b1cc): it does not separate objects.
  - The mem_stream slots (0x417930..0x417b50) are shared through ICF with `bin_writer`/`bin_reader`
    (0x417c10/0x417c50 build those vtables), so `mem_stream` may well start the resource_manager object;
    the content of 0x417010..0x417930 (effect/body-part records, two text-stream ctor/dtor, key-array
    serialisers) is a grab bag that does not settle it.

## Shared headers (one layout per class, offsets verified against the disassembly)

| header | contents |
|---|---|
| `rgl_string.h` | `rgl::string` (16 bytes: pos, buf, cap, len) and the string object's `mb_error` / `mb_file_size` |
| `rgl_math.h` | `rgl::vector4`, `rgl::matrix` (s, f, u, o), colour packing (from rgl.h + the mesh slices) |
| `stream.h` | `stream_base` (vtable 0x54a740), `file_writer_base`/`file_reader_base`, `text_stream`, `text_stream2`, `tstream3`, `mem_stream`, `bin_writer`, `bin_reader` |
| `resource.h` | `named_object` (resource root, vtable 0x54a6c8), `texture`, `material`, `skin`, `effect_set`, `shape_base`, `body_part`, `key_array`, `key_clip`, `vec_table`, `frame_set`, `resource_manager` + `g_resmgr` |
| `mesh.h` | `rgl::mesh` (0x260 bytes) and its element types, `aabb`, `rgl::scene` |
| module headers | `psys.h` (particle, mesh_pool, psys), `noise.h`, `text_stream.h` (file helpers), `mem_stream.h` (key-array serialisers), `resource_manager.h` (options, globals) |

Every module includes the headers it needs as `"./x.h"` (equiv.py reads `// 0x` annotations only from
headers the .cpp includes directly by relative path).

Class names: the slices' names are kept where `symbols/slices/*.txt` maps vtables by them (`??_7skin@@6B@`,
`??_7material@@6B@`, ...): renaming a class to a name another symbols line maps to a different address
would make the resolver map our vtable wrongly. In BRF terms `material` (0x54ab30: alternatives, pixel
shader) is a **shader** and `skin` (0x54abf0: shader name, five textures, colour) is a **material**; the
resource manager's lists are named that way (`shaders`, `materials`).

## Header conflicts resolved

- **String.** Three layouts for the class at 0x42bbd0..0x42c840: `tstring` {pos, buf, cap, len}
  (s00414960, s00417850: verified by `at_end`, `read_from`, `w_stream`, `flush`), `mstr` {p, size, cap,
  extra} (mesh slices, fields unused) and match/include/rgl.h's `rgl::string` {p, size, cap, extra},
  marked "fields unverified". The buffer is at +4: `rgl::string` in `rgl_string.h` has the verified
  layout, and src/rgl does not include rgl.h. **rgl.h's `string` should be corrected** (orchestrator).
- **stream_base is not a named_object.** s00414960 derived `stream_base` from `named_object` (with a
  pure `reset` as slot 0); the stream ctors (0x417c10, 0x417c50, 0x4172d0) store 0x54a740 first and never
  0x54a6c8, and the stream vtable has no destructor slot. `stream_base` is its own root with `name` at +4.
- **One resource root.** `named_object` (s00414960, s00417850), `res_base` (s00416ef0), `mesh_base`
  (mesh.h), `res_hdr`/`resource` (s00418a70) are the class with vtable 0x54a6c8: vptr, `name` +4,
  `f14` +0x14 (zeroed by the inline ctor, see 0x417010/0x417e10), slot 0 the pure virtual dtor, slot 1
  `type()` (0x408020).
- **`~named_object`.** The derived deleting dtors (0x417600 ...) inline its body, and an out-of-line
  copy sits at 0x4150f0. The header defines it inline; text_stream.cpp defines
  `RGL_NAMED_OBJECT_DTOR_OUT_OF_LINE` and the body non-inline. (Its only caller, the mesh dtor 0x4127f0,
  is in an earlier object; the copy kept at 0x4150f0 is presumably an ICF/COMDAT pick.)
- **mesh.** `Psys` (s00416ef0: the class whose deleting dtor is 0x417730) and `mesh_obj` (s00418a70)
  are `rgl::mesh` (vtable 0x54a6f4, 0x260 bytes). Its last member (+0x22c) is the 0x34-byte `aabb`
  (`vec3x` in mesh.h; s00418a70 called it 0x38 bytes, which would overflow 0x260). mesh.h's `res_t` is
  `skin`, `mat_t` is `material` (`get_flags` 0x40eab0), `res_obj4` is `texture`. `kids` (+0xb0) holds
  `psys*`: `mesh::render` calls 0x414960 on them, which is `psys::update` (s00413320 had `render_child`).
- **psys.** `scene2` (psys.h) is `rgl::scene` (+0x1c4 is the particle mesh pool); `view_t : mesh_pool`
  is one struct `mesh_pool` (meshes +0xc8, axis +0x30c, eye +0x31c). The corner uvs are `vec2 uv[4]`
  (the slices had `float uv[4][2]` and `float u, v`).
- **skin = material_def.** s00418a70's `material_def::load` (0x4190d0) is slot 2 of skin's vtable: it is
  `skin::load`. Its `vec3f v34` + `int f40` is skin's `vector4 color` (+0x34; load reads x, y, z).
  `tex[5]` are `texture*`; +0x30 is the `material*` (shader) that mesh::render reads, +0x68 a callback.
- **key_array = vec_array.** `key_array` {data, keys, n} (s00416ef0) and `vec_array` {v, idx, n}
  (s00417850, s00418150) are one 12-byte record; its 16-byte values are `rgl::vector4` (s00417850's
  `vec16` with an out-of-line ctor and `f4` give the same bytes). `read_vec_array` (s00417850's name for
  0x4173b0) is `r_key_array_yzwx`.
- **texture.** s00417850 (padding), s00418150 (full fields) and s00418a70 (loaded/kind) are one layout;
  `release()` and `unload()` (0x4167c0) are one function.
- **effect_set.** `box[4]`/`loaded` (s00417850) are `h[4]`/`resolved` (s00416ef0).
- **text_stream2 / tstream2** (0x54a890): one class, `text_stream2`.
- `resource_manager`: `find_v90` -> `find_effect`, list `v90` -> `effects`; `get_material` returns
  `skin*`, `shaders` holds `material*`. `resolve_effect_sets` takes `ID3DXEffect*` (was `void*`).

## Removed slice-local tricks / stand-ins

- `list_res::_Buynode` (a hand-written copy of `std::list<T*>::_Buynode(next, prev, val)`) and the
  `/EHa` builds: `template class std::list<res_obj*>` gives both 0x418050 and 0x4187b0 byte-exact.
- Wrapper functions `copy_vec_arrays`, `copy_backward_vec_arrays`, `fill_vec_arrays`: replaced by explicit
  instantiations of `std::_Copy_opt`, `std::_Copy_backward_opt`, `std::fill` (the real symbols).
- `vec12_size` (`__fastcall` stand-in): it is `std::vector<T>::size()` for a 12-byte `T`
  (explicit instantiation with `key_array`, a guess: any 12-byte element gives these bytes).
- Stand-in ctors `Psys::Psys`, `texture::texture`, `effect_set::effect_set` and out-of-line `~skin`,
  `~material` that the slices defined to get the vtables and deleting dtors emitted: cl emits a real
  `??_E` for a class when the object contains an array new of it. Two stand-in functions remain
  (`deleting_dtor_standin` in mem_stream.cpp, `deleting_dtor_standin2` in resource_manager.cpp: array
  news + `delete[]`, not in the binary). The resource dtors are inline in resource.h (the original's
  deleting dtors inline them; material's scalar path calls the out-of-line copy 0x4168f0).

## Kept stand-ins (flagged)
- `user_path` (resource_manager.cpp): a local copy of the other object's static 0x4bab70, needed for its
  ESI argument convention in `options::save`. The audit lists it as a local copy (it agrees).
- The two `deleting_dtor_standin*` functions above.
- `E8`, `res_obj`: element types of `_Uninit_fill_n` / `std::list` instantiations whose users are not in
  these objects' surviving code (E8 may be s00418a70's 8-byte `pair_item`).

## Resolution audit

`resaudit_all.py` over the four manifests (91 rows): 83 agree, 1 unverified, 7 not-audited.
- unverified: 0x415240 `stream_base::rewind` (`lea ecx,[ecx+0x20]; jmp 0x42bef0`: two instructions,
  below the audit's 3-instruction pairing run). The same in its slice (s00414960).
- not-audited (tool limitation, papercut): for `src:` rows `find_symbol` does not read the module
  manifest, only source markers and names, and no marker can name `??_E...` (0x417600, 0x417690,
  0x417730, 0x4177b0, 0x417f20, 0x417fb0) or `??4key_array` (0x417850). The same holds for dxframework:
  its rows 0x401870 and 0x401a30 are "audited" on the stand-in `MBRecord34_DeleteArray` and on
  `~CD3DApplication` instead of the deleting destructors.
- `work/migrate/scratch_rgl_main/audit_manifest.py` re-runs the audit with our symbol taken from the
  manifest row: **all 91 rows agree** except 0x415240 (unverified, above), including the 7 rows above.
- Local copies (agree; the original calls the real function): `vector<vertex60>::_Insert_n` -> 0x40fed0
  (0x413bb0), `allocator<E8>::construct` -> 0x40f610 (0x418150), `allocator<int*>::construct` ->
  0x436d80 (0x4184b0), `user_path` -> 0x4bab70 (0x4181c0). Same as in the slices.

## Equivalence harness

Run on the module sources as `equiv.py <module> <va> --src src/rgl/<module>.cpp --flags "/O2 /MT /Gy /TP"`
(the module name as the id: no slice headers get mixed into the annotations). Logs:
`work/migrate/scratch_rgl_main/equiv_nm1.log`, `equiv_nm2.log`, `equiv_exact.log`. **No mismatch anywhere.**

Nonmatching rows:
- PASS: 0x414ae0 `noise_gradient` (400 valid, all edges), 0x414fc0 `noise_fractal` (379 valid, all edges).
- WEAK, no mismatch (harness limits, not missing behaviour):
  - 0x413d30 `mesh_pool::get_mesh`: 302 valid, 17/18 edges on both builds; both miss only the `r == 0`
    path (the default-material lookup through `g_resmgr` faults on random state). The slice version is
    also WEAK under the current edge gate (15/18 on both builds); its "PASS" was the old instruction gate.
  - 0x414960 `psys::update`: orig 5/8, ours 5/6 (+2 unreachable past a C++ throw); the particle loop
    calls the original `emit`, which throws on random pointers. The slice version: orig 4/8, ours 4/6.
  - 0x418e90 `resolve_shader`: 215/800 valid, orig 16-17/20 edges (it was UNSUPPORTED, 0/400 valid, in
    its slice; under today's harness the slice version is WEAK 18/20).
  Re-running with `--inputs 2000/3000` did not close the missing edges.
- UNSUPPORTED: the four catch funclets (not callable; verified byte-identical instead).
- The partial 0x413e50 was not re-run (partial; FAIL only through stack garbage, see psys.partial.txt).

Byte-exact rows sampled: PASS 0x4172d0 (`tstream3` ctor, the improved one), 0x417310; WEAK with no
mismatch 0x418a70, 0x418b30, 0x418d60, 0x415880, 0x415590; UNSUPPORTED (harness) 0x415290 (writes
through `fopen`'s result), 0x417e50 (malloc/arity), 0x4190d0 (`_purecall` in the stream vtable).
0x417850 cannot be addressed by equiv.py (same `find_symbol` limitation as the audit).

## Flags
1. The `.data`/`.bss` noise tables (0x5a8160, 0x5a8260, 0x5d6f98) stay `extern` in noise.h; they are
   presumably defined in noise.obj.
2. Link order: the "alphabetical per folder" observation of docs/tu_map.md does not hold for these names
   (psys < noise < text_stream < mem_stream < resource_manager in the binary). The names are content
   names, not original file names.
3. The forward-call rule needs the caller's object to be bottom-up. text_stream.cpp has one internal
   call (0x415790 -> 0x415100, backward); the next object is not bottom-up (`r_int` 0x415930 calls
   `mb_parse_int` 0x4159a0 forward, a helper with no other caller), and `string::read_from` (0x4151e0,
   defined here) is inlined in 0x415d60 (next object). This is consistent with `read_from` being inline
   in the string header, but also with 0x4150f0..0x4163xx being one object compiled caller-first; the
   0x415900 boundary is therefore less certain than "high".
4. `skin`/`material` naming (see above) and `named_object`'s second slot name `type()` are provisional.

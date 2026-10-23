# rgl/core migration (0x415900-0x41ef19)

The staging slices `s00415320` (rows from 0x415900), `s00415c80`, `s00416540`, `s00416ef0` (aabb rows), `s00418a70` (rows from
0x419190), `s00419420`, `s00419a30`, `s0041a2c0`, `s0041ac10`, `s0041b5a0`, `s0041bf40`, `s0041c760`, `s0041ce80`, `s0041d6f0`,
`s0041db20`, `s0041e470`, `s0041ee20` (first two rows) are merged into three modules. 0x417010..0x419190 (mem_stream.cpp,
resource_manager.cpp) was migrated earlier and is untouched. `match/slices/` is unchanged. Flags are the defaults
`/O2 /MT /Gy /TP` everywhere (no `/EH*`; the STL try/catch helpers match without it, as in the earlier parts).

## Summary

| module | range | rows | byte-exact before (slices) | byte-exact after | improved | regressed | nonmatching | partial |
|---|---|---|---|---|---|---|---|---|
| resource_io.cpp | 0x415900..0x417010 | 32 | 30 | 30 | 0 | 0 | 2 | 0 |
| resmgr.cpp | 0x419190..0x41e470 | 83 | 62 | 62 | 0 | 0 | 20 (10 functions + 10 symbol-less catch/COMDAT tails) | 1 |
| frustum_grid.cpp | 0x41e470..0x41ef30 | 8 | 7 | 7 | 0 | 0 | 1 (catch funclet 0x41ef19) | 0 |
| **total** | | **123** | **99** | **99** | **0** | **0** | **23** | **1** |

No regression: every function byte-exact in its slice is byte-exact in its module (`run_all.py --manifest`: 30/30, 62/62, 7/7).
Nothing improved: every nonmatching row is the slice's row re-measured (same or equal diff); the reasons are in
`<module>.nonmatching.txt`. Several rows (the `_Insert_n` tails, `get_render_target`, `load_stream`) were re-tried with
the unified headers (texture constructor in 14 statement orders, vec_array constructor shapes) without gain.
Resolution audit (`work/migrate/scratch_rgl_core_audit.jsonl`): 97 agree, 0 disagree, 2 unverified (0x41a9b0 and 0x41c760: the
only unpaired callees are referenced from the symbol-less COMDAT tails 0x41ab16 / 0x41c916..., which the audit does not
disassemble), 16 rows with "local copies" (ICF-folded STL helpers: ours runs where the original calls the folded copy).
Equivalence (nonmatching rows, no FAIL): PASS 0x416930, 0x419360, 0x41b960; WEAK (no mismatch) 0x4195e0, 0x419a30, 0x41b7d0,
0x41ca60, 0x41ce80; UNSUPPORTED (harness: pure-virtual stream calls / random pointers / unmodelled imports) 0x416d50,
0x41a6b0, 0x41d6f0, 0x41db20. The 0-byte rows are the symbol-less catch(...) funclets, verified identical against the
parent COMDAT tail with `work/migrate/scratch_rgl_core_funclet.py`.

## Boundaries

| edge | evidence |
|---|---|
| 0x415900 start | hard: `text_stream::r_tstr` 0x415880 (text_stream.cpp) calls `expect_token` 0x415900 forward |
| 0x415930 -> 0x4159a0, 0x4169a0 -> 0x416ad0, 0x416b10 -> 0x416bc0 | forward calls that do NOT prove a boundary: `mb_parse_int`, `stage_t::load/save` are header-inline functions, which land right after their first caller (`stage_t::load/save` are `noinline` here to reproduce it) |
| 0x417010 end of resource_io | the start of mem_stream.cpp (class change, kept from part 1); nothing in 0x415900..0x417010 calls forward past it |
| 0x419190 start of resmgr | content only |
| 0x41e470 resmgr | frustum_grid | class change only (resmgr -> frustum/world helpers); no call crosses it. Kept separate because it needs the scene headers (scene.h pulls mesh.h, not mesh_obj.h) |
| 0x41ef30 end | `0x41a2c0 -> 0x41ef30` and `0x41ca60 -> 0x41f970` are forward calls out of resmgr; scene/world_items.cpp starts at 0x41ef30 |

Findings for the TU map (not applied to the other modules, which are not mine):
- **resmgr.cpp is very probably the same TU as resource_manager.cpp (0x417bb0..0x419190)** and, by the same argument, extends
  down into mem_stream.cpp. `resource_manager` (resource.h) and `resmgr` (here) are two views of ONE object (g_resmgr, 0x5d6fa0): its
  members are interleaved (lookups 0x418870.., `get_object` 0x419540, `save` 0x419a30, `add` 0x41ba90...). The vector deleting
  destructors `??_Eskin/material/mesh/body_part` (0x417600..0x4177b0) and `??_Etexture/effect_set` (0x417f20/0x417fb0) are only
  emitted in an object that runs `new T[n]`; the array news of exactly these classes are in `resmgr::load_stream`,
  `get_render_target`, `get_small` (this module). Boundary candidate would then be about 0x417600, not 0x417010/0x419190.
- `world::release_links` (0x41ee20) and the `std::list<int>` `_Buynode` (0x41eeb0) look like the start of the scene/world_items.cpp
  object: the other members of the same list instance are at 0x41f310/0x41f640/0x41f680 (entity.cpp), so the linker's copy
  of the template sits before its users. frustum_grid.cpp may be the head of world_items.cpp.
- resource_io.cpp: the class-change cuts at 0x416540 (texture), 0x416830 (material), 0x416cd0 (skin), 0x416e40 (aabb) of the map are
  not boundaries here; no forward call separates them from the text reader, so they are one module.

## Headers

New: `resource_io.h` (token parser, matrix/skeleton serialisers, `Bone`/`skeleton`, `stage_io`, `material_io`, render globals),
`resmgr.h` (the full `resmgr` layout, `skel_anim`, `vec_array`, `lod_group`, `mesh_owner`, `mesh_x`, string/object views, the reader
class chains of `load_pack`), `frustum_grid.h`. Existing headers used: `../stream.h`, `../resource.h`, `../resource_manager.h`,
`../mem_stream.h`, `../text_stream.h`, `../noise.h`, `../particles/mesh_obj.h` (the correct mesh header: `bone_influence` with
8-byte `idx_weight`, not mesh.h's `elem14`), `../scene/frustum.h`, `../scene/world.h`. Nothing outside this directory was
edited. Layouts: Bone from 0x41b0c0 (ctor), 0x41b180 (read_frame_set) and 0x416490 (w_skeleton); resmgr from 0x41b000, 0x41ba90, 0x419a30,
0x419e90; skel_anim from 0x41b4e0, 0x41c060; lod_group from 0x41c2a0, 0x41ca60; grid fields from 0x41ed00.

Header conflicts and gaps (worked around locally, to be absorbed upstream):
1. `rgl::string` (rgl_string.h) lacks the copy constructor (0x42c110) and `range_equals` (0x42c160); `read_from` (0x4151e0) is
   header-inline in the original (resource_io.cpp inlines it in `open_read_text` through a static copy). -> `str_x`.
2. resource.h: `stage_t` has no load/save, `material` no `create_shaders` (0x416c00), `texture()` is declared out of line though the
   original expands it inline in `new texture[n]` (defined `inline` in resmgr.cpp), `key_array` has no constructor/destructor
   (`vec_array` derives it), `frame_set` keeps bone count/array as ints (f18/f1c) and its destructor (defined in resmgr.cpp, also deleting
   `table.data`), `named_object` has no slots 2/3 (load/save) -> `named_object_x`, `vec_table` has no destructor, and its
   `shape_base` conflicts with shapes/shapes.h's `shape_base` (two definitions; read_body uses local derived classes, because
   shapes.h's constructors are inline where the original calls 0x408600/0x408980/0x40c670).
3. resource_manager.h: `rgl_config` has no fields (save indexes `this` as 27 ints); the lookups' `find_v20/find_effect/resolve_shader`
   are used through `resmgr::base()`.
4. mesh_obj.h: `vertex60` names +0 `pos` and +0x30 `normal`, but the stream reads the normal into +0 and copies it to +0x30
   (tangent builder: +0 normal, +0x10 tangent, +0x20 bitangent); `lod_mesh` lacks the owning lists, constructor and name
   (`lod_group`); `mesh` lacks compute_bounds/linearise/compute_tangents/clone (`mesh_x`). scene/scene.h includes mesh.h, not
   mesh_obj.h, and declares `mesh_ext::clone` for the same 0x41c6d0.
5. stream.h: `open_write/close/open_read` exist on `text_stream` only (called here on bin_writer/bin_reader through a cast); the
   original reader chains (`~text_stream2`, `~mem_stream` user-declared and inline) differ from stream.h's, so `load_pack`
   uses `text_reader`/`file_reader` classes with the slice's layout (vtables 0x54ac88/0x54ae48).
6. Duplicates elsewhere: `resmgr` stubs in game/entity.h, game/agent/item_kind.h, game/strat/strat_ext.h; `frustum`/`plane` in
   game/renderer.h, strat_ext.h, gui_manager.h vs scene/frustum.h; rgl::scene/resmgr() function in scene.h named like my class
   (my class is global `::resmgr`, so the name is not used inside namespace rgl here).
7. Audit: the particles manifest names the 0x40fa10 instance `_Uninit_fill_n<idx_weight>`, the same name this object's
   vector<idx_weight> needs for 0x418150, so the call is declared (not instantiated) with `// 0x418150`.

## Tricks kept / removed
- Removed: every slice-local copy of the stream, string, resource and mesh classes; the slices' `E8/E4/E44` placeholder records
  (now `idx_weight`, `named_object*`, `rec44`); `res_dc` (= `Bone`).
- Kept (each needed for bytes): `__declspec(noinline)` on `~mesh_owner` and `stage_io::load/save` (cl otherwise inlines callees defined in the
  same object, the original calls them); `vec_array()` stores n first (as the inlined ctor in 0x41b4e0); `texture::texture` body order;
  `: width/height` order variants; `operator==(const char*)` for tag compares (not `equals`).
- In the original the 0x419580/0x4195e0/0x419640 members are the device-reset trio; names `restore/invalidate/linearise_meshes` are provisional.

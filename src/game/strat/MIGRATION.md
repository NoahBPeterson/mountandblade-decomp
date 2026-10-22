# src/game/strat migration: strat_entity, strat_ctx, tstring, gui_manager

Region 0x42a090..0x42d820 (68 functions; 0x42d820 starts TinyXML, `src/tinyxml`) moved from the eight slices
s00429890 (only 0x42a090/0x42a1a0 of it), s0042a1f0, s0042aaf0, s0042b080, s0042b790, s0042c0f0, s0042c9b0,
s0042d360 into four translation units. `match/slices/` is untouched. All modules build with `/O2 /MT /Gy /TP`
(no per-row flags, no /EH).

## Summary

| module | range | functions | byte-exact before (slices) | byte-exact after | nonmatching | partial |
|---|---|---|---|---|---|---|
| strat_entity.cpp | 0x42a090..0x42b6d0 | 8 | 3 | 3 | 4 | 1 |
| strat_ctx.cpp | 0x42b6d0..0x42bac0 | 3 | 3 | 3 | 0 | 0 |
| tstring.cpp | 0x42bac0..0x42cb70 | 38 | 33 | 33 | 5 | 0 |
| gui_manager.cpp | 0x42cb70..0x42d820 | 19 | 17 | 17 | 2 | 0 |
| **total** | | **68** | **56** | **56** | **11** | **1** |

No regression (every one of the 56 slice-exact functions is exact in its module), no improvement (the 12 slice
nonmatching functions stay as they were; their diffs are register allocation / operand order, unchanged by the
unified TU). `gui_manager.cpp` counts the three std::vector instance helpers 0x42d360 / 0x42d390 / 0x42d570 as
functions (they were byte-exact rows of s0042d360 under custom struct names).
The slices' 12 nonmatching rows: update_shadows, update_ground_cache, query_box, build_shadow_frame, create_shadow_mesh,
grow, reserve, get_token, get_string, strip_tags, mouse_event, gui_manager ctor.
create_shadow_mesh is `partial` here, see "Equivalence".

## Boundaries (evidence)

The region sits in the "T regime" (map: caller-first emission around 0x42a090..0x42c0b0), so forward calls inside
0x42a090..0x42b790 (render -> first_dir_light, update_shadows -> update_ground_cache / create_shadow_mesh /
shadow_tick, update_ground_cache -> build_shadow_frame -> sample_lights) do NOT prove boundaries: all of them
match when compiled together in address order. The map's TUs 34 (strat_ent), 35 (widget) and 36 (gui_manager)
were re-cut:

- **0x42b6d0 (strat_entity | strat_ctx): new split, codegen evidence.** `Ctx::ambient` (0x42b6d0) is called
  forward by `create_shadow_mesh` (0x42b080). Compiled in the same file as create_shadow_mesh (even marked
  `noinline`), cl shares the stack slot of the returned vector with other locals and the frame shrinks to 0x104
  bytes (original 0x124, size 1590 vs 1601); compiled as a separate object (as here, and in the slice's second
  file) the frame is the original's 0x124 and the size 1606. Everything from ambient on (shadow_tick,
  find_entity) is kept with it: shadow_tick calls find_entity forward, and nothing ties them to the code before.
  Confidence: medium (one behaviour, plus the forward call).
- **0x42bac0 (strat_ctx | tstring): class change, medium.** `tstring::grow` (0x42bac0) starts the string class:
  38 member functions, every intra-group call goes backwards (grow/reserve before push_back/write/append/ctors, 17/17),
  i.e. the clean bottom-up order of a library-style file, unlike the StratEnt group (8 forward calls). The StratEnt
  functions call into it forwards (0x42a1f0, 0x42b080 -> 0x42bbe0/0x42bc40; 0x42b790 -> 0x42bcb0).
- **0x42cb70 (tstring | gui_manager): end of the string class, low.** `tstring::split` (0x42c9b0..0x42cb60) is its
  last member; bump_counter (0x42cb70) starts free functions. `src/rgl/rgl_string.h` says "the same object" for the
  helpers (0x42bb50..0x42cc80); no evidence either way (0x42cb70..0x42cd20 has no calls into tstring other than
  mb_error's backward call to log_stream::flush). If the helpers (bump_counter, mb_warning, file_length,
  log_stream::flush, mb_error) belong with tstring.cpp, moving them is a pure cut-and-paste.
- **widget | gui_manager not split (map said 0x42ce50 and 0x42d1b0).** The classes interleave: gui_manager
  (set_o564/set_o568 0x42cd80..0x42ce50), widget (0x42ce50..0x42d1b0), gui_manager (reset 0x42d1b0..), with gui_timer
  and widget::refresh_all before them. No forward call crosses any point (all 0 forward calls in 0x42cb70..0x42d570), so
  the unsplit run is kept; the module is named after the biggest class.
- **0x42a090 start (given): class change Ctx::render_scene | StratEnt::render.** Not touched. 0x42d820 end: TinyXML
  (`TiXmlString::quit`), per src/tinyxml/MIGRATION.md.
- **Probable further splits (not made, no evidence):** 33 "free gaps" in the map; e.g. query_box/sample_lights/
  first_dir_light are Ctx methods and could be one file, update_shadows..shadow_tick another.

## Headers (one per class)

- `strat_entity.h`: Light, GFace/GVert/Ground, cell, Ctx, MeshEnt (the base part of the entity), StratEnt. Each field cites
  the functions that read/write it. Includes `strat_ext.h`.
- `strat_ext.h`: classes this directory uses but does not own: mesh (+ its vectors' element types), scene, frustum, skin,
  resmgr, FxDef, Psys, shapes (capsule), aabb ... only the fields used here. Global names kept (`scene`, `mesh`: the
  symbols and mangled names use them; `src/rgl/mesh.h` has the namespaced `rgl::mesh`, `rgl::scene` partial twins).
- `gui_manager.h`: log_stream, gui_timer, widget, gui_obj, pair2, gui_manager. `scene` / `frustum` / `mesh` appear there
  as sized stand-ins (0x1d8 / 0x120 / 0x260 bytes with their ctor/dtor addresses) because `strat_ext.h` needs
  `match/include/rgl.h`, which cannot be mixed with `src/rgl/stream.h` (rgl::string collides, see rgl_string.h).
- Reused: `src/game/tstring.h` (tstring), `src/game/mat4.h` (rgl::mat4, rgl::identity() by value for the shadow
  mesh render calls), `src/rgl/stream.h` (stream_base / file_writer_base / STREAM_SLOTS for log_stream),
  `src/rgl/rgl_math.h`, match/include/rgl.h.

Conflicts found in other headers (not edited, worked around locally):
1. `src/game/tstring.h` lacks `tstring::split(const char&, int&) const` (0x42c9b0) and declares `range_equals` (0x42c160) as
   `bool` although the function returns `char` (mangled `...QBEDABH0ABU1@@Z`). tstring.cpp rewrites those two declarations
   with macros while the header is included. Fix: add the split declaration, change the return type to `char`.
2. `::mesh`, `::scene`, `::frustum` (global namespace, from the slices) are defined (partially) in strat_ext.h AND as stand-ins
   in gui_manager.h; other regions (0x41ef30..0x421df0, 0x427d80..0x42a090: Ctx::render_scene, scene, frustum) will define
   the same names, and `Ctx` / `MeshEnt` / `Light` as well. They need merging once those modules exist.
3. `src/rgl/stream.h`'s `stream_base` is the slices' `bin_writer_base`-based root; `log_stream` (vtable not yet mapped in
   symbols/) derives from `file_writer_base` as the slice did.

## Canonical names (slices -> modules)

| slices | here |
|---|---|
| `mstr` (shadow.h/strat.h) | `tstring` |
| `Ctx::amb[3]` (shadow.h) / `Ctx::amb` vector4 (strat.h) | `vector4 amb` at 0x68 (ambient 0x42b6d0) |
| `ents_a/ents_b` of `StratEnt*` (shadow.h) / `MeshEnt*` (strat.h) | `std::vector<MeshEnt*>`; find_entity casts to StratEnt* |
| `tstring_vec`, `vec4_vec` (hand-written layouts) | `std::vector<tstring>` (member `strs`), `std::vector<rgl::vector4>` |
| `blob_x`, `blob_y`, gui.h `mesh` | stand-ins `scene` (0x429740 ctor, 0x41f4e0 dtor), `frustum` (0x41ef60), `mesh` (0x412a90, 0x4127f0) |
| `bin_writer_base`, `BIN_STREAM_SLOTS` (mbbin.h) | `file_writer_base`, `STREAM_SLOTS` (src/rgl/stream.h) |
| `*identity(&m)` (`mat4* identity(mat4*)`) | `rgl::identity()` by value |

Template rows: 0x42d360 is `std::vector<tstring>::_Destroy(first, last)` (a member, not the slices' `__stdcall` free
function), 0x42d390 `std::vector<tstring>::_Tidy`, 0x42d570 `std::vector<rgl::vector4>::_Buy`; the last is produced by an
explicit instantiation `template class std::vector<vector4>;` (only `_Buy` is a row). All three are byte-exact; their
names are in `symbols/slices/gui_manager.txt` (with `_Xlen` 0x43e2a0, the callee in the next object).

## Removed slice-local tricks

- Copies of tstring::grow..rfind into s0042c0f0 (to give cl the callee bodies): one TU now.
- `ambient` in its own file (s0042b080_b.cpp): now its own module (strat_ctx.cpp).
- hand-written `tstring_vec` / `vec4_vec` / `blob_x` / `blob_y`; per-slice copies of gui.h, mbbin.h, strat.h, shadow.h,
  tstring.h (8 headers -> 3).
- `mat4`/`mat3`, `mstr`, `vpush`/`vat`, `Logger`, `lod_mesh`, `mesh_list`, `mesh_pool` and the other unused parts of strat.h.

Kept: `__forceinline` helper functions for the point transform / normalise in build_shadow_frame and sample_lights
(operand order in the sums of products; they are what the byte-exact sample_lights needs); `#include <new>` in tstring.cpp
(without it cl 13.10 calls the scalar new/delete where the original calls 0x535482/0x53570d, found by the audit).

## Verification

- `run_all.py --manifest`: tstring 33/33, strat_entity 3/3, strat_ctx 3/3, gui_manager 17/17 byte-exact.
  (`chk.py` shows 0x42cc80 mb_error as "1 diff, size 145 vs 144"; run_all and the slice's manifest accept it
  as exact: the `stdout` operand relocates against `__iob`+0x20. Same output on the slice itself.)
- Resolution audit (`work/migrate/scratch_strat_audit.jsonl`, 56 rows): 55 agree, 1 unverified (0x42cb70 bump_counter,
  one instruction). The audit scans only the includes of the .cpp itself, so the .cpp files include `strat_ext.h`,
  `../tstring.h`, `../../rgl/stream.h` directly. The local-copy `_Xlen` is mapped in symbols/slices/gui_manager.txt.
- Equivalence (`work/migrate/scratch_strat_equiv.log`), flags `/O2 /MT /Gy /TP`:
  PASS: grow, reserve, get_string, gui_manager ctor (386 valid, all edges).
  WEAK, no mismatch: get_token (25/28 edges both sides), strip_tags (14/40: harness strings rarely contain '<'),
  update_shadows (15/18), update_ground_cache (1/66, needs real ground data), query_box (7/8; PASS in the slice build),
  build_shadow_frame (1/2), mouse_event (40/44). Missed edges are the same on both builds.
  **FAIL: create_shadow_mesh 0x42b080**, input 232 of the run (replay in work/difftest/fail_inputs/strat_entity_0042b080.pkl):
  a degenerate case (face->n == 0 leaves vtx[][] at 0xcccccccc in both builds; add_face then reads fault-repaired memory;
  two lanes of face normals in a block freed afterwards differ). The slice's source fails the identical replay, so this
  predates the migration; the slice's own run never reached add_face. Per the rules the row is in
  `strat_entity.partial.txt` (source believed complete); the add_face call order (1,2,0 / 0,2,3) and the vtx addressing
  were re-checked against the disassembly. The orchestrator may want to re-tag it nonmatching after a harness check.

## Problems

- The three template/compiler-generated rows depend on `symbols/slices/gui_manager.txt` (written, new file).
- `Ctx::ambient` forces a module split on codegen evidence alone; if the real TU boundary is elsewhere, strat_ctx.cpp and
  strat_entity.cpp should simply be reconsidered together (all rows still match either way, only create_shadow_mesh's
  frame size depends on it).

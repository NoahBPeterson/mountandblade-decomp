# src/game/terrain_a migration: wind, array set, game state, terrain description and the first part of the terrain object (0x461200..0x46e590)

Three ranges of the TU map (`work/migrate/tu_map.csv` TUs 53, 57-61, "wind", "arr_set", "game_state", "shadow_map",
"terrain_desc", "terrain_obj") are moved from their `match/slices/` staging slices (`s004608d0` (2 functions),
`s00461280`, `s00462e20` (2), `s00463790`, `s004640d0`, `s00464a60`, `s004650f0`, `s00465a70`, `s004661e0`, `s00466aa0`,
`s00467250`, `s00467bf0`, `s004684d0`, `s00468da0`, `s00468fc0`, `s00469a70`, `s0046a270`, `s0046ad50`, `s0046b6e0`,
`s0046bc70`, `s0046c4f0`, `s0046ca50`, `s0046d060`, `s0046d8c0`) into three translation units. The slices are unchanged.
0x461630..0x4636d0 was already migrated (`src/game/renderer.cpp`, `src/game/ui/item_ref.cpp`, `troop_inv.cpp`) and
0x46e590.. belongs to `src/game/terrain_b`.

All modules build with `/O2 /MT /Gy /TP`, no `/EH*` (the `try`/`catch(...)` STL helpers are byte-exact without it).

## Summary

| module | range | functions | exact before (slices) | exact after | nonmatching | partial |
|---|---|---|---|---|---|---|
| wind.cpp | 0x461200..0x461630 | 8 | 7 | 7 | 1 | 0 |
| game_state.cpp | 0x4636d0..0x464200 | 17 | 10 | 10 | 7 | 0 |
| terrain_a.cpp | 0x464200..0x46e590 | 131 rows (120 functions + 11 catch funclets) | 84 (+3 `$L` funclet rows, see below) | 84 | 45 (11 are funclets) | 2 |
| **total** | | **156 rows** | **101 (+3)** | **101** | **53** | **2** |

Regressions: none. Every function that was byte-exact in a slice is byte-exact in its module, checked by diffing the VA
sets. The only differences in bookkeeping are three `$L67443/$L67465/$L67483` rows (0x468216, 0x468286, 0x4682f6):
these are the 9-byte `catch(...) { throw; }` funclets of `uninit_copy24/100/52`, which the slice manifest listed because
its `/EHa` build gives the funclet a label symbol. Without `/EHa` (the module flags) the funclet has no symbol, like the 8
other funclets of this range, so they moved to `terrain_a.nonmatching.txt` with 0 diff bytes. They are byte-identical:
`work/migrate/scratch_terrain_a_funclets.py <obj>` compares the 11 funclet tails (9, 21 or 33 bytes) of our parents with the
image (relocations masked); all 11 match.

Improvements: none in byte-exact counts. Every nonmatching row of the slices keeps its diff size within a few bytes
(`0x4661e0`..`0x46d8c0` identical to the slices; `0x46bc70` 926 -> 855). Seven of the slices' "PASS" rows now read WEAK and two
FAIL, see Equivalence.

Verification (all run on the final sources): `chk.py` on every VA of each module; `run_all.py --manifest` 7/7, 10/10, 84/84;
resolution audit 101/101 `agree`; `equiv.py` on every nonmatching/partial row; `tools/status.py --no-write`.

## Boundaries

No forward call proves any boundary in the region: all calls out of 0x461200..0x46e590 to a higher address go to the CRT
(0x534xxx..0x538xxx). The table lists what each module edge rests on.

| edge | evidence |
|---|---|
| 0x461200 start (wind) | `dialog_manager.cpp` ends at 0x461200 (its own migration notes: `Agent::init_max_hp` has no tie to the dialog code). Class change, kept |
| 0x461630 end (wind) | `src/game/renderer.cpp` starts there; class change wind|renderer (low) |
| 0x4636d0 | `src/game/ui/troop_inv.cpp` ends there (class change Inv|ArrSet, low) |
| 0x464200 (game_state | terrain_a) | class change ShadowMap|TerrainDesc (low). Nothing ties the shadow map to the terrain description or the reverse; it could also be one TU with game_state.cpp |
| 0x46e590 end (terrain_a) | assigned: the range ends there. The object does not end there (see below) |

Evidence that 0x4646b0..0x46d8c0 is **one object** (hard-ish, from codegen):
- cl keeps a callee's "no side effects" knowledge only when the callee body was compiled earlier in the same TU. The slices
  had to define stand-in bodies of `terrain_cell::kind` (0x4646b0) for `pick_diagonals` (0x4666b0), `pick_triangle_types`
  (0x466930) and `accumulate_kinds` (0x466aa0), of `Terrain::spec_weight` (0x464bf0) for `scatter` (0x4650f0) and of
  `mesh_x::count` (0x468620) for `GeomBuffer::upload` (0x46d060): the original objects contained those callees. Those pairs
  cover 0x4646b0..0x46d060, and the module defines the real bodies in the same file (the stand-ins are gone).
- The cell record `Blend` of the first slice (0x4644e0 init, 0x4645b0 visible, 0x4646b0 dominant) and the terrain object's
  cell are the same 0x94-byte record (`dominant` = `kind`), so 0x4644e0..0x4646b0 belongs to the same object as the passes.
- Mixed classes in between (Mission3, Prop, Missile, Texture/material, mesh, Inv auto-equip, ShadowMap::extend/test, vector
  and stable_sort helpers) are consistent with one big object; no gap among them has any evidence of a split, so there is
  none.

The map's TU 61 (`terrain_obj`) runs 0x4645b0..0x478830, so the object crosses 0x46e590; `terrain_a.cpp` stops exactly at the
function boundary 0x46e590 (`drawer::draw` ends there; the next function is `src/game/terrain_b`'s first). The TU is one
object only by the evidence above, to 0x46d8c0; I did not verify where it ends. Placement facts for the combiner: the
`vector<E8>` stable_sort helpers (0x46b7d0.. and 0x46c260..) are interleaved with other functions in the binary but come from
one instantiation here (see "Tricks"); `uninit_fill_n*`, `uninit_copy36/136` etc. have their first callers in 0x4724c0.. (terrain_b).

Possible merges: `wind.cpp` and `game_state.cpp` could be the head of `terrain_a.cpp` (`tri_normal` 0x4612d0 is called by
`compute_normals` 0x46ad50; `ArrSet`'s dtor by the `uninit_copy_arrset` funclet 0x46c23a), but a call proves nothing for a
non-inlined function, so they stay apart.

## Headers (one layout per class, offsets cite the function that proves them)

| header | contents |
|---|---|
| `common.h` | POD element types of the vector instantiations (`elem24`, `rec36`, `elem52`, `particle100`, `rec136`, `s516`, `E8`), `g_ticks`, `noise_fractal`, `mb_error`, `rand_below` |
| `wind.h` | `Agent` (hp +0x1bc, max_hp +0x1c0 only), `wind` (0x7c bytes) |
| `game_state.h` | `ArrSet`, `Ramp`, `Block16`, `GameState` (path helpers), `ShadowMap`, `LightCam`, `ShSkin`, `ShResMgr` |
| `terrain.h` | `terrain_cell` (0x94), `terrain_quad` (0x38), `particle`/`particle_list`, `ptype`, `GroundType`, `TerrainDesc*`, `HexWords`, `BitWords`, `Pct`, `Terrain` + `terrain_obj` |
| `misc.h` | everything else in terrain_a.cpp: `Mission3`, vec/alloc stubs, `Prop`, `Missile`, `renderer` calls, `texture_x/material_x/skin_x`, `mesh_x`, `EnvRec`/`FxDef2`, `TroopEq`, `ViewParams`, `AgentEq`, `FireAgent`, `Scene`, `GeomBuffer`, `drawer` |

Layout merges (slices that used different names for the same bytes):
- **Cell record 0x94** = slices' `Blend`, `TerrainCell`, `terrain_cell`: `kind()` loops `+0x4c..+0x80` (floats 1..14 of a
  15-weight array at +0x48), the sco layers 0..10 are stored at +0x58 + 4 * i (`load_layer`), `f28` of the passes is the
  normal's z (`nrm` +0x20, accumulated by `compute_normals`), `+0x30/+0x38` hold the flow `disp`/`dir` vec2s (0x468fc0) and the
  `(x*0.7, y*0.7, 0, 1)` vector of `init` (= `disp.xy`, `dir = (0, 1)`).
- **Quad record 0x38** = slices' `Seg`: `Seg::init` (0x464760) is `terrain_quad::init`; `f14`/`f24` of `count_low_quads` are the
  z lanes of the two triangle normals.
- **Terrain layers**: `Terrain::layer[14]` at +0x28 (0x464f10) and the slices' `tlayer layers[11]` at +0xb8 (0x467250,
  type at +0x10) are the same records seen 0x10 bytes apart; `bit58` is `layer[1].f8` and `f78` is `layer[2].type`
  (0x4664e0). Both views live in one union.
- `GroundSpec` (0x464bf0, 0x4650f0) = `ptype` (0x466cd0): the table at `*0x5e28f8`; `ttype` (0x467250) = `GroundType`
  (0x8ed4df8). `ScatterSet` = `particle_list`'s tail (`nsrc` +0x2800004, `src[32]` +0x2800008); `set1/set0/set2` = `list_a/list_b/list_c`.
- `TerrainDesc` of 0x4656c0 (`mode`, `elev_idx`, `level_idx`, `e[]`) = `TerrainDesc` of 0x464440 (`n`, `n_minus2`, `n_minus1`, `layers[]`).
- `Probe::test` 0x468c80 and `BodyHolder::extend` 0x468c20 are `ShadowMap` members (`w2/h2` +0x20/+0x24, `surf` +0xc, `cam`
  +0x1f0; `read_depth` is `ShadowMap::read_pixel`); `Fx::reset` 0x4686a0 is `EnvRec::reset` (`f60/f64/f68` = `k0/k1/k2`).
- `terrain_obj` derives from `Terrain` (same bytes, no vtable) so the mangled names of both slice families are kept.

## Header conflicts (shared headers are unchanged; workarounds are local)

- `Agent` (`wind.h`, only hp/max_hp): `src/game/agent/agent.h` and `src/game/agent3/agent_types.h` define other Agents; neither names +0x1bc (agent_types.h has `int f1bc`).
- `ShadowMap`, `Terrain`, `particle`, `particle_list`, `ptype`, `Probe`, `TCell`: `src/game/terrain_b/{shadow_map,terrain}.h`
  declare smaller versions of the same classes for 0x46e590+. Mine are the complete ones (`terrain_b` says so: "the rest belongs
  to terrain_a's functions"); they must be merged into one when the two modules are linked (`g_bit` is `unsigned __int64` in both).
  Their `ptype` has `boxes` at +0x1c, mine `f1c`; their `particle_list.items` vs my `items` + `nsrc` + `src[32]` agree on offsets.
- `renderer` (`misc.h`): partial redeclaration with only the calls this object makes (`src/game/renderer.h` needs `rgl.h`, which collides with `rgl_math.h` used by the ui/rgl headers).
- `src/rgl/mesh.h` cannot be used (no `setup_material`, opaque `elem30`); `src/rgl/particles/mesh_obj.h` is its drop-in replacement and is included. Missing members live on `mesh_x : rgl::mesh`, `mesh_list_x`, `lod_mesh_x`, `texture_x : texture`, `material_x : material`, `skin_x : skin`, `frustum_x : rgl::frustum`, `TroopEq : Troop`, `ItemRefX : ItemRef` (the mangled names of these rows say `_x`).
- `ItemKind` (`item_ref.h`) has no name for +0x20c (read with `item_f20c()`); `Troop::flags30` is unsigned (`pick_slot_item` casts the item flags to int for the sign test).
- `rgl::string` vs `tstring`: `stream_base::r_tstr/w_tstr` take `rgl::string*`; calls cast.
- `g_ground_types` (0x8ed4df8) and `g_lod_max` (0x8ee3ac4) are outside the `// 0x` annotation range, `g_log` is bound too: `symbols/slices/terrain_a.txt`.

## Tricks removed / kept

Removed (the real bodies are in the same file now): the stand-in `terrain_cell::kind`, the same-TU copy of `spec_weight`, the
duplicated `sort_e8` per slice, per-slice header copies (`terrain.h` x3, `mesh.h` x4), `TerrainCell`/`Terr`/`Probe`/`BodyHolder` aliases, `/EHa`.

Kept / new, each with its evidence:
- `terrain_obj::cell_of` (`void`) is a second declaration of `Terrain::cell_of` (0x464b40, returns `out`): with the `int*`
  result `emit_particle` (0x466cd0) is 7 bytes off, with a void result it is byte-exact (the original caller ignores the
  result). `#pragma comment(linker, "/alternatename:...")` binds the two names (as `src/rgl/shapes` does).
- `terrain_cell::kind` is `__declspec(noinline)` and defined before its callers (body visible, call kept).
- `Terrain::rand_avg4` is a member (called with `ecx = this`, which `seed_brushes`' 374 bytes need: the free function gives 372 bytes and 220 diff bytes instead of 142).
- `fill_vertices` reads the parent vectors as `local_a1 ? v50 : parent->get50()` (an inlined test plus the out-of-line call);
  plain `get50()` (the noinline loop of mesh_obj.h) is 987 bytes off instead of 43.
- the `vector<E8>` stable_sort helpers come from `void sort_e8(...)` and the `list<int>::_Buynode` from `make_int_list()`: two
  functions that are not in the binary and carry the COMDATs; the original has the six stable_sort functions in two groups with
  other functions between them (0x46b7d0/0x46b860, 0x46c260..0x46c4a0). Both must be dropped (`/OPT:REF` does) or replaced by the first
  caller when the real callers (terrain_b: 0x472870, 0x472ad0, 0x474410) are linked.
- the `uninit_copy*` helpers take a `std::_Nonscalar_ptr_iterator_tag` argument (the `_Ucopy` thunks pass one; the slices defined them with `int`).
- two compile-order effects stay as in the slices: `Terrain::scatter` needs `spec_weight` defined before it, `GeomBuffer::upload`
  needs `mesh_x::count` defined before it.
- `ViewParams`, `EnvRec` inline ctor/dtor and `FxDef2::set_env_count` are exact as in the slices.

## Equivalence (tools/difftest/equiv.py, flags `/O2 /MT /Gy /TP`)

PASS: 0x461480, 0x463a80, 0x463bc0, 0x463c20, 0x463cc0, 0x463e90, 0x464070, 0x4644e0, 0x4647e0, 0x464960, 0x465e70, 0x466ed0,
0x467250, 0x467940, 0x467fb0, 0x4686a0, 0x46ad50, 0x46b940, 0x46baf0, 0x46c920.

WEAK (no mismatch; both builds miss the same unreached edges, a harness steering/coverage limit; the slices recorded most of
these as PASS with other random inputs): 0x463790 (the `g_use_shaders` gamma edge), 0x464bf0, 0x464d50, 0x464f10, 0x4650f0,
0x465540, 0x4656c0, 0x4657c0, 0x4658c0, 0x465ee0, 0x4661e0, 0x4666b0, 0x466fa0, 0x468c80 (66.7% coverage), 0x468da0, 0x468fc0,
0x469a70, 0x46bc70, 0x46c4f0, 0x46d060 (28.5%: needs live D3D buffers).

UNSUPPORTED: 0x46b190 and 0x46d8c0. The resolution audit pairs instructions by position and reports `g_bit` high/low dword
(0x46b190: the original loads the high dword first) and three `g_tss_*`/`g_stage_arg` operands (0x46d8c0) as disagreeing. The slices'
sources have the same pairing (`equiv.py s0046ad50 0046b190`, `equiv.py s0046d8c0 0046d8c0` print the same lines); I read the
original at 0x46df27..0x46e07d: the four cache blocks (0xa17e78, 0xa17e7c, 0xa17e6c, 0xa17e70) and the `g_stage_arg[0]/[5]`
arguments are in the same order and with the same constants as the source, the instruction schedules differ. Also the 11 funclets.

**Moved to partial (equiv FAIL, new with this module's harness run; the slice code is identical)**:
- 0x465af0 `Terrain::sample`: 1/387 inputs differ by a signed zero (x lane -0.0 vs +0.0). Tried all 64 combinations of `<` / `<=` for the six comparisons: none passes.
- 0x46a270 `terrain_obj::spawn_particles`: 5/359 inputs differ in the uninitialised 4th lanes of the particle frame (0xcccccccc in the original, 0 here).
Both rows are complete in intent and 887 vs 781 / 2778 vs 2780 bytes.

## Not done / open

- No nonmatching row was brought to byte-exact; every one is a register-allocation or scheduling difference that the slices already tried 6-10 variants on. The `void cell_of` / member `rand_avg4` finds show that callee signatures seen by the original caller matter; the same idea was not tried on the other WEAK rows.
- `GeomBuffer::upload` (0x46d060): ours is 138 bytes shorter (cl tail-merges two identical blocks, the original cannot); `drawer::draw` (0x46d8c0): 3254 vs 3278.
- Where the TU ends past 0x46d8c0 and whether wind/game_state belong to it are unproven.

# src/game/mission5 migration (0x51eb90..0x5305b0), wave M4

Region of the TU map (`work/migrate/tu_map.csv` TUs 114 `troop_rec`, 115 `game_2`, 116 `mission_5`, 117 `mesh_3`; all "T" regime).
The staging slices (`match/slices/s0051eb90` .. `s0052fe40`, 31 of them) are unchanged. Every module builds with `/O2 /MT /Gy /TP`
(one flag set; the slices needed no per-row flags either). 96 index entries = 92 functions + 4 Ghidra split pieces
(0x520b2d, 0x524cb6, 0x524d3a, 0x52ba1d).

## Summary

| module | range | rows | exact before (slices) | exact after | nonmatching | partial |
|---|---|---|---|---|---|---|
| troop_rec | 0x51eb90..0x520b20 | 13 | 4 | 4 | 9 | 0 |
| game_2 | 0x520b20..0x525160 | 23 functions + 3 split rows | 7 | 8 | 15 + 3 split rows | 0 |
| mission_5 | 0x525160..0x52cf60 | 38 functions + 1 split row | 15 | 15 | 23 + 1 split row | 0 |
| mesh_3 | 0x52cf60..0x5305b0 | 18 | 0 | 0 | 18 | 0 |
| **total** | | **92 functions + 4 split rows** | **26** | **27** | | **0** |

No regression: the 26 byte-exact slice VAs are byte-exact in the modules (`run_all.py --manifest` 4/4, 8/8, 15/15). One row improved:
0x5248e0 `SceneFile::SceneFile` (the scene record constructor, slice `RecY::RecY`, 34 bytes off) is now exact: the two embedded
`file_stream` members are the real `stream_base` subclasses of `src/rgl/stream.h` with their inline constructors. Every function that had
compilable source still has it; nothing became partial. Measured against the slices' recorded diff sizes: 53 of the 67 nonmatching rows
have the same diff; 9 slice rows had recorded no diff (0 or the function size: 0x527da0, 0x528060, 0x528410, 0x52ae70, 0x52b5c0, 0x52ba10,
0x52bdb0, 0x52bf40, 0x520b20's interior), the module's measured numbers are in the rows; four rows got larger because the stream / record
classes are now the shared ones: `start_new_game` 1104 -> 1138 (the 18-byte WinLicense marker of the original is part of the gap), `more_start`
1448 -> 1469, `build_scene` 1278 -> 1303, `load_game_data` 2347 -> 2461 (all four stay nonmatching with unchanged call sets); one got smaller
(`build_map_trees` 6899 -> 6895).

## Boundaries

Evidence is the call graph (`work/migrate/facts.json`; in the T regime a function is followed by its callees, so forward calls prove
nothing) plus class names:

- **0x51eb90 start**: first function of `Scene` after `Game::load_scene_parties` 0x51eaa0 (which calls it); class change only.
- **0x520b20** (`Game::build_map` after `Animator::reset`): class change only, no evidence either way. troop_rec and game_2 can be one
  file (the new-game sequence `start_new_game` 0x51f1a0 / `build_map` 0x520b20 / `build_scene` 0x520d60 is one family, called from
  `LoadingPanel::update` as GameStep4 / 4b / 6); they are kept apart so a wrong boundary costs one file move.
- **0x5249f0 (map boundary TU115 | TU116) is NOT kept.** `Game::load_game_data` 0x5236f0 calls 0x524360, 0x5243c0, 0x524420, 0x524690,
  0x524710, 0x524860, 0x5248e0, 0x5249f0, 0x524a70 and 0x524d60 directly, and 0x524a70 -> 0x524bc0 -> 0x524cd0, 0x524d60 -> 0x524f00: the
  record constructors / readers and the `std::vector<tstring>` helpers follow their one caller in the order of its calls, so they are
  helpers of the same object. game_2 therefore ends at 0x525160, `Game::handle_event`, the next function that is neither called from
  there nor part of the loaders (class change only). The map's "QuestRec | Faction" class change at 0x5249f0 is a coincidence of the
  record family.
- **0x525160 .. 0x52cf60 is one module (mission_5)**: `Screen::tick` 0x5261e0 calls 0x527c90, 0x527f90, 0x528410, 0x5284a0, 0x529130,
  0x5295a0, 0x529d80, 0x529ec0, 0x52ae70, 0x52b300, 0x52b5c0 and 0x52ba10 in turn (all followed by their own callees), and
  `handle_event` 0x525160 -> 0x5257f0 -> 0x526070 / 0x526130. Class names change every few functions (Game, Agent, Screen, GameData,
  MissionHud, Mission, Terrain, SceneBrush, Elem): none of those changes is a boundary.
- **0x52cf60 / 0x5305b0 (mesh_3 start and end)**: class change only (Elem | Mesh, Mesh | MapEditorDialog). `Mission::update_ui`
  0x52ba1d calls 0x52cf60, 0x52f250 and 0x5305b0, so the editor entry points may belong to the caller's object (the map editor,
  0x52cf60..0x531590, would then be one module with `map_editor_dialog`); left as the map has it.
- The four Ghidra split rows are one function each: 0x520b20 + 0x520b2d (`build_map`, 215 bytes; 0x520b2d is in
  `symbols/not_functions.txt`), 0x524cd0 + 0x524d3a and 0x524bc0 + 0x524cb6 (catch funclets, in `not_functions.txt`),
  0x52ba10 + 0x52ba1d (`Mission::update_ui`, 620 bytes in our build).

## Class identities (slice names were provisional and partly wrong)

| slice name | here | evidence |
|---|---|---|
| `GameState` / `World` / `Game` (0x51f1a0..0x520d60) | `Game` of party/game.h | one `this` with `Game::triggers` +0x139e8 (the sound bank, `snd_bank::randomize` 0x462790), `simple` +0x139f4 (the slices' `DelayList`, 0x30-byte items with a `Timer` at +0x20), `troops` +0x13a00, `timer13b18` / `timer1a298` (the "clock" fields), `scene_parties` +0x1405c (the random table) |
| `Mgr` / `PartyMgr` at Game+0xf8 | `Scene` of party/game.h | `read` 0x51eb90, `clear` 0x51eda0, `get` 0x51ee50 (+ `lookup` 0x48ad00, `add_page` 0x493830) |
| `Anim` (0x520520) | `Animator` of mesh_slot/mesh_slot.h (`g_anim` 0x8ed4f60) | `set_anim_a` 0x473800 is the slice's `begin`, `end_anim` 0x4eb660 its `end` |
| `Container` at Game+0xcc, `order_begin/end` at +0x13a64 | `SpriteTable` (`pages` +0x18), `TrackVec` (`TrackRec` 0x5c) | game.h |
| `Store` (map.txt, 0xa732b8) | `MapStore` (local; `V36`/`V136` of world_grid.h are its vectors) | `NavGraph` / `NavCore` are other views of the same object |
| `g_scene` 0xa732b4, `NodeMgr`, `Point`, `Node`, `Wind` | `CellGrid` (`g_level`), `NavData` (`core` +0xb0), `NavVert`, `NavCell`, `Wind` of terrain_b/cell_grid.h | field-for-field (verts +4 0x24 bytes, cells +0x14 0x88 bytes, winds +0xbc) |
| `Faction`, `TroopRec`, `QuestRec`, `RecY`/`SceneRec` text records (0x524360..) | `FactionFile`, `TroopFile`, `QuestFile`, `SceneFile` (local) | records.h's `RecT` / `RecS` / `RecQ` / `RecY` are their destructor-side views: `RecS` is 0x504 bytes (the record is 0x508), `RecT` is padded to 0x5330 (the record is 0x300), `RecY` hides the embedded streams, none has a constructor, so none can be `new[]`'d |
| `Screen` (0x52ae70, 0x5261e0, 0x529ec0), `ViewObj`, `Game` (0x525160) | the Mission object 0x5e292c (+0x29c order state, +0x654c base z, +0x6698 NavMesh, +0x672c..0x6740 order meshes) / the game-loop screen | offsets agree with mission.h where it declares them |
| `Mesh` (0x52da20..) | `PolyMesh` of terrain_b/poly_mesh.h (`mface` = MVert, `medge` = MEdge, `mpoly` = MPoly; +0x18/+0x28/+0x38 = NavCore verts/faces/edges) | `rebuild` 0x470920 = `build_adjacency` |
| `MissionSub::save` 0x524f00 (s00527c90) = `Local::read` (s00524cd0) | one function, `Local::read`, which writes (w_float / w_uint / w_int slots) | slot numbers: the slices' `op`/`wi`/`wf` are `w_float`/`w_uint`/`w_int` |

Stream slots (the slices numbered them by byte offset): +0x38 `r_float`, +0x3c `r_bool`, +0x44 `r_u64`, +0x48 `r_i64`, +0x4c `r_int`,
+0x54 `r_tstr`. The slices' `Scene::read` helper calls (`r_int` at +0x38, `r_raw` at +0x4c, ...) are re-spelled with the shared names.

## Headers

- troop_rec.h, game_2.h, mission_5.h, mesh_3.h: this directory. `tstring` (src/game/tstring.h), the streams (src/rgl/stream.h),
  `rgl::vector4` / `rgl::matrix` (rgl_math.h), `rgl::mesh` (rgl/mesh.h), `Game` / `Scene` / `SimpleTag` (party/game.h), `Troop` /
  `TroopRec` (include/troop.h), `ItemRef`, `snd_bank`, `Timer`, `CellGrid`/`Wind`/`NavData`, `Animator`, `IfaceB`, `ptr_tag` are the
  shared ones; no `match/include/rgl.h`.
- **Canonical headers extended (pure additions, method declarations only, no layout change)**: `src/game/party/game.h` (`Scene::lookup/add_page/clear/read/get`,
  `Game::module_path/start_new_game/more_start/build_map/build_scene/release_orders/build_map_trees/load_game_data`, `SimpleTag::arm`);
  `src/include/troop.h` (`TroopRec::auto_equip/reset_items/apply_items/level_up/raise_skills/raise_stats`);
  `src/game/mesh_slot/mesh_slot.h` (`Animator::end_anim/reset`).
- `symbols/slices/{troop_rec,game_2,mission_5,mesh_3}.txt`: vtables, hex-named globals out of the annotation range, the slice-local
  class names of the mangled callee names. `symbols/not_functions.txt`: 0x520b2d, 0x524d3a, 0x524cb6.
- **mission_5.h / mesh_3.h keep the slices' partial views of the same few objects** (mission 0x5e292c, game data 0x5e2930, agents, message
  log, menu widgets, the polygon mesh) as separate classes with a per-slice suffix (`Mission_s261e0` = the layout of slice s005261e0). Every
  global that was declared with a view type (`g_mission`, `g_game`, `g_gui`, ...) is declared once as a raw pointer / byte array
  (`extern void* g_mission; // 0x5e292c`) and the slices' names are cast macros over it. Merging these views into mission.h /
  agent_obj.h / game_data.h / poly_mesh.h is open work: each view lists only the offsets its slice proved, so a merge is mechanical but
  touches ~6,000 lines of nonmatching code.
- Name clashes found while merging: `g_map` is 0x5e292c in s0052f250 and 0x5e28f8 in s0052fe40 (the latter is `g_map_pts` here);
  `g_axis_x/y` differ between s005284a0 and s0052fe40 (different modules, no clash); `src/game/party/game.h` / `terrain_b.h` /
  `include/game_data.h` cannot be included together (two `g_game` types), so game_2.h declares `g_level` itself.

## Tricks kept, tricks removed

Kept (each needed by a byte-exact row or a documented nonmatching shape):
- `bin_writer_i` (mission_5.h): `bin_writer` with its constructor inline, as `GameData::save_scene` 0x527c90 builds it on the stack (the shared class has the
  out-of-line 0x417c10); `file_text` (game_2.h) is the three-level inline-constructor chain of the text reader (as src/rgl/core/resmgr.h).
- `__declspec(noinline)` on `read_map`, `release_orders`, `Scene::clear/read/get`, `PartyRecIO::read`, `TroopRec::*` helpers (T regime: the original calls them out of line).
- `volatile int* hp_max_p` in `TroopRec::level_up` and `*(volatile int*)&s->item` in `init_slot` (register-pressure levers from the slices).
- `stream_base* sp = &stream` in `start_new_game` (the original calls the text reader's slots through the vtable).
Removed: per-slice `tstring` / `stream_base` / `bin_writer` / `text_stream` copies (mbbin.h), `rgl.h`, slice-local `World` / `Entity` / `resmgr` / `Wind` /
`Container` copies in troop_rec and game_2, `_fltused` definitions, `GameState`-style slice names.

## Verification (what was run)

1. `tools/matching/chk.py` on every module: 4 + 8 + 15 + 0 exact rows (the 0x520b20 row is verified with `run_all`; `chk` sees only the
   13-byte Ghidra extent).
2. `run_all.py --manifest`: 4 / 4, 8 / 8, 15 / 15 byte-exact. `compile_all.py --filter src/game/mission5`: 4 / 4.
3. Resolution audit (`resaudit_all.py --slices src:...troop_rec,game_2,mission_5`): 27 rows, **26 agree**, 1 unverified (0x520b20
   `Game::build_map`: the index extent of the function is the 13-byte Ghidra piece, so the tool cannot pair the 215-byte body; the slice row
   is "unverified" for the same reason, and the 215 bytes are byte-identical), 0 disagree.
4. `sweep_nonmatching.py` (equiv `--fp-tol`, full mismatch tally) on all 69 nonmatching rows (troop_rec 9, game_2 18, mission_5 24, mesh_3 18):
   PASS 14 + PASS-FP 1, WEAK 26 (no mismatch; unreachable edges are stream/vtable paths the harness cannot populate), UNSUPPORTED 24 (harness
   limits stated in each row: 0 valid inputs because of stream virtual calls / `RaiseException` / `malloc`, the 8 KB tree builder, the
   resolution audit pairing a shorter stream with the original's at a shifted instruction, the 4 split / funclet rows without a symbol of their own), FAIL 4:
   0x5234e0 `ChainGraph::build_chain`, 0x52dfd0 `poly_ray`, 0x52e510 `toggle_flag` (only `eax` at the return of a void function; the callers
   overwrite it, explained in each row) and 0x52ba10 `update_ui` (1 of 653 inputs: ours writes edx; the row says so). Every mismatch signature of the
   four is the single explained one. Each row keeps the slice's text and appends the sweep verdict.
5. `tools/status.py --no-write`: migrated 3269 / 3272; byte-exact 2183.

## Known problems and open work

- Merge the per-slice views of mission_5.h / mesh_3.h into the canonical headers (above).
- `Game::start_new_game` has an 18-byte WinLicense marker (`EB 10 "WL" ...`) in the original; not reproducible.
- The 8 KB `Game::build_map_trees` and the 6.5 KB `Screen::tick` remain reconstructions (frame layout and x87 scheduling differ).

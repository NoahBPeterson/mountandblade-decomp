# src/game/options_panel migration: item records, config dialog, mesh editor, application, mission screen, settings pages, options panel (0x4ce960..0x4d92c0)

Wave M4. Region of the TU map (`work/migrate/tu_map.csv` TU 106 `options_panel`, 86 map rows: 82 real rows, see below).
The 23 staging slices in `match/slices/` (s004ce960 .. s004d8c20, of which s004d8c20 also holds 0x4d92c0..0x4d9440, which
belong to the next TU and are not taken) are unchanged and stay the reference. Every module builds with
`/O2 /MT /Gy /TP` (one flag set; the `/EHa` of the three `_Uninit_fill_n` rows is not needed).

## Summary

| module | range | rows | exact before (slices) | exact after | nonmatching | partial |
|---|---|---|---|---|---|---|
| mission_items.cpp | 0x4ce960..0x4cf100 | 13 | 4 | 4 | 9 | 0 |
| config_dialog.cpp | 0x4cf100..0x4d0620 | 3 | 0 | 0 | 3 | 0 |
| trigger_records.cpp | 0x4d0620..0x4d0c20 | 1 | 0 | 0 | 1 | 0 |
| module_load.cpp | 0x4d0c20..0x4d1410 | 12 (11 functions + catch funclet 0x4d120a) | 3 | 3 | 9 | 0 |
| mesh_edit.cpp | 0x4d1410..0x4d3330 | 6 | 0 | 0 | 6 | 0 |
| ragdoll_models.cpp | 0x4d3330..0x4d3720 | 3 | 2 | 2 | 1 | 0 |
| world_app.cpp | 0x4d3720..0x4d4b80 | 12 | 5 | 5 | 7 | 0 |
| hud.cpp | 0x4d4b80..0x4d5ba0 | 8 | 4 | 4 | 4 | 0 |
| quest_page.cpp | 0x4d5ba0..0x4d63b0 | 4 | 2 | 2 | 2 | 0 |
| video_page.cpp | 0x4d63b0..0x4d6b80 | 1 | 0 | 0 | 1 | 0 |
| keys_page.cpp | 0x4d6b80..0x4d7370 | 8 | 4 | 4 | 4 | 0 |
| options_panel.cpp | 0x4d7370..0x4d92c0 | 11 | 8 | 8 | 3 | 0 |
| **total** | | **82** | **32** | **32** | **50** | **0** |

The map lists 86 rows; four are not functions: 0x4d4554 is inside the `push imm32` at 0x4d4553 of `Core::msg_proc` and
0x4d4a28 / 0x4d4ab8 / 0x4d4b4b are the catch funclets of the three `_Uninit_fill_n` instances (all four in
`symbols/not_functions.txt`; the funclets are inside their parents, which are byte-exact). The one interior row kept is the
catch funclet 0x4d120a of 0x4d11a0 (not in `not_functions.txt`; nonmatching with its own diff). The map's status column
(10 exact / 37 nonmatching / 27 partial / 12 missing) was stale: the real baseline is 32 byte-exact rows
(`run_all.py` on the slice manifests of the range) and every other row is complete, compilable source.

No regressions: every row that was byte-exact in a slice is byte-exact in its module (32 of 32, the same VAs; checked by
`run_all.py --manifest` on each module manifest). Nothing became partial. The diff-byte counts of the nonmatching rows moved
as follows (module vs slice, positional bytes as printed by `chk.py`): improved 0x4cf7b0 2682 -> 2086, 0x4d4140 2983 -> 1857,
0x4d2df0 1243 -> 1212, 0x4d27d0 1470 -> ~1449, 0x4d1880 1081 -> 1068, 0x4d4b80 1361 -> 1332, 0x4d0240 820 -> 813,
0x4d8c20 1303 -> 1301, 0x4d63b0 1638 -> 1636; slightly worse: 0x4cf100 1485 -> 1501 (the string layout is now the real one),
0x4d3ec0 412 -> 423, 0x4d1d80 1196 -> 1257 (it now follows the original's evaluation order, see "Bugs found"), 0x4ce960
108 -> 109 (the real 0x120-byte `rgl::frustum`), 0x4d5410 1061 -> 1075. The rows that listed as "improved" or "worse" are all
nonmatching before and after.

## Boundaries (what the evidence says)

`work/migrate/facts.json` over the range: the only forward calls and tail jumps are 0x4ceea0 -> 0x4ceec0 (the deleting
destructor calls the releasing one of the same class), 0x4d0620 -> 0x4d0bf1 (a jump inside the function: the WinLicense
`EB 10 57 4C` marker), 0x4d5290 -> 0x4d5410, 0x4d5410 -> 0x4d5940, 0x4d5a20 -> 0x4d5ba0, 0x4d5f20 -> 0x4d6270 / 0x4d6300 /
0x4d6dd0, 0x4d63b0 -> 0x4d6b80 / 0x4d6dd0, 0x4d6e50 -> 0x4d6f90 / 0x4d6fd0 / 0x4d7000 / 0x4d7120, 0x4d7120 -> 0x4d7270 /
0x4d7370 / 0x4d7d10, 0x4d7370 -> 0x4d7610, 0x4d7d90 -> 0x4d7de0 / 0x4d7e00, 0x4d7e00 -> 0x4d7e50, 0x4d8000 -> 0x4d86d0 /
0x4d8c20 / 0x4d9260. All of them but the first two are in the T regime (0x4d3e50..): helpers right after their caller,
forward calls prove nothing. The B-regime part (0x4ce960..0x4d3e50) has no forward call at all. So there is no hard TU
boundary evidence anywhere in the 0x4ce960..0x4d92c0 range, and none entering it (no forward call from 0x4ce870 or
before; the start is the map's class change ItemRec | ItemD_vt, kept: 0x4ce960 is the constructor of the record that
0x4ce870 / 0x4ce8d0 build next to).

The map says one TU. The content is mixed (item records, a launcher dialog, a polygon-mesh editor, the application object,
the mission screen, settings pages, the character export), which looks like several objects (the map itself lists 11
possible inner splits), but the evidence for splitting is weak, so the following is a decision about types, not about the
original object files: the range is cut into twelve contiguous files at points where the classes change AND where the slices'
local views of shared stubs (`Widget`, `MainMenu`, `Hud`, `Mesh`) would otherwise have to be unified without proof. Any two
neighbouring files may be merged by concatenation (the functions are in binary order inside each file; a merge only needs
the stub classes of the two files reconciled, the first conflict being `Widget`/`MainMenu` of quest_page / video_page /
keys_page / hud). In detail:

- **mission_items | config_dialog at 0x4cf100**: class change only (the walkers 0x4cf070.. end the record code).
- **config_dialog | trigger_records at 0x4d0620**: 0x4d0620 `init_mission_trigger_records` builds thirty records with the
  constructors of 0x4ce960.. / 0x4cee50.. (module 1) and of other TUs; it carries two WinLicense marker blocks
  (`EB 10 ... "WL  "`, a jump over data inside a function: the tu_map tool's known false forward call 0x4d0620 -> 0x4d0bf1).
- **trigger_records | module_load at 0x4d0c20**: class change (module-system loaders); its container helpers
  0x4d10f0..0x4d1360 sit in the same file because the loader constructs those vectors.
- **module_load | mesh_edit at 0x4d1410**: class change (record loaders -> polygon-mesh editor).
- **mesh_edit | ragdoll_models at 0x4d3330**: class change (`Mesh::add_quad` 0x4d2df0 is a mesh member; `RagBone::load` 0x4d3330 is
  the first TinyXML user).
- **ragdoll_models | world_app at 0x4d3720**: class change (record constructors -> mission object `Big` / application object).
  0x4d3720 .. 0x4d4b80 is the start of the T regime (0x4d3e50 `TreeW::erase` is the first T row): the vector fill helpers
  0x4d49c0 / 0x4d4a50 / 0x4d4ae0 sit after their first caller (`Core::msg_proc`, which only calls them through the
  vector code of other TUs: no direct call; kept in binary order at the end of world_app.cpp).
- **world_app | hud at 0x4d4b80**: class change (application -> `Mesh::on_selection_changed`, which belongs to the editor
  class of mesh_edit.cpp by its first argument and calls 0x4d1d80 `add_triangle`, but sits here by address; it includes
  mesh_edit.h).
- **hud | quest_page at 0x4d5ba0**, **quest_page | video_page at 0x4d63b0**, **video_page | keys_page at 0x4d6b80**,
  **keys_page | options_panel at 0x4d7370**: class change per page (`QuestPage`, `VideoPage`, the key table page, the panel).
- **End: 0x4d92c0.** `HashTab::operator[]` (0x4d92c0, in slice s004d8c20) is the next agent's.

Class-name unifications made while moving (aliases the slices used for one object):

| here | slice names | evidence |
|---|---|---|
| `Mesh : PolyMesh` (mesh_edit.h; `MVert` / `MPoly` / `MEdge` of terrain_b/poly_mesh.h) | `Mesh` with `Vtx` / `Face` / `Edge` (five variants), `QuadMesh` with `E24` / `E100` / `E52`, `FaceM`, `EdgeM` | same offsets +0x18 / +0x28 / +0x38 and element sizes 0x18 / 0x64 / 0x34; one `this` in 0x4d1410..0x4d4b80 |
| `options_panel` (one class) | `options_panel` of s004d7370 and of s004d7d10 | key words at +0, +8, +0x10, +0x18 of 0x4d7370 and the menu words of 0x4d7d10 are one table; +0x18 is `key_next` and `menu_choice` (union) |
| `GameIo : Game` (two member functions of `Game`) | `game_data::char_export_write` (s004d86d0), `game_globals::export_character` / `import_apply` and `Game::import_char` (s004d7d10, s004d8c20) | 0x4d86d0 and 0x4d8c20 are called with `ecx = g_game` |
| `outfile` / `outfile_a` / `outfile_b` | `stream_text` (s004d86d0) | slot 10 of the export file's vtable (+0x28) is the line writer |
| `Core::Core()` = `GameApp::GameApp()` 0x4d3ec0, `Core::msg_proc` = `GameApp::msg_proc` 0x4d4140 | `GameApp`, `Application` | `Core : CD3DApplication` in ../config/core.h has the same layout (+0x348 font, +0x390 module names, +0x590 mode) and vtable 0x5948e0; not merged, see below |
| `Big` (0x4d3720 / 0x4d3880) = `World` (0x4d51d0) = `Mission` | `Big`, `World`, `WorldBase` | the object `g_5e292c` = `g_mission` (../mission.h); not merged, see below |
| `RagBone` | `Bone` (s004d2df0) | the ragdoll bone with mass and cone limits, not rgl's skeleton `Bone` (src/rgl/core/resource_io.h) |
| `ControlKeyTable`, `KeysPage` | `KeyTable`, `VideoPage` (s004d6b80) | `KeyTable` is src/include/key_table.h's key-frame table, `VideoPage` is video_page.cpp's class |

## Headers

Canonical headers included (all unchanged): ../tstring.h (`tstring`, replacing the slices' seven local `tstring` variants,
three of which had the buffer pointer at the wrong offset: `p` at +0 in s004cf100 / s004cf7b0 / s004d5ba0), ../../rgl/rgl_math.h
(`rgl::vector4`, replacing `match/include/rgl.h`), ../../include/frustum.h (`rgl::frustum`, 0x120 bytes, constructor 0x41ef60;
replaces the local 0xd4-byte stubs of mission_items.cpp and trigger_records.h), ../../rgl/stream.h (`stream_base`,
`file_reader_base`, `text_stream`, `text_stream2`, `bin_writer`), ../terrain_b/poly_mesh.h (`PolyMesh`, `NavCore`,
`MVert`, `MPoly`, `MEdge`, `vat`, `MeshExt`), ../int_vec.h (`IntVec`), ../party/game.h (`Game`, `Faction`, `quests`,
`troops`, `cur_troop`), ../../include/troop.h (`Troop`, `TroopRec`), ../../include/game_recs.h (`QuestRec`),
../../tinyxml/tinyxml.h (`TiXmlElement`, `TiXmlHandle`). **No canonical header was edited.** (I started to add the two
declarations and three field names to ../config/core.h for the application constructor and `msg_proc`; the triggers agent has
uncommitted edits in that file, so I reverted mine and kept a local view; the declarations to add are in the table above.)

Headers of this directory (classes that exist nowhere else): mesh_edit.h (editor view of `PolyMesh`: `Mesh`, `NewEdge`,
`NewVert`, `PartFace`, `PartEdge`, `QuadEdge`, `Vec4`, `OwnedIntVec`, `MeshCalls`), trigger_records.h (the thirty record
stubs of 0x4d0620), module_load.h (the module-system records and `text_reader_l`, vtable 0x54ac88 with its constructor
inline as 0x4d0c20 builds it), hud.h (`Widget`, `MissionItem`, `MissionSource`, `Hud`, `MenuScreen`, `TimerList`),
options_panel.h (`options_panel`, `ctr`, `ui_control`, `text_store`, `MenuRec`, `outfile*`, `ini_file`, `GameIo`).
mission_items.cpp, config_dialog.cpp, world_app.cpp, quest_page.cpp, video_page.cpp and keys_page.cpp keep their stub classes
in the .cpp. Layout offsets are cited per field or per function in the comments.

### Local views that duplicate a canonical class (not merged; to be folded in by whoever owns the class)

- `Big` / `World` in world_app.cpp / hud.cpp are `Mission` (../mission.h). mission.h models the vectors at +0x48.. as nested
  types (`AgentTable` at +4, `Pool<RecJ>` at +0x30, `NavMesh` at +0x6698) and has no constructor or destructor for 0x4d3880 /
  0x4d3720; moving the two functions onto it needs the field mapping of `Pool`, `TerrainDesc` and the nine clock words.
- `GameApp` in world_app.cpp is `Core` (../config/core.h). `Core` lacks `Core()` (0x4d3ec0), `msg_proc` (0x4d4140), the
  `+0x34c` word, the launcher flag `+0x350`, the module picture `+0x354`, the window `+0x364` and the module count `+0x38c`
  (all inside its `pad348` split; pure additions, offsets unchanged) and would have to lose the manual vptr store (0x5948e0).
- `Widget` has four incompatible stubs: hud.h (vtable +0x2c activate, +0x30 value, +0x38 float, +0x44 used, +0xc0 kind),
  quest_page.cpp and video_page.cpp (older, +0x44 changed, set_text at +0xa4 / +0xac) and keys_page.cpp (the 2026-10-22
  rewrite, the verified one: +0x6c scale, +0x70 position, +0x94 default, +0x98 colour, +0xa4 text, +0xb0..+0xbc show / hide /
  enable / disable). quest_page and video_page still use the older offsets of the slices (their rows are nonmatching).
- `MainMenu` has three stubs (hud.h, quest_page.cpp / video_page.cpp, options_panel.h `OptMainMenu`) for 0xa73980;
  `StringTable` / `StrTable` (`lookup` 0x43a1e0, `find` 0x4c6010) four, against ../dialog_manager.h's `StrTable`; `Game`
  stubs inside quest_page.cpp use the canonical `Game` now. `tstring` in keys_page.cpp is `typedef rgl::string tstring`
  (as that slice was written), which is the same 16-byte class.
- `mesh_edit.h` replaces the five slice meshes by `PolyMesh`; `Mesh` adds only member declarations.
- `RagBone` / `Body` and the `model_a` / `model_b` records exist nowhere else.

## Bugs found by the equivalence runs and fixed

- **0x4d4b80 `Mesh::on_selection_changed`** (hud.cpp): the slice read `v[idx[1]]` relative to `v = &verts[idx[0]]`; the
  original indexes from the first vertex (`verts[idx[1]] - verts[idx[0]]`: it loads `[this+0x1c]` once and uses `idx*0x18`
  against that base). With the old form equiv FAIL (a steered input where all four selected vertices coincide: the
  original returns false, the slice's reads walked off to a different triple and called `add_triangle`). Fixed; the row
  is WEAK (no mismatch) and 29 diff bytes better. Also `rebuild()` (0x4cb840 of 0x4d22d0) is `build_adjacency` (0x470920)
  in this function (the slice named it right here; mesh_edit.h's `rebuild` is the other function).
- **0x4d1d80 `Mesh::add_triangle`** (mesh_edit.cpp): the original sums the dot product as `cz*nz + cy*ny + cx*nx` (the
  slice wrote x first), rounds `d2x/d2y/d2z`, `cx`, `cy` to floats (spills) and keeps `d1` and `cz` in x87 registers;
  the swap of `tri[0]`/`tri[2]` depends on the sign of that sum, so with the slice's order a near-zero sum flipped the swap
  (equiv FAIL on a memory-pool compare of `tri`). The source now has the original's association and volatile spills for
  those five values; the local list `found` is the `OwnedIntVec` (inline `push_back`, out-of-line `_Insert_n` 0x43e7b0) instead
  of a `std::vector<int>` whose own `_Insert_n` copy allocated differently under the harness. Row WEAK, no mismatch.
- **0x4d9260** (options_panel.cpp): `export_path` is a thiscall member (`ret 4`, `this` unused) of the panel, which
  `import_export` calls on `this`; the slice had it as a `__stdcall` free function. Same bytes, byte-exact in both.

## Verification

- `chk.py` on every VA of every module: 32 MATCH (the same 32 as the slices); the rest as in the `*.nonmatching.txt` files
  (each row has its measured diff-byte count and the equiv verdict).
- `run_all.py --manifest src/game/options_panel/<m>.manifest.txt`: 4/4 mission_items, 3/3 module_load, 2/2 ragdoll_models, 5/5
  world_app, 4/4 hud, 2/2 quest_page, 4/4 keys_page, 8/8 options_panel (config_dialog, trigger_records, mesh_edit and
  video_page have no byte-exact row; their manifests only carry a comment).
- `compile_all.py --filter src/game/options_panel`: 12 / 12 sources compile.
- Resolution audit (`resaudit_all.py --slices src:...`, the eight manifests): 32 rows, 30 agree, 1 unverified and 1 not
  audited (the 5-instruction wrappers `walk50` / `walk68` / `walkb0` / the deleting destructor 0x4ceea0 have nothing to
  resolve), 0 disagree, 1 local copy (keys_page `user_path` 0x4bab70, kept as the slice had it). One row needed a symbol line:
  0x4d7e50's `"%s:  %d"` literal. `symbols/slices/s004f0820.txt` maps the same literal name to 0x598ac0, where the image
  holds `"%s: %d"` (one space), so the audit saw a disagree (the same one is on the slice's row `s004d7d10` before this
  migration); `symbols/slices/options_panel.txt` maps it to 0x596ba4, the real copy. With that line s004f0820's own row
  (0x4f12be, source `"%s:  %d"`) now shows its real literal bug (original 0x598ac0 is the one-space string); fixing that
  slice is the owner's.
- Equivalence (`sweep_nonmatching.py`, `--fp-tol`, full mismatch tally, 50 rows): **0 FAIL**. PASS 8 (0x4cea10, 0x4d1360,
  0x4d3880, 0x4d5290, 0x4d6b80, 0x4d6f90, 0x4d7270, 0x4d7370), WEAK 13 (0x4cefe0, 0x4d11a0, 0x4d1230, 0x4d1410, 0x4d1880,
  0x4d1d80, 0x4d22d0, 0x4d27d0, 0x4d2df0, 0x4d3720, 0x4d3e70, 0x4d4b80, 0x4d86d0; all "no mismatch", coverage below the
  gate), UNSUPPORTED 29 with the harness reason in the row (undefined vtable/heap pointers of constructors, constructors
  of other TUs that the harness stubs, 0/400 inputs valid for the big UI functions, unresolved references of stub
  classes). The two rows that FAILed first are the bugs above.
- `tools/status.py --no-write`: run last, see the report.

## Tricks kept, tricks removed

Kept: the inline `__asm` tail of `Dialog::proc` 0x4d0240 (the C spelling of the switch/return shape does not compile under
cl 13.10; the slice's note); `__declspec(noinline)` on the T-regime helpers (`ctr::key_name` 0x4d7610 defined after its caller
0x4d7370, `options_panel::update_cursor` / `refresh_texts` / `draw_faction_report`, `ControlKeyTable::restore`,
`flora_alloc::construct` and its siblings); the explicit `~tstring()` calls of video_page.cpp (the slice's device to reproduce
the original's destructor placement); `ItemSource` in hud.h (the callee 0x4d5410 seen without its body, so `handle_buttons`
calls it with the C convention instead of cl's whole-TU register convention); `MeshCalls` / `MeshExt` casts in mesh_edit.cpp
for the same reason; the volatile spills in `add_triangle`; local copies of `user_path` and `decode_utf8` in keys_page.cpp
(static helpers in the original with register conventions; the audit lists them as "local copies").

Removed: `match/include/rgl.h` (every file), the slices' seven `tstring` structs, the two local `frustum` stubs, the
`/EHa` of `_Uninit_fill_n` (not needed), the local `IntVec` (now ../int_vec.h), the `Vtx` / `Face` / `Edge` /
`FaceM` / `EdgeM` / `E24` / `E52` / `E100` / `QuadMesh` copies, the slice-local TinyXML header, the local `Game`, `GameL`,
`QuestRec` stubs.

Open items (not done): the `Core` / `Mission` / `Widget` merges above; 0x4d0c20 `load_data_files` (was `load_module_files`, the name of 0x50d5e0) is 262 bytes short without
EH frames and 34 long with `/EHsc` (the original has one frame per stream temporary), not tried further; the uninitialised
bytes of `Edge` / `Face` temporaries that `add_triangle` etc. copy into the vectors differ from the original's stack
garbage (harmless; the only observable of the 0x4d1d80 FAIL before the fix).

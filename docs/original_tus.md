# Original translation units of the 1.011 exe versus our module split

Read-only analysis (nothing under `src/` was touched). Inputs: `work/migrate/facts.json` (per-function calls, absolute
data references, register-argument scan), the shared analysis image (`resolve.R.Image()`: `.CRT$XCU` table at
0x5a8008..0x5a80c0 and relocations), `src/**/*.manifest.txt` / `*.nonmatching.txt` for our module of each function
(3277 rows cover 3265 of the 3298 functions below 0x534300) and the unmatched bytes (the diff-byte column of the
nonmatching rows; `work/near_miss.json` has no byte count for 130 of its rows, so I used the row files). The tree is
being edited while I read it (for example `map_panel.cpp` already holds the former map_update/map_picker/map_info, with map_tooltip kept separate because `update` needs its bodies opaque), so
counts are a snapshot. Prior work that this builds on and does not repeat: `docs/tu_map.md` (121-TU map, signals
measured on the six DX9 and three TinyXML objects) and the per-directory MIGRATION.md / script NOTES.md files.

## 1. What each kind of evidence is worth (what I checked, not what I assumed)

| evidence | what I found | how I use it |
|---|---|---|
| Custom register convention (arg in EAX/ESI/EDI) | The `regargs` scan flags 15 functions below 0x534300. Six are false positives: `lea ebx,[ebx]` is a 3-byte NOP that reads EBX (0x4759c0, 0x4c54f0, 0x4c5670, 0x4b8fc0) or the function is a vtable/callback (0x4d4554, 0x4f1460). Genuine (I disassembled 0x4350c0, 0x43a280, 0x4904a0 and 0x487b00; the others have same-module callers and I took them from the scan): 0x4035d0, 0x4350c0, 0x43a280, 0x462ac0, 0x48d830, 0x487b00, 0x4904a0, 0x49d160, 0x49d360. Premise (docs/codegen.md 6.3): cl 13.10 without LTCG only picks such a convention for a function with internal linkage, so every caller sees its body. **A single calling module therefore ties callee and caller into one TU.** | strong when callers are in one module; weak when callers are in several modules far apart: 0x487b00 `decode_utf8` has 8 caller modules from 0x4520d0 to 0x4fd700, which cannot be one TU (the original linked with `/OPT:ICF`, docs/matching.md), so it is a header-inline helper whose identical copies were folded. Same caveat for 0x4350c0 (5 callers in two modules). |
| Forward call crossing a module boundary | Holds in the "B" (callee-first) regime, which covers 0x401000..0x4cd000 (docs/tu_map.md: 75/75 intra-TU calls go backward on the DX9 objects). In the "T" regime (0x408000-0x40d000, 0x41f000-0x42b000, 0x4d4000-0x533000) forward calls are normal and prove nothing. Also unreliable when the callee is a std template COMDAT (the "high" boundary at 0x49b1c0 rests on `insert` -> `_Insert_n`). Of 216 module-run boundaries: B 104 without and 16 with a forward call; T 54 without and 41 with. | used only as evidence AGAINST a merge (B regime, non-COMDAT callee); the absence of a forward call is not evidence for a merge. |
| File statics / data ownership | Of 1011 distinct data VAs referenced from code, 490 are used by one of our modules, 163 by two, 81 by three, 142 by four or more, 135 also by non-module code. The game is mostly extern singletons (g_game 0x5e2930 and so on), so "shared by only A and B" is what any extern looks like. Only a handful of VAs are informative (below). `g_script_error` is the one proven file static (all of 0x5e2590..0x5e25a4 lies inside 0x49cc60..0x4b10d7). | tie, medium at most; listed per candidate. |
| `.CRT$XCU` initialisers | 46 entries. The constructed global's referrers are mostly many modules (g_resmgr 0x5d6fa0: about 60 modules). Informative ones: 0x5a9f20/0x5a9f28 (initialiser 0x547a60) are referenced only by cls_map and mission_window; 0x5e2580/0x5e2584 (initialisers 0x547a20/0x547a30) only by terrain_a and terrain_render; 0x5a9be0/0x5a9be8 only by cls_map. | tie, medium: a dynamically initialised global is defined in the TU that holds its `$E` function. Which TU it is cannot be read from the address alone (the `$E` functions are all in the post-CRT area), so this says "same TU or extern". |
| `.rdata` literal pools | Not used. With `/GF` string pooling, `/Gy` COMDAT floats and `/OPT:ICF`, a literal shared by two objects sits with the first user, so a shared literal does not distinguish one TU from two, and docs/tu_map.md measured 96.6% in-order for single-TU items without any merge signal. I did not run a per-module first-literal check. | none. |
| COMDAT placement | `??_G`/`??_E` deleting destructors and std template instances sit in the address range of the object that first defined them, whichever module of ours uses them. 241 function rows lie in runs away from their module's main run (section 4 lists the notable ones; some are an artefact of how our modules were cut, not only COMDAT placement). The vector deleting destructors are only emitted in an object that runs `new T[n]` (src/rgl/core/MIGRATION.md), which is a usable per-object marker I applied by hand only to resmgr. | explains "stray" rows; evidence of membership only for the vector-deleting-destructor marker. |
| Single-caller helper counts | `Operation::execute` (0x49fa10) alone has 182 callees that nothing else calls. The count is high for any hub, and a counter-example exists: `fx_keys` has 12 single-caller helpers of `fx_anim` yet the visibility-twin test shows `update_poses2` saw them as external. | not used as evidence; reported for context only. |
| Visibility twins | A caller that is byte-exact only when its callee is declaration-only proves two objects (docs/tu_map.md lists seven: key_groups, agent_pick, troop_adjust, terrain_render, fx_keys, poly_mesh, the three mesh rays). The converse, "exact only with the real body", proves a merge, as camera+script_interpreter did. | the only codegen-based evidence; I cannot compile, so I only cite rows where someone already measured it. |

## 2. Inferred boundaries

Boundaries that I can support from address plus call or register evidence (confidence is mine):

| boundary | evidence | conf |
|---|---|---|
| 0x4035d0, 0x407250, 0x42f720 | forward calls inside B regime on the DX9 / TinyXML ground truth | high (known objects) |
| 0x4b14b0 (end of the interpreter TU, start of party_2) | `Operation::execute` 0x49fa10 calls 0x4b14b0, 0x4b14f0, 0x4b1700, 0x4b1d80 forward, B regime, non-COMDAT callees | medium-high (window 0x4b1160..0x4b14b0) |
| 0x437770 (agent.cpp / troop_adjust), 0x4320b0, 0x433040, 0x4681b0, 0x470a00, 0x52dae0/0x52dcc0/0x52dfd0, 0x422db0 | visibility twins (callee body hidden from the caller in the original) | medium-high; the twin test is a byte-diff observation |
| 0x460160 (end of agent_logic) | `PlacedObj::update_1c0` 0x461200 is called forward from 0x44c210; `ScriptState::expand_token` starts at the next 16-aligned address | medium |
| 0x4ca600 (gui_edit / mesh_2) | forward call 0x4ca550 -> 0x4ca600 (src/rgl/mesh2/MIGRATION.md) | medium |
| every other module boundary of ours in the T regime (about 95 of 216) | no usable evidence: class names only (about 50% precision on the ground truth). These are the least reliable of our cuts. | none |

The full per-boundary table (all 216 consecutive module runs, with forward and backward call counts) is in the appendix.

## 3. A. Groups of our modules that were probably one original file, most valuable first

"Unmatched" is the sum of the diff bytes of the modules' nonmatching rows. "Unblocks" names rows where the notes or
the codegen rules point at same-TU information; I have not compiled any of this.

### A1. Agent logic: agent_ai + agent_combat + phys_set + agent_fx_sound + agent_logic, and the tail of agent_stats (about 0x43a280..0x460160). Unmatched about 95,000 B (agent_logic 77,679). Confidence: medium-high that these are one TU; low that it reaches back to agent.cpp.
- Evidence: `ui_weapon_class_name` 0x43a280 takes its type in EAX and a buffer in ESI. Its only caller is `process_blow` 0x457b40 (agent_logic). A register-convention function with one calling TU is internal to it, so 0x43a280..0x457b40 is one object. It contains the end of agent_stats, agent_ai, agent_combat, phys_set, agent_fx_sound and the first 0x13000 bytes of agent_logic. None of the five internal boundaries has a forward call (B regime), 142 calls cross between the modules, 43 functions are called only from another member module, and `PlacedObj::update_1c0` is the only forward target and sits after the range. src/game/agent4/MIGRATION.md reached the same conclusion; agent_logic.cpp still holds only 0x444c50..0x460160 and the other four are separate files.
- Not supported: `slot_class_impl` 0x4350c0 (EAX; callers 0x4385e0 in agent_stats and four in agent_logic) would push the start back to 0x4350c0, across the visibility-twin boundary at 0x437770 (`Troop::adjust` exact only when `falloff` is opaque). One of the two is wrong, and a header-inline function folded by ICF reconciles them (the 0x487b00 pattern). So the lower end is 0x43a280, not 0x4350c0.
- Unblocks: not `process_blow` itself (agent_logic.cpp already defines `ui_weapon_class_name` as a `static noinline` at line 5934, and agent_stats.cpp carries a stand-in caller, so the convention is already reproduced there); the gain is for the rows in agent_ai, agent_combat and agent_fx_sound that call agent_stats or agent_logic helpers through a declaration only (cl keeps register and side-effect knowledge of callees defined earlier in the TU, docs/codegen.md 6.3). Whole-TU context "did not move any row" in the one test src/game/agent3/MIGRATION.md recorded, so expect part of this to be neutral.

- 2026-11-12 check (scratchpad n_tu): visible-callee signs across the file give no consistent boundary; making 22 earlier functions opaque to the tail costs Agent::update 0x45f660 its match (Agent::ai_idle_tick 0x44df30 alone: 0 -> 157), so 0x43a280..0x45f660 is one object. 0x45ff30 has no evidence either way. Visible bodies that hurt rows here (Troop::nibble, ItemRef::stat_1f4/stat_214/mesh_name, Skeleton::all_shapes, Map10007::lookup, rotate_about_*) belong to other units and stay opaque.
### A2. Front of the script interpreter: world_data (0x4904a0..0x4958f0) + mesh_hud + obj_tbl + widget_mgr + widget_node + script_interpreter (0x4904a0..0x4b1160). Unmatched about 15,000 B beyond the interpreter's own 57,403 (mesh_hud 10,898; widget_mgr 2,027; widget_node 741; obj_tbl 316; the world_data part). Confidence: medium for 0x4904a0..0x49ad20, medium-low across 0x49ad20.
- Measured 2026-11-06, not merged (src/game/script/NOTES.md, Merge test): world_data loses exact rows 0x490a30, 0x490fa0 and 0x491e50, which need `lookup` opaque where execute needs it visible; the case gain comes only from obj_tbl's `spawn_scaled`/`spawn_scaled2` being visible, available without a merge; mesh_hud, widget_mgr and widget_node showed no gain.
- Evidence: `agent_of` / `trigger_item_mesh` 0x4904a0 takes EAX; its only callers are in `Operation::execute` 0x49fa10 (script NOTES.md open problem 3: "takes EAX in the original; a plain call here"). By the same premise as A1 the object spans 0x4904a0..0x49fa10 at least. Inside that span 46 functions in world_data, obj_tbl and widget_mgr (and 0x495970 in mesh_hud) are called only from 0x49fa10, and mesh_hud's 0x495b20 is called only from inside the interpreter range (0x49bbb0, 0x49bea0), so the range is interpreter glue and not a class implementation file. Script NOTES.md already puts the TU's start at or below 0x49a0b0. The file static `g_script_error` is confirmed only inside 0x49cc60..0x4b10d7.
- Against: `DropDown::key` 0x49aca0 calls `DropDown` 0x49ad20 forward in B regime (the map's medium boundary), and mesh_hud holds large widget vtable methods (0x496100..0x4985f0) that nothing calls directly, which is not what script-op helpers look like. If 0x49ad20 is a real boundary, then 0x4904a0..0x49ad20 is one object and widget_node + interpreter another, and `agent_of`'s TU is not the interpreter's; the script NOTES' claim that the interpreter's TU holds `agent_of` would then be wrong. I could not decide this from the image.
- Unblocks: row 0x4904a0 (53 B, blocked because "the callers live in the script interpreter TU"), plus any `Operation::execute` case that calls these helpers while their bodies are opaque (the notes list `GameCompanion::create_companion_party`, `Party::max_stacks` and the validators). Execute alone is 45,965 of the interpreter's 57,403 unmatched bytes, so this is the largest single lever once the frame (0x1ee8 vs 0x1ec0) is solved.

### A3. cls_map (both runs) + mission_window, probably + progress_bar (0x4d92c0..0x4e87d0). Unmatched 3,991 B (14,958 with progress_bar). Confidence: medium for cls_map+mission_window, low-medium for progress_bar.
- **Measured 2026-11-06: not one file.** Merging cls_map + mission_window lost an exact row (0x4db1f0 or 0x4db810, depending on the UiA ctor's form) with no gains; adding progress_bar gained nothing; 0x5a9f20/24/28 are used only by 0x4db1f0 and its initialiser, so they don't tie cls_map in. 0x4db1f0 calls UiA's ctor out of line and 0x4db810 expands it, which points to a real boundary at 0x4db810. Kept separate.
- Evidence: our cls_map.cpp has two runs (0x4d92c0..0x4dae40 and 0x4db810..0x4e3eb0) with mission_window (0x4dae40..0x4db810) wedged between them, so the current cut is wrong somewhere. Globals 0x5a9f20/0x5a9f28, constructed by initialiser 0x547a60, are referenced by cls_map and mission_window only. For progress_bar: its first function 0x4e3eb0 (ctor) is called only by `new_progress_bar` 0x4e3d50 at the end of cls_map (helper-after-caller, T regime), and 6 calls go back into cls_map.
- Against: only one call crosses the cls_map/mission_window pair (0x4db660 -> 0x4db810, `new_button`); T regime, so no forward-call test.

### A4. resource_manager + resmgr (0x417bb0..0x41e470). Unmatched 2,387 B (resmgr 2,228). Confidence: medium.
- Evidence: src/rgl/core/MIGRATION.md already argues "very probably the same TU": the two are views of one object (g_resmgr 0x5d6fa0) with interleaved members; `??_E` vector deleting destructors for skin/material/mesh/body_part/texture/effect_set are only emitted where `new T[n]` runs, and those are in `resmgr::load_stream`, `get_render_target`, `get_small`; 19 calls from resmgr back into resource_manager, no forward call, 11 functions called only from the other member. The `??_E` marker would put the lower boundary at about 0x417600 inside mem_stream, not at 0x417010.
- Unblocks: resmgr rows near-miss on calls into resource_manager helpers (the nonmatching note for 0x41a510 etc. uses `--transparent` to test the equivalence only).

### A5. config + trigger_vec (0x4b9e70..0x4c2c50, with config's strays). Unmatched 6,186 B. Confidence: low-medium.
- Evidence: no forward call across the boundary (B regime); 14 calls from trigger_vec back into config and 12 config functions used only by trigger_vec; class `GameApp` has methods on both sides (config 0x4bace0.., trigger_vec 0x4bee10..); `CreateGameOptionsPage` too. Class names are our own guesses (about 50% precision per docs/tu_map.md), so this is only a ranking hint. config.cpp also owns six COMDAT ctors/dtors (strays) that sit in gui_edit, mesh_2 and screen ranges.

### A6. mission_ctx + game_tick (0x513710..0x5191e0). Unmatched 14,643 B. Confidence: low.
- Evidence: T regime helper-after-caller: game_tick's first function 0x516ac0 is called only from mission_ctx 0x514d40; one forward call. Nothing else (no shared data, no register convention). Listed because of the unmatched bytes; I would not merge on this alone.

### A7. Game + troop_rec + game_2 (0x51c5a0..0x525160). Unmatched 14,170 B. Confidence: low.
- Evidence: class `Game` has methods in both game.cpp (0x51cc10..) and game_2.cpp (0x520b20, 0x5213a0); `Game::build_map`/`build_scene` are called forward from 0x51c720; data 0xa73090/0xa732b8 are shared only by these modules. The game_2 note ("a stub visible add_vertex drops the frame to 0x2ac260 but 0x413c50 is not in this TU") shows someone already has a same-TU lever pending in this file.

### A8. cls_map-style T-regime chains with no usable evidence: char_sheet (28 modules, 0x4e87d0..0x4f7d70, 62 KB in the map as one TU), options_panel (16 modules), inventory_panel (17), mesh_slot (9; 0x47aa40..0x487c40, about 36,000 B unmatched). The map (docs/tu_map.md) has char_sheet, options_panel and inventory_panel as one TU each (119, 86 and 118 functions); our split follows class names. mesh_slot (9 modules, about 36,000 B unmatched): its eight internal boundaries are all B regime with no forward call, a weak hint (the DX9 objects had forward calls at 4 of 6 boundaries). Confidence: low. I list them so that nobody mistakes our cut points for evidence.

### A9. mesh_3 + mesh_vert_ray + mesh_edge_ray + mesh_poly_ray: NOT a merge. The three splits are proven by visibility twins (docs/tu_map.md); listed because they look like one block by address. Do not touch.

### A10. map screens: already merged in the tree (`map_panel.cpp`, 0x5191e0..0x51c5a0, 12 functions). The nonmatching note records two blockers that exist only because 0x51a690 and 0x51c090 were separate; check that the row files and symbols follow the merge.

Ranking by (unmatched bytes x confidence): A1, A2, A3 (with progress_bar), A4, A5, A7, A6, A8, A9 and A10 are not candidates. Only A1 and A2 have register-convention evidence; A3 has initialiser-global evidence; A4 has a compiler-generated-function marker; the rest are hints.

## 4. B. Our modules that span more than one original file

| module | what | evidence | conf |
|---|---|---|---|
| cls_map.cpp | two runs around mission_window (see A3) | wedge | medium |
| config.cpp | strays 0x4c5110, 0x4ca830, 0x4cd720, 0x4ebea0 (`dev_buffer`/`buffer_pair`/`render_batch` ctors and dtors), 0x49b4f0 in widget_mgr range | these COMDATs sit in the ranges of gui_edit, mesh_2, screen; the first-defining object is the one whose range contains them. No bytes depend on it, only link order. | medium |
| world_data.cpp | main run 0x490030..0x4958f0 plus `??_G` stubs at 0x431400, 0x431b90, 0x431cb0 (they sit inside records.cpp) | deleting destructors follow the object that runs `new[]`; the main run is also split between the interpreter (A2) and whatever precedes 0x4904a0. | medium (for the strays), low (main run) |
| mission_ui.cpp | row 0x502300 (`??_GMissionUi`) lies between party_window's two runs | emitted by party_window's object | medium |
| party_window.cpp | two runs around that row; 0x501a20..0x502300 vs 0x502320..0x502cc0 | same | medium |
| gui_manager.cpp / widget_slots.cpp | interleaved at 0x42cb70..0x42d820 (three alternations) | the module cut among 0x42cb70, 0x42cd60, 0x42cd80, 0x42ce50, 0x42d1b0 is not supported; one object, probably with tstring | low-medium |
| rec_tables.cpp / trigger_records.cpp / ragdoll_models.cpp / gui_edit.cpp / mesh_2.cpp / config.cpp | interleaved rows between 0x4c2c50 and 0x4d3720 (e.g. trigger_records at 0x4c3120 and 0x4d0620) | same COMDAT-placement effect | low |
| shapes.cpp / agent_combat.cpp | `std::logic_error` COMDATs of agent_combat at 0x40c2c0..0x40c670 inside shapes | first-defining object | low (no bytes) |
| terrain_a.cpp | main 0x464bf0..0x467f80 plus 0x464200..0x464b40 and 0x467fb0..0x4681b0 | the strays are the Terrain/Missile code that mesh_terrain calls; classes differ (`Missile` vs `Terrain`) | low |

I did not find a module that contains two obviously separate original files with evidence strong enough to state it
(the twin-split work already removed those).

## 5. Things that contradict "same TU" in this analysis

1. **0x487b00 `decode_utf8` (EDI/EDX)** has 8 caller modules spread over 0x4520d0..0x4fd700 (700 KB). A register convention does not imply one TU when the callers are many; it is a header-inline function folded by `/OPT:ICF`. The same logic may apply to `slot_class_impl` 0x4350c0.
2. **A2 versus the forward call 0x49aca0 -> 0x49ad20** (and the "medium" boundary the map places there). Either the EAX argument of 0x4904a0 does not mean the interpreter's TU, or a forward call does not prove a boundary in this region.
3. **fx_anim/fx_keys**: 12 of 20 fx_keys functions are single-caller helpers of `update_poses2`, yet the same function was found to see them as opaque, so single-caller counts are not membership evidence (I did not use them as such).
4. **Forward calls into std COMDATs** (`insert` 0x49b170 -> `_Insert_n` 0x49b1c0) are the stated "high" evidence for the interpreter TU starting at 0x49b1c0; COMDAT order inside one object does not follow call order, so this boundary is weaker than the map says (and script NOTES.md already extends the TU below it).
5. **Class continuity** (our class names put `graph_obj` in both gui_edit and mesh_2, `TiXmlBase` in both tinyxml and tinyxmlparser) crosses boundaries that are proven by forward calls, so class continuity cannot override the call evidence. 0x4ca600 and 0x42f720 are the examples.
6. **Initialiser table**: the 46 entries belong to the TU order in the original, but the owner of each constructed global is rarely knowable, so I did not use the order to place boundaries (the map's own check, docs/tu_map.md "Unresolved", is still open).

## 6. Not done

- No compile or byte-diff experiments (read-only, no builds). Every "unblocks" line is an inference from the notes.
- No first-literal or `.rdata` order check, and no check of the `$E` atexit thunks beyond the initialiser table.
- The regime tag per boundary is the nearest-20-functions majority of `work/migrate/tu_functions.csv` (B/T), which has its own error.
- The first-pass module ranges come from row files that omit 33 functions (EH stubs and initialisers outside code); their effect on the table is nil.

## Appendix 1: our modules by address

"main run" is the module's longest contiguous run; "(+n)" is the number of other runs; strays are those runs (COMDATs or cut artefacts). Unmatched = diff bytes of nonmatching rows. Rows are manifest plus nonmatching rows.

| module | main run | rows | unmatched B | runs | strays |
|---|---|---|---|---|---|
| dxframework/d3dapp.cpp | 401000-4035d0 | 27 | 0 | 1 |  |
| dxframework/d3denumeration.cpp | 4035d0-404500 | 15 | 0 | 1 |  |
| dxframework/d3dfont.cpp | 404500-4055e0 | 10 | 0 | 1 |  |
| dxframework/d3dsettings.cpp | 4055e0-407250 | 22 | 0 | 1 |  |
| dxframework/d3dutil.cpp | 407250-407660 | 1 | 0 | 1 |  |
| dxframework/dxutil.cpp | 407660-407c10 | 7 | 0 | 1 |  |
| rgl/shapes/shapes.cpp | 407c10-40c2c0 (+2) | 150 | 6275 | 3 | 40c360-40c580, 40c670-40e940 |
| game/agent3/agent_combat.cpp | 43e190-441360 (+2) | 50 | 3485 | 3 | 40c2c0-40c360, 40c580-40c670 |
| rgl/particles/mesh_obj.cpp | 40e940-413b60 | 137 | 793 | 1 |  |
| rgl/psys.cpp | 413b60-414ae0 | 6 | 1592 | 1 |  |
| rgl/noise.cpp | 414ae0-4150f0 | 2 | 787 | 1 |  |
| rgl/text_stream.cpp | 4150f0-415900 | 24 | 0 | 1 |  |
| rgl/core/resource_io.cpp | 415900-417010 | 32 | 1 | 1 |  |
| rgl/mem_stream.cpp | 417010-417bb0 | 30 | 0 | 1 |  |
| rgl/resource_manager.cpp | 417bb0-419190 | 39 | 159 | 1 |  |
| rgl/core/resmgr.cpp | 419190-41e470 | 83 | 2228 | 1 |  |
| rgl/core/frustum_grid.cpp | 41e470-41ef30 | 8 | 0 | 1 |  |
| rgl/scene/world_items.cpp | 41ef30-41f1d0 | 5 | 0 | 1 |  |
| game/entity.cpp | 41f1d0-41fba0 | 19 | 0 | 1 |  |
| rgl/scene/entity_base.cpp | 41fba0-41fee0 | 15 | 0 | 1 |  |
| rgl/scene/entity_recalc.cpp | 41fee0-4201b0 | 3 | 0 | 1 |  |
| rgl/scene/entity_render.cpp | 4201b0-4205b0 | 2 | 8 | 1 |  |
| rgl/scene/scene_objects.cpp | 4205b0-4218e0 | 27 | 86 | 1 |  |
| rgl/scene/fxdef_set_color.cpp | 4218e0-421990 | 1 | 0 | 1 |  |
| rgl/scene/mesh_list_color.cpp | 421990-4219c0 | 1 | 0 | 1 |  |
| rgl/scene/strategic_emitters.cpp | 4219c0-421df0 | 5 | 107 | 1 |  |
| rgl/dynamic/fx_anim.cpp | 421df0-422db0 | 4 | 1505 | 1 |  |
| rgl/dynamic/fx_keys.cpp | 422db0-423c00 | 20 | 1442 | 1 |  |
| rgl/dynamic/world_grid.cpp | 423c00-425370 | 34 | 1649 | 1 |  |
| rgl/dynamic/dynamic_entity.cpp | 425370-427d80 | 42 | 3422 | 1 |  |
| rgl/scene/entity_types.cpp | 427d80-4285e0 | 16 | 32 | 1 |  |
| rgl/scene/shape_load.cpp | 4285e0-428820 | 3 | 0 | 1 |  |
| rgl/scene/fxdef_init.cpp | 428820-428b10 | 2 | 0 | 1 |  |
| rgl/scene/fx_support.cpp | 428b10-428ff0 | 6 | 0 | 1 |  |
| rgl/scene/fxdef_render.cpp | 428ff0-4291c0 | 1 | 0 | 1 |  |
| rgl/scene/fxdef_skin.cpp | 4291c0-429670 | 1 | 994 | 1 |  |
| rgl/scene/render_scene.cpp | 429670-42a090 | 5 | 1870 | 1 |  |
| game/strat/strat_entity.cpp | 42a090-42b6d0 | 8 | 2387 | 1 |  |
| game/strat/strat_ctx.cpp | 42b6d0-42bac0 | 3 | 0 | 1 |  |
| game/strat/tstring.cpp | 42bac0-42cb70 | 38 | 345 | 1 |  |
| game/strat/gui_manager.cpp | 42cb70-42cd60 (+2) | 14 | 388 | 3 | 42cd80-42ce50, 42d1b0-42d820 |
| game/ui/widget_slots.cpp | 42ce50-42d1b0 (+1) | 5 | 0 | 2 | 42cd60-42cd80 |
| tinyxml/tinystr.cpp | 42d820-42d9a0 | 4 | 0 | 1 |  |
| tinyxml/tinyxml.cpp | 42d9a0-42f720 | 66 | 0 | 1 |  |
| tinyxml/tinyxmlparser.cpp | 42f720-431400 | 21 | 0 | 1 |  |
| game/world_data/world_data.cpp | 490030-4958f0 (+3) | 91 | 6297 | 4 | 431400-431420, 431b90-431c40, 431cb0-431d30 |
| rgl/xmlcfg/records.cpp | 431420-431b90 (+3) | 21 | 0 | 4 | 431c40-431cb0, 431d30-431e10, 432010-4320b0 |
| game/mission5/game_2.cpp | 520b20-525160 (+1) | 28 | 7124 | 2 | 431e10-431f40 |
| game/terrain_b/poly_mesh.cpp | 470a00-478830 (+2) | 107 | 9703 | 3 | 431f40-432010, 467f80-467fb0 |
| rgl/xmlcfg/key_groups.cpp | 4320b0-432350 | 2 | 0 | 1 |  |
| rgl/xmlcfg/game_util.cpp | 432350-433040 | 29 | 608 | 1 |  |
| rgl/xmlcfg/agent_pick.cpp | 433040-4338e0 | 18 | 368 | 1 |  |
| game/agent/curve.cpp | 4338e0-433bf0 | 6 | 0 | 1 |  |
| game/agent/item_kind.cpp | 433bf0-434b20 | 7 | 188 | 1 |  |
| game/agent/agent.cpp | 434b20-437770 | 71 | 1897 | 1 |  |
| game/agent/troop_adjust.cpp | 437770-437830 | 3 | 0 | 1 |  |
| game/agent2/agent_stats.cpp | 437830-43b100 | 73 | 3490 | 1 |  |
| game/agent2/agent_ai.cpp | 43b100-43e190 | 27 | 7168 | 1 |  |
| game/agent/phys_set.cpp | 441360-441ff0 | 5 | 1811 | 1 |  |
| game/agent3/agent_fx_sound.cpp | 441ff0-444c50 | 29 | 4834 | 1 |  |
| game/agent4/agent_logic.cpp | 444c50-460160 | 65 | 77679 | 1 |  |
| game/dialog_manager.cpp | 460160-461200 | 21 | 0 | 1 |  |
| game/terrain_a/wind.cpp | 461200-461630 | 8 | 207 | 1 |  |
| game/renderer.cpp | 461630-4625d0 | 21 | 0 | 1 |  |
| game/ui/item_ref.cpp | 4625d0-462ac0 | 7 | 0 | 1 |  |
| game/ui/troop_inv.cpp | 462ac0-4636d0 | 14 | 436 | 1 |  |
| game/terrain_a/game_state.cpp | 4636d0-464200 | 17 | 891 | 1 |  |
| game/terrain_a/terrain_a.cpp | 464bf0-467f80 (+2) | 61 | 5106 | 3 | 464200-464b40, 467fb0-4681b0 |
| game/terrain_a/cell_of.cpp | 464b40-464bf0 | 1 | 0 | 1 |  |
| game/terrain_a/terrain_render.cpp | 4681b0-46e590 | 68 | 7124 | 1 |  |
| game/terrain_b/terrain_b.cpp | 46e590-470a00 | 5 | 7251 | 1 |  |
| game/terrain_b/mission.cpp | 478830-47aa40 | 19 | 4653 | 1 |  |
| game/mesh_slot/mesh_ops.cpp | 47aa40-47cae0 | 13 | 5324 | 1 |  |
| game/mesh_slot/mesh_scene.cpp | 47cae0-47e520 | 12 | 4030 | 1 |  |
| game/mesh_slot/mesh_pass.cpp | 47e520-47f650 | 2 | 3749 | 1 |  |
| game/mesh_slot/mesh_char.cpp | 47f650-47ff60 | 6 | 834 | 1 |  |
| game/mesh_slot/mesh_terrain.cpp | 47ff60-482240 | 7 | 2953 | 1 |  |
| game/mesh_slot/mesh_appear.cpp | 482240-4832b0 | 1 | 3222 | 1 |  |
| game/mesh_slot/mesh_game.cpp | 4832b0-484590 | 6 | 3292 | 1 |  |
| game/mesh_slot/mesh_start.cpp | 484590-4875e0 | 2 | 9889 | 1 |  |
| game/mesh_slot/slot.cpp | 4875e0-488560 | 9 | 2649 | 1 |  |
| game/ui/mission_ui.cpp | 488560-48b2d0 (+1) | 120 | 2412 | 2 | 502300-502320 |
| game/ui/ui_widgets_a.cpp | 48b2d0-48b550 | 7 | 195 | 1 |  |
| game/ui/ui_slider.cpp | 48b550-48b5a0 | 2 | 0 | 1 |  |
| game/ui/ui_slider_key.cpp | 48b5a0-48b6e0 | 1 | 0 | 1 |  |
| game/ui/ui_widgets_b.cpp | 48b6e0-48b930 | 6 | 0 | 1 |  |
| game/mission_ui2/ui_d_value.cpp | 48b930-48b960 | 1 | 0 | 1 |  |
| game/mission_ui2/ui_d_key.cpp | 48b960-48baa0 | 1 | 0 | 1 |  |
| game/config/ui_widgets_c.cpp | 48baa0-48be00 | 4 | 303 | 1 |  |
| game/mission_ui2/stat_panel_x.cpp | 48be00-48c330 | 8 | 257 | 1 |  |
| game/mission_ui2/mission_3.cpp | 48c330-48e5d0 | 41 | 3893 | 1 |  |
| game/mission_ui2/slot_api.cpp | 48e5d0-48ef40 | 15 | 961 | 1 |  |
| game/mission_ui2/mission_4.cpp | 48ef40-48f790 | 12 | 880 | 1 |  |
| game/mission_ui2/party.cpp | 48f790-48fd60 | 5 | 45 | 1 |  |
| game/mission_ui2/game_data.cpp | 48fd60-490030 | 3 | 0 | 1 |  |
| game/world_data/mesh_hud.cpp | 4958f0-499050 | 15 | 10898 | 1 |  |
| game/world_data/obj_tbl.cpp | 499050-499830 | 7 | 316 | 1 |  |
| game/config/widget_mgr.cpp | 499830-49ad20 (+1) | 11 | 2027 | 2 | 49b4f0-49b630 |
| game/config/widget_node.cpp | 49ad20-49b1c0 | 4 | 741 | 1 |  |
| game/script/script_interpreter.cpp | 49b630-4b1160 (+1) | 61 | 57403 | 2 | 49b1c0-49b4f0 |
| game/party2/party_2.cpp | 4b2180-4b8f40 (+1) | 81 | 14752 | 2 | 4b1160-4b2160 |
| game/party2/party_2_copy.cpp | 4b2160-4b2180 | 1 | 0 | 1 |  |
| game/config/options_dlg.cpp | 4b8f40-4b9e70 | 12 | 1194 | 1 |  |
| game/config/config.cpp | 4b9e70-4be730 (+4) | 68 | 2192 | 5 | 4c5110-4c5190, 4ca830-4ca8c0, 4cd720-4cd7a0, 4ebea0-4ebfd0 |
| game/triggers/trigger_vec.cpp | 4be730-4c2c50 | 56 | 3994 | 1 |  |
| rgl/mesh2/rec_tables.cpp | 4c3160-4c48d0 (+1) | 42 | 899 | 2 | 4c2c50-4c3120 |
| game/options_panel/trigger_records.cpp | 4c3120-4c3160 (+1) | 2 | 883 | 2 | 4d0620-4d0c20 |
| rgl/mesh2/gui_edit.cpp | 4c5190-4c80c0 (+2) | 90 | 9978 | 3 | 4c48d0-4c50ea, 4c8110-4ca600 |
| game/options_panel/ragdoll_models.cpp | 4d3580-4d3720 (+1) | 3 | 0 | 2 | 4c80c0-4c8110 |
| rgl/mesh2/mesh_2.cpp | 4ca8c0-4cd720 (+2) | 73 | 3287 | 3 | 4ca600-4ca830, 4cd7a0-4ce870 |
| game/triggers/item_rec.cpp | 4ce870-4ce960 | 3 | 0 | 1 |  |
| game/options_panel/mission_items.cpp | 4ce960-4cf100 | 13 | 0 | 1 |  |
| game/options_panel/config_dialog.cpp | 4cf100-4d0620 | 3 | 3587 | 1 |  |
| game/options_panel/module_load.cpp | 4d0c20-4d1410 | 9 | 107 | 1 |  |
| game/options_panel/mesh_edit.cpp | 4d1410-4d3330 | 6 | 0 | 1 |  |
| game/options_panel/world_app.cpp | 4d3720-4d4b4b | 12 | 1977 | 1 |  |
| game/options_panel/hud.cpp | 4d4b80-4d5410 | 3 | 1332 | 1 |  |
| game/options_panel/hud_source.cpp | 4d5410-4d5ba0 | 5 | 1075 | 1 |  |
| game/options_panel/quest_page.cpp | 4d5ba0-4d63b0 | 4 | 0 | 1 |  |
| game/options_panel/video_page.cpp | 4d63b0-4d6b80 | 1 | 1636 | 1 |  |
| game/options_panel/keys_page.cpp | 4d6b80-4d7370 | 8 | 58 | 1 |  |
| game/options_panel/options_panel.cpp | 4d7370-4d92c0 | 11 | 1008 | 1 |  |
| game/cls_map/cls_map.cpp | 4db810-4e3eb0 (+1) | 70 | 3385 | 2 | 4d92c0-4dae40 |
| game/cls_map/mission_window.cpp | 4dae40-4db810 | 6 | 606 | 1 |  |
| game/cls_map/progress_bar.cpp | 4e3eb0-4e87d0 | 41 | 10967 | 1 |  |
| game/char_sheet/save_game.cpp | 4e87d0-4e8f10 | 5 | 257 | 1 |  |
| game/char_sheet/save_state.cpp | 4e8f10-4e9850 | 17 | 0 | 1 |  |
| game/char_sheet/save_records.cpp | 4e9850-4ea0c0 | 9 | 6 | 1 |  |
| game/char_sheet/save_panel.cpp | 4ea0c0-4ea8e0 | 3 | 1550 | 1 |  |
| game/char_sheet/cam_view.cpp | 4ea8e0-4eb270 | 2 | 1419 | 1 |  |
| game/char_sheet/facegen_misc.cpp | 4eb270-4ebc10 | 10 | 1835 | 1 |  |
| game/char_sheet/screen.cpp | 4ebc10-4ebea0 (+1) | 6 | 972 | 2 | 4ebfd0-4ec4d0 |
| game/char_sheet/screen_init.cpp | 4ec4d0-4ece20 | 2 | 1764 | 1 |  |
| game/char_sheet/screen_camera.cpp | 4ece20-4ed6f0 | 3 | 1617 | 1 |  |
| game/char_sheet/render_shape.cpp | 4ed6f0-4ed8d0 | 1 | 0 | 1 |  |
| game/char_sheet/pick_entity.cpp | 4ed8d0-4ee230 | 1 | 1972 | 1 |  |
| game/char_sheet/quest_log.cpp | 4ee230-4eeb10 | 4 | 838 | 1 |  |
| game/char_sheet/cs_update.cpp | 4eeb10-4ef420 | 1 | 1216 | 1 |  |
| game/char_sheet/cs_troop.cpp | 4ef420-4ef9f0 | 4 | 414 | 1 |  |
| game/char_sheet/cs_detail.cpp | 4ef9f0-4f04f0 | 1 | 2045 | 1 |  |
| game/char_sheet/cs_weapons.cpp | 4f04f0-4f0820 | 2 | 0 | 1 |  |
| game/char_sheet/cs_panels.cpp | 4f0820-4f1390 | 1 | 2170 | 1 |  |
| game/char_sheet/msg_log_ui.cpp | 4f1390-4f1460 | 2 | 0 | 1 |  |
| game/char_sheet/facegen_win.cpp | 4f1460-4f2730 | 3 | 0 | 1 |  |
| game/char_sheet/terrain_page.cpp | 4f2730-4f2d70 | 7 | 0 | 1 |  |
| game/char_sheet/terrain_page_update.cpp | 4f2d70-4f3570 | 2 | 1592 | 1 |  |
| game/char_sheet/terrain_page_input.cpp | 4f3570-4f3e10 | 6 | 1122 | 1 |  |
| game/char_sheet/dust.cpp | 4f3e10-4f46d0 | 2 | 1954 | 1 |  |
| game/char_sheet/e96_vec.cpp | 4f46d0-4f49d0 | 6 | 0 | 1 |  |
| game/char_sheet/facegen_init.cpp | 4f49d0-4f5d20 | 1 | 3825 | 1 |  |
| game/char_sheet/editor_ui.cpp | 4f5d20-4f66e0 | 13 | 314 | 1 |  |
| game/char_sheet/conv_open.cpp | 4f66e0-4f73c0 | 1 | 2759 | 1 |  |
| game/char_sheet/conv_refresh.cpp | 4f73c0-4f7d70 | 1 | 2190 | 1 |  |
| game/agent5/agent_hud.cpp | 4f7d70-4fe0b0 | 27 | 5315 | 1 |  |
| game/agent5/agent_vec.cpp | 4fe0b0-4fe860 | 10 | 677 | 1 |  |
| game/agent5/editor_dialog.cpp | 4fe860-500ad0 | 1 | 0 | 1 |  |
| game/agent5/party_create.cpp | 500ad0-501a20 | 2 | 2826 | 1 |  |
| game/party/party_window.cpp | 502320-502cc0 (+1) | 13 | 2357 | 2 | 501a20-502300 |
| game/inventory_panel/party_screen.cpp | 502cc0-507400 | 11 | 9231 | 1 |  |
| game/inventory_panel/char_sheet.cpp | 507400-5093d0 | 1 | 95 | 1 |  |
| game/inventory_panel/char_window.cpp | 5093d0-509980 | 5 | 17 | 1 |  |
| game/inventory_panel/view_open.cpp | 509980-50a1c0 | 2 | 1684 | 1 |  |
| game/inventory_panel/note_window.cpp | 50a1c0-50cc60 | 1 | 2909 | 1 |  |
| game/inventory_panel/widget_text.cpp | 50cc60-50d5e0 | 12 | 988 | 1 |  |
| game/inventory_panel/module_files.cpp | 50d5e0-50ec40 | 1 | 0 | 1 |  |
| game/inventory_panel/game_app.cpp | 50ec40-50f440 | 3 | 1263 | 1 |  |
| game/inventory_panel/loading_screen.cpp | 50f440-50fc60 | 6 | 17 | 1 |  |
| game/inventory_panel/skeleton_load.cpp | 50fc60-50fe20 | 1 | 448 | 1 |  |
| game/inventory_panel/inventory_panel.cpp | 50fe20-513710 | 17 | 9052 | 1 |  |
| game/inventory_panel/mission_ctx.cpp | 513710-516ac0 | 27 | 6422 | 1 |  |
| game/inventory_panel/game_tick.cpp | 516ac0-5191e0 | 19 | 8221 | 1 |  |
| game/inventory_panel/map_panel.cpp | 5191e0-51be20 | 9 | 7151 | 1 |  |
| game/inventory_panel/map_tooltip.cpp | 51be20-51c5a0 | 3 | 2 | 1 |  |
| game/party/game.cpp | 51c5a0-51eb90 | 39 | 2858 | 1 |  |
| game/mission5/troop_rec.cpp | 51eb90-520b20 | 13 | 4188 | 1 |  |
| game/mission5/mission_5.cpp | 525160-52cf60 | 39 | 16283 | 1 |  |
| game/mission5/mesh_3.cpp | 52cf60-52dae0 | 2 | 1401 | 1 |  |
| game/mission5/mesh_vert_ray.cpp | 52dae0-52dcc0 | 2 | 0 | 1 |  |
| game/mission5/mesh_edge_ray.cpp | 52dcc0-52dfd0 | 2 | 311 | 1 |  |
| game/mission5/mesh_poly_ray.cpp | 52dfd0-5305b0 | 12 | 7154 | 1 |  |
| game/map_editor/map_editor.cpp | 5305b0-531590 | 5 | 365 | 1 |  |
| game/map_editor/exchange_screens.cpp | 531590-5342a6 | 21 | 7115 | 1 |  |

## Appendix 2: every boundary between consecutive module runs

Regime: B = callee-first emission (a forward call is evidence of a boundary), T = caller-first (it is not). Forward calls exclude std template COMDATs, deleting destructors and funclets. "helper-after-caller" counts functions among the first four of the later run whose only callers are in the earlier run.

| boundary VA | regime | before | after | fwd calls (non-COMDAT) | back calls | other |
|---|---|---|---|---|---|---|
| 4035d0 | B | dxframework/d3dapp | dxframework/d3denumeration | 1 401910->4036e0 | 0 |  |
| 404500 | B | dxframework/d3denumeration | dxframework/d3dfont | 0  | 0 |  |
| 4055e0 | B | dxframework/d3dfont | dxframework/d3dsettings | 0  | 0 |  |
| 407250 | B | dxframework/d3dsettings | dxframework/d3dutil | 3 405ac0->407250, 405e00->407250 .. | 0 |  |
| 407660 | B | dxframework/d3dutil | dxframework/dxutil | 0  | 0 |  |
| 407c10 | B | dxframework/dxutil | rgl/shapes/shapes | 0  | 0 |  |
| 40c2c0 | T | rgl/shapes/shapes | game/agent3/agent_combat | 0  | 0 |  |
| 40c360 | T | game/agent3/agent_combat | rgl/shapes/shapes | 0  | 0 |  |
| 40c580 | T | rgl/shapes/shapes | game/agent3/agent_combat | 0  | 0 |  |
| 40c670 | T | game/agent3/agent_combat | rgl/shapes/shapes | 0  | 0 |  |
| 40e940 | T | rgl/shapes/shapes | rgl/particles/mesh_obj | 0  | 11 |  |
| 413b60 | B | rgl/particles/mesh_obj | rgl/psys | 1 413750->414960 | 5 |  |
| 414ae0 | B | rgl/psys | rgl/noise | 0  | 0 |  |
| 4150f0 | B | rgl/noise | rgl/text_stream | 0  | 0 |  |
| 415900 | B | rgl/text_stream | rgl/core/resource_io | 1 415880->415900 | 0 |  |
| 417010 | B | rgl/core/resource_io | rgl/mem_stream | 0  | 0 |  |
| 417bb0 | B | rgl/mem_stream | rgl/resource_manager | 0  | 4 |  |
| 419190 | B | rgl/resource_manager | rgl/core/resmgr | 0  | 10 |  |
| 41e470 | B | rgl/core/resmgr | rgl/core/frustum_grid | 0  | 0 |  |
| 41ef30 | B | rgl/core/frustum_grid | rgl/scene/world_items | 0  | 2 |  |
| 41f1d0 | B | rgl/scene/world_items | game/entity | 0  | 0 |  |
| 41fba0 | B | game/entity | rgl/scene/entity_base | 0  | 0 |  |
| 41fee0 | B | rgl/scene/entity_base | rgl/scene/entity_recalc | 1 41fe90->41fee0 | 0 |  |
| 4201b0 | B | rgl/scene/entity_recalc | rgl/scene/entity_render | 1 41ffe0->4201b0 | 0 |  |
| 4205b0 | B | rgl/scene/entity_render | rgl/scene/scene_objects | 1 420310->4205b0 | 0 |  |
| 4218e0 | T | rgl/scene/scene_objects | rgl/scene/fxdef_set_color | 1 4218a0->4218e0 | 0 |  helper-after-caller x1 |
| 421990 | T | rgl/scene/fxdef_set_color | rgl/scene/mesh_list_color | 1 4218e0->421990 | 0 |  |
| 4219c0 | T | rgl/scene/mesh_list_color | rgl/scene/strategic_emitters | 0  | 1 |  |
| 421df0 | T | rgl/scene/strategic_emitters | rgl/dynamic/fx_anim | 1 421b70->421df0 | 0 |  |
| 422db0 | T | rgl/dynamic/fx_anim | rgl/dynamic/fx_keys | 15 422130->422db0, 422130->422de0 .. | 0 | visibility twin (update_poses2 opaque) helper-after-caller x4 |
| 423c00 | T | rgl/dynamic/fx_keys | rgl/dynamic/world_grid | 0  | 0 |  |
| 425370 | T | rgl/dynamic/world_grid | rgl/dynamic/dynamic_entity | 0  | 0 |  |
| 427d80 | T | rgl/dynamic/dynamic_entity | rgl/scene/entity_types | 0  | 2 |  |
| 4285e0 | T | rgl/scene/entity_types | rgl/scene/shape_load | 1 4284e0->4285e0 | 2 |  |
| 428820 | T | rgl/scene/shape_load | rgl/scene/fxdef_init | 2 4286e0->428820, 4286e0->4288c0 | 0 |  |
| 428b10 | T | rgl/scene/fxdef_init | rgl/scene/fx_support | 3 4288c0->428b10, 4288c0->428c60 .. | 0 |  helper-after-caller x2 |
| 428ff0 | T | rgl/scene/fx_support | rgl/scene/fxdef_render | 1 428f80->428ff0 | 0 |  |
| 4291c0 | T | rgl/scene/fxdef_render | rgl/scene/fxdef_skin | 1 428ff0->4291c0 | 0 |  |
| 429670 | T | rgl/scene/fxdef_skin | rgl/scene/render_scene | 2 4291c0->429670, 4291c0->4296d0 | 1 |  helper-after-caller x2 |
| 42a090 | T | rgl/scene/render_scene | game/strat/strat_entity | 0  | 0 |  |
| 42b6d0 | T | game/strat/strat_entity | game/strat/strat_ctx | 2 42a1f0->42b790, 42b080->42b6d0 | 1 |  helper-after-caller x2 |
| 42bac0 | T | game/strat/strat_ctx | game/strat/tstring | 1 42b790->42bcb0 | 0 |  |
| 42cb70 | B | game/strat/tstring | game/strat/gui_manager | 0  | 0 |  |
| 42cd60 | B | game/strat/gui_manager | game/ui/widget_slots | 0  | 0 |  |
| 42cd80 | B | game/ui/widget_slots | game/strat/gui_manager | 0  | 0 |  |
| 42ce50 | B | game/strat/gui_manager | game/ui/widget_slots | 0  | 0 |  |
| 42d1b0 | B | game/ui/widget_slots | game/strat/gui_manager | 0  | 0 |  |
| 42d820 | B | game/strat/gui_manager | tinyxml/tinystr | 0  | 0 |  |
| 42d9a0 | B | tinyxml/tinystr | tinyxml/tinyxml | 0  | 25 |  |
| 42f720 | B | tinyxml/tinyxml | tinyxml/tinyxmlparser | 3 42e4f0->42fc70, 42f210->42fc70 .. | 22 |  |
| 431400 | B | tinyxml/tinyxmlparser | game/world_data/world_data | 0  | 0 |  |
| 431420 | B | game/world_data/world_data | rgl/xmlcfg/records | 0  | 0 |  |
| 431b90 | B | rgl/xmlcfg/records | game/world_data/world_data | 0  | 0 |  |
| 431c40 | B | game/world_data/world_data | rgl/xmlcfg/records | 0  | 0 |  |
| 431cb0 | B | rgl/xmlcfg/records | game/world_data/world_data | 0  | 0 |  |
| 431d30 | B | game/world_data/world_data | rgl/xmlcfg/records | 0  | 0 |  |
| 431e10 | B | rgl/xmlcfg/records | game/mission5/game_2 | 0  | 0 |  |
| 431f40 | B | game/mission5/game_2 | game/terrain_b/poly_mesh | 0  | 0 |  |
| 432010 | B | game/terrain_b/poly_mesh | rgl/xmlcfg/records | 0  | 0 |  |
| 4320b0 | B | rgl/xmlcfg/records | rgl/xmlcfg/key_groups | 0  | 0 | visibility twin (docs/tu_map.md) |
| 432350 | B | rgl/xmlcfg/key_groups | rgl/xmlcfg/game_util | 0  | 1 |  |
| 433040 | B | rgl/xmlcfg/game_util | rgl/xmlcfg/agent_pick | 0  | 1 | visibility twin |
| 4338e0 | B | rgl/xmlcfg/agent_pick | game/agent/curve | 0  | 0 |  |
| 433bf0 | B | game/agent/curve | game/agent/item_kind | 0  | 0 |  |
| 434b20 | B | game/agent/item_kind | game/agent/agent | 0  | 1 |  |
| 437770 | B | game/agent/agent | game/agent/troop_adjust | 0  | 1 | visibility twin |
| 437830 | B | game/agent/troop_adjust | game/agent2/agent_stats | 0  | 0 |  |
| 43b100 | B | game/agent2/agent_stats | game/agent2/agent_ai | 0  | 15 |  |
| 43e190 | B | game/agent2/agent_ai | game/agent3/agent_combat | 0  | 5 |  |
| 441360 | B | game/agent3/agent_combat | game/agent/phys_set | 0  | 0 |  |
| 441ff0 | B | game/agent/phys_set | game/agent3/agent_fx_sound | 0  | 0 |  |
| 444c50 | B | game/agent3/agent_fx_sound | game/agent4/agent_logic | 0  | 37 |  |
| 460160 | B | game/agent4/agent_logic | game/dialog_manager | 0  | 0 |  |
| 461200 | B | game/dialog_manager | game/terrain_a/wind | 0  | 0 |  |
| 461630 | B | game/terrain_a/wind | game/renderer | 0  | 0 |  |
| 4625d0 | B | game/renderer | game/ui/item_ref | 0  | 0 |  |
| 462ac0 | B | game/ui/item_ref | game/ui/troop_inv | 0  | 0 |  |
| 4636d0 | B | game/ui/troop_inv | game/terrain_a/game_state | 0  | 0 |  |
| 464200 | B | game/terrain_a/game_state | game/terrain_a/terrain_a | 0  | 0 |  |
| 464b40 | B | game/terrain_a/terrain_a | game/terrain_a/cell_of | 0  | 0 |  |
| 464bf0 | B | game/terrain_a/cell_of | game/terrain_a/terrain_a | 0  | 2 |  |
| 467f80 | B | game/terrain_a/terrain_a | game/terrain_b/poly_mesh | 0  | 0 |  |
| 467fb0 | B | game/terrain_b/poly_mesh | game/terrain_a/terrain_a | 0  | 0 |  |
| 4681b0 | B | game/terrain_a/terrain_a | game/terrain_a/terrain_render | 0  | 3 | visibility twin |
| 46e590 | B | game/terrain_a/terrain_render | game/terrain_b/terrain_b | 0  | 0 |  |
| 470a00 | B | game/terrain_b/terrain_b | game/terrain_b/poly_mesh | 0  | 3 | visibility twin |
| 478830 | B | game/terrain_b/poly_mesh | game/terrain_b/mission | 0  | 2 |  |
| 47aa40 | B | game/terrain_b/mission | game/mesh_slot/mesh_ops | 0  | 0 |  |
| 47cae0 | B | game/mesh_slot/mesh_ops | game/mesh_slot/mesh_scene | 0  | 9 |  |
| 47e520 | B | game/mesh_slot/mesh_scene | game/mesh_slot/mesh_pass | 0  | 0 |  |
| 47f650 | B | game/mesh_slot/mesh_pass | game/mesh_slot/mesh_char | 0  | 2 |  |
| 47ff60 | B | game/mesh_slot/mesh_char | game/mesh_slot/mesh_terrain | 0  | 2 |  |
| 482240 | B | game/mesh_slot/mesh_terrain | game/mesh_slot/mesh_appear | 0  | 1 |  |
| 4832b0 | B | game/mesh_slot/mesh_appear | game/mesh_slot/mesh_game | 0  | 1 |  |
| 484590 | B | game/mesh_slot/mesh_game | game/mesh_slot/mesh_start | 0  | 2 |  |
| 4875e0 | B | game/mesh_slot/mesh_start | game/mesh_slot/slot | 0  | 0 |  |
| 488560 | B | game/mesh_slot/slot | game/ui/mission_ui | 0  | 1 |  |
| 48b2d0 | B | game/ui/mission_ui | game/ui/ui_widgets_a | 1 48b200->48b2d0 | 4 |  |
| 48b550 | B | game/ui/ui_widgets_a | game/ui/ui_slider | 1 48b4f0->48b550 | 0 |  |
| 48b5a0 | B | game/ui/ui_slider | game/ui/ui_slider_key | 0  | 1 |  |
| 48b6e0 | B | game/ui/ui_slider_key | game/ui/ui_widgets_b | 1 48b5a0->48b6e0 | 0 |  |
| 48b930 | B | game/ui/ui_widgets_b | game/mission_ui2/ui_d_value | 1 48b8d0->48b930 | 0 |  |
| 48b960 | B | game/mission_ui2/ui_d_value | game/mission_ui2/ui_d_key | 0  | 1 |  |
| 48baa0 | B | game/mission_ui2/ui_d_key | game/config/ui_widgets_c | 1 48b960->48baa0 | 0 |  |
| 48be00 | B | game/config/ui_widgets_c | game/mission_ui2/stat_panel_x | 0  | 0 |  |
| 48c330 | B | game/mission_ui2/stat_panel_x | game/mission_ui2/mission_3 | 0  | 1 |  |
| 48e5d0 | B | game/mission_ui2/mission_3 | game/mission_ui2/slot_api | 0  | 1 |  |
| 48ef40 | B | game/mission_ui2/slot_api | game/mission_ui2/mission_4 | 0  | 0 |  |
| 48f790 | B | game/mission_ui2/mission_4 | game/mission_ui2/party | 0  | 0 |  |
| 48fd60 | B | game/mission_ui2/party | game/mission_ui2/game_data | 0  | 0 |  |
| 490030 | B | game/mission_ui2/game_data | game/world_data/world_data | 0  | 0 |  |
| 4958f0 | B | game/world_data/world_data | game/world_data/mesh_hud | 0  | 0 |  |
| 499050 | B | game/world_data/mesh_hud | game/world_data/obj_tbl | 0  | 3 |  |
| 499830 | B | game/world_data/obj_tbl | game/config/widget_mgr | 0  | 1 |  |
| 49ad20 | B | game/config/widget_mgr | game/config/widget_node | 1 49aca0->49ad20 | 0 |  |
| 49b1c0 | B | game/config/widget_node | game/script/script_interpreter | 0  | 0 |  |
| 49b4f0 | B | game/script/script_interpreter | game/config/widget_mgr | 0  | 0 |  |
| 49b630 | B | game/config/widget_mgr | game/script/script_interpreter | 0  | 0 |  |
| 4b1160 | B | game/script/script_interpreter | game/party2/party_2 | 4 49fa10->4b14b0, 49fa10->4b14f0 .. | 1 |  |
| 4b2160 | B | game/party2/party_2 | game/party2/party_2_copy | 0  | 0 |  |
| 4b2180 | B | game/party2/party_2_copy | game/party2/party_2 | 0  | 2 |  |
| 4b8f40 | B | game/party2/party_2 | game/config/options_dlg | 0  | 0 |  |
| 4b9e70 | B | game/config/options_dlg | game/config/config | 0  | 0 |  |
| 4be730 | B | game/config/config | game/triggers/trigger_vec | 0  | 14 |  |
| 4c2c50 | B | game/triggers/trigger_vec | rgl/mesh2/rec_tables | 0  | 1 |  |
| 4c3120 | T | rgl/mesh2/rec_tables | game/options_panel/trigger_records | 0  | 0 |  |
| 4c3160 | T | game/options_panel/trigger_records | rgl/mesh2/rec_tables | 0  | 0 |  |
| 4c48d0 | ? | rgl/mesh2/rec_tables | rgl/mesh2/gui_edit | 0  | 0 |  |
| 4c5110 | B | rgl/mesh2/gui_edit | game/config/config | 0  | 0 |  |
| 4c5190 | B | game/config/config | rgl/mesh2/gui_edit | 0  | 0 |  |
| 4c80c0 | B | rgl/mesh2/gui_edit | game/options_panel/ragdoll_models | 0  | 0 |  |
| 4c8110 | B | game/options_panel/ragdoll_models | rgl/mesh2/gui_edit | 0  | 0 |  |
| 4ca600 | B | rgl/mesh2/gui_edit | rgl/mesh2/mesh_2 | 1 4ca550->4ca600 | 0 |  |
| 4ca830 | B | rgl/mesh2/mesh_2 | game/config/config | 0  | 0 |  |
| 4ca8c0 | B | game/config/config | rgl/mesh2/mesh_2 | 0  | 0 |  |
| 4cd720 | B | rgl/mesh2/mesh_2 | game/config/config | 0  | 0 |  |
| 4cd7a0 | B | game/config/config | rgl/mesh2/mesh_2 | 0  | 0 |  |
| 4ce870 | B | rgl/mesh2/mesh_2 | game/triggers/item_rec | 0  | 0 |  |
| 4ce960 | B | game/triggers/item_rec | game/options_panel/mission_items | 0  | 0 |  |
| 4cf100 | B | game/options_panel/mission_items | game/options_panel/config_dialog | 0  | 0 |  |
| 4d0620 | B | game/options_panel/config_dialog | game/options_panel/trigger_records | 0  | 0 |  |
| 4d0c20 | B | game/options_panel/trigger_records | game/options_panel/module_load | 0  | 0 |  |
| 4d1410 | B | game/options_panel/module_load | game/options_panel/mesh_edit | 0  | 0 |  |
| 4d3580 | B | game/options_panel/mesh_edit | game/options_panel/ragdoll_models | 0  | 0 |  |
| 4d3720 | B | game/options_panel/ragdoll_models | game/options_panel/world_app | 0  | 0 |  |
| 4d4b80 | T | game/options_panel/world_app | game/options_panel/hud | 0  | 0 |  |
| 4d5410 | T | game/options_panel/hud | game/options_panel/hud_source | 1 4d5290->4d5410 | 0 |  |
| 4d5ba0 | T | game/options_panel/hud_source | game/options_panel/quest_page | 1 4d5a20->4d5ba0 | 0 |  helper-after-caller x1 |
| 4d63b0 | T | game/options_panel/quest_page | game/options_panel/video_page | 0  | 0 |  |
| 4d6b80 | T | game/options_panel/video_page | game/options_panel/keys_page | 2 4d63b0->4d6b80, 4d63b0->4d6dd0 | 0 |  |
| 4d7370 | T | game/options_panel/keys_page | game/options_panel/options_panel | 2 4d7120->4d7370, 4d7120->4d7d10 | 0 |  helper-after-caller x1 |
| 4d92c0 | T | game/options_panel/options_panel | game/cls_map/cls_map | 0  | 0 |  |
| 4dae40 | T | game/cls_map/cls_map | game/cls_map/mission_window | 0  | 0 |  |
| 4db810 | T | game/cls_map/mission_window | game/cls_map/cls_map | 1 4db660->4db810 | 0 |  |
| 4e3eb0 | T | game/cls_map/cls_map | game/cls_map/progress_bar | 1 4e3d50->4e3eb0 | 6 |  helper-after-caller x1 |
| 4e87d0 | T | game/cls_map/progress_bar | game/char_sheet/save_game | 0  | 0 |  |
| 4e8f10 | T | game/char_sheet/save_game | game/char_sheet/save_state | 1 4e8980->4e8f10 | 0 |  helper-after-caller x1 |
| 4e9850 | T | game/char_sheet/save_state | game/char_sheet/save_records | 3 4e8f10->4e99d0, 4e8f10->4e9b50 .. | 4 |  helper-after-caller x2 |
| 4ea0c0 | T | game/char_sheet/save_records | game/char_sheet/save_panel | 0  | 0 |  |
| 4ea8e0 | T | game/char_sheet/save_panel | game/char_sheet/cam_view | 0  | 0 |  |
| 4eb270 | T | game/char_sheet/cam_view | game/char_sheet/facegen_misc | 3 4ea8e0->4eb270, 4ea8e0->4eb320 .. | 0 |  helper-after-caller x2 |
| 4ebc10 | T | game/char_sheet/facegen_misc | game/char_sheet/screen | 0  | 0 |  |
| 4ebea0 | T | game/char_sheet/screen | game/config/config | 0  | 0 |  |
| 4ebfd0 | T | game/config/config | game/char_sheet/screen | 0  | 0 |  |
| 4ec4d0 | T | game/char_sheet/screen | game/char_sheet/screen_init | 0  | 0 |  |
| 4ece20 | T | game/char_sheet/screen_init | game/char_sheet/screen_camera | 2 4ec4d0->4ece20, 4ec4d0->4ed630 | 1 |  |
| 4ed6f0 | T | game/char_sheet/screen_camera | game/char_sheet/render_shape | 0  | 0 |  |
| 4ed8d0 | T | game/char_sheet/render_shape | game/char_sheet/pick_entity | 0  | 0 |  |
| 4ee230 | T | game/char_sheet/pick_entity | game/char_sheet/quest_log | 0  | 0 |  |
| 4eeb10 | T | game/char_sheet/quest_log | game/char_sheet/cs_update | 0  | 0 |  |
| 4ef420 | T | game/char_sheet/cs_update | game/char_sheet/cs_troop | 3 4eeb10->4ef420, 4eeb10->4ef6d0 .. | 0 |  helper-after-caller x1 |
| 4ef9f0 | T | game/char_sheet/cs_troop | game/char_sheet/cs_detail | 0  | 0 |  |
| 4f04f0 | T | game/char_sheet/cs_detail | game/char_sheet/cs_weapons | 1 4ef9f0->4f04f0 | 0 |  |
| 4f0820 | T | game/char_sheet/cs_weapons | game/char_sheet/cs_panels | 0  | 0 |  |
| 4f1390 | T | game/char_sheet/cs_panels | game/char_sheet/msg_log_ui | 0  | 0 |  |
| 4f1460 | T | game/char_sheet/msg_log_ui | game/char_sheet/facegen_win | 0  | 0 |  |
| 4f2730 | T | game/char_sheet/facegen_win | game/char_sheet/terrain_page | 2 4f1460->4f2730, 4f1460->4f2960 | 0 |  |
| 4f2d70 | T | game/char_sheet/terrain_page | game/char_sheet/terrain_page_update | 0  | 6 |  |
| 4f3570 | T | game/char_sheet/terrain_page_update | game/char_sheet/terrain_page_input | 4 4f2d70->4f3570, 4f2d70->4f37f0 .. | 0 |  helper-after-caller x1 |
| 4f3e10 | T | game/char_sheet/terrain_page_input | game/char_sheet/dust | 1 4f3cf0->4f3e10 | 0 |  |
| 4f46d0 | T | game/char_sheet/dust | game/char_sheet/e96_vec | 0  | 0 |  |
| 4f49d0 | T | game/char_sheet/e96_vec | game/char_sheet/facegen_init | 0  | 0 |  |
| 4f5d20 | T | game/char_sheet/facegen_init | game/char_sheet/editor_ui | 7 4f49d0->4f5d20, 4f49d0->4f5d90 .. | 0 |  helper-after-caller x4 |
| 4f66e0 | T | game/char_sheet/editor_ui | game/char_sheet/conv_open | 0  | 2 |  |
| 4f73c0 | T | game/char_sheet/conv_open | game/char_sheet/conv_refresh | 1 4f66e0->4f73c0 | 0 |  |
| 4f7d70 | T | game/char_sheet/conv_refresh | game/agent5/agent_hud | 0  | 0 |  |
| 4fe0b0 | T | game/agent5/agent_hud | game/agent5/agent_vec | 0  | 0 |  |
| 4fe860 | T | game/agent5/agent_vec | game/agent5/editor_dialog | 0  | 0 |  |
| 500ad0 | T | game/agent5/editor_dialog | game/agent5/party_create | 0  | 0 |  |
| 501a20 | T | game/agent5/party_create | game/party/party_window | 1 500ad0->501a20 | 0 |  helper-after-caller x1 |
| 502300 | T | game/party/party_window | game/ui/mission_ui | 0  | 0 |  |
| 502320 | T | game/ui/mission_ui | game/party/party_window | 0  | 0 |  |
| 502cc0 | T | game/party/party_window | game/inventory_panel/party_screen | 0  | 0 |  |
| 507400 | T | game/inventory_panel/party_screen | game/inventory_panel/char_sheet | 0  | 0 |  |
| 5093d0 | T | game/inventory_panel/char_sheet | game/inventory_panel/char_window | 5 507400->5093d0, 507400->509410 .. | 0 |  helper-after-caller x4 |
| 509980 | T | game/inventory_panel/char_window | game/inventory_panel/view_open | 0  | 0 |  |
| 50a1c0 | T | game/inventory_panel/view_open | game/inventory_panel/note_window | 1 50a140->50a1c0 | 0 |  |
| 50cc60 | T | game/inventory_panel/note_window | game/inventory_panel/widget_text | 4 50a1c0->50cc60, 50a1c0->50cc70 .. | 0 |  helper-after-caller x4 |
| 50d5e0 | T | game/inventory_panel/widget_text | game/inventory_panel/module_files | 0  | 1 |  |
| 50ec40 | T | game/inventory_panel/module_files | game/inventory_panel/game_app | 0  | 2 |  |
| 50f440 | T | game/inventory_panel/game_app | game/inventory_panel/loading_screen | 1 50f390->50f440 | 0 |  helper-after-caller x1 |
| 50fc60 | T | game/inventory_panel/loading_screen | game/inventory_panel/skeleton_load | 0  | 1 |  |
| 50fe20 | T | game/inventory_panel/skeleton_load | game/inventory_panel/inventory_panel | 0  | 0 |  |
| 513710 | T | game/inventory_panel/inventory_panel | game/inventory_panel/mission_ctx | 1 50fe20->513710 | 0 |  |
| 516ac0 | T | game/inventory_panel/mission_ctx | game/inventory_panel/game_tick | 1 514d40->516ac0 | 0 |  helper-after-caller x1 |
| 5191e0 | T | game/inventory_panel/game_tick | game/inventory_panel/map_panel | 0  | 0 |  |
| 51c5a0 | T | game/inventory_panel/map_panel | game/party/game | 0  | 0 |  |
| 51eb90 | T | game/party/game | game/mission5/troop_rec | 2 51c720->51f1a0, 51eaa0->51eb90 | 1 |  |
| 520b20 | T | game/mission5/troop_rec | game/mission5/game_2 | 0  | 1 |  |
| 525160 | T | game/mission5/game_2 | game/mission5/mission_5 | 0  | 1 |  |
| 52cf60 | T | game/mission5/mission_5 | game/mission5/mesh_3 | 1 52ba1d->52cf60 | 0 |  helper-after-caller x1 |
| 52dae0 | T | game/mission5/mesh_3 | game/mission5/mesh_vert_ray | 2 52cf60->52dc00, 52da20->52dae0 | 0 | visibility twin helper-after-caller x2 |
| 52dcc0 | T | game/mission5/mesh_vert_ray | game/mission5/mesh_edge_ray | 1 52dc00->52dcc0 | 0 | visibility twin helper-after-caller x1 |
| 52dfd0 | T | game/mission5/mesh_edge_ray | game/mission5/mesh_poly_ray | 1 52df40->52dfd0 | 0 | visibility twin helper-after-caller x1 |
| 5305b0 | T | game/mission5/mesh_poly_ray | game/map_editor/map_editor | 0  | 1 |  |
| 531590 | T | game/map_editor/map_editor | game/map_editor/exchange_screens | 0  | 0 |  |

# Views inventory: decompilation-only types over other objects' memory

Ticket 006 (layout-neutral structs), companion of docs/layout64_inventory.md. Measured 2026-11-19 on main (8645dad2 plus
the working tree of the integration lane). Regenerate the tables with
`tools/port/find_views.py --md docs/views_inventory.md` (about 15 s; the JSON with every record and every cast site goes to
`work/views/views.json`, `--name X` prints one record with its sites).

## Method

A **view** is a type the original source did not have: a struct naming a few fields of another object's memory (pads plus
`// +0x..` fields), reached by casting a pointer of another type; vector-shaped `{alloc, first, last, end}` stand-ins; fake
vtables; per-slice `*_s2c2b0` records; X-suffixed twins that only add names. Not views: the real classes (their pads are
listed at the end as incomplete real types) and port/ shims.

`tools/port/find_views.py` is text-only (no compiler, so it runs while src/ is being edited). Signals:

- (a) layout: pad arrays, offset comments, pointer-holding members (T*, tstring, std:: containers), 32-bit offsets from the
  comments or summed sizes;
- (b) casts: C-style / reinterpret / static casts to a record pointer or reference, `slot_as<T>(n)`-style template helpers,
  `#define` macros containing a cast and zero-argument accessors (`MissionX* mission() { return (MissionX*)g_mission; }`):
  each macro / accessor use is a site. The operand's type is resolved lexically (this, locals / parameters, members,
  globals, `->` chains, `(char*)p + N` with its offset);
- (c) names: `_s[0-9a-f]{5,6}`, `View`, `Vt` / `Vtbl`, `_pub`, X-twins of defined records, address names;
- (d) use: constructors, `new T`, by-value declarations, and how often the name occurs at all versus how often it is cast to.

A record is a view when it is cast to from another record or raw memory, is never constructed and is rarely named otherwise
(refs <= 3 x casts + 6), or when its name says so and it has a layout / casts / a base, or when it is vector-shaped.
Records annotated with their own image vtable (`vtable 0x5.....`) are real classes. The **overlaid real type** is the
best-weighted candidate from resolved cast sources (x2), the record's own comments (`Mission::player_id`, the name on the
struct line, x2) and bases / twin names, preferring candidates that are not views. **64?** is `yes` when a pointer-holding
member follows a pad, an int member is commented as a pointer, or the real type has pointers and the view names fields
behind pads; `likely` when only pads and scalars are named. **path**: `reach:N` = N cast sites in functions the original's
direct call graph (work/migrate/facts.json, no virtual calls) reaches from the port's run path seeds (GameApp ctor,
OneTimeSceneInit, RestoreDeviceObjects, load_module_files, load_game_data, start_new_game, Game::build_map /
build_map_trees / build_scene, ItemA::s_last, FieldPanel::relayout); `doc` = named in port/INTEGRATION_STATUS.md or
docs/layout64_inventory.md. **rows e/n/-**: cast sites in byte-exact manifest rows / nonmatching rows / no VA (inline
helpers in headers, which are inlined into rows of either kind).

Class rules (auto, then hand overrides in the tool's `OVERRIDES`, shown **bold** in the notes): a codegen comment on the
definition -> F; vtable name or virtual-only declarations -> E; vector-shaped or std:: base -> D; only `slot_as` sources ->
A (the slot's screen class); no real candidate -> C; a named member of the real type at the same offset with an
incompatible type -> B (int vs pointer and int-ish vs int-ish are not conflicts: the typed side wins); else A.

**Caveats.** Precision is moderate: a hand check of 16 random views found 12 clear views, 4 doubtful (a real data record,
a real screen class since excluded by the vtable rule, a widget subclass, an animation record). Use the `conf` column:
`high` = name signal, or pads plus a cast from another record, or a this-wrapper; `low` = defined in include/ or rgl/,
named often, or reached only from raw pointers (most false positives are there). Operand typing is lexical: method-call
results and template expressions stay `?`, so "overlays" can be a hint picked from comments. Same-named records in
several TUs are separate rows (casts bind to the nearest definition). `reach` follows direct calls only: the menu screens
(entered through vtables) are under-counted, and load_game_data / build_scene pull in a large part of the game.
"real type has no offsets there: check" means the real class has no offset comments covering the field; the fold is
then not proven mechanical.

## Do first

Ordered by the rule (64-bit wrong and on the port path, then cast sites), adjusted where one fix covers a family:

1. (done 2026-11-20, lane 7: MissionX derives from Mission, its data view is gone) **MissionX** (B, script_interpreter.h:453): the script's Mission, 157 `mission()` calls, pointer members behind pads, on
   the new-game path (start scripts). Reconcile EntryPoint[64] / teams / cam_frame with game/mission.h, then `mission()`
   returns `Mission*`.
2. **SceneEnt** (A, game/scene_ent.h): rgl::entity seen through a numbered vtable and pads (spawn_missile / spawn_prop,
   10 sites): rgl::entity has the members; the battle path's first entity reads.
3. **AgentBodyV / SkelBodyV / AgentEq / Agent_s\*** (A): the agent body and agent records with pads before pointers
   (`camp`, `skel`); one pass over AgentBody / Agent removes the family.
4. **Curve** (B, game/agent/curve.h): 11 sites from `body->skel`; pointer members (`recs`, `p68`) disagree with
   Skeleton's: decide whether the object is the skeleton or its channel record first.
5. **AnimRec** (A, rgl/dynamic/fx_anim.h): the FxAnim / EnvRec record, 14 sites, 64-bit masks and pointer members behind pads.
6. (lane 7 folded GameData_s261e0, Mission_s29130 / Agent_s29130 / GameData_s29130; lane V1 Game_s2cf60, Frame__s2f250; lane V4 Scene_s2cf60 / gui_manager_s2cf60 and the PolyMesh family: graph_obj, EditMesh, TearObj, Shape, Mesh_s2da20 / s2dfd0 / s2e960, Local, EditorSceneView) **GameData_s29130 and the other mission5 slice views** (GameData_s261e0 48 sites, Frame__s2f250 47, CampaignGame_s2b5c0
   29, Game_s2cf60 18, Mission_s29130 15, Scene_s2cf60 88): the "12 game-object views of mission_5.h" INTEGRATION_STATUS
   lists; every field is a pad-offset at 64 bits. GameData / Mission have most of the members already.
7. **View** (A, script_interpreter.cpp:2362, `doc`): the presentation view with a 32-bit pad before `list` (+0x2c).
8. (done 2026-11-20, lane V1: emitter is rgl::psys, TabSpawn / TabTarget are skin / texture; VoiceTable left) **emitter / TabSpawn / VoiceTable** (B): psys / skin / Animator views with vectors or pointers behind pads, exact rows.
9. (done 2026-11-20, lanes V2 + V3) **WVt + PsOps / text_slot** (E): MissionUi's virtuals have their real signatures
   (`const vec2&` / `const unsigned&` parameters, `V2 get_xy()` / `V2 set_text(const tstring&)` by value, set_target ->
   set_hilight_color, put -> get_text(tstring&)), ~600 call sites spell the temporaries; WVt, text_slot, MapText and the
   TU-local float pairs (mh_float2, wn_float2, PairF, PsVec2, FieldV2, Vec2 / float2 at the slots) are gone; MissionUi's
   pairs are vec2 members (anim_pos_to / anim_size_to / anim_pos_from / anim_size_from / cur_size / text_offset /
   text_scale). Left: MissionUi::screen[4] and the fE0..fEC union (pairs read through casts in 4 sites), the overlay API's
   `const float* pos` (widget_mgr::set_size / set_position / animate_*, HudOuter::scroll_refresh: the script's vector4 by
   pointer), Font::layout_text / layout_map (pointer out-params).
10. (GameX done 2026-11-20, lane V1: with GameW / GameCompanion / TroopX, game() is g_game) **GameX** (A, 139 sites bound here; with the `game()` calls about 240) and **CamSlot** (A, 61 sites, all exact rows; done lane 7: Mission's cam_mode / frame_a / frame_b / blend_* members): mechanical renames into
    GameData / Mission members; cheap and remove two of the largest cast sources.

Related, not views but the same constraint as class F: `MB_TSTRING_SIZE` (game/tstring.h) sizes raw tstring storage
(C4c3050, 0x4d63b0); D-class replacements must keep the template instance the original's calls bind to
(tools/relink/target_gate.py: an ICF-folded `vector<int>::_Insert_n` is not `vector<T*>`'s copy, e.g. EntVec::insert_n).

## Totals per class and tables

<!-- generated by tools/port/find_views.py --md docs/views_inventory.md: edit the tool (OVERRIDES), not this part -->

| class | views | 64-bit wrong (yes) | on the port path | cast sites |
|---|---:|---:|---:|---:|
| A | 117 | 1 | 58 | 324 |
| B | 9 | 1 | 4 | 17 |
| C | 10 | 0 | 4 | 14 |
| D | 51 | 8 | 12 | 69 |
| E | 5 | 0 | 2 | 2 |
| F | 2 | 0 | 2 | 2 |
| all | 194 | 10 | 82 | 428 |

### A. Fold into the real type (117)

| view | file:line | overlays | fields named | casts (files) | 64? | path | rows e/n/- | conf | notes |
|---|---|---|---|---|---|---|---|---|---|
| SceneRec48 | game/world_data/obj_tbl.h:24 | world (rgl/scene/world.h:78) | name, flags, px, py +7 | 4 (obj_tbl.cpp) | yes | reach:4 | 4/0/0 | medium | fold into world (real type has no offsets there: check) |
| WItem | game/config/widget_mgr.h:72 | std::vector<MissionUi> | 1 methods | 39 (widget_mgr.cpp) | no | reach:35 | 35/0/4 | medium | fold into std::vector<MissionUi> (pads cover the fields) |
| TroopRec | include/troop.h:132 | Troop (include/troop.h:20) | 9 methods | 37 (inventory_panel.cpp, troop_rec.cpp) | no | doc,reach:22 | 16/21/0 | high | method twin: declare the 9 method(s) on Troop |
| string_view_ | game/script/script_interpreter.h:58 | tstring (game/tstring.h:19) | 1 methods | 36 (script_interpreter.cpp, script_interpreter.h) | no | reach:35 | 1/34/1 | medium | method twin: declare the 1 method(s) on tstring |
| scene_ms | game/mesh_slot/mesh_pass.cpp:33 | scene (rgl/scene_rec.h:40) | 1 methods | 24 (mesh_pass.cpp) | no | - | 0/24/0 | high | method twin: declare the 1 method(s) on scene |
| MenuWidget | game/inventory_panel/mission_ctx.h:13 | ? | f4, f14, f18 | 19 (inventory_panel.cpp) | likely | - | 19/0/0 | medium | a screen in g_windows.arr[n] (slot_as<T>(n)): fold into that slot's screen class (head_rec family, game/screen_records.h) |
| Blob16 | game/ui/mission.h:21 | vector4 (rgl/rgl_math.h:14) | a, b, c, d | 14 (mission_ui.cpp) | no | reach:14 | 14/0/0 | medium | **16-byte block copy of Sel::rec[].a / .c (int[4]): give those members a 4-int struct type and assign it; no 64-bit effect** |
| AgentX | game/mission5/mission_5.h:379 | Agent (game/agent_obj.h:52) | 3 methods | 8 (mission_5.cpp) | no | doc | 0/8/0 | high | method twin: declare the 3 method(s) on Agent |
| Obj3 | game/agent3/agent_types.h:59 | psys (rgl/scene/emitter.h:32) | 3 methods | 7 (agent_logic.cpp) | no | reach:3 | 5/2/0 | high | method twin: declare the 3 method(s) on psys |
| EV | game/options_panel/mesh_edit.cpp:131 | std::vector<MEdge> | v | 6 (mesh_edit.cpp) | no | - | 6/0/0 | medium | fold into std::vector<MEdge> (pads cover the fields) |
| DiDev | game/char_sheet/screen.h:93 | IDirectInputDevice8A | vtbl | 5 (screen.cpp) | no | - | 0/0/5 | medium | fold into IDirectInputDevice8A (pads cover the fields) |
| Matrix12_s2fe40 | game/mission5/mesh_3.h:312 | float | v | 5 (mesh_3.h, mesh_poly_ray.cpp) | no | reach:4 | 1/4/0 | high | fold into float (pads cover the fields) |
| Sel | game/ui/mission.h:41 | Leader (game/script/script_interpreter.cpp:673) | rec | 5 (script_interpreter.cpp) | no | reach:4 | 2/2/1 | high | fold into Leader (pads cover the fields) |
| MapEditorDialog | game/agent5/editor_dialog.cpp:50 | GameApp (game/config/core.h:20) | 2 methods | 4 (editor_dialog.cpp) | no | - | 4/0/0 | low | method twin: declare the 2 method(s) on GameApp |
| sel_list | game/inventory_panel/widget_text.cpp:94 | std::vector | v | 4 (note_window.cpp) | likely | doc | 4/0/0 | medium | fold into std::vector (pads cover the fields) |
| EquipSet | game/mesh_slot/mesh_appear.cpp:32 | ItemRef (game/item_ref.h:13) | weapon, helmet, body, boots | 3 (agent_hud.cpp, mesh_game.cpp +1) | no | reach:1 | 1/1/1 | high | fold into ItemRef (real type has no offsets there: check) |
| TriggerMeshObj | game/script/script_interpreter.h:150 | mesh_inst (game/renderer.h:34) | 1 methods | 3 (script_interpreter.cpp) | no | reach:4 | 0/3/0 | high | method twin: declare the 1 method(s) on mesh_inst |
| mesh_node | game/party2/party_2.h:53 | mesh (rgl/mesh.h:58) | 1 methods | 3 (party_2.cpp) | no | doc | 0/0/3 | low | method twin: declare the 1 method(s) on mesh |
| rot3x | game/agent4/agent_logic.cpp:7597 | matrix (rgl/rgl_math.h:69) | 1 methods | 3 (agent_logic.cpp) | no | - | 0/0/3 | high | method twin: declare the 1 method(s) on matrix |
| rot3x | rgl/xmlcfg/game_util.h:54 | matrix (rgl/rgl_math.h:69) | 1 methods | 3 (game_util.cpp) | no | reach:3 | 3/0/0 | high | method twin: declare the 1 method(s) on matrix |
| vec4n | game/agent4/agent_logic.cpp:7362 | vector4 (rgl/rgl_math.h:14) | 2 methods | 3 (agent_logic.cpp) | no | reach:3 | 0/3/0 | medium | method twin: declare the 2 method(s) on vector4 |
| vec_arr | game/terrain_b/render_batch.h:127 | vec_table (include/vec_table.h:10) | data, count | 3 (terrain_b.cpp) | no | - | 0/3/0 | medium | fold into vec_table (pads cover the fields) |
| Alloc204 | rgl/mesh2/rec_tables.h:154 | MapDialog (rgl/mesh2/gui_edit.h:179) | 1 methods | 2 (gui_edit.cpp) | no | reach:2 | 2/0/0 | low | method twin: declare the 1 method(s) on MapDialog |
| Alloc2c | rgl/mesh2/rec_tables.h:156 | MapDialog (rgl/mesh2/gui_edit.h:179) | 1 methods | 2 (gui_edit.cpp) | no | reach:2 | 2/0/0 | low | method twin: declare the 1 method(s) on MapDialog |
| Alloc4 | rgl/mesh2/rec_tables.h:155 | MapDialog (rgl/mesh2/gui_edit.h:179) | 1 methods | 2 (gui_edit.cpp) | no | reach:2 | 2/0/0 | low | method twin: declare the 1 method(s) on MapDialog |
| EditorApp | game/agent5/editor_dialog.cpp:51 | GameApp (game/config/core.h:20) | 1 methods | 2 (editor_dialog.cpp) | no | - | 2/0/0 | medium | method twin: declare the 1 method(s) on GameApp |
| EditorObj | rgl/mesh2/rec_tables.cpp:173 | GameApp (game/config/core.h:20) | 1 methods | 2 (rec_tables.cpp) | no | - | 2/0/0 | low | method twin: declare the 1 method(s) on GameApp |
| Event92 | include/game_data.h:44 | event_rec (game/party2/party_2.cpp:2111) | v | 2 (party_2.cpp) | no | reach:1 | 2/0/0 | low | fold into event_rec (real type has no offsets there: check) |
| GameIo | game/options_panel/options_panel.h:41 | Game (game/party/game.h:147) | 2 methods | 2 (options_panel.cpp) | no | - | 2/0/0 | high | method twin: declare the 2 method(s) on Game |
| I64VecW | game/world_data/world_data.h:110 | VarVec (include/script_state.h:28) | 4 methods | 2 (world_data.cpp) | no | reach:2 | 2/0/0 | high | method twin: declare the 4 method(s) on VarVec |
| MenuScreen | game/options_panel/hud.h:20 | ? | f18 | 2 (hud_source.cpp) | likely | - | 2/0/0 | medium | a screen in g_windows.arr[n] (slot_as<T>(n)): fold into that slot's screen class (head_rec family, game/screen_records.h) |
| MissionLog | game/agent4/agent_logic.cpp:7825 | Mission (game/mission.h:74) | 1 methods | 2 (agent_logic.cpp) | no | reach:1 | 0/1/1 | medium | method twin: declare the 1 method(s) on Mission |
| PSys | game/script/script_interpreter.h:263 | std::vector<psys> | 1 methods | 2 (script_interpreter.cpp) | no | reach:3 | 0/2/0 | high | fold into std::vector<psys> (pads cover the fields) |
| PanelMesh | game/inventory_panel/loading_screen.cpp:28 | mesh (rgl/mesh.h:58) | 1 methods | 2 (loading_screen.cpp) | no | - | 2/0/0 | medium | method twin: declare the 1 method(s) on mesh |
| Profile | game/char_sheet/cs_update.h:66 | ? | troop | 2 (cs_update.cpp) | likely | - | 2/0/0 | medium | a screen in g_windows.arr[n] (slot_as<T>(n)): fold into that slot's screen class (head_rec family, game/screen_records.h) |
| SetTimeKeys | game/party2/party_2.cpp:2897 | keyframes (include/keyframes.h:16) | frames, times, count | 2 (party_2.cpp) | no | - | 0/0/2 | medium | fold into keyframes (pads cover the fields) |
| map_ground | game/mission5/game_2.cpp:335 | NavData (game/terrain_b/nav_data.h:48) | verts, cells, edges | 2 (game_2.cpp) | no | reach:2 | 0/2/0 | medium | fold into NavData (pads cover the fields) |
| node_gt | game/hnode.h:23 | HeapGt (game/agent2/agent_types.h:119) | 1 methods | 2 (party_2.cpp, party_2.h) | no | - | 1/0/1 | low | method twin: declare the 1 method(s) on HeapGt |
| rgl_config | rgl/resource_manager.h:15 | Config (game/config/config_block.h:13) | 1 methods | 2 (resource_manager.cpp) | no | reach:2 | 2/0/0 | low | method twin: declare the 1 method(s) on Config |
| skin_calls | game/terrain_b/render_batch.h:77 | skin (rgl/resource.h:92) | 1 methods | 2 (terrain_b.cpp) | no | - | 0/2/0 | high | method twin: declare the 1 method(s) on skin |
| tstring_alloc | game/mission5/data_files.h:22 | MapDialog (rgl/mesh2/gui_edit.h:179) | - | 2 (game_2.cpp, gui_edit.cpp) | no | reach:2 | 2/0/0 | high | fold into MapDialog (pads cover the fields) |
| AgentX | game/agent3/agent_types.h:56 | Agent (game/agent_obj.h:52) | 1 methods | 1 (agent_logic.cpp) | no | doc | 0/1/0 | high | method twin: declare the 1 method(s) on Agent |
| Blk2dd0 | game/options_panel/module_load.h:29 | EnvRow (game/env_row.h:13) | 1 methods | 1 (module_load.cpp) | no | - | 1/0/0 | medium | method twin: declare the 1 method(s) on EnvRow |
| Block | game/inventory_panel/module_files.cpp:115 | ItemKind (include/item_kind.h:44) | 1 methods | 1 (module_files.cpp) | no | reach:1 | 1/0/0 | medium | method twin: declare the 1 method(s) on ItemKind |
| CarryRef | game/world_data/world_data.h:182 | ItemRef (game/item_ref.h:13) | item | 1 (script_interpreter.cpp) | no | reach:1 | 1/0/0 | medium | fold into ItemRef (pads cover the fields) |
| ElemCopier | game/party2/party_2.h:63 | allocator | 6 methods | 1 (party_2.cpp) | no | reach:1 | 0/0/1 | high | fold into allocator (pads cover the fields) |
| EventRec | game/party2/party_2.h:76 | E92 (game/party/game.h:114) | target, angle, f14, pos | 1 (party_2.cpp) | no | reach:1 | 0/1/0 | medium | fold into E92 (real type has no offsets there: check) |
| FieldHost | game/mission_ui2/field_panel.h:20 | ContainerBase (game/ui/container.h:16) | views | 1 (field_panel.h) | no | - | 0/0/1 | medium | fold into ContainerBase (real type has no offsets there: check) |
| FireAgent | game/agent4/agent_logic.h:20 | Missile (game/missile.h:16) | 1 methods | 1 (agent_logic.cpp) | no | - | 0/1/0 | medium | method twin: declare the 1 method(s) on Missile |
| FireRay | game/terrain_a/misc.h:99 | capsule (include/shape.h:80) | from, to, width | 1 (agent_logic.cpp) | no | - | 0/1/0 | high | fold into capsule (real type has no offsets there: check) |
| GameData | game/agent5/party_create.cpp:71 | GameData (include/game_data.h:101) | 1 methods | 1 (party_create.cpp) | no | doc,reach:1 | 1/0/0 | high | method twin: declare the 1 method(s) on GameData |
| HashTab | game/cls_map/cls_map.cpp:67 | GlyphMap (game/font.h:30) | 1 methods | 1 (cls_map.cpp) | no | doc,reach:1 | 1/0/0 | medium | method twin: declare the 1 method(s) on GlyphMap |
| HudOuter | game/agent5/agent_hud.h:53 | Camera (game/party2/camera_class.h:21) | 1 methods | 1 (agent_hud.cpp) | no | doc | 1/0/0 | low | method twin: declare the 1 method(s) on Camera |
| HudOuter | game/script/script_interpreter.cpp:147 | Camera (game/party2/camera_class.h:21) | 1 methods | 1 (script_interpreter.cpp) | no | doc | 0/0/1 | low | method twin: declare the 1 method(s) on Camera |
| I64VecW | game/script/script_interpreter.cpp:920 | I64Vec (include/i64_vec.h:7) | 1 methods | 1 (script_interpreter.cpp) | no | - | 0/0/1 | high | method twin: declare the 1 method(s) on I64Vec |
| LabelBox | game/agent4/agent_logic.cpp:7841 | matrix (rgl/rgl_math.h:69) | a, b, scale | 1 (agent_logic.cpp) | no | - | 1/0/0 | medium | fold into matrix (real type has no offsets there: check) |
| MapBlock | game/cls_map/progress_bar.h:46 | E68 (include/e68.h:14) | - | 1 (progress_bar.cpp) | no | reach:1 | 1/0/0 | medium | fold into E68 (pads cover the fields) |
| MapEditorDialog_s2f250 | game/mission5/mesh_3.h:231 | GameApp (game/config/core.h:20) | 1 methods | 1 (mesh_poly_ray.cpp) | no | - | 0/1/0 | high | method twin: declare the 1 method(s) on GameApp |
| MapPanel | game/inventory_panel/mission_ctx.cpp:120 | CampaignScreen (game/campaign_screen.h:17) | 1 methods | 1 (mission_ctx.cpp) | no | doc | 0/1/0 | low | method twin: declare the 1 method(s) on CampaignScreen |
| ObjectList | game/options_panel/module_load.h:30 | ptype (game/ptype.h:25) | 1 methods | 1 (module_load.cpp) | no | doc | 1/0/0 | medium | method twin: declare the 1 method(s) on ptype |
| PairStack | game/ui/mission_ui.h:155 | ScriptStack (game/party2/camera.h:32) | pairs, count | 1 (script_interpreter.cpp) | no | reach:1 | 1/0/0 | medium | fold into ScriptStack (pads cover the fields) |
| RotFrame | game/script/script_interpreter.cpp:329 | rgl::matrix (rgl/rgl_math.h:69) | 1 methods | 1 (script_interpreter.cpp) | no | reach:2 | 0/1/0 | high | fold into rgl::matrix (pads cover the fields) |
| RuleRec | game/terrain_a/misc.h:29 | SpawnRec (game/mode_rec.h:19) | f0 | 1 (terrain_emit.cpp) | no | - | 1/0/0 | medium | fold into SpawnRec (pads cover the fields) |
| SkinApplyExt | game/terrain_b/troop_mgr.h:13 | Skin (include/skin.h:45) | 1 methods | 1 (poly_mesh.cpp) | no | reach:2 | 1/0/0 | high | method twin: declare the 1 method(s) on Skin |
| TStringArray | game/agent5/agent_hud.h:165 | tstring (game/tstring.h:19) | 1 methods | 1 (agent_hud.cpp) | no | - | 0/1/0 | high | method twin: declare the 1 method(s) on tstring |
| Team | game/inventory_panel/mission_ctx.h:48 | TeamRec (game/mission.h:60) | f0, leader, cls, relation | 1 (mission_ctx.cpp) | no | - | 1/0/0 | medium | fold into TeamRec (pads cover the fields) |
| TerrainExt | game/terrain_b/terrain.h:14 | Terrain (game/terrain_a/terrain.h:137) | 1 methods | 1 (poly_mesh.cpp) | no | - | 1/0/0 | high | method twin: declare the 1 method(s) on Terrain |
| TimeKey | game/script/script_interpreter.h:51 | Light (game/light.h:11) | 1 methods | 1 (script_interpreter.cpp) | no | reach:1 | 0/1/0 | medium | method twin: declare the 1 method(s) on Light |
| TriggerItem | game/script/script_interpreter.h:147 | rgl::lod_mesh (rgl/mesh.h:195) | 1 methods | 1 (script_interpreter.cpp) | no | reach:2 | 0/1/0 | high | fold into rgl::lod_mesh (pads cover the fields) |
| TroopX | game/mission_ui2/stat_panel_x.cpp:37 | Troop (include/troop.h:20) | 1 methods | 1 (stat_panel_x.cpp) | no | reach:1 | 1/0/0 | high | method twin: declare the 1 method(s) on Troop |
| WItem | game/script/script_interpreter.cpp:158 | Camera (game/party2/camera_class.h:21) | 1 methods | 1 (script_interpreter.cpp) | no | - | 0/1/0 | medium | method twin: declare the 1 method(s) on Camera |
| alloc34 | game/terrain_b/records.h:36 | vec18 (game/terrain_b/records.h:53) | - | 1 (poly_mesh.cpp) | no | - | 1/0/0 | medium | fold into vec18 (pads cover the fields) |
| alloc64 | game/terrain_b/records.h:36 | vec18 (game/terrain_b/records.h:53) | - | 1 (poly_mesh.cpp) | no | - | 1/0/0 | medium | fold into vec18 (pads cover the fields) |
| alloc_E12 | game/alloc_construct.h:13 | ElemCopier (game/party2/party_2.h:63) | 1 methods | 1 (party_2.h) | no | reach:1 | 1/0/0 | low | method twin: declare the 1 method(s) on ElemCopier |
| alloc_rec136 | game/alloc_construct.h:12 | vecArr (game/terrain_b/records.h:86) | 1 methods | 1 (poly_mesh.cpp) | no | - | 1/0/0 | medium | method twin: declare the 1 method(s) on vecArr |
| block_copier | rgl/mesh2/gui_edit.h:48 | edit_vec (rgl/mesh2/gui_edit.h:328) | 1 methods | 1 (gui_edit.cpp) | no | reach:1 | 1/0/0 | low | method twin: declare the 1 method(s) on edit_vec |
| contact_inl | rgl/dynamic/world_collide.cpp:416 | contact_list_m (rgl/scene/scene_entity.h:106) | 5 methods | 1 (world_collide.cpp) | no | - | 1/0/0 | low | method twin: declare the 5 method(s) on contact_list_m |
| curve4 | rgl/scene/emitter.h:24 | float_pair (rgl/scene/emitter.h:26) | t0, v0, t1, v1 | 1 (psys.cpp) | no | - | 0/1/0 | low | fold into float_pair (real type has no offsets there: check) |
| key_clip | rgl/resource.h:150 | named_object (rgl/resource.h:19) | tracks, extra | 1 (resmgr.cpp) | likely | reach:1 | 0/1/0 | low | fold into named_object (real type has no offsets there: check) |
| material_io | rgl/core/resource_io.h:43 | material (rgl/resource.h:66) | 1 methods | 1 (resmgr.cpp) | no | - | 1/0/0 | high | method twin: declare the 1 method(s) on material |
| mesh_flip | game/mesh_slot/mesh_ops.cpp:37 | mesh (rgl/mesh.h:58) | 1 methods | 1 (mesh_ops.cpp) | no | reach:1 | 0/1/0 | medium | method twin: declare the 1 method(s) on mesh |
| mesh_node | game/mesh_slot/mesh_scene.cpp:27 | rgl::mesh (rgl/mesh.h:58) | 1 methods | 1 (mesh_scene.cpp) | no | doc,reach:1 | 1/0/0 | high | fold into rgl::mesh (pads cover the fields) |
| mesh_record | game/config/config.h:104 | MeshTable (include/mesh_table.h:10) | resource, flags, id, pos +2 | 1 (rec_tables.cpp) | likely | reach:1 | 1/0/0 | low | fold into MeshTable (real type has no offsets there: check) |
| mesh_scene | game/inventory_panel/module_files.cpp:107 | Animator (game/animator.h:18) | 1 methods | 1 (module_files.cpp) | no | doc,reach:1 | 1/0/0 | medium | method twin: declare the 1 method(s) on Animator |
| mi_ops | game/terrain_b/render_batch.h:128 | mesh (rgl/mesh.h:58) | 1 methods | 1 (terrain_b.cpp) | no | - | 0/1/0 | high | method twin: declare the 1 method(s) on mesh |
| music_set | game/inventory_panel/module_files.cpp:100 | SoundMgr (include/sound_mgr.h:26) | 1 methods | 1 (module_files.cpp) | no | reach:1 | 1/0/0 | high | method twin: declare the 1 method(s) on SoundMgr |
| sound_record | game/config/config.h:115 | music_name (rgl/mesh2/rec_tables.h:176) | name, f10, samples, nsamples | 1 (rec_tables.cpp) | likely | - | 0/0/1 | medium | fold into music_name (real type has no offsets there: check) |
| texture_x | game/terrain_a/misc.h:63 | texture (rgl/resource.h:34) | 1 methods | 1 (terrain_render.cpp) | no | - | 1/0/0 | high | method twin: declare the 1 method(s) on texture |
| vec2x | game/agent4/agent_logic.cpp:3467 | vec2 (include/vec.h:13) | 1 methods | 1 (agent_logic.cpp) | no | - | 0/1/0 | high | method twin: declare the 1 method(s) on vec2 |
| AgentX | rgl/xmlcfg/game_util.h:31 | Agent (game/agent_obj.h:52) | 3 methods | 0 | no | doc | - | high | method twin: declare the 3 method(s) on Agent |
| BeardMeshNew | game/cls_map/progress_bar.cpp:1216 | BeardMesh (include/skin.h:36) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on BeardMesh |
| BlowInit | game/blow.h:39 | Agent (game/agent_obj.h:52) | 2 methods | 0 | no | doc | - | high | method twin: declare the 2 method(s) on Agent |
| GameDataV | game/inventory_panel/inv_common.h:33 | GameData (include/game_data.h:101) | 8 methods | 0 | no | - | - | high | method twin: declare the 8 method(s) on GameData |
| MissionV | game/inventory_panel/mission_ctx.h:23 | Mission (game/mission.h:74) | 2 methods | 0 | no | - | - | high | method twin: declare the 2 method(s) on Mission |
| MissionW | game/world_data/world_data.h:93 | Mission (game/mission.h:74) | 8 methods | 0 | no | - | - | high | method twin: declare the 8 method(s) on Mission |
| PartyV | game/inventory_panel/mission_ctx.cpp:126 | Party (include/party.h:43) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on Party |
| PartyX | game/party_x.h:11 | Party (include/party.h:43) | 17 methods | 0 | no | - | - | high | method twin: declare the 17 method(s) on Party |
| TroopIO | game/party/game.h:123 | Troop (include/troop.h:20) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on Troop |
| V2_s29130 | game/mission5/mission_5.cpp:1566 | V2 (include/vec.h:31) | 3 methods | 0 | no | - | - | high | method twin: declare the 3 method(s) on V2 |
| V2r | game/config/widget_mgr.cpp:235 | V2 (include/vec.h:31) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on V2 |
| WidgetDtor | game/party/party_window.h:36 | MissionUi (game/ui/mission_ui.h:31) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on MissionUi |
| edit_vec | rgl/mesh2/gui_edit.h:328 | block_copier (rgl/mesh2/gui_edit.h:48) | 1 methods | 0 | no | reach:1 | - | high | method twin: declare the 1 method(s) on block_copier |
| frustum_view | game/renderer.h:49 | frustum (include/frustum.h:21) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on frustum |
| mat3_value | game/script/script_interpreter.cpp:2706 | mat3 (rgl/rgl_math.h:58) | - | 0 | no | - | - | high | fold into mat3 (pads cover the fields) |
| material_x | game/terrain_a/misc.h:66 | material (rgl/resource.h:66) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on material |
| mesh_anim_x | game/mesh_slot/mesh_scene.cpp:437 | mesh (rgl/mesh.h:58) | 1 methods | 0 | no | reach:1 | - | high | method twin: declare the 1 method(s) on mesh |
| mesh_calls | game/terrain_b/render_batch.h:33 | mesh (rgl/mesh.h:58) | 2 methods | 0 | no | - | - | high | method twin: declare the 2 method(s) on mesh |
| mesh_flip | game/terrain_b/terrain_b.h:39 | mesh (rgl/mesh.h:58) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on mesh |
| mesh_node | rgl/xmlcfg/game_util.h:72 | mesh (rgl/mesh.h:58) | 1 methods | 0 | no | doc | - | high | method twin: declare the 1 method(s) on mesh |
| scene_ms2 | game/mesh_slot/mesh_slot.h:38 | scene (rgl/scene_rec.h:40) | - | 0 | no | - | - | high | fold into scene (pads cover the fields) |
| skin_x | game/terrain_a/misc.h:69 | skin (rgl/resource.h:92) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on skin |
| string_view_ | rgl/shapes/shapes.h:32 | string_view_ (game/script/script_interpreter.h:58) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on string_view_ |
| tstr | game/script/script_interpreter.h:97 | tstring (game/tstring.h:19) | 1 methods | 0 | no | reach:1 | - | high | method twin: declare the 1 method(s) on tstring |
| vec | game/agent5/agent_hud.h:48 | xlen_vec (include/vec_xlen.h:10) | 1 methods | 0 | no | reach:1 | - | high | method twin: declare the 1 method(s) on xlen_vec |
| vec2c | game/strat/strat_ext.h:33 | vec2 (include/vec.h:13) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on vec2 |
| vec2r | game/party2/party_2.h:72 | vec2 (include/vec.h:13) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on vec2 |
| vec2x | rgl/xmlcfg/game_util.h:46 | vec2 (include/vec.h:13) | 2 methods | 0 | no | - | - | high | method twin: declare the 2 method(s) on vec2 |

### B. Real type exists but conflicts (9)

| view | file:line | overlays | fields named | casts (files) | 64? | path | rows e/n/- | conf | notes |
|---|---|---|---|---|---|---|---|---|---|
| DistObj | game/inventory_panel/map_tooltip.cpp:48 | Party (include/party.h:43) | x, y, px, py | 2 (map_panel.cpp) | yes | - | 0/2/0 | high | members of Party at the same offsets disagree: +0x30 float x vs PartyV4 pos; +0x30 float y vs PartyV4 pos |
| GlyphNode | game/font.h:26 | iterator (rgl/dynamic/dynamic_entity.h:39) | next, prev, key, val | 4 (agent_logic.cpp, cls_map.cpp) | no | doc,reach:2 | 2/0/2 | medium | members of iterator at the same offsets disagree: +0x0 GlyphNode* next vs Contact* _Ptr |
| ItemStackG | game/mesh_slot/mesh_game.cpp:38 | ItemRef (game/item_ref.h:13) | b | 3 (mesh_game.cpp, script_interpreter.cpp) | no | reach:4 | 3/0/0 | high | members of ItemRef at the same offsets disagree: +0x0 unsigned a b vs int item |
| list_int | include/list_int.h:12 | list_node (game/options_panel/world_app.cpp:141) | head, w04, count | 3 (cls_map.cpp, world_app.cpp) | likely | reach:1 | 0/1/2 | low | members of list_node at the same offsets disagree: +0x4 list_node_int* w04 vs HNode* sentinel |
| FrustumBlock | game/char_sheet/terrain_page_input.h:43 | frustum (include/frustum.h:21) | d | 2 (terrain_page_input.cpp) | no | - | 2/0/0 | medium | members of frustum at the same offsets disagree: +0x0 int[0x48] d vs char perspective |
| OptionsMenu | game/cls_map/cls_map.h:182 | GameKeys (game/game_keys.h:7) | f00, f04, f08, f0c +23 | 2 (cls_map.cpp) | no | - | 2/0/0 | high | members of GameKeys at the same offsets disagree: +0x0 int f00 vs int[0x25][2] entry |
| CandRec | game/terrain_a/misc.h:30 | Stack (include/party.h:30) | f0, troop, f8 | 1 (terrain_emit.cpp) | no | - | 1/0/0 | medium | members of Stack at the same offsets disagree: +0x8 unsigned char f8 vs int flags |
| MissionX | game/script/script_interpreter.h:276 | Mission (game/mission.h:74) | 28 methods | 0 | no | reach:28 | - | high | **the script's Mission (157 mission() calls): EntryPoint[64] at +0x670 vs Mission's MarkRec[64], teams / mode / cam_frame to reconcile with game/mission.h, then mission() returns Mission*** |
| mat3_s2cf60 | game/mission5/mesh_3.h:113 | matrix (rgl/rgl_math.h:69) | m | 0 | no | - | - | high | members of matrix at the same offsets disagree: +0x0 float[12] m vs vector4 s |

### C. Real type not yet defined (10)

| view | file:line | overlays | fields named | casts (files) | 64? | path | rows e/n/- | conf | notes |
|---|---|---|---|---|---|---|---|---|---|
| mat3_s2f250 | game/mission5/mesh_3.h:225 | ? | s, f, u | 5 (mesh_3.h, mesh_poly_ray.cpp) | no | reach:1 | 1/4/0 | high | overlays raw memory / no real record found (sources: ?) |
| rec34 | include/rec34.h:10 | ? | f00, name, f14, body | 3 (game_2.cpp, rec34.h) | likely | reach:2 | 2/0/1 | low | overlays raw memory / no real record found (sources: void) |
| HudOuter | game/config/widget_mgr.cpp:307 | ? | 1 methods | 1 (widget_mgr.cpp) | no | doc,reach:1 | 1/0/0 | low | overlays raw memory / no real record found (sources: define) |
| MeshRec | game/script/script_interpreter.h:245 | ? | name | 1 (script_interpreter.cpp) | likely | reach:1 | 0/1/0 | medium | overlays raw memory / no real record found (sources: void) |
| RecK | include/rec_dtors.h:31 | ? | head, name, s | 1 (gui_edit.cpp) | likely | - | 0/0/1 | low | overlays raw memory / no real record found (sources: void) |
| RenderXform | game/cls_map/progress_bar.cpp:689 | ? | m, f30, f34, f38 | 1 (progress_bar.cpp) | no | - | 0/1/0 | low | overlays raw memory / no real record found (sources: char) |
| frustum_view | game/terrain_b/render_batch.h:41 | ? | 1 methods | 1 (render_batch.h) | no | - | 0/0/1 | high | overlays raw memory / no real record found (sources: void) |
| obj_c0 | include/mesh_parts.h:37 | ? | out, in | 1 (entity.cpp) | likely | - | 1/0/0 | low | overlays raw memory / no real record found (sources: void) |
| grid_view | rgl/core/frustum_grid.h:10 | ? | n, lo, step | 0 | likely | - | - | high | overlays raw memory / no real record found |
| mat4_ro | game/renderer.h:46 | ? | 1 methods | 0 | no | - | - | high | overlays raw memory / no real record found |

### D. Std container stand-ins (51)

| view | file:line | overlays | fields named | casts (files) | 64? | path | rows e/n/- | conf | notes |
|---|---|---|---|---|---|---|---|---|---|
| I64Vec | include/i64_vec.h:7 | std::vector<__int64> | _Myfirst, _Mylast, _Myend | 5 (game.cpp, script_interpreter.cpp) | yes | doc | 3/0/2 | high | vector-shaped (_Myend/_Myfirst/_Mylast) |
| tvec | rgl/mesh2/mesh_2.h:63 | xlen_vec (include/vec_xlen.h:10) | begin, end, cap | 0 | yes | reach:1 | - | high | vector-shaped (begin/cap/end) |
| Vec1c8 | game/agent/agent.h:71 | std::vector<PlacedObj> | first, last | 2 (agent_logic.cpp) | yes | - | 0/2/0 | high | vector-shaped (first/last) |
| Vec5c | game/inventory_panel/game_tick.h:24 | Rec5c (game/rec5c.h:9) | _Myfirst, _Mylast, _Myend | 1 (game_tick.h) | yes | - | 0/0/1 | high | vector-shaped (_Myend/_Myfirst/_Mylast) |
| RawVec | include/raw_vec.h:9 | ? | first, last, end | 0 | yes | - | - | low | vector-shaped (end/first/last) |
| Vec2c | game/agent/agent.h:70 | ? | first, last | 0 | yes | - | - | medium | vector-shaped (first/last) |
| Vec38 | game/agent/agent.h:69 | ? | first, last | 0 | yes | - | - | medium | vector-shaped (first/last) |
| vec3_holder | rgl/mesh2/mesh_2.h:45 | ? | begin, end, cap | 0 | yes | - | - | low | vector-shaped (begin/cap/end) |
| PropVec | game/prop_vec.h:11 | std::vector<PlacedObj> | 3 methods | 39 (agent.cpp, agent_logic.cpp +7) | no | reach:11 | 18/21/0 | medium | vector-shaped (std:: base) |
| xlen_vec | include/vec_xlen.h:10 | vec (rgl/shapes/shapes.h:86) | 2 methods | 9 (dust.cpp, dynamic_entity.h +5) | no | reach:5 | 8/0/1 | low | vector-shaped (std:: base) |
| vec_cfg_pub | game/inventory_panel/widget_text.cpp:212 | vec_model (game/inventory_panel/widget_text.cpp:37) | - | 3 (widget_text.cpp) | no | reach:3 | 3/0/0 | high | vector-shaped (std:: base) |
| MeshPtrVecPub | game/terrain_b/mission.cpp:81 | MeshVec (game/terrain_b/tmesh.h:10) | - | 2 (mission.cpp) | no | reach:1 | 1/0/1 | high | vector-shaped (std:: base) |
| rawvec | rgl/scene/world_items.cpp:78 | std::vector<entity> | al, first, last, end | 2 (world_items.cpp) | no | reach:2 | 2/0/0 | low | vector-shaped (end/first/last) |
| BoardMeshVec | game/char_sheet/facegen_win.cpp:35 | std::vector<mesh> | 1 methods | 1 (facegen_win.cpp) | no | - | 1/0/0 | medium | vector-shaped (std:: base) |
| E92VecPub | game/party/game.cpp:938 | TrackRec (game/party/game.h:95) | - | 1 (game.cpp) | no | - | 1/0/0 | high | vector-shaped (std:: base) |
| IntVecTidy | game/world_data/world_data.cpp:134 | std::vector<int> | - | 1 (world_data.cpp) | no | reach:1 | 1/0/0 | medium | vector-shaped (std:: base) |
| MeshPush | game/terrain_b/mission.cpp:509 | std::vector<mesh> | 2 methods | 1 (mission.cpp) | no | reach:1 | 0/1/0 | high | vector-shaped (std:: base) |
| PartyVecTidy | game/world_data/world_data.cpp:133 | std::vector<T> | - | 1 (world_data.cpp) | no | reach:1 | 1/0/0 | medium | vector-shaped (std:: base) |
| SceneObjVec | game/char_sheet/cs_common.h:74 | ? | - | 1 (cs_common.h) | no | - | 0/0/1 | medium | vector-shaped (std:: base) |
| BufTriple | rgl/mesh2/gui_edit.h:287 | PartyTemplate (include/game_recs.h:94) | first, last, end | 0 | likely | - | - | low | vector-shaped (end/first/last) |
| EventVec | game/party2/party_2.h:108 | Party (include/party.h:43) | al, first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| ObjPtrVecPub | game/terrain_b/mission.cpp:84 | ? | - | 0 | no | - | - | high | vector-shaped (std:: base) |
| PVec | game/script/script_interpreter.cpp:197 | xlen_vec (include/vec_xlen.h:10) | f0, first, last, end | 0 | likely | reach:1 | - | high | vector-shaped (end/first/last) |
| V4List | game/ui/map_list.h:14 | ? | alloc, first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| VecPtr3 | game/agent2/agent_ai.h:13 | ? | first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| buy_view | rgl/shapes/shapes.h:113 | ? | 1 methods | 0 | no | - | - | high | vector-shaped (std:: base) |
| contact_vec | rgl/scene/scene_entity.h:98 | Contact (rgl/scene/scene_entity.h:97) | alloc, first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| str_vec | rgl/mesh2/mesh_2.h:114 | ? | first, last, end | 0 | likely | - | - | low | vector-shaped (end/first/last) |
| tstring_vec_pub | rgl/mesh2/mesh_2.h:150 | ? | - | 0 | no | - | - | high | vector-shaped (std:: base) |
| vec | rgl/mesh.h:167 | ? | alloc, first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| vec | rgl/shapes/shapes.h:86 | ? | al, first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| vec100 | game/terrain_a/misc.h:43 | ? | _Alval, first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| vec12 | game/agent2/agent_types.h:92 | PolyMesh (game/terrain_b/poly_mesh.h:113) | first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| vec18 | game/terrain_b/records.h:53 | alloc18 (game/terrain_b/records.h:36) | first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| vec24 | game/terrain_a/misc.h:40 | ? | _Alval, first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| vec34 | game/terrain_b/records.h:55 | alloc34 (game/terrain_b/records.h:36) | first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| vec36 | game/terrain_a/misc.h:39 | ? | _Alval, first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| vec516 | game/terrain_a/misc.h:42 | ? | alloc, first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| vec52 | game/terrain_a/misc.h:41 | ? | _Alval, first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| vec64 | game/terrain_b/records.h:54 | alloc64 (game/terrain_b/records.h:36) | first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| vecArr | game/terrain_b/records.h:86 | allocArr (game/arr_set.h:25) | first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| vecArr2 | game/terrain_b/records.h:87 | allocArr (game/arr_set.h:25) | first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| vecArrS | game/terrain_b/records.h:88 | allocArr (game/arr_set.h:25) | first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| vecE1c8 | game/agent2/agent_types.h:103 | allocE1c8 (game/agent2/agent_types.h:91) | first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| vecMRow36 | game/terrain_b/records.h:52 | alloc24 (game/terrain_a/misc.h:35) | first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| vec_e1f8 | game/triggers/trigger_vec.h:46 | GameKeys (game/game_keys.h:7) | n, begin, end | 0 | no | - | - | low | vector-shaped (begin/end) |
| vec_e50 | game/triggers/trigger_vec.h:45 | GameKeys (game/game_keys.h:7) | n, begin, end | 0 | no | - | - | low | vector-shaped (begin/end) |
| vec_eb0 | game/triggers/trigger_vec.h:47 | GameKeys (game/game_keys.h:7) | n, begin, end | 0 | no | - | - | low | vector-shaped (begin/end) |
| vec_model | game/inventory_panel/widget_text.cpp:37 | Skin (include/skin.h:45) | alloc, first, last, end | 0 | likely | reach:1 | - | high | vector-shaped (end/first/last) |
| vecint | game/agent2/agent_types.h:102 | alloc_int (game/alloc_construct.h:14) | first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| vecstr | game/agent2/agent_types.h:97 | alloc_tstring (game/alloc_construct.h:10) | first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |

### E. Fake vtable / slot views (5)

| view | file:line | overlays | fields named | casts (files) | 64? | path | rows e/n/- | conf | notes |
|---|---|---|---|---|---|---|---|---|---|
| AccessStream | game/inventory_panel/widget_text.cpp:66 | Map10007 (include/map10007.h:6) | 20 methods | 1 (module_files.cpp) | no | reach:1 | 1/0/0 | medium | virtual-slot view: calls through it stand for the real virtual interface |
| EmptySlot | game/ui/mission.h:22 | PlacedObj (game/placed_obj.h:18) | 1 methods | 1 (mission_ui.cpp) | no | doc,reach:1 | 1/0/0 | medium | virtual-slot view: calls through it stand for the real virtual interface |
| EntVt | game/world_data/world_data.h:48 | MissionUi (game/ui/mission_ui.h:31) | 25 methods | 0 | no | - | - | high | virtual-slot view: calls through it stand for the real virtual interface |
| ItemD_vt | game/options_panel/mission_item_recs.h:181 | item_base (game/options_panel/mission_item_recs.h:17) | panel, back_alloc, blk0, fwd_alloc +8 | 0 | likely | - | - | high | virtual-slot view: calls through it stand for the real virtual interface |
| ScrollVt | game/world_data/mesh_hud.h:58 | WItem (game/config/widget_mgr.h:72) | 28 methods | 0 | no | - | - | high | virtual-slot view: calls through it stand for the real virtual interface |

### F. Byte-match blocked (2)

| view | file:line | overlays | fields named | casts (files) | 64? | path | rows e/n/- | conf | notes |
|---|---|---|---|---|---|---|---|---|---|
| contact_ops | rgl/dynamic/dynamic_entity.h:37 | contact_list_m (rgl/scene/scene_entity.h:106) | 12 methods | 2 (world_collide.cpp) | no | reach:1 | 0/2/0 | high | **D-shaped (VC7.1 vector<Contact> members written out) and load-bearing: the stock <vector> does not give 0x427450 / 0x427540 / 0x427590; the 64-bit build can use std::vector<Contact> under a non-MSVC branch** |
| ElemVt | game/ui/mission_ui.h:152 | FxElem (include/fx.h:41) | 2 methods | 0 | no | doc | - | high | **declaration-only polymorphic class kept because cl 13.10 swaps two loads in Recf0::frame_update (0x4d63b0) without it; test removing it with run_all on video_page.cpp** |

### Incomplete real types (146 records with pads that are not views)

Name (file:line) pads/pad bytes, by pad count. These are the classes the views above fold into, or records reached
only directly; their pads are the members still to name.

- `game` (33): Agent (agent_obj.h:52) 13/0x2c, Mission (mission.h:74) 9/0x64a, CharSheet (char_sheet_panel.h:21) 7/0x9a, Missile (missile.h:16) 4/0xe, Rec1208 (conv_screen.h:18) 3/0x2b, AnimBox (ptype.h:15) 2/0x8, C4c3050 (screen_records.h:123) 2/0x7, C4c31c0 (screen_records.h:74) 2/0x8, C4c3290 (screen_records.h:99) 2/0x1c, Event (event.h:23) 2/0x8, HitInfo (blow.h:12) 2/0x34, MarkRec (mark_rec.h:13) 2/0xc, PartyScreen (party_screen_class.h:20) 2/0x7, Rec5c (rec5c.h:9) 2/0x44, ResultWindow (result_window.h:17) 2/0xb, music_player (music_player.h:21) 2/0x7, CampaignScreen (campaign_screen.h:17) 1/0x10b, EmitDef (emit.h:18) 1/0x8, Font (font.h:41) 1/0x4, GlyphMap (font.h:30) 1/0x4, InputRec (agent_ai.h:19) 1/0x14, MapFlags (map_flags.h:7) 1/0x1, Rec118 (screen_records.h:153) 1/0x8, Rec1bc (screen_records.h:303) 1/0x3, Slot (slot.h:16) 1/0x7, SlotRec (screen_records.h:66) 1/0x1, SpawnRec (mode_rec.h:19) 1/0x4, StrBucket (str_table.h:19) 1/0x4, TerrainPage (terrain_page.h:18) 1/0x28, head_rec (head_rec.h:45) 1/0x4, ini_entry (ini_file.h:11) 1/0xc, junk_tag (junk_tag.h:7) 1/0x4, mesh_inst (renderer.h:34) 1/0x2c
- `include` (15): GameData (game_data.h:101) 12/0x64, Party (party.h:43) 7/0x17, ItemKind (item_kind.h:44) 4/0x10, Elem (dialog_rec.h:11) 3/0xc, EnvRec (fx.h:85) 3/0xb, renderer (renderer.h:16) 3/0xf, OwnedRec48 (rec_dtors.h:40) 2/0x24, SceneEditor (game_data.h:50) 2/0x7, ScriptState (script_state.h:33) 2/0x7, FactionRec (game_recs.h:62) 1/0x3, QuestRec (game_recs.h:17) 1/0x8, Tail2 (tail2.h:9) 1/0xc, Timer (timer.h:16) 1/0x4, Troop (troop.h:20) 1/0x10, frustum (frustum.h:21) 1/0x2
- `game/terrain_a` (9): Terrain (terrain.h:137) 7/0x5f0, RenderScene (misc.h:144) 6/0x34, view_t (misc.h:188) 6/0x15a, light_src (misc.h:118) 4/0x20, FireAgent (misc.h:106) 2/0x58, TerrainParticle (terrain.h:87) 2/0x8, light_t (misc.h:177) 2/0x40, flag_obj (misc.h:184) 1/0x10, fog_rec (misc.h:132) 1/0x7
- `rgl/mesh2` (9): MapDialog (gui_edit.h:179) 5/0x6660, MapRecord (gui_edit.h:146) 5/0x1a8, csv_row (mesh_2.h:122) 5/0x4c, MapEditorDialog (gui_edit.h:226) 4/0x362, edit_vert (gui_edit.h:317) 2/0x7, screen_gadget (screen_cfg.h:37) 2/0x20, MapObject (gui_edit.h:124) 1/0x10, Rec0xb0Tail (gui_edit.h:293) 1/0x84, item38_vec (gui_edit.h:339) 1/0x8
- `game/party` (4): Game (game.h:147) 20/0x285, PartyWindow (party_window_class.h:39) 2/0x6, TrackRec (game.h:95) 2/0x44, GameTimer (game.h:49) 1/0x4
- `game/char_sheet` (10): MapScreen (save_game.h:81) 9/0x845d913, SaveWriter (save_game.h:38) 3/0x2b, TextRow (editor_ui.h:72) 2/0x1f8, cell_rec (dust.h:60) 2/0x28, EditorApp (editor_ui.h:60) 1/0xe8, OuterGame (save_records.h:50) 1/0x13a00, SaveCtx (save_game.h:61) 1/0xc, ScrollHost (cs_update.h:63) 1/0x484, UiDsv (cs_update.h:52) 1/0xf0, cPairs96 (save_records.h:35) 1/0x300
- `game/inventory_panel` (9): MissionCtx (mission_ctx_rec.h:7) 4/0x150, TutorialMenuRec (char_sheet.cpp:91) 4/0x4c, PartyCellRec (map_tooltip.cpp:33) 3/0x9c, BigRec (map_panel.cpp:79) 2/0x84, GroundItem (map_panel.cpp:87) 2/0x48, PartyInfo (map_tooltip.cpp:27) 2/0x54, note_window (note_window.cpp:204) 2/0x28, FleetRec (map_panel.cpp:664) 1/0x4c, PartyTplRec (module_files.cpp:89) 1/0x80
- `game/ui` (8): SelRec (mission.h:28) 5/0x3c, MissionUi (mission_ui.h:31) 3/0x8, NamedRec (named_rec.h:12) 2/0x14, NamedSelRec (world.h:23) 1/0x1a7, NodeA (container.h:45) 1/0x7, SortVal (item_ref.h:26) 1/0x4, UiA (ui_widgets.h:14) 1/0x2, UiC (ui_widgets.h:58) 1/0x3
- `game/config` (5): InputSet (config.h:55) 6/0x72c, GameApp (core.h:20) 4/0x1b, Config (config_block.h:13) 1/0x4, CreateGameOptionsPage (config.h:42) 1/0x374, psys_record (config.h:95) 1/0x9c
- `game/script` (6): SubOrder (script_interpreter.cpp:660) 3/0x24, Leader (script_interpreter.cpp:673) 2/0x24, SiteRec (script_interpreter.cpp:2448) 2/0x1cc, EntryPoint (script_interpreter.h:257) 1/0xf0, MTimer (script_interpreter.h:268) 1/0x4, PickArg (script_interpreter.cpp:2447) 1/0x400
- `game/mission5` (5): MapNav (game_2.h:106) 3/0x14, MapEdge (game_2.h:99) 2/0x1c, MapNavVert (game_2.h:86) 2/0x8, FactionFile (data_files.h:102) 1/0x3, MapNode (game_2.h:93) 1/0x60
- `game/terrain_b` (3): light (render_batch.h:89) 4/0x38, SceneFile (scene_file.h:38) 3/0x83, MEdge (poly_mesh.h:36) 1/0x8
- `game/world_data` (5): CarryKind (world_data.h:174) 3/0x22f, EmitterCtl (world_data.h:210) 2/0x59f0, I64VecOp (world_data.h:116) 1/0x4, IntVecW (world_data.h:124) 1/0x4, ScrollHolder (mesh_hud.h:68) 1/0x17c
- `game/strat` (1): gui_manager (gui_manager.h:55) 7/0x1b
- `game/cls_map` (3): MissionScreen (cls_map.h:110) 3/0x128, NameRec48 (cls_map.cpp:512) 2/0x1c, E20b (cls_map.h:85) 1/0xc
- `rgl/xmlcfg` (3): Obj3 (game_util.h:83) 3/0x134, RecF (records.h:33) 2/0x78, RecP (records.h:49) 1/0x4
- `game/options_panel` (3): ValueSource (hud.h:59) 3/0x164, TreeW (world_app.cpp:263) 1/0x8, list_node (world_app.cpp:141) 1/0x4
- `game/triggers` (4): ItemRec (item_rec.h:27) 1/0x4, ItemRec2 (item_rec.h:49) 1/0x4, MouseRec (trigger_vec.h:110) 1/0x2c, TimedVal (trigger_vec.cpp:1242) 1/0x4
- `game/agent4` (1): Obj3Place_ (agent_logic.cpp:234) 3/0x134
- `game/agent2` (2): VecPair2 (agent_ai.h:22) 2/0x14, V32 (agent_types.h:61) 1/0x14
- `rgl/core` (1): buf3 (frustum_grid.h:20) 2/0xcc
- `game/party2` (2): ScriptStack (camera.h:32) 1/0x4, event_rec (party_2.cpp:2111) 1/0x40
- `game/mesh_slot` (2): TimeKey (slot.cpp:36) 1/0x44, vec_i4 (mesh_terrain.cpp:663) 1/0x4
- `game/agent` (1): Rec20 (agent.h:25) 1/0x10
- `game/map_editor` (1): TargetW (exchange_screens.h:93) 1/0x30c
- `rgl/scene` (1): strategic_entity (scene_entity.h:144) 1/0x28

<!-- end generated -->

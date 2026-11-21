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
| A | 74 | 0 | 33 | 210 |
| B | 5 | 0 | 2 | 6 |
| C | 7 | 0 | 2 | 12 |
| D | 5 | 0 | 1 | 2 |
| E | 2 | 0 | 1 | 1 |
| F | 1 | 0 | 1 | 0 |
| all | 94 | 0 | 40 | 231 |

### A. Fold into the real type (74)

| view | file:line | overlays | fields named | casts (files) | 64? | path | rows e/n/- | conf | notes |
|---|---|---|---|---|---|---|---|---|---|
| scene_ms2 | game/mesh_slot/mesh_slot.h:38 | scene (rgl/scene_rec.h:40) | - | 42 (mesh_char.cpp, mesh_pass.cpp) | no | - | 18/24/0 | high | fold into scene (pads cover the fields) |
| TroopRec | include/troop.h:131 | Troop (include/troop.h:19) | 9 methods | 37 (inventory_panel.cpp, troop_rec.cpp) | no | doc,reach:22 | 16/21/0 | high | method twin: declare the 9 method(s) on Troop |
| string_view_ | game/script/script_interpreter.h:55 | tstring (game/tstring.h:19) | 1 methods | 36 (script_interpreter.cpp, script_interpreter.h) | no | reach:35 | 1/34/1 | medium | method twin: declare the 1 method(s) on tstring |
| AgentX | game/mission5/mission_5.h:379 | Agent (game/agent_obj.h:52) | 3 methods | 8 (mission_5.cpp) | no | doc | 0/8/0 | high | method twin: declare the 3 method(s) on Agent |
| EV | game/options_panel/mesh_edit.cpp:131 | std::vector<MEdge> | v | 6 (mesh_edit.cpp) | no | - | 6/0/0 | medium | fold into std::vector<MEdge> (pads cover the fields) |
| mesh_record | game/config/config.h:98 | MeshTable (include/mesh_table.h:10) | resource, flags, id, pos +2 | 6 (obj_tbl.cpp, rec_tables.cpp +1) | likely | reach:6 | 5/1/0 | low | fold into MeshTable (real type has no offsets there: check) |
| Matrix12_s2fe40 | game/mission5/mesh_3.h:303 | float | v | 5 (mesh_3.h, mesh_poly_ray.cpp) | no | reach:4 | 1/4/0 | high | fold into float (pads cover the fields) |
| sel_list | game/inventory_panel/widget_text.cpp:81 | std::vector | v | 4 (note_window.cpp) | likely | doc | 0/4/0 | medium | fold into std::vector (pads cover the fields) |
| EquipSet | game/mesh_slot/mesh_appear.cpp:32 | ItemRef (game/item_ref.h:14) | weapon, helmet, body, boots | 3 (agent_hud.cpp, mesh_game.cpp +1) | no | reach:1 | 1/1/1 | high | fold into ItemRef (real type has no offsets there: check) |
| mesh_node | game/party2/party_2.h:52 | mesh (rgl/mesh.h:48) | 1 methods | 3 (party_2.cpp) | no | doc | 0/0/3 | low | method twin: declare the 1 method(s) on mesh |
| rot3x | game/agent4/agent_logic.cpp:7483 | matrix (rgl/rgl_math.h:69) | 1 methods | 3 (agent_logic.cpp) | no | - | 0/0/3 | high | method twin: declare the 1 method(s) on matrix |
| rot3x | rgl/xmlcfg/game_util.h:54 | matrix (rgl/rgl_math.h:69) | 1 methods | 3 (game_util.cpp) | no | reach:3 | 3/0/0 | high | method twin: declare the 1 method(s) on matrix |
| vec4n | game/agent4/agent_logic.cpp:7244 | vector4 (rgl/rgl_math.h:14) | 2 methods | 3 (agent_logic.cpp) | no | reach:3 | 0/3/0 | medium | method twin: declare the 2 method(s) on vector4 |
| vec_arr | game/terrain_b/render_batch.h:127 | vec_table (include/vec_table.h:10) | data, count | 3 (terrain_b.cpp) | no | - | 0/3/0 | medium | fold into vec_table (pads cover the fields) |
| Blob16 | game/ui/mission.h:21 | vector4 (rgl/rgl_math.h:14) | a, b, c, d | 2 (mission_ui.cpp) | no | reach:2 | 2/0/0 | medium | **16-byte block copy of Sel::rec[].a / .c (int[4]): give those members a 4-int struct type and assign it; no 64-bit effect** |
| EditorObj | rgl/mesh2/rec_tables.cpp:175 | GameApp (game/config/core.h:20) | 1 methods | 2 (rec_tables.cpp) | no | - | 2/0/0 | low | method twin: declare the 1 method(s) on GameApp |
| MissionLog | game/agent4/agent_logic.cpp:7711 | Mission (game/mission.h:78) | 1 methods | 2 (agent_logic.cpp) | no | reach:1 | 0/1/1 | medium | method twin: declare the 1 method(s) on Mission |
| PanelMesh | game/inventory_panel/loading_screen.cpp:28 | mesh (rgl/mesh.h:48) | 1 methods | 2 (loading_screen.cpp) | no | - | 2/0/0 | medium | method twin: declare the 1 method(s) on mesh |
| Profile | game/char_sheet/cs_update.h:54 | ? | troop | 2 (cs_update.cpp) | likely | - | 2/0/0 | medium | a screen in g_windows.arr[n] (slot_as<T>(n)): fold into that slot's screen class (head_rec family, game/screen_records.h) |
| SetTimeKeys | game/party2/party_2.cpp:2818 | keyframes (include/keyframes.h:16) | frames, times, count | 2 (party_2.cpp) | no | - | 0/0/2 | medium | fold into keyframes (pads cover the fields) |
| node_gt | game/hnode.h:23 | HeapGt (game/agent2/agent_types.h:110) | 1 methods | 2 (party_2.cpp, party_2.h) | no | - | 1/0/1 | low | method twin: declare the 1 method(s) on HeapGt |
| rgl_config | rgl/resource_manager.h:15 | Config (game/config/config_block.h:13) | 1 methods | 2 (resource_manager.cpp) | no | reach:2 | 2/0/0 | low | method twin: declare the 1 method(s) on Config |
| skin_calls | game/terrain_b/render_batch.h:77 | skin (rgl/resource.h:92) | 1 methods | 2 (terrain_b.cpp) | no | - | 0/2/0 | high | method twin: declare the 1 method(s) on skin |
| AgentX | game/agent3/agent_types.h:42 | Agent (game/agent_obj.h:52) | 1 methods | 1 (agent_logic.cpp) | no | doc | 0/1/0 | high | method twin: declare the 1 method(s) on Agent |
| Blk2dd0 | game/options_panel/module_load.h:29 | EnvRow (game/env_row.h:13) | 1 methods | 1 (module_load.cpp) | no | - | 1/0/0 | medium | method twin: declare the 1 method(s) on EnvRow |
| CarryRef | game/world_data/world_data.h:124 | ItemRef (game/item_ref.h:14) | item | 1 (script_interpreter.cpp) | no | reach:1 | 1/0/0 | medium | fold into ItemRef (pads cover the fields) |
| MapBlock | game/cls_map/progress_bar.h:46 | E68 (include/e68.h:14) | - | 1 (progress_bar.cpp) | no | reach:1 | 1/0/0 | medium | fold into E68 (pads cover the fields) |
| MapPanel | game/inventory_panel/mission_ctx.cpp:121 | CampaignScreen (game/campaign_screen.h:17) | 1 methods | 1 (mission_ctx.cpp) | no | doc | 0/1/0 | low | method twin: declare the 1 method(s) on CampaignScreen |
| PairStack | game/ui/mission_ui.h:153 | ScriptStack (game/party2/camera.h:32) | pairs, count | 1 (script_interpreter.cpp) | no | reach:1 | 1/0/0 | medium | fold into ScriptStack (pads cover the fields) |
| RotFrame | game/script/script_interpreter.cpp:237 | rgl::matrix (rgl/rgl_math.h:69) | 1 methods | 1 (script_interpreter.cpp) | no | reach:2 | 0/1/0 | high | fold into rgl::matrix (pads cover the fields) |
| RuleRec | game/terrain_a/misc.h:29 | SpawnRec (game/mode_rec.h:19) | f0 | 1 (terrain_emit.cpp) | no | - | 1/0/0 | medium | fold into SpawnRec (pads cover the fields) |
| SkinApplyExt | game/terrain_b/troop_mgr.h:13 | Skin (include/skin.h:48) | 1 methods | 1 (poly_mesh.cpp) | no | reach:2 | 1/0/0 | high | method twin: declare the 1 method(s) on Skin |
| SkinVert | rgl/scene/fx.h:19 | std::vector | w, bone, bonef | 1 (fxdef_skin.cpp) | no | reach:1 | 0/1/0 | low | fold into std::vector (pads cover the fields) |
| TStringArray | game/agent5/agent_hud.h:154 | tstring (game/tstring.h:19) | 1 methods | 1 (agent_hud.cpp) | no | - | 0/1/0 | high | method twin: declare the 1 method(s) on tstring |
| Team | game/inventory_panel/mission_ctx.h:39 | TeamRec (game/mission.h:60) | f0, leader, cls, relation | 1 (mission_ctx.cpp) | no | - | 1/0/0 | medium | fold into TeamRec (pads cover the fields) |
| TerrainExt | game/terrain_b/terrain.h:14 | Terrain (game/terrain_a/terrain.h:137) | 1 methods | 1 (poly_mesh.cpp) | no | - | 1/0/0 | high | method twin: declare the 1 method(s) on Terrain |
| TimeKey | game/script/script_interpreter.h:50 | Light (game/light.h:11) | 1 methods | 1 (script_interpreter.cpp) | no | reach:1 | 0/1/0 | medium | method twin: declare the 1 method(s) on Light |
| TriggerItem | game/script/script_interpreter.h:144 | rgl::lod_mesh (rgl/mesh.h:174) | 1 methods | 1 (script_interpreter.cpp) | no | reach:2 | 0/1/0 | high | fold into rgl::lod_mesh (pads cover the fields) |
| TroopIO | game/party/game.h:75 | Troop (include/troop.h:19) | 1 methods | 1 (game.cpp) | no | - | 1/0/0 | high | method twin: declare the 1 method(s) on Troop |
| TroopX | game/mission_ui2/stat_panel_x.cpp:38 | Troop (include/troop.h:19) | 1 methods | 1 (stat_panel_x.cpp) | no | reach:1 | 1/0/0 | high | method twin: declare the 1 method(s) on Troop |
| convv | game/cls_map/cls_map.cpp:484 | PartyTemplate (include/game_recs.h:86) | 1 methods | 1 (cls_map.cpp) | no | - | 1/0/0 | medium | method twin: declare the 1 method(s) on PartyTemplate |
| curve4 | rgl/scene/emitter.h:24 | float_pair (rgl/scene/emitter.h:26) | t0, v0, t1, v1 | 1 (psys.cpp) | no | - | 0/1/0 | low | fold into float_pair (real type has no offsets there: check) |
| key_clip | rgl/resource.h:150 | named_object (rgl/resource.h:19) | tracks, extra | 1 (resmgr.cpp) | likely | reach:1 | 0/1/0 | low | fold into named_object (real type has no offsets there: check) |
| material_io | rgl/core/resource_io.h:43 | material (rgl/resource.h:66) | 1 methods | 1 (resmgr.cpp) | no | - | 1/0/0 | high | method twin: declare the 1 method(s) on material |
| mesh_anim_x | game/mesh_slot/mesh_scene.cpp:433 | mesh (rgl/mesh.h:48) | 1 methods | 1 (mesh_scene.cpp) | no | reach:2 | 1/0/0 | high | method twin: declare the 1 method(s) on mesh |
| mesh_flip | game/mesh_slot/mesh_ops.cpp:37 | mesh (rgl/mesh.h:48) | 1 methods | 1 (mesh_ops.cpp) | no | reach:1 | 0/1/0 | medium | method twin: declare the 1 method(s) on mesh |
| mesh_node | game/mesh_slot/mesh_scene.cpp:27 | rgl::mesh (rgl/mesh.h:48) | 1 methods | 1 (mesh_scene.cpp) | no | doc,reach:1 | 1/0/0 | high | fold into rgl::mesh (pads cover the fields) |
| mi_ops | game/terrain_b/render_batch.h:128 | mesh (rgl/mesh.h:48) | 1 methods | 1 (terrain_b.cpp) | no | - | 0/1/0 | high | method twin: declare the 1 method(s) on mesh |
| music_set | game/inventory_panel/module_files.cpp:93 | SoundMgr (include/sound_mgr.h:26) | 1 methods | 1 (module_files.cpp) | no | reach:1 | 1/0/0 | high | method twin: declare the 1 method(s) on SoundMgr |
| skin_x | game/terrain_a/misc.h:67 | skin (rgl/resource.h:92) | 1 methods | 1 (terrain_render.cpp) | no | - | 1/0/0 | high | method twin: declare the 1 method(s) on skin |
| sound_record | game/config/config.h:109 | music_name (rgl/mesh2/rec_tables.h:165) | name, f10, samples, nsamples | 1 (rec_tables.cpp) | likely | - | 0/0/1 | medium | fold into music_name (real type has no offsets there: check) |
| texture_x | game/terrain_a/misc.h:61 | texture (rgl/resource.h:34) | 1 methods | 1 (terrain_render.cpp) | no | - | 1/0/0 | high | method twin: declare the 1 method(s) on texture |
| vec2x | game/agent4/agent_logic.cpp:3383 | vec2 (include/vec.h:13) | 1 methods | 1 (agent_logic.cpp) | no | - | 0/1/0 | high | method twin: declare the 1 method(s) on vec2 |
| AgentX | rgl/xmlcfg/game_util.h:31 | Agent (game/agent_obj.h:52) | 3 methods | 0 | no | doc | - | high | method twin: declare the 3 method(s) on Agent |
| BeardMeshNew | game/cls_map/progress_bar.cpp:1200 | BeardMesh (include/skin.h:39) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on BeardMesh |
| BlowInit | game/blow.h:39 | Agent (game/agent_obj.h:52) | 2 methods | 0 | no | doc | - | high | method twin: declare the 2 method(s) on Agent |
| MissionV | game/inventory_panel/mission_ctx.h:14 | Mission (game/mission.h:78) | 2 methods | 0 | no | - | - | high | method twin: declare the 2 method(s) on Mission |
| MissionW | game/world_data/world_data.h:65 | Mission (game/mission.h:78) | 8 methods | 0 | no | - | - | high | method twin: declare the 8 method(s) on Mission |
| PartyV | game/inventory_panel/mission_ctx.cpp:124 | Party (include/party.h:45) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on Party |
| PartyX | game/party_x.h:11 | Party (include/party.h:45) | 16 methods | 0 | no | - | - | high | method twin: declare the 16 method(s) on Party |
| V2_s29130 | game/mission5/mission_5.cpp:1567 | V2 (include/vec.h:31) | 3 methods | 0 | no | - | - | high | method twin: declare the 3 method(s) on V2 |
| V2r | game/config/widget_mgr.cpp:236 | V2 (include/vec.h:31) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on V2 |
| WidgetDtor | game/party/party_window.h:36 | MissionUi (game/ui/mission_ui.h:31) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on MissionUi |
| frustum_view | game/renderer.h:43 | frustum (include/frustum.h:21) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on frustum |
| mat3_value | rgl/identity_value.h:8 | mat3 (rgl/rgl_math.h:58) | - | 0 | no | - | - | high | fold into mat3 (pads cover the fields) |
| material_x | game/terrain_a/misc.h:64 | material (rgl/resource.h:66) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on material |
| mesh_calls | game/terrain_b/render_batch.h:33 | mesh (rgl/mesh.h:48) | 2 methods | 0 | no | - | - | high | method twin: declare the 2 method(s) on mesh |
| mesh_flip | game/terrain_b/terrain_b.h:39 | mesh (rgl/mesh.h:48) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on mesh |
| mesh_node | rgl/xmlcfg/game_util.h:72 | mesh (rgl/mesh.h:48) | 1 methods | 0 | no | doc | - | high | method twin: declare the 1 method(s) on mesh |
| string_view_ | rgl/shapes/shapes.h:32 | string_view_ (game/script/script_interpreter.h:55) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on string_view_ |
| tstr | game/script/script_interpreter.h:94 | tstring (game/tstring.h:19) | 1 methods | 0 | no | reach:1 | - | high | method twin: declare the 1 method(s) on tstring |
| vec2c | game/strat/strat_ext.h:33 | vec2 (include/vec.h:13) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on vec2 |
| vec2r | game/party2/party_2.h:71 | vec2 (include/vec.h:13) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on vec2 |
| vec2x | rgl/xmlcfg/game_util.h:46 | vec2 (include/vec.h:13) | 2 methods | 0 | no | - | - | high | method twin: declare the 2 method(s) on vec2 |

### B. Real type exists but conflicts (5)

| view | file:line | overlays | fields named | casts (files) | 64? | path | rows e/n/- | conf | notes |
|---|---|---|---|---|---|---|---|---|---|
| ItemStackG | game/mesh_slot/mesh_game.cpp:38 | ItemRef (game/item_ref.h:14) | b | 3 (mesh_game.cpp, script_interpreter.cpp) | no | reach:4 | 3/0/0 | high | members of ItemRef at the same offsets disagree: +0x0 unsigned a b vs int item |
| FrustumBlock | game/char_sheet/terrain_page_input.h:43 | frustum (include/frustum.h:21) | d | 2 (terrain_page_input.cpp) | no | - | 2/0/0 | medium | members of frustum at the same offsets disagree: +0x0 int[0x48] d vs char perspective |
| CandRec | game/terrain_a/misc.h:30 | Stack (include/party.h:30) | f0, troop, f8 | 1 (terrain_emit.cpp) | no | - | 1/0/0 | medium | members of Stack at the same offsets disagree: +0x8 unsigned char f8 vs int flags |
| MissionX | game/script/script_interpreter.h:264 | Mission (game/mission.h:78) | 28 methods | 0 | no | reach:28 | - | high | **the script's Mission (157 mission() calls): EntryPoint[64] at +0x670 vs Mission's MarkRec[64], teams / mode / cam_frame to reconcile with game/mission.h, then mission() returns Mission*** |
| mat3_s2cf60 | game/mission5/mesh_3.h:108 | matrix (rgl/rgl_math.h:69) | m | 0 | no | - | - | high | members of matrix at the same offsets disagree: +0x0 float[12] m vs vector4 s |

### C. Real type not yet defined (7)

| view | file:line | overlays | fields named | casts (files) | 64? | path | rows e/n/- | conf | notes |
|---|---|---|---|---|---|---|---|---|---|
| mat3_s2f250 | game/mission5/mesh_3.h:219 | ? | s, f, u | 5 (mesh_3.h, mesh_poly_ray.cpp) | no | reach:1 | 1/4/0 | high | overlays raw memory / no real record found (sources: ?) |
| rec34 | include/rec34.h:10 | ? | f00, name, f14, body | 4 (game_2.cpp, gui_edit.cpp +1) | likely | reach:2 | 2/0/2 | low | overlays raw memory / no real record found (sources: void) |
| RenderXform | game/cls_map/progress_bar.cpp:689 | ? | m, f30, f34, f38 | 1 (progress_bar.cpp) | no | - | 0/1/0 | low | overlays raw memory / no real record found (sources: char) |
| frustum_view | game/terrain_b/render_batch.h:41 | ? | 1 methods | 1 (render_batch.h) | no | - | 0/0/1 | high | overlays raw memory / no real record found (sources: void) |
| obj_c0 | include/mesh_parts.h:37 | ? | out, in | 1 (entity.cpp) | likely | - | 1/0/0 | low | overlays raw memory / no real record found (sources: void) |
| grid_view | rgl/core/frustum_grid.h:10 | ? | n, lo, step | 0 | likely | - | - | high | overlays raw memory / no real record found |
| mat4_ro | game/renderer.h:40 | ? | 1 methods | 0 | no | - | - | high | overlays raw memory / no real record found |

### D. Std container stand-ins (5)

| view | file:line | overlays | fields named | casts (files) | 64? | path | rows e/n/- | conf | notes |
|---|---|---|---|---|---|---|---|---|---|
| rawvec | rgl/scene/world_items.cpp:78 | std::vector<entity> | al, first, last, end | 2 (world_items.cpp) | no | reach:2 | 2/0/0 | low | vector-shaped (end/first/last) |
| EventVec | game/party2/party_2.h:99 | Party (include/party.h:45) | al, first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| vec_e1f8 | game/triggers/trigger_vec.h:46 | GameKeys (game/game_keys.h:7) | n, begin, end | 0 | no | - | - | low | vector-shaped (begin/end) |
| vec_e50 | game/triggers/trigger_vec.h:45 | GameKeys (game/game_keys.h:7) | n, begin, end | 0 | no | - | - | low | vector-shaped (begin/end) |
| vec_eb0 | game/triggers/trigger_vec.h:47 | GameKeys (game/game_keys.h:7) | n, begin, end | 0 | no | - | - | low | vector-shaped (begin/end) |

### E. Fake vtable / slot views (2)

| view | file:line | overlays | fields named | casts (files) | 64? | path | rows e/n/- | conf | notes |
|---|---|---|---|---|---|---|---|---|---|
| EmptySlot | game/ui/mission.h:22 | PlacedObj (game/placed_obj.h:18) | 1 methods | 1 (mission_ui.cpp) | no | doc,reach:1 | 1/0/0 | medium | virtual-slot view: calls through it stand for the real virtual interface |
| ItemD_vt | game/options_panel/mission_item_recs.h:187 | item_base (game/options_panel/mission_item_recs.h:17) | panel, back_alloc, blk0, fwd_alloc +8 | 0 | likely | - | - | high | virtual-slot view: calls through it stand for the real virtual interface |

### F. Byte-match blocked (1)

| view | file:line | overlays | fields named | casts (files) | 64? | path | rows e/n/- | conf | notes |
|---|---|---|---|---|---|---|---|---|---|
| ElemVt | game/ui/mission_ui.h:150 | FxElem (include/fx.h:35) | 2 methods | 0 | no | doc | - | high | **declaration-only polymorphic class kept because cl 13.10 swaps two loads in Recf0::frame_update (0x4d63b0) without it; test removing it with run_all on video_page.cpp** |

### Incomplete real types (64 records with pads that are not views)

Name (file:line) pads/pad bytes, by pad count. These are the classes the views above fold into, or records reached
only directly; their pads are the members still to name.

- `game` (24): Missile (missile.h:16) 4/0xe, Agent (agent_obj.h:52) 3/0xc, Mission (mission.h:78) 3/0x638, Rec1208 (conv_screen.h:18) 3/0xf, C4c31c0 (screen_records.h:74) 2/0x8, C4c3290 (screen_records.h:99) 2/0x1c, Event (event.h:23) 2/0x8, HitInfo (blow.h:12) 2/0x34, ResultWindow (result_window.h:17) 2/0xb, C4c3050 (screen_records.h:123) 1/0x4, CampaignScreen (campaign_screen.h:17) 1/0x10b, EmitDef (emit.h:19) 1/0x8, Font (font.h:16) 1/0x4, InputRec (agent_ai.h:19) 1/0x14, MapFlags (map_flags.h:7) 1/0x1, PartyScreen (party_screen_class.h:20) 1/0x4, Rec118 (screen_records.h:152) 1/0x8, Rec1bc (screen_records.h:302) 1/0x3, Slot (slot.h:16) 1/0x7, SlotRec (screen_records.h:66) 1/0x1, SpawnRec (mode_rec.h:19) 1/0x4, TerrainPage (terrain_page.h:19) 1/0x28, ini_entry (ini_file.h:11) 1/0xc, junk_tag (junk_tag.h:7) 1/0x4
- `game/terrain_a` (4): light_src (misc.h:100) 4/0x20, TerrainParticle (terrain.h:87) 2/0x8, light_t (misc.h:118) 2/0x40, Terrain (terrain.h:137) 1/0x5dd
- `include` (7): Elem (dialog_rec.h:11) 3/0xc, GameData (game_data.h:103) 1/0x40, ScriptState (script_state.h:28) 1/0x4, Tail2 (tail2.h:9) 1/0xc, Timer (timer.h:16) 1/0x4, frustum (frustum.h:21) 1/0x2, renderer (renderer.h:16) 1/0xc
- `game/config` (3): InputSet (config.h:55) 6/0x72c, Config (config_block.h:13) 1/0x4, CreateGameOptionsPage (config.h:42) 1/0x374
- `game/terrain_b` (3): light (render_batch.h:89) 4/0x38, SceneFile (scene_file.h:39) 2/0x80, MEdge (poly_mesh.h:36) 1/0x8
- `game/inventory_panel` (2): MissionCtx (mission_ctx_rec.h:7) 4/0x150, BigRec (map_panel.cpp:79) 2/0x84
- `game/script` (4): SiteRec (script_interpreter.cpp:2324) 2/0x1cc, EntryPoint (script_interpreter.h:249) 1/0xf0, MTimer (script_interpreter.h:256) 1/0x4, PickArg (script_interpreter.cpp:2323) 1/0x400
- `game/char_sheet` (3): TextRow (editor_ui.h:66) 2/0x1f8, cell_rec (dust.h:62) 2/0x28, SaveCtx (save_game.h:51) 1/0xc
- `game/ui` (4): MissionUi (mission_ui.h:31) 1/0x5, NodeA (container.h:45) 1/0x7, UiA (ui_widgets.h:14) 1/0x2, UiC (ui_widgets.h:58) 1/0x3
- `game/world_data` (1): CarryKind (world_data.h:116) 3/0x22f
- `game/triggers` (3): ItemRec (item_rec.h:27) 1/0x4, ItemRec2 (item_rec.h:49) 1/0x4, TimedVal (trigger_vec.cpp:1187) 1/0x4
- `game/agent` (1): Rec20 (agent.h:25) 1/0x10
- `game/party2` (1): ScriptStack (camera.h:32) 1/0x4
- `game/mesh_slot` (1): TimeKey (slot.cpp:36) 1/0x44
- `game/options_panel` (1): TreeW (world_app.cpp:123) 1/0x8
- `game/agent2` (1): V32 (agent_types.h:61) 1/0x14
- `rgl/scene` (1): strategic_entity (scene_entity.h:135) 1/0x28

<!-- end generated -->

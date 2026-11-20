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
6. (lane 7 folded GameData_s261e0, Mission_s29130 / Agent_s29130 / GameData_s29130; lane V1 Game_s2cf60, Frame__s2f250) **GameData_s29130 and the other mission5 slice views** (GameData_s261e0 48 sites, Frame__s2f250 47, CampaignGame_s2b5c0
   29, Game_s2cf60 18, Mission_s29130 15, Scene_s2cf60 88): the "12 game-object views of mission_5.h" INTEGRATION_STATUS
   lists; every field is a pad-offset at 64 bits. GameData / Mission have most of the members already.
7. **View** (A, script_interpreter.cpp:2362, `doc`): the presentation view with a 32-bit pad before `list` (+0x2c).
8. (done 2026-11-20, lane V1: emitter is rgl::psys, TabSpawn / TabTarget are skin / texture; VoiceTable left) **emitter / TabSpawn / VoiceTable** (B): psys / skin / Animator views with vectors or pointers behind pads, exact rows.
9. (mostly done 2026-11-20, lane V2: 570 -> 25 sites) **WVt + PsOps / text_slot** (E): the widget calls name MissionUi's
   virtuals (animate_* for the old set_a..set_f); left: world_data.cpp's 5 calls and widget_mgr::scene (WVt*, slot 50 =
   ContainerBase::add_child), and text_slot (note_window.cpp), which needs set_text's real signature. The slot signatures
   are still pointer stand-ins: MeshWidgetVt / text_slot / text_store / reused scratch buffers in cls_map show `const vec2&`,
   `const int&` / `const unsigned&` parameters and `vec2 set_text(const tstring&)` / `vec2 get_xy()` by-value returns
   (checked: references and member returns of vec2 give the original's hidden-pointer / temp-slot code).
10. (GameX done 2026-11-20, lane V1: with GameW / GameCompanion / TroopX, game() is g_game) **GameX** (A, 139 sites bound here; with the `game()` calls about 240) and **CamSlot** (A, 61 sites, all exact rows; done lane 7: Mission's cam_mode / frame_a / frame_b / blend_* members): mechanical renames into
    GameData / Mission members; cheap and remove two of the largest cast sources.

Related, not views but the same constraint as class F: `MB_TSTRING_SIZE` (game/tstring.h) sizes raw tstring storage
(C4c3050, 0x4d63b0); D-class replacements must keep the template instance the original's calls bind to
(tools/relink/target_gate.py: an ICF-folded `vector<int>::_Insert_n` is not `vector<T*>`'s copy, e.g. EntVec::insert_n).

## Totals per class and tables

<!-- generated by tools/port/find_views.py --md docs/views_inventory.md: edit the tool (OVERRIDES), not this part -->

| class | views | 64-bit wrong (yes) | on the port path | cast sites |
|---|---:|---:|---:|---:|
| A | 241 | 49 | 90 | 788 |
| B | 36 | 21 | 13 | 190 |
| C | 26 | 5 | 9 | 26 |
| D | 68 | 17 | 15 | 157 |
| E | 11 | 0 | 3 | 42 |
| F | 2 | 0 | 2 | 2 |
| all | 384 | 92 | 132 | 1205 |

### A. Fold into the real type (241)

| view | file:line | overlays | fields named | casts (files) | 64? | path | rows e/n/- | conf | notes |
|---|---|---|---|---|---|---|---|---|---|
| SceneEnt | game/scene_ent.h:14 | rgl::entity (rgl/scene/scene_entity.h:33) | type, p18, local, shape +8 | 10 (agent_logic.cpp) | yes | doc,reach:8 | 0/10/0 | high | fold into rgl::entity (real type has no offsets there: check) |
| AgentBodyV | game/agent4/agent_logic.h:157 | strategic_entity (rgl/scene/scene_entity.h:141) | f8, camp, f294, f298 | 9 (agent_logic.cpp) | yes | doc,reach:7 | 2/7/0 | high | fold into strategic_entity (real type has no offsets there: check) |
| Agent_s284a0 | game/mission5/mission_5.h:470 | Agent (game/agent_obj.h:72) | used, kind, state, horse +17 | 3 (mission_5.cpp, mission_5.h) | yes | doc | 0/2/1 | high | fold into Agent (real type has no offsets there: check) |
| AnimDefV | game/agent4/agent_logic.cpp:9569 | RecH (game/triggers/trigger_vec.cpp:42) | info | 3 (agent_logic.cpp) | yes | reach:3 | 0/3/0 | high | fold into RecH (real type has no offsets there: check) |
| MissionView | game/ui/mission.h:36 | IfaceB (include/iface.h:21) | v18, p64, vd8, v118 +7 | 3 (script_interpreter.cpp, world_data.cpp) | yes | reach:5 | 1/2/0 | high | fold into IfaceB (real type has no offsets there: check) |
| Wielder | game/agent/agent.h:39 | Missile (game/missile.h:13) | a, b | 3 (agent_logic.cpp) | yes | reach:1 | 1/2/0 | high | fold into Missile (pads cover the fields) |
| KeyObj | rgl/dynamic/fx_anim.h:40 | KeyTable (include/key_table.h:10) | tables, tbl | 2 (agent_stats.cpp, fx_anim.h) | yes | reach:1 | 1/0/1 | high | fold into KeyTable (real type has no offsets there: check) |
| PropEnt | game/prop_ent.h:10 | entity (rgl/scene/scene_entity.h:33) | lods, meshes | 2 (mission_3.cpp) | yes | doc | 0/0/2 | high | **a 32-bit-offset view of rgl::entity (docs/layout64_inventory.md: pad4[0x108 - 4], Vec of int words): mission_3 / terrain_a PlacedObj should read rgl::entity lods / meshes** |
| View | game/script/script_interpreter.cpp:2352 | Presentation (game/script/script_interpreter.h:210) | items, list | 2 (script_interpreter.cpp) | yes | reach:2 | 0/2/0 | high | fold into Presentation (real type has no offsets there: check) |
| Scene_s2cf60 | game/mission5/mesh_3.h:150 | Mission (game/mission.h:73) | nav | 88 (mesh_3.cpp) | yes | - | 0/88/0 | high | **Mission with only +0x6698 nav named (g_scene_s2cf60, 88 sites in the map editor): Mission::nav exists; the sub-view gui_manager_s2cf60 is the editor's nav mesh (graph_obj)** |
| CampaignGame_s2b5c0 | game/mission5/mission_5.h:1094 | GameData (include/game_data.h:101) | game_mode, f1a098, color, f1a0ac +4 | 29 (mission_5.cpp) | yes | - | 3/13/13 | high | fold into GameData (real type has no offsets there: check) |
| MenuDoor | rgl/mesh2/gui_edit.h:215 | menu_item (game/menu_rec.h:13) | name, text, key | 12 (gui_edit.cpp) | yes | - | 12/0/0 | high | fold into menu_item (pads cover the fields) |
| AnimDefW | game/agent4/agent_logic.cpp:11673 | RecH (game/triggers/trigger_vec.cpp:42) | subs | 6 (agent_logic.cpp) | yes | - | 6/0/0 | high | fold into RecH (real type has no offsets there: check) |
| CampaignGame_s2bf40 | game/mission5/mission_5.h:1175 | GameData (include/game_data.h:101) | game_mode, brush_radius | 5 (mission_5.cpp) | yes | - | 5/0/0 | high | fold into GameData (real type has no offsets there: check) |
| BlockOwner | game/cls_map/cls_map.h:174 | owned_block (include/owned_block.h:6) | index, f14, timer, block | 4 (cls_map.cpp) | yes | - | 4/0/0 | high | fold into owned_block (real type has no offsets there: check) |
| ModRec | rgl/mesh2/gui_edit.h:208 | item_mod (include/item_mod.h:7) | name, key | 4 (gui_edit.cpp) | yes | - | 4/0/0 | high | fold into item_mod (pads cover the fields) |
| RecA | rgl/mesh2/gui_edit.h:133 | ItemKind (include/item_kind.h:44) | name, text, field, key | 4 (gui_edit.cpp) | yes | - | 4/0/0 | high | fold into ItemKind (real type has no offsets there: check) |
| mesh_s2ae70 | game/mission5/mission_5.h:948 | rgl::mesh (rgl/mesh.h:59) | res, f138, f164 | 4 (mission_5.cpp) | yes | - | 4/0/0 | high | fold into rgl::mesh (real type has no offsets there: check) |
| EditorView_s2f250 | game/mission5/mesh_3.h:553 | GuiCtx (game/char_sheet/gui_ctx.h:17) | f2f9 | 3 (mesh_poly_ray.cpp) | yes | - | 0/2/1 | high | a screen in g_windows.arr[n] (slot_as<T>(n)): fold into that slot's screen class (head_rec family, game/screen_records.h) |
| Mission_s29840 | game/mission5/mission_5.h:720 | Mission (game/mission.h:73) | scene, agents, scene_idx, props | 3 (mission_5.cpp, mission_5.h) | yes | - | 1/1/1 | high | fold into Mission (pads cover the fields) |
| Anim | game/agent2/agent_types.h:58 | KeyHolder (game/agent/agent.h:90) | elems | 2 (agent_logic.cpp, script_interpreter.cpp) | yes | - | 1/0/1 | medium | fold into KeyHolder (pads cover the fields) |
| Entity_s2ae70 | game/mission5/mission_5.h:964 | rgl::entity (rgl/scene/scene_entity.h:33) | visible | 2 (mission_5.cpp) | yes | - | 2/0/0 | high | fold into rgl::entity (pads cover the fields) |
| ItemKind_s29ec0 | game/mission5/mission_5.h:814 | ItemKind (include/item_kind.h:44) | type_flags, difficulty | 2 (mission_5.cpp) | yes | - | 0/2/0 | high | fold into ItemKind (real type has no offsets there: check) |
| ObjRec20_s2f250 | game/mission5/mesh_3.h:546 | ptype (game/ptype.h:25) | variants | 2 (mesh_poly_ray.cpp) | yes | - | 0/2/0 | high | fold into ptype (pads cover the fields) |
| VecPair2 | game/agent2/agent_ai.h:31 | E56 (game/agent3/agent_types.h:40) | a, b | 2 (agent_logic.cpp) | yes | - | 2/0/0 | high | fold into E56 (real type has no offsets there: check) |
| DustEntry | game/terrain_b/terrain.h:14 | TerrainParticle (game/terrain_a/terrain.h:94) | x, y, z | 1 (mission.cpp) | yes | - | 0/1/0 | high | fold into TerrainParticle (real type has no offsets there: check) |
| DustParticle | game/char_sheet/dust.h:108 | TerrainParticle (game/terrain_a/terrain.h:94) | type, frame, dir, anim +4 | 1 (dust.cpp) | yes | - | 0/0/1 | high | fold into TerrainParticle (pads cover the fields) |
| Game_s257f0 | game/mission5/mission_5.h:124 | GameData (include/game_data.h:101) | player, chars | 1 (mission_5.cpp) | yes | - | 1/0/0 | high | fold into GameData (real type has no offsets there: check) |
| Game_s26130 | game/mission5/mission_5.h:179 | GameData (include/game_data.h:101) | player, chars | 1 (mission_5.cpp) | yes | - | 1/0/0 | high | fold into GameData (real type has no offsets there: check) |
| Group | game/script/script_interpreter.cpp:2562 | RecH (game/triggers/trigger_vec.cpp:42) | src, count | 1 (script_interpreter.cpp) | yes | - | 0/0/1 | high | fold into RecH (real type has no offsets there: check) |
| KeysPageView | game/options_panel/keys_page.cpp:118 | KeysPage (game/options_panel/keys_page.cpp:137) | option, w30, w34, w38 +5 | 1 (keys_page.cpp) | yes | - | 0/0/1 | high | fold into KeysPage (pads cover the fields) |
| MapObject_s29ec0 | game/mission5/mission_5.h:880 | MapObject (rgl/mesh2/gui_edit.h:229) | name, mesh | 1 (mission_5.cpp) | yes | - | 0/1/0 | high | fold into MapObject (pads cover the fields) |
| MeshOwner_s2fe40 | game/mission5/mesh_3.h:675 | PlacedObj (game/placed_obj.h:17) | f188, f18c, name, f1a0 | 1 (mesh_3.h) | yes | - | 0/0/1 | high | fold into PlacedObj (pads cover the fields) |
| Mission_s284a0 | game/mission5/mission_5.h:520 | Mission (game/mission.h:73) | agents, player_id | 1 (mission_5.h) | yes | - | 0/0/1 | high | fold into Mission (pads cover the fields) |
| PartV | game/agent4/agent_logic.cpp:11200 | AnimDef (game/agent_obj.h:56) | f0c, v2c | 1 (agent_logic.cpp) | yes | - | 0/1/0 | high | fold into AnimDef (real type has no offsets there: check) |
| Record | rgl/xmlcfg/game_util.h:26 | AnimDef (game/agent_obj.h:56) | items, count | 1 (game_util.h) | yes | - | 1/0/0 | high | fold into AnimDef (real type has no offsets there: check) |
| SceneEntity_s29ec0 | game/mission5/mission_5.h:802 | entity (rgl/scene/scene_entity.h:33) | vtbl, game_id, game_kind, pos +1 | 1 (mission_5.cpp) | yes | - | 0/1/0 | high | fold into entity (real type has no offsets there: check) |
| Screen_s29ec0 | game/mission5/mission_5.h:909 | MissionBlocker (game/options_panel/mission_item_recs.h:37) | pitch, sel_mode, sel_target, sel_valid | 1 (mission_5.cpp) | yes | - | 0/1/0 | high | fold into MissionBlocker (pads cover the fields) |
| SkillNames | game/inventory_panel/inventory_panel.cpp:57 | SkillRec (include/skill_rec.h:8) | f10, f20 | 1 (inventory_panel.cpp) | yes | - | 1/0/0 | high | fold into SkillRec (pads cover the fields) |
| ViewCtrl_s284a0 | game/mission5/mission_5.h:537 | MissionBlocker (game/options_panel/mission_item_recs.h:37) | flags44, yaw, pitch, yaw_rate +7 | 1 (mission_5.cpp) | yes | - | 0/1/0 | high | fold into MissionBlocker (real type has no offsets there: check) |
| cell_obj | game/char_sheet/dust.h:121 | entity (rgl/scene/scene_entity.h:33) | pos, list, list_end, active | 1 (dust.cpp) | yes | - | 0/0/1 | high | fold into entity (real type has no offsets there: check) |
| psys_rec | game/mesh_slot/mesh_game.cpp:394 | ShadowMap (game/shadow_map.h:18) | scene | 1 (mesh_game.cpp) | yes | - | 0/1/0 | high | fold into ShadowMap (real type has no offsets there: check) |
| AgentTable_s284a0 | game/mission5/mission_5.h:461 | AgentTable (game/agent_obj.h:433) | count, pages | 0 | yes | - | - | high | fold into AgentTable (real type has no offsets there: check) |
| AgentTable_s29840 | game/mission5/mission_5.h:647 | AgentTable (game/agent_obj.h:433) | count, pages | 0 | yes | - | - | high | fold into AgentTable (real type has no offsets there: check) |
| Agent_s29840 | game/mission5/mission_5.h:655 | Agent (game/agent_obj.h:72) | kind, state, horse, pos +9 | 0 | yes | - | - | high | fold into Agent (pads cover the fields) |
| Agent_s29ec0 | game/mission5/mission_5.h:832 | Agent (game/agent_obj.h:72) | kind, state, horse, rider +6 | 0 | yes | - | - | high | fold into Agent (real type has no offsets there: check) |
| MapEditor_s2f250 | game/mission5/mesh_3.h:602 | MapEditorDialog (rgl/mesh2/gui_edit.h:344) | view, dir_x, dir_y, dir_z +7 | 0 | yes | - | - | high | fold into MapEditorDialog (pads cover the fields) |
| SceneObjRec_s27c90 | game/mission5/mission_5.h:373 | entity (rgl/scene/scene_entity.h:33) | objs, stream_a, stream_b | 0 | yes | - | - | high | fold into entity (real type has no offsets there: check) |
| gui_manager_s2cf60 | game/mission5/mesh_3.h:123 | graph_obj (rgl/mesh2/graph_obj.h:61) | marker_poly, marker_spikes, marker_ribbons, need_redraw | 0 | yes | - | - | high | fold into graph_obj (real type has no offsets there: check) |
| WItem | game/config/widget_mgr.h:116 | std::vector<MissionUi> | 2 methods | 39 (widget_mgr.cpp) | no | reach:35 | 35/0/4 | medium | fold into std::vector<MissionUi> (pads cover the fields) |
| widget_mgr | game/script/script_interpreter.h:60 | Presentation (game/script/script_interpreter.h:210) | 25 methods | 39 (script_interpreter.cpp, script_interpreter.h) | no | doc,reach:39 | 25/14/0 | medium | method twin: declare the 25 method(s) on Presentation |
| TroopRec | include/troop.h:132 | Troop (include/troop.h:20) | 9 methods | 37 (inventory_panel.cpp, troop_rec.cpp) | no | doc,reach:22 | 16/21/0 | high | method twin: declare the 9 method(s) on Troop |
| string_view_ | game/script/script_interpreter.h:59 | tstring (game/tstring.h:19) | 1 methods | 36 (script_interpreter.cpp, script_interpreter.h) | no | reach:35 | 1/34/1 | medium | method twin: declare the 1 method(s) on tstring |
| Widget_s261e0 | game/mission5/mission_5.h:236 | ? | mode, troop, back_state | 35 (mission_5.cpp) | likely | - | 14/21/0 | high | a screen in g_windows.arr[n] (slot_as<T>(n)): fold into that slot's screen class (head_rec family, game/screen_records.h) |
| scene_ms | game/mesh_slot/mesh_pass.cpp:33 | scene (rgl/scene_rec.h:41) | 1 methods | 24 (mesh_pass.cpp) | no | - | 0/24/0 | high | method twin: declare the 1 method(s) on scene |
| Mission_s2b5c0 | game/mission5/mesh_3.h:47 | Mission (game/mission.h:73) | 1 methods | 21 (mesh_3.h, mission_5.cpp) | no | - | 5/9/7 | high | method twin: declare the 1 method(s) on Mission |
| MenuWidget | game/inventory_panel/mission_ctx.h:13 | ? | f4, f14, f18 | 19 (inventory_panel.cpp) | likely | - | 19/0/0 | medium | a screen in g_windows.arr[n] (slot_as<T>(n)): fold into that slot's screen class (head_rec family, game/screen_records.h) |
| FxAnim | game/agent3/agent_combat.h:27 | FxDef (include/fx.h:144) | 2 methods | 18 (agent_logic.cpp) | no | reach:6 | 8/8/2 | high | method twin: declare the 2 method(s) on FxDef |
| Blob16 | game/ui/mission.h:29 | vector4 (rgl/rgl_math.h:14) | a, b, c, d | 14 (mission_ui.cpp) | no | reach:14 | 14/0/0 | medium | **16-byte block copy of Sel::rec[].a / .c (int[4]): give those members a 4-int struct type and assign it; no 64-bit effect** |
| CreateGameOptionsPage | game/agent5/editor_dialog.cpp:71 | GameApp (game/config/core.h:18) | 1 methods | 12 (editor_dialog.cpp) | no | - | 12/0/0 | medium | method twin: declare the 1 method(s) on GameApp |
| AgentX | game/mission5/mission_5.h:1255 | Agent (game/agent_obj.h:72) | 3 methods | 11 (mission_5.cpp, mission_5.h) | no | doc | 0/8/3 | high | method twin: declare the 3 method(s) on Agent |
| WidgetMgrW | game/script/script_interpreter.h:87 | Presentation (game/script/script_interpreter.h:210) | 4 methods | 8 (script_interpreter.cpp, script_interpreter.h) | no | reach:8 | 4/4/0 | medium | method twin: declare the 4 method(s) on Presentation |
| Obj3 | game/agent3/agent_types.h:59 | psys (rgl/scene/emitter.h:32) | 3 methods | 7 (agent_logic.cpp) | no | reach:3 | 5/2/0 | high | method twin: declare the 3 method(s) on psys |
| EV | game/options_panel/mesh_edit.cpp:131 | std::vector<MEdge> | v | 6 (mesh_edit.cpp) | no | - | 6/0/0 | medium | fold into std::vector<MEdge> (pads cover the fields) |
| graph_obj | game/agent5/editor_dialog.cpp:58 | PolyMesh (game/terrain_b/poly_mesh.h:120) | 6 methods | 6 (editor_dialog.cpp) | no | doc | 6/0/0 | high | method twin: declare the 6 method(s) on PolyMesh |
| DiDev | game/char_sheet/screen.h:93 | IDirectInputDevice8A | vtbl | 5 (screen.cpp) | no | - | 0/0/5 | medium | fold into IDirectInputDevice8A (pads cover the fields) |
| Matrix12_s2fe40 | game/mission5/mesh_3.h:684 | float | v | 5 (mesh_3.h, mesh_poly_ray.cpp) | no | reach:4 | 1/4/0 | high | fold into float (pads cover the fields) |
| OptionsDlg | game/options_panel/world_app.cpp:410 | GameApp (game/config/core.h:18) | 1 methods | 5 (config_dialog.cpp, world_app.cpp) | no | - | 3/2/0 | low | method twin: declare the 1 method(s) on GameApp |
| Sel | game/ui/mission.h:77 | Leader (game/script/script_interpreter.cpp:712) | rec | 5 (script_interpreter.cpp) | no | reach:4 | 2/2/1 | high | fold into Leader (pads cover the fields) |
| lod_mesh_x | game/lod_mesh_x.h:12 | lod2_a (game/mesh_slot/mesh_appear.cpp:37) | 4 methods | 5 (mesh_appear.cpp, mesh_scene.cpp) | no | doc,reach:5 | 4/1/0 | high | method twin: declare the 4 method(s) on lod2_a |
| sel_list | game/inventory_panel/note_window.cpp:208 | std::vector | 1 methods | 5 (note_window.cpp) | no | doc | 5/0/0 | medium | fold into std::vector (pads cover the fields) |
| AgentW | game/world_data/world_data.h:96 | Agent (game/agent_obj.h:72) | 1 methods | 4 (world_data.cpp, world_data.h) | no | reach:3 | 3/0/1 | high | method twin: declare the 1 method(s) on Agent |
| EditorEntity | game/editor_entity.h:6 | ? | f2f9 | 4 (game_app.cpp, map_editor.cpp) | likely | - | 0/4/0 | medium | a screen in g_windows.arr[n] (slot_as<T>(n)): fold into that slot's screen class (head_rec family, game/screen_records.h) |
| Editor_s2b5c0 | game/mission5/mission_5.h:1150 | GameApp (game/config/core.h:18) | 2 methods | 4 (mission_5.cpp) | no | - | 0/2/2 | high | method twin: declare the 2 method(s) on GameApp |
| Psys | game/mesh_slot/mesh_start.cpp:89 | ShadowMap (game/shadow_map.h:18) | 4 methods | 4 (mesh_start.cpp) | no | - | 0/4/0 | medium | method twin: declare the 4 method(s) on ShadowMap |
| TabRec | game/party2/party_2.h:102 | std::vector<E68> | w, h, holder | 4 (party_2.cpp) | likely | reach:4 | 3/0/1 | high | fold into std::vector<E68> (pads cover the fields) |
| flora_kind | game/options_panel/module_load.h:30 | E50 (include/vec_elems.h:15) | name, f10, f14, f18 +1 | 4 (module_load.cpp, world_app.cpp) | likely | - | 0/0/4 | low | fold into E50 (real type has no offsets there: check) |
| lod2_a | game/mesh_slot/mesh_appear.cpp:37 | Poser (game/agent3/agent_fx_sound.h:46) | 7 methods | 4 (mesh_appear.cpp) | no | doc,reach:10 | 0/3/1 | high | method twin: declare the 7 method(s) on Poser |
| snd_slot | game/mesh_slot/mesh_game.cpp:37 | SndSlotS (game/mesh_slot/mesh_game.cpp:40) | 1 methods | 4 (mesh_game.cpp, mission_5.cpp) | no | reach:1 | 1/3/0 | medium | method twin: declare the 1 method(s) on SndSlotS |
| BlockOwner | game/agent5/agent_hud.h:46 | owned_block (include/owned_block.h:6) | 1 methods | 3 (agent_hud.cpp, mission_item_recs.h) | no | - | 1/0/2 | high | method twin: declare the 1 method(s) on owned_block |
| EditorPanel_s2cf60 | game/mission5/mesh_3.h:170 | GameApp (game/config/core.h:18) | 1 methods | 3 (mesh_3.cpp) | no | - | 0/3/0 | high | method twin: declare the 1 method(s) on GameApp |
| EquipSet | game/mesh_slot/mesh_appear.cpp:54 | ItemRef (game/item_ref.h:13) | weapon, helmet, body, boots | 3 (agent_hud.cpp, mesh_game.cpp +1) | no | reach:1 | 1/1/1 | high | fold into ItemRef (real type has no offsets there: check) |
| MapEntry_s2fe40 | game/mission5/mesh_3.h:659 | ptype (game/ptype.h:25) | name, flags, count, names | 3 (mesh_poly_ray.cpp) | likely | - | 0/3/0 | high | fold into ptype (real type has no offsets there: check) |
| Mesh_s2da20 | game/mission5/mesh_3.h:281 | gui_manager_s2cf60 (game/mission5/mesh_3.h:123) | 6 methods | 3 (mesh_3.h) | no | - | 0/0/3 | high | method twin: declare the 6 method(s) on gui_manager_s2cf60 |
| Rec0xb0 | rgl/mesh2/gui_edit.h:420 | conv_rec (rgl/mesh2/gui_edit.h:429) | name, tail | 3 (gui_edit.cpp) | likely | reach:3 | 3/0/0 | low | fold into conv_rec (pads cover the fields) |
| TriggerMeshObj | game/script/script_interpreter.h:194 | mesh_inst (game/renderer.h:34) | 1 methods | 3 (script_interpreter.cpp) | no | reach:4 | 0/3/0 | high | method twin: declare the 1 method(s) on mesh_inst |
| lod_mesh_ms | game/mesh_slot/mesh_slot.h:42 | lod_group (rgl/core/resmgr.h:47) | 1 methods | 3 (mesh_char.cpp) | no | reach:4 | 0/3/0 | high | method twin: declare the 1 method(s) on lod_group |
| mesh_node | game/party2/party_2.h:56 | mesh (rgl/mesh.h:59) | 1 methods | 3 (party_2.cpp) | no | doc | 0/0/3 | low | method twin: declare the 1 method(s) on mesh |
| rot3x | game/agent4/agent_logic.cpp:7613 | matrix (rgl/rgl_math.h:69) | 1 methods | 3 (agent_logic.cpp) | no | - | 0/0/3 | high | method twin: declare the 1 method(s) on matrix |
| rot3x | rgl/xmlcfg/game_util.h:56 | matrix (rgl/rgl_math.h:69) | 1 methods | 3 (game_util.cpp) | no | reach:3 | 3/0/0 | high | method twin: declare the 1 method(s) on matrix |
| snd_bank | game/mesh_slot/mesh_start.cpp:53 | TriggerTag (include/trigger_tags.h:9) | 1 methods | 3 (game_tick.cpp, mesh_start.cpp +1) | no | reach:1 | 1/2/0 | medium | method twin: declare the 1 method(s) on TriggerTag |
| vec4n | game/agent4/agent_logic.cpp:7377 | vector4 (rgl/rgl_math.h:14) | 2 methods | 3 (agent_logic.cpp) | no | reach:3 | 0/3/0 | medium | method twin: declare the 2 method(s) on vector4 |
| AimRange_s29130 | game/mission5/mission_5.cpp:1595 | float | lo, hi | 2 (mission_5.cpp) | no | - | 0/2/0 | high | fold into float (pads cover the fields) |
| Alloc204 | rgl/mesh2/rec_tables.h:154 | MapDialog (rgl/mesh2/gui_edit.h:284) | 1 methods | 2 (gui_edit.cpp) | no | reach:2 | 2/0/0 | low | method twin: declare the 1 method(s) on MapDialog |
| Alloc2c | rgl/mesh2/rec_tables.h:156 | MapDialog (rgl/mesh2/gui_edit.h:284) | 1 methods | 2 (gui_edit.cpp) | no | reach:2 | 2/0/0 | low | method twin: declare the 1 method(s) on MapDialog |
| Alloc4 | rgl/mesh2/rec_tables.h:155 | MapDialog (rgl/mesh2/gui_edit.h:284) | 1 methods | 2 (gui_edit.cpp) | no | reach:2 | 2/0/0 | low | method twin: declare the 1 method(s) on MapDialog |
| DefList | game/agent_obj.h:64 | AnimSet (include/anim_set.h:9) | defs, count | 2 (mesh_game.cpp) | likely | doc,reach:2 | 0/2/0 | medium | fold into AnimSet (real type has no offsets there: check) |
| EditorApp | game/agent5/editor_dialog.cpp:70 | GameApp (game/config/core.h:18) | 1 methods | 2 (editor_dialog.cpp) | no | - | 2/0/0 | medium | method twin: declare the 1 method(s) on GameApp |
| EditorGame | game/char_sheet/editor_ui.h:59 | ? | f2f9 | 2 (editor_ui.cpp) | likely | - | 2/0/0 | medium | a screen in g_windows.arr[n] (slot_as<T>(n)): fold into that slot's screen class (head_rec family, game/screen_records.h) |
| EditorObj | rgl/mesh2/rec_tables.cpp:184 | GameApp (game/config/core.h:18) | 1 methods | 2 (rec_tables.cpp) | no | - | 2/0/0 | low | method twin: declare the 1 method(s) on GameApp |
| Event92 | include/game_data.h:44 | event_rec (game/party2/party_2.cpp:2119) | v | 2 (party_2.cpp) | no | reach:1 | 2/0/0 | low | fold into event_rec (real type has no offsets there: check) |
| GameData | game/agent5/party_create.cpp:71 | GameData (include/game_data.h:101) | 1 methods | 2 (party_create.cpp, script_interpreter.h) | no | doc,reach:2 | 2/0/0 | high | method twin: declare the 1 method(s) on GameData |
| GameIo | game/options_panel/options_panel.h:51 | Game (game/party/game.h:149) | 2 methods | 2 (options_panel.cpp) | no | - | 2/0/0 | high | method twin: declare the 2 method(s) on Game |
| I64VecW | game/world_data/world_data.h:117 | VarVec (include/script_state.h:28) | 4 methods | 2 (world_data.cpp) | no | reach:2 | 2/0/0 | high | method twin: declare the 4 method(s) on VarVec |
| KeyNames_s284a0 | game/mission5/mission_5.h:568 | Slot (game/slot.h:16) | 1 methods | 2 (mission_5.cpp) | no | - | 0/2/0 | high | method twin: declare the 1 method(s) on Slot |
| MapEditorDialog_s2f250 | game/mission5/mesh_3.h:558 | GameApp (game/config/core.h:18) | 2 methods | 2 (mesh_poly_ray.cpp) | no | - | 0/2/0 | high | method twin: declare the 2 method(s) on GameApp |
| MatrixExt_s284a0 | game/mission5/mission_5.h:561 | matrix (rgl/rgl_math.h:69) | 2 methods | 2 (mission_5.cpp) | no | - | 0/2/0 | high | method twin: declare the 2 method(s) on matrix |
| MenuScreen | game/options_panel/hud.h:20 | ? | f18 | 2 (hud_source.cpp) | likely | - | 2/0/0 | medium | a screen in g_windows.arr[n] (slot_as<T>(n)): fold into that slot's screen class (head_rec family, game/screen_records.h) |
| MissionLog | game/agent4/agent_logic.cpp:7842 | Mission (game/mission.h:73) | 1 methods | 2 (agent_logic.cpp) | no | reach:1 | 0/1/1 | medium | method twin: declare the 1 method(s) on Mission |
| PSys | game/script/script_interpreter.h:341 | std::vector<psys> | 1 methods | 2 (script_interpreter.cpp) | no | reach:3 | 0/2/0 | high | fold into std::vector<psys> (pads cover the fields) |
| PanelMesh | game/inventory_panel/loading_screen.cpp:38 | mesh (rgl/mesh.h:59) | 1 methods | 2 (loading_screen.cpp) | no | - | 2/0/0 | medium | method twin: declare the 1 method(s) on mesh |
| Pose | game/mission5/mission_5.cpp:443 | ViewObj_s257f0 (game/mission5/mission_5.h:138) | b | 2 (mission_5.cpp) | no | - | 0/2/0 | medium | fold into ViewObj_s257f0 (pads cover the fields) |
| Profile | game/char_sheet/cs_update.h:66 | ? | troop | 2 (cs_update.cpp) | likely | - | 2/0/0 | medium | a screen in g_windows.arr[n] (slot_as<T>(n)): fold into that slot's screen class (head_rec family, game/screen_records.h) |
| SetTimeKeys | game/party2/party_2.cpp:2906 | keyframes (include/keyframes.h:16) | frames, times, count | 2 (party_2.cpp) | no | - | 0/0/2 | medium | fold into keyframes (pads cover the fields) |
| TableauRec | game/world_data/tableau.h:15 | std::vector<E68> | f28, f2c, f30, f34 | 2 (obj_tbl.cpp) | likely | reach:2 | 2/0/0 | high | fold into std::vector<E68> (pads cover the fields) |
| Widget_s12 | game/mission5/mission_5.cpp:109 | ? | f10 | 2 (mission_5.cpp) | likely | - | 2/0/0 | medium | a screen in g_windows.arr[n] (slot_as<T>(n)): fold into that slot's screen class (head_rec family, game/screen_records.h) |
| block_copier | rgl/mesh2/gui_edit.h:48 | edit_vec (rgl/mesh2/gui_edit.h:458) | 1 methods | 2 (gui_edit.cpp, mission_items.cpp) | no | reach:2 | 2/0/0 | low | method twin: declare the 1 method(s) on edit_vec |
| res_rec | rgl/mesh2/mesh_2.h:37 | world (rgl/scene/world.h:75) | 21 methods | 2 (mesh_2.cpp) | no | - | 2/0/0 | low | method twin: declare the 21 method(s) on world |
| rgl_config | rgl/resource_manager.h:15 | Config (game/config/config_block.h:13) | 1 methods | 2 (resource_manager.cpp) | no | reach:2 | 2/0/0 | low | method twin: declare the 1 method(s) on Config |
| skin_calls | game/terrain_b/render_batch.h:77 | skin (rgl/resource.h:92) | 1 methods | 2 (terrain_b.cpp) | no | - | 0/2/0 | high | method twin: declare the 1 method(s) on skin |
| tstring_alloc | game/mission5/data_files.h:22 | MapDialog (rgl/mesh2/gui_edit.h:284) | - | 2 (game_2.cpp, gui_edit.cpp) | no | reach:2 | 2/0/0 | high | fold into MapDialog (pads cover the fields) |
| AgentX | game/agent3/agent_types.h:56 | Agent (game/agent_obj.h:72) | 1 methods | 1 (agent_logic.cpp) | no | doc | 0/1/0 | high | method twin: declare the 1 method(s) on Agent |
| Area_s29ec0 | game/mission5/mission_5.h:898 | MissionTemplate (game/mode_rec.h:37) | - | 1 (mission_5.cpp) | no | - | 0/0/1 | high | fold into MissionTemplate (pads cover the fields) |
| ArrHolder | game/script/script_interpreter.h:52 | FxDef (include/fx.h:144) | 1 methods | 1 (script_interpreter.cpp) | no | doc,reach:1 | 0/1/0 | medium | method twin: declare the 1 method(s) on FxDef |
| Blk2dd0 | game/options_panel/module_load.h:59 | mission_type (game/mission_type.h:15) | 1 methods | 1 (module_load.cpp) | no | - | 1/0/0 | medium | method twin: declare the 1 method(s) on mission_type |
| Block | game/inventory_panel/module_files.cpp:116 | ItemKind (include/item_kind.h:44) | 1 methods | 1 (module_files.cpp) | no | reach:1 | 1/0/0 | medium | method twin: declare the 1 method(s) on ItemKind |
| Camera_s2b5c0 | game/mission5/mission_5.h:1084 | MapWidget (game/map_widget.h:15) | 1 methods | 1 (mission_5.cpp) | no | - | 1/0/0 | high | method twin: declare the 1 method(s) on MapWidget |
| CarryRef | game/world_data/world_data.h:210 | ItemRef (game/item_ref.h:13) | item | 1 (script_interpreter.cpp) | no | reach:1 | 1/0/0 | medium | fold into ItemRef (pads cover the fields) |
| ConfigDialog | game/options_panel/world_app.cpp:411 | OptionsDlg (game/config/options_dlg.h:39) | 1 methods | 1 (world_app.cpp) | no | - | 0/1/0 | high | method twin: declare the 1 method(s) on OptionsDlg |
| E68W | game/world_data/world_data.h:163 | E68 (include/e68.h:15) | 3 methods | 1 (world_data.cpp) | no | reach:1 | 1/0/0 | high | method twin: declare the 3 method(s) on E68 |
| ElemCopier | game/party2/party_2.h:66 | allocator | 6 methods | 1 (party_2.cpp) | no | reach:1 | 0/0/1 | high | fold into allocator (pads cover the fields) |
| EventRec | game/party2/party_2.h:79 | E92 (game/party/game.h:116) | target, angle, f14, pos | 1 (party_2.cpp) | no | reach:1 | 0/1/0 | medium | fold into E92 (real type has no offsets there: check) |
| FieldHost | game/mission_ui2/field_panel.h:20 | ContainerBase (game/ui/container.h:16) | views | 1 (field_panel.h) | no | - | 0/0/1 | medium | fold into ContainerBase (real type has no offsets there: check) |
| FireAgent | game/agent4/agent_logic.h:21 | Missile (game/missile.h:13) | 1 methods | 1 (agent_logic.cpp) | no | - | 0/1/0 | medium | method twin: declare the 1 method(s) on Missile |
| FireRay | game/terrain_a/misc.h:118 | capsule (include/shape.h:80) | from, to, width | 1 (agent_logic.cpp) | no | - | 0/1/0 | high | fold into capsule (real type has no offsets there: check) |
| ForceLabel | game/mission5/mission_5.h:1266 | TeamRec (game/mission.h:60) | 1 methods | 1 (mission_5.cpp) | no | - | 0/1/0 | low | method twin: declare the 1 method(s) on TeamRec |
| ForceLabel | game/script/script_interpreter.cpp:145 | Leader (game/script/script_interpreter.cpp:712) | 1 methods | 1 (script_interpreter.cpp) | no | reach:1 | 1/0/0 | low | method twin: declare the 1 method(s) on Leader |
| GlobVec | game/world_data/world_data.h:181 | Presentation (game/script/script_interpreter.h:210) | list | 1 (world_data.cpp) | likely | reach:1 | 1/0/0 | medium | fold into Presentation (pads cover the fields) |
| HashTab | game/cls_map/cls_map.cpp:71 | GlyphMap (game/font.h:29) | 1 methods | 1 (cls_map.cpp) | no | doc,reach:1 | 1/0/0 | medium | method twin: declare the 1 method(s) on GlyphMap |
| HudOuter | game/agent5/agent_hud.h:54 | Camera (game/party2/camera_class.h:26) | 1 methods | 1 (agent_hud.cpp) | no | doc | 1/0/0 | low | method twin: declare the 1 method(s) on Camera |
| I64VecW | game/script/script_interpreter.cpp:959 | I64Vec (include/i64_vec.h:7) | 1 methods | 1 (script_interpreter.cpp) | no | - | 0/0/1 | high | method twin: declare the 1 method(s) on I64Vec |
| ItemKind_s284a0 | game/mission5/mission_5.h:436 | ItemKind (include/item_kind.h:44) | - | 1 (mission_5.cpp) | no | - | 0/1/0 | high | fold into ItemKind (pads cover the fields) |
| ItemKind_s2b5c0 | game/mission5/mission_5.h:1039 | ItemKind (include/item_kind.h:44) | - | 1 (mission_5.h) | no | - | 0/0/1 | high | fold into ItemKind (pads cover the fields) |
| ItemRef_s2b5c0 | game/mission5/mission_5.h:1046 | ItemRef (game/item_ref.h:13) | id, count | 1 (mission_5.cpp) | no | - | 0/1/0 | high | fold into ItemRef (real type has no offsets there: check) |
| LabelBox | game/agent4/agent_logic.cpp:7858 | matrix (rgl/rgl_math.h:69) | a, b, scale | 1 (agent_logic.cpp) | no | - | 1/0/0 | medium | fold into matrix (real type has no offsets there: check) |
| MapBlock | game/cls_map/progress_bar.h:89 | E68 (include/e68.h:15) | - | 1 (progress_bar.cpp) | no | reach:1 | 1/0/0 | medium | fold into E68 (pads cover the fields) |
| MapDialog | game/agent5/editor_dialog.cpp:68 | Alloc204 (rgl/mesh2/rec_tables.h:154) | 1 methods | 1 (editor_dialog.cpp) | no | reach:6 | 1/0/0 | high | method twin: declare the 1 method(s) on Alloc204 |
| MapEditor_s2cf60 | game/mission5/mission_5.h:1283 | Mission_s2b5c0 (game/mission5/mission_5.h:1111) | 1 methods | 1 (mission_5.h) | no | - | 0/0/1 | high | method twin: declare the 1 method(s) on Mission_s2b5c0 |
| MapEditor_s2f250 | game/mission5/mission_5.h:1286 | Mission_s2b5c0 (game/mission5/mission_5.h:1111) | 1 methods | 1 (mission_5.h) | no | - | 0/0/1 | high | method twin: declare the 1 method(s) on Mission_s2b5c0 |
| MapPanel | game/inventory_panel/mission_ctx.cpp:118 | CampaignScreen (game/campaign_screen.h:17) | 1 methods | 1 (mission_ctx.cpp) | no | doc | 0/1/0 | low | method twin: declare the 1 method(s) on CampaignScreen |
| MenuKind | game/options_panel/hud.h:44 | MenuKindRec (game/cls_map/cls_map.h:163) | name, flags, sound, f20 | 1 (hud.h) | likely | doc | 0/0/1 | low | fold into MenuKindRec (pads cover the fields) |
| MissileU | game/mesh_slot/mesh_game.cpp:359 | Missile (game/missile.h:13) | 1 methods | 1 (mesh_game.cpp) | no | reach:1 | 0/1/0 | high | method twin: declare the 1 method(s) on Missile |
| MissionProps | game/mission5/mission_5.h:410 | Mission (game/mission.h:73) | 1 methods | 1 (mission_5.cpp) | no | - | 0/1/0 | high | method twin: declare the 1 method(s) on Mission |
| ObjectList | game/options_panel/module_load.h:61 | ptype (game/ptype.h:25) | 1 methods | 1 (module_load.cpp) | no | doc | 1/0/0 | medium | method twin: declare the 1 method(s) on ptype |
| PVec | game/inventory_panel/widget_text.cpp:25 | IntVec (game/int_vec.h:16) | 1 methods | 1 (widget_text.cpp) | no | reach:1 | 0/0/1 | high | method twin: declare the 1 method(s) on IntVec |
| PairStack | game/ui/mission_ui.h:151 | ScriptStack (game/party2/camera.h:32) | pairs, count | 1 (script_interpreter.cpp) | no | reach:1 | 1/0/0 | medium | fold into ScriptStack (pads cover the fields) |
| PropAnim | game/mesh_slot/mesh_game.cpp:381 | PlacedObj (game/placed_obj.h:17) | 1 methods | 1 (mesh_game.cpp) | no | reach:1 | 0/1/0 | high | method twin: declare the 1 method(s) on PlacedObj |
| RefItem | rgl/mesh2/gui_edit.h:113 | PresentationRec (include/prsnt_rec.h:10) | name, flags, triggers, f20 | 1 (gui_edit.cpp) | likely | reach:1 | 1/0/0 | low | fold into PresentationRec (pads cover the fields) |
| RotFrame | game/script/script_interpreter.cpp:332 | rgl::matrix (rgl/rgl_math.h:69) | 1 methods | 1 (script_interpreter.cpp) | no | reach:2 | 0/1/0 | high | fold into rgl::matrix (pads cover the fields) |
| RuleRec | game/terrain_a/misc.h:29 | SpawnRec (game/mode_rec.h:19) | f0 | 1 (terrain_emit.cpp) | no | - | 1/0/0 | medium | fold into SpawnRec (pads cover the fields) |
| SceneBaseCtor | game/world_data/world_data.h:169 | SceneBase (game/world_data/world_data.h:247) | 1 methods | 1 (world_data.h) | no | - | 0/0/1 | medium | method twin: declare the 1 method(s) on SceneBase |
| SimpleTrigger | game/mission5/game_2.h:80 | SimpleRec (game/party/game.h:80) | f0, stmt | 1 (game_2.cpp) | no | reach:1 | 1/0/0 | low | fold into SimpleRec (real type has no offsets there: check) |
| SkinApplyExt | game/terrain_b/troop_mgr.h:13 | Skin (include/skin.h:45) | 1 methods | 1 (poly_mesh.cpp) | no | reach:2 | 1/0/0 | high | method twin: declare the 1 method(s) on Skin |
| SkyF | game/mesh_slot/mesh_game.cpp:386 | float | f48, f1f8, f208 | 1 (mesh_game.cpp) | likely | - | 0/0/1 | high | fold into float (pads cover the fields) |
| TStringArray | game/agent5/agent_hud.h:167 | tstring (game/tstring.h:19) | 1 methods | 1 (agent_hud.cpp) | no | - | 0/1/0 | high | method twin: declare the 1 method(s) on tstring |
| Team | game/inventory_panel/mission_ctx.h:48 | TeamRec (game/mission.h:60) | f0, leader, cls, relation | 1 (mission_ctx.cpp) | no | - | 1/0/0 | medium | fold into TeamRec (pads cover the fields) |
| TearObj | game/agent5/editor_dialog.cpp:66 | graph_obj (rgl/mesh2/graph_obj.h:61) | 1 methods | 1 (editor_dialog.cpp) | no | - | 1/0/0 | high | method twin: declare the 1 method(s) on graph_obj |
| TerrainExt | game/terrain_b/terrain.h:22 | Terrain (game/terrain_a/terrain.h:150) | 1 methods | 1 (poly_mesh.cpp) | no | - | 1/0/0 | high | method twin: declare the 1 method(s) on Terrain |
| TimeKey | game/script/script_interpreter.h:51 | Light (game/light.h:11) | 1 methods | 1 (script_interpreter.cpp) | no | reach:1 | 0/1/0 | medium | method twin: declare the 1 method(s) on Light |
| TriggerItem | game/script/script_interpreter.h:191 | ListHolder (game/mission_ui2/mission_3.h:23) | 1 methods | 1 (script_interpreter.cpp) | no | reach:2 | 0/1/0 | high | method twin: declare the 1 method(s) on ListHolder |
| TroopX | game/mission_ui2/stat_panel_x.cpp:37 | Troop (include/troop.h:20) | 1 methods | 1 (stat_panel_x.cpp) | no | reach:1 | 1/0/0 | high | method twin: declare the 1 method(s) on Troop |
| WItem | game/script/script_interpreter.cpp:161 | Camera (game/party2/camera_class.h:26) | 1 methods | 1 (script_interpreter.cpp) | no | - | 0/1/0 | medium | method twin: declare the 1 method(s) on Camera |
| alloc34 | game/terrain_b/records.h:36 | vec18 (game/terrain_b/records.h:53) | - | 1 (poly_mesh.cpp) | no | - | 1/0/0 | medium | fold into vec18 (pads cover the fields) |
| alloc64 | game/terrain_b/records.h:36 | vec18 (game/terrain_b/records.h:53) | - | 1 (poly_mesh.cpp) | no | - | 1/0/0 | medium | fold into vec18 (pads cover the fields) |
| alloc_E12 | game/alloc_construct.h:13 | ElemCopier (game/party2/party_2.h:66) | 1 methods | 1 (party_2.h) | no | reach:1 | 1/0/0 | low | method twin: declare the 1 method(s) on ElemCopier |
| alloc_rec136 | game/alloc_construct.h:12 | vecArr (game/terrain_b/records.h:86) | 1 methods | 1 (poly_mesh.cpp) | no | - | 1/0/0 | medium | method twin: declare the 1 method(s) on vecArr |
| contact_inl | rgl/dynamic/world_collide.cpp:416 | contact_list_m (rgl/scene/scene_entity.h:103) | 5 methods | 1 (world_collide.cpp) | no | - | 1/0/0 | low | method twin: declare the 5 method(s) on contact_list_m |
| curve4 | rgl/scene/emitter.h:24 | float_pair (rgl/scene/emitter.h:26) | t0, v0, t1, v1 | 1 (psys.cpp) | no | - | 0/1/0 | low | fold into float_pair (real type has no offsets there: check) |
| key_clip | rgl/resource.h:155 | named_object (rgl/resource.h:19) | tracks, extra | 1 (resmgr.cpp) | likely | reach:1 | 0/1/0 | low | fold into named_object (real type has no offsets there: check) |
| material_io | rgl/core/resource_io.h:50 | material (rgl/resource.h:66) | 1 methods | 1 (resmgr.cpp) | no | - | 1/0/0 | high | method twin: declare the 1 method(s) on material |
| mesh_flip | game/mesh_slot/mesh_ops.cpp:37 | mesh (rgl/mesh.h:59) | 1 methods | 1 (mesh_ops.cpp) | no | reach:1 | 0/1/0 | medium | method twin: declare the 1 method(s) on mesh |
| mesh_node | game/mesh_slot/mesh_scene.cpp:30 | rgl::mesh (rgl/mesh.h:59) | 1 methods | 1 (mesh_scene.cpp) | no | doc,reach:1 | 1/0/0 | high | fold into rgl::mesh (pads cover the fields) |
| mesh_s2f250 | game/mission5/mesh_3.h:487 | rgl::mesh (rgl/mesh.h:59) | 2 methods | 1 (mesh_3.h) | no | - | 0/0/1 | high | fold into rgl::mesh (pads cover the fields) |
| mesh_scene | game/inventory_panel/module_files.cpp:107 | Animator (game/animator.h:18) | 1 methods | 1 (module_files.cpp) | no | doc,reach:1 | 1/0/0 | medium | method twin: declare the 1 method(s) on Animator |
| mi_ops | game/terrain_b/render_batch.h:128 | mesh (rgl/mesh.h:59) | 1 methods | 1 (terrain_b.cpp) | no | - | 0/1/0 | high | method twin: declare the 1 method(s) on mesh |
| music_set | game/inventory_panel/module_files.cpp:100 | SoundMgr (include/sound_mgr.h:26) | 1 methods | 1 (module_files.cpp) | no | reach:1 | 1/0/0 | high | method twin: declare the 1 method(s) on SoundMgr |
| prsnt_record | game/options_panel/module_load.h:60 | EnvRow (game/env_row.h:12) | 1 methods | 1 (module_load.cpp) | no | - | 1/0/0 | medium | method twin: declare the 1 method(s) on EnvRow |
| scene_api | game/script/script_interpreter.cpp:2349 | scene_obj (game/script/script_interpreter.cpp:2323) | 1 methods | 1 (script_interpreter.cpp) | no | - | 0/0/1 | high | method twin: declare the 1 method(s) on scene_obj |
| sound_record | game/config/config.h:119 | music_name (rgl/mesh2/rec_tables.h:183) | name, f10, samples, nsamples | 1 (rec_tables.cpp) | likely | - | 0/0/1 | medium | fold into music_name (real type has no offsets there: check) |
| texture_x | game/terrain_a/misc.h:81 | texture (rgl/resource.h:34) | 1 methods | 1 (terrain_render.cpp) | no | - | 1/0/0 | high | method twin: declare the 1 method(s) on texture |
| trigger_rec | game/config/config.h:92 | TriggerRec (include/trigger_rec.h:11) | check_interval, delay_interval, rearm_interval, cond +1 | 1 (trigger_vec.cpp) | likely | doc,reach:1 | 1/0/0 | high | fold into TriggerRec (pads cover the fields) |
| vec2x | game/agent4/agent_logic.cpp:3472 | vec2 (include/vec.h:13) | 1 methods | 1 (agent_logic.cpp) | no | - | 0/1/0 | high | method twin: declare the 1 method(s) on vec2 |
| AgentX | rgl/xmlcfg/game_util.h:33 | Agent (game/agent_obj.h:72) | 3 methods | 0 | no | doc | - | high | method twin: declare the 3 method(s) on Agent |
| BeardMeshNew | game/cls_map/progress_bar.cpp:1262 | BeardMesh (include/skin.h:36) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on BeardMesh |
| BlowInit | game/blow.h:39 | Agent (game/agent_obj.h:72) | 2 methods | 0 | no | doc | - | high | method twin: declare the 2 method(s) on Agent |
| Blow_s284a0 | game/mission5/mission_5.h:446 | Blow (game/blow.h:18) | attacker, pos, bone, dmg_in +1 | 0 | no | - | - | high | fold into Blow (real type has no offsets there: check) |
| DialogMgrX | game/script/script_interpreter.h:126 | DialogMgr (game/dialog_mgr.h:19) | 1 methods | 0 | no | reach:1 | - | high | method twin: declare the 1 method(s) on DialogMgr |
| FxAnimQ | rgl/dynamic/fx_anim.cpp:28 | FxAnim (rgl/dynamic/fx_anim.h:111) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on FxAnim |
| GameDataV | game/inventory_panel/inv_common.h:33 | GameData (include/game_data.h:101) | 8 methods | 0 | no | - | - | high | method twin: declare the 8 method(s) on GameData |
| GameDataX | game/map_editor/exchange_screens.h:75 | GameData (include/game_data.h:101) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on GameData |
| ItemRef_s29ec0 | game/mission5/mission_5.h:824 | ItemRef (game/item_ref.h:13) | item | 0 | no | - | - | high | fold into ItemRef (pads cover the fields) |
| Item_s2fe40 | game/mission5/mesh_3.h:652 | ItemKind (include/item_kind.h:44) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on ItemKind |
| MapName_s2fe40 | game/mission5/mesh_3.h:658 | ItemKind (include/item_kind.h:44) | - | 0 | no | - | - | high | fold into ItemKind (pads cover the fields) |
| MapView_s2cf60 | game/mission5/mesh_3.h:156 | MapWidget (game/map_widget.h:15) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on MapWidget |
| MissionV | game/inventory_panel/mission_ctx.h:23 | Mission (game/mission.h:73) | 2 methods | 0 | no | - | - | high | method twin: declare the 2 method(s) on Mission |
| MissionW | game/world_data/world_data.h:99 | Mission (game/mission.h:73) | 9 methods | 0 | no | - | - | high | method twin: declare the 9 method(s) on Mission |
| PartyV | game/inventory_panel/mission_ctx.cpp:124 | Party (include/party.h:43) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on Party |
| PartyX | game/party_x.h:11 | Party (include/party.h:43) | 17 methods | 0 | no | - | - | high | method twin: declare the 17 method(s) on Party |
| SceneObj_s2f250 | game/mission5/mesh_3.h:519 | matrix (rgl/rgl_math.h:69) | frame, meshes, visible | 0 | no | - | - | high | fold into matrix (real type has no offsets there: check) |
| ShadowMapP | game/mesh_slot/mesh_slot.h:74 | ShadowMap (game/shadow_map.h:18) | 4 methods | 0 | no | - | - | high | method twin: declare the 4 method(s) on ShadowMap |
| Shape_s2ae70 | game/mission5/mission_5.h:975 | Shape (game/cls_map/progress_bar.h:68) | 3 methods | 0 | no | - | - | high | method twin: declare the 3 method(s) on Shape |
| TroopIO | game/party/game.h:125 | Troop (include/troop.h:20) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on Troop |
| WidgetDtor | game/party/party_window.h:36 | MissionUi (game/ui/mission_ui.h:30) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on MissionUi |
| edit_vec | rgl/mesh2/gui_edit.h:458 | block_copier (rgl/mesh2/gui_edit.h:48) | 1 methods | 0 | no | reach:1 | - | high | method twin: declare the 1 method(s) on block_copier |
| frustum_view | game/renderer.h:49 | frustum (include/frustum.h:21) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on frustum |
| lod_mesh_a | game/mesh_slot/mesh_scene.cpp:479 | lod_mesh (rgl/mesh.h:185) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on lod_mesh |
| mat3_value | game/script/script_interpreter.cpp:2825 | mat3 (rgl/rgl_math.h:58) | - | 0 | no | - | - | high | fold into mat3 (pads cover the fields) |
| material_x | game/terrain_a/misc.h:84 | material (rgl/resource.h:66) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on material |
| medge_s2da20 | game/mission5/mesh_3.h:239 | MEdge (game/terrain_b/poly_mesh.h:35) | v0, v1, nfaces, flag +2 | 0 | likely | - | - | high | fold into MEdge (real type has no offsets there: check) |
| medge_s2dfd0 | game/mission5/mesh_3.h:306 | MEdge (game/terrain_b/poly_mesh.h:35) | v0, v1, nfaces, flag +2 | 0 | likely | - | - | high | fold into MEdge (real type has no offsets there: check) |
| mesh_anim_x | game/mesh_slot/mesh_scene.cpp:476 | mesh (rgl/mesh.h:59) | 1 methods | 0 | no | reach:1 | - | high | method twin: declare the 1 method(s) on mesh |
| mesh_calls | game/terrain_b/render_batch.h:33 | mesh (rgl/mesh.h:59) | 2 methods | 0 | no | - | - | high | method twin: declare the 2 method(s) on mesh |
| mesh_flip | game/terrain_b/terrain_b.h:40 | mesh (rgl/mesh.h:59) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on mesh |
| mesh_list_x | game/lod_mesh_x.h:8 | mesh_list (rgl/mesh.h:181) | 2 methods | 0 | no | - | - | high | method twin: declare the 2 method(s) on mesh_list |
| mesh_node | rgl/xmlcfg/game_util.h:74 | mesh (rgl/mesh.h:59) | 1 methods | 0 | no | doc | - | high | method twin: declare the 1 method(s) on mesh |
| mface_s2da20 | game/mission5/mesh_3.h:232 | MVert (game/terrain_b/poly_mesh.h:27) | pos, flag, index | 0 | likely | - | - | high | fold into MVert (pads cover the fields) |
| mface_s2dfd0 | game/mission5/mesh_3.h:299 | MVert (game/terrain_b/poly_mesh.h:27) | pos, flag, index | 0 | likely | - | - | high | fold into MVert (pads cover the fields) |
| mface_s2e960 | game/mission5/mesh_3.h:366 | MVert (game/terrain_b/poly_mesh.h:27) | pos, flag, index | 0 | likely | - | - | high | fold into MVert (pads cover the fields) |
| mpoly_s2da20 | game/mission5/mesh_3.h:250 | MPoly (game/terrain_b/poly_mesh.h:57) | stamp, parent, centroid, normal +6 | 0 | likely | - | - | high | fold into MPoly (real type has no offsets there: check) |
| mpoly_s2dfd0 | game/mission5/mesh_3.h:317 | MPoly (game/terrain_b/poly_mesh.h:57) | stamp, parent, centroid, normal +6 | 0 | likely | - | - | high | fold into MPoly (real type has no offsets there: check) |
| scene_ms2 | game/mesh_slot/mesh_slot.h:45 | scene (rgl/scene_rec.h:41) | - | 0 | no | - | - | high | fold into scene (pads cover the fields) |
| shape_base_s2b5c0 | game/mission5/mission_5.h:1073 | face (include/shape.h:104) | m4, m8, mc, min_x +3 | 0 | no | - | - | high | fold into face (real type has no offsets there: check) |
| skin_x | game/terrain_a/misc.h:87 | skin (rgl/resource.h:92) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on skin |
| smat_s2f250 | game/mission5/mesh_3.h:516 | matrix (rgl/rgl_math.h:69) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on matrix |
| string_view_ | rgl/shapes/shapes.h:32 | string_view_ (game/script/script_interpreter.h:59) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on string_view_ |
| tstr | game/script/script_interpreter.h:134 | tstring (game/tstring.h:19) | 1 methods | 0 | no | reach:1 | - | high | method twin: declare the 1 method(s) on tstring |
| vec | game/agent5/agent_hud.h:49 | MenuLayout (game/agent5/agent_hud.h:134) | 1 methods | 0 | no | reach:2 | - | high | method twin: declare the 1 method(s) on MenuLayout |
| vec2c | game/strat/strat_ext.h:33 | vec2 (include/vec.h:13) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on vec2 |
| vec2r | game/party2/party_2.h:75 | vec2 (include/vec.h:13) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on vec2 |
| vec2x | rgl/xmlcfg/game_util.h:48 | vec2 (include/vec.h:13) | 2 methods | 0 | no | - | - | high | method twin: declare the 2 method(s) on vec2 |
| vec3_s257f0 | game/mission5/mission_5.h:152 | Vec3 (include/vec.h:40) | x, y, z | 0 | no | - | - | high | fold into Vec3 (pads cover the fields) |

### B. Real type exists but conflicts (36)

| view | file:line | overlays | fields named | casts (files) | 64? | path | rows e/n/- | conf | notes |
|---|---|---|---|---|---|---|---|---|---|
| lod_mesh | rgl/mesh.h:185 | node_groups (game/skeleton.h:22) | lists, count, cur, m8c +1 | 32 (agent_hud.cpp, agent_logic.cpp +7) | yes | doc,reach:7 | 22/7/3 | high | **rgl::lod_mesh (rgl/mesh.h) is itself a view: its comment says the 0xe0-byte object is lod_group / lod_mesh2 (ctor 0x41c2a0); reconcile lists / m8c / mask with rgl/scene/scene.h lod_mesh2 and resmgr.h lod_group** |
| AnimRec | rgl/dynamic/fx_anim.h:50 | EnvRec (include/fx.h:92) | weight, active_mask, flip_mask, f18 +19 | 14 (fx_anim.cpp, fx_anim.h) | yes | reach:13 | 0/13/1 | high | members of EnvRec at the same offsets disagree: +0x20 int clock vs gui_timer tm |
| Owned | game/mission_ui2/mission_3.h:17 | GameData (include/game_data.h:101) | owner, refs | 3 (mission_3.cpp) | yes | reach:3 | 3/0/0 | medium | members of GameData at the same offsets disagree: +0x2c Owned* owner vs rgl::mesh* map_track |
| VoiceTable | game/agent4/agent_logic.h:44 | Animator (game/animator.h:18) | recs | 3 (agent_logic.cpp) | yes | reach:5 | 1/2/0 | high | members of Animator at the same offsets disagree: +0x18 std::vector<VoiceRec> recs vs SkinVec skin_vec |
| PLodGroup | rgl/mesh2/gui_edit.h:198 | lod_group (rgl/core/resmgr.h:47) | lists0 | 1 (gui_edit.cpp) | yes | reach:1 | 1/0/0 | high | members of lod_group at the same offsets disagree: +0x34 std::vector<PMesh*> lists0 vs mesh_owner[5] lists |
| EditorSceneView | game/agent5/editor_dialog.cpp:113 | Mission (game/mission.h:73) | brush_count, brushes, object_kind, edit_obj +3 | 24 (editor_dialog.cpp) | yes | - | 21/0/3 | high | members of Mission at the same offsets disagree: +0x63d0 int brush_count vs TerrainDesc terrain; +0x6688 std::vector<EditorTerrainView> world vs std::vector<PlacedObj> props |
| Map_s2f250 | game/mission5/mesh_3.h:536 | Mission (game/mission.h:73) | object, records | 23 (mesh_poly_ray.cpp) | yes | - | 0/23/0 | high | members of Mission at the same offsets disagree: +0x65e4 SceneObj_s2f250* object vs rgl::entity* edit_obj |
| GameData_s29840 | game/mission5/mission_5.h:683 | GameData (include/game_data.h:101) | cur_party, cur_troop, parties, wind +4 | 11 (mission_5.cpp, mission_5.h) | yes | - | 5/4/2 | high | members of GameData at the same offsets disagree: +0x13a50 char* levels vs SceneFile* scenes |
| GameData_s29ec0 | game/mission5/mission_5.h:858 | GameData (include/game_data.h:101) | player, chars, levels, nlevels +2 | 11 (mission_5.cpp) | yes | - | 0/11/0 | high | members of GameData at the same offsets disagree: +0x13a50 char* levels vs SceneFile* scenes |
| Screen_s2ae70 | game/mission5/mission_5.h:987 | MissionBlocker (game/options_panel/mission_item_recs.h:37) | world, order_state, order, order_timer +8 | 11 (mission_5.cpp) | yes | - | 0/11/0 | high | members of MissionBlocker at the same offsets disagree: +0x2a8 Timer order_timer vs gui_timer timer_a |
| Game_s284a0 | game/mission5/mission_5.h:528 | GameData (include/game_data.h:101) | f139e4, map_editor | 9 (mission_5.cpp) | yes | - | 0/9/0 | high | members of GameData at the same offsets disagree: +0x1a090 int map_editor vs SceneEditor editor |
| Mesh_s2dfd0 | game/mission5/mesh_3.h:333 | graph_obj (rgl/mesh2/graph_obj.h:61) | verts, polys, edges, verts2 +4 | 4 (mesh_3.h) | yes | - | 0/0/4 | high | members of graph_obj at the same offsets disagree: +0x18 std::vector<mface_s2dfd0> verts vs std::vector<node> nodes; +0x28 std::vector<mpoly_s2dfd0> polys vs std::vector<graph_face> faces |
| DistObj | game/inventory_panel/map_tooltip.cpp:48 | Party (include/party.h:43) | x, y, px, py | 2 (map_panel.cpp) | yes | - | 0/2/0 | high | members of Party at the same offsets disagree: +0x30 float x vs PartyV4 pos; +0x30 float y vs PartyV4 pos |
| GameData_s27c90 | game/mission5/mission_5.h:344 | GameData (include/game_data.h:101) | levels | 2 (mission_5.cpp) | yes | - | 1/1/0 | high | members of GameData at the same offsets disagree: +0x13a50 SceneObjRec_s27c90* levels vs SceneFile* scenes |
| MapObjList_s29ec0 | game/mission5/mission_5.h:888 | MenuRec (game/menu_rec.h:23) | count, objs | 2 (mission_5.cpp) | yes | - | 0/2/0 | high | members of MenuRec at the same offsets disagree: +0x58 MapObject_s29ec0* objs vs menu_item* items |
| Mesh_s2e960 | game/mission5/mesh_3.h:428 | PolyMesh (game/terrain_b/poly_mesh.h:120) | verts, polys, edges, verts2 +4 | 2 (mesh_3.h) | yes | - | 0/0/2 | high | members of PolyMesh at the same offsets disagree: +0x28 std::vector<mpoly_s2e960> polys vs std::vector<MPoly> faces |
| CamParams | game/agent/agent.h:75 | Agent (game/agent_obj.h:72) | v40, v50, f60 | 1 (agent_logic.cpp) | yes | - | 0/1/0 | high | members of Agent at the same offsets disagree: +0x60 float f60 vs vector4 vel |
| ViewObj_s257f0 | game/mission5/mission_5.h:138 | Rec1208 (game/conv_screen.h:17) | window, cam_frame, world, blend +1 | 1 (mission_5.cpp) | yes | - | 1/0/0 | high | members of Rec1208 at the same offsets disagree: +0x11d4 float blend vs int quest_mode |
| Agent_s2b5c0 | game/mission5/mission_5.h:1059 | Agent (game/agent_obj.h:72) | orders, active, wield, frame | 0 | yes | - | - | high | members of Agent at the same offsets disagree: +0x250 AgentFrame_s2b5c0* frame vs rgl::strategic_entity* body |
| Mission_s2b5c0 | game/mission5/mission_5.h:1111 | Mission (game/mission.h:73) | scene, pages, view, pick_timer +9 | 0 | yes | - | - | high | members of Mission at the same offsets disagree: +0x65d0 UiObj_s2b5c0* f65d0 vs rgl::entity* cursor; +0x65dc UiObj_s2b5c0* f65dc vs rgl::entity* sel_marker |
| SceneObj_s27c90 | game/mission5/mission_5.h:386 | PlacedObj (game/placed_obj.h:17) | f10, f14, frame, f158 +8 | 0 | yes | - | - | high | members of PlacedObj at the same offsets disagree: +0x158 float f158 vs rgl::vector4 scale; +0x158 float f15c vs rgl::vector4 scale |
| SkelBone | rgl/mesh2/mesh_2.h:134 | Bone (rgl/core/bone.h:21) | f0, parent, local, world +8 | 8 (mesh_2.cpp) | likely | reach:8 | 8/0/0 | low | members of Bone at the same offsets disagree: +0xa0 int cone_min vs float twist_limit |
| GlyphNode | game/font.h:25 | iterator (rgl/dynamic/dynamic_entity.h:39) | next, prev, key, val | 4 (agent_logic.cpp, cls_map.cpp) | no | doc,reach:2 | 2/0/2 | medium | members of iterator at the same offsets disagree: +0x0 GlyphNode* next vs Contact* _Ptr |
| TimedVal | game/triggers/trigger_vec.cpp:1252 | TriggerRec (include/trigger_rec.h:11) | id, t | 4 (trigger_vec.cpp) | likely | reach:1 | 1/0/3 | medium | members of TriggerRec at the same offsets disagree: +0x0 int id vs float check_interval; +0x8 __int64 t vs float rearm_interval |
| grid_cell | rgl/dynamic/world_grid.h:56 | cell (rgl/scene/world.h:34) | objs, f1c, ids, head | 4 (world_cells.cpp, world_grid.cpp) | likely | doc,reach:1 | 1/3/0 | low | members of cell at the same offsets disagree: +0x10 shape_list objs vs std::list<int> b |
| ItemStackG | game/mesh_slot/mesh_game.cpp:45 | ItemRef (game/item_ref.h:13) | b | 3 (mesh_game.cpp, script_interpreter.cpp) | no | reach:4 | 3/0/0 | high | members of ItemRef at the same offsets disagree: +0x0 unsigned a b vs int item |
| MenuKindRec | game/cls_map/cls_map.h:163 | TimerList (game/ui/timer_list.h:18) | name, a, sound, first | 3 (cls_map.cpp) | likely | doc | 3/0/0 | low | members of TimerList at the same offsets disagree: +0x0 tstring name vs int count |
| list_int | include/list_int.h:12 | list_node (game/options_panel/world_app.cpp:142) | head, w04, count | 3 (cls_map.cpp, world_app.cpp) | likely | reach:1 | 0/1/2 | low | members of list_node at the same offsets disagree: +0x4 list_node_int* w04 vs HNode* sentinel |
| FrustumBlock | game/char_sheet/terrain_page_input.h:43 | frustum (include/frustum.h:21) | d | 2 (terrain_page_input.cpp) | no | - | 2/0/0 | medium | members of frustum at the same offsets disagree: +0x0 int[0x48] d vs char perspective |
| OptionsMenu | game/cls_map/cls_map.h:217 | GameKeys (game/game_keys.h:7) | f00, f04, f08, f0c +23 | 2 (cls_map.cpp) | no | - | 2/0/0 | high | members of GameKeys at the same offsets disagree: +0x0 int f00 vs int[0x25][2] entry |
| CandRec | game/terrain_a/misc.h:30 | Stack (include/party.h:30) | f0, troop, f8 | 1 (terrain_emit.cpp) | no | - | 1/0/0 | medium | members of Stack at the same offsets disagree: +0x8 unsigned char f8 vs int flags |
| MeshData_s2da20 | game/mission5/mesh_3.h:264 | MPoly (game/terrain_b/poly_mesh.h:57) | verts, polys, edges, verts2 +4 | 0 | likely | - | - | high | members of MPoly at the same offsets disagree: +0x18 vec_s2da20<mface_s2da20> verts vs vector4 normal; +0x48 vec_s2da20<mface_s2da20> verts2 vs int n |
| MissionX | game/script/script_interpreter.h:354 | Mission (game/mission.h:73) | 29 methods | 0 | no | reach:29 | - | high | **the script's Mission (157 mission() calls): EntryPoint[64] at +0x670 vs Mission's MarkRec[64], teams / mode / cam_frame to reconcile with game/mission.h, then mission() returns Mission*** |
| mat3_s2cf60 | game/mission5/mesh_3.h:176 | matrix (rgl/rgl_math.h:69) | m | 0 | no | - | - | high | members of matrix at the same offsets disagree: +0x0 float[12] m vs vector4 s |
| medge_s2e960 | game/mission5/mesh_3.h:373 | MEdge (game/terrain_b/poly_mesh.h:35) | v0, v1, flag, t0 +5 | 0 | likely | - | - | high | members of MEdge at the same offsets disagree: +0x18 float t0 vs float[3] crease |
| mpoly_s2e960 | game/mission5/mesh_3.h:390 | MPoly (game/terrain_b/poly_mesh.h:57) | parent, centroid_x, centroid_y, centroid_z +10 | 0 | likely | - | - | high | members of MPoly at the same offsets disagree: +0x8 float centroid_x vs vector4 centroid; +0x18 float normal_x vs vector4 normal |

### C. Real type not yet defined (26)

| view | file:line | overlays | fields named | casts (files) | 64? | path | rows e/n/- | conf | notes |
|---|---|---|---|---|---|---|---|---|---|
| CarryCtx | game/world_data/world_data.h:211 | ? | c1, c2, c3 | 1 (script_interpreter.cpp) | yes | reach:1 | 1/0/0 | medium | overlays raw memory / no real record found (sources: void) |
| Rec88 | game/mission5/game_2.h:174 | ? | count, nb, obj | 1 (game_2.cpp) | yes | doc,reach:1 | 0/1/0 | medium | overlays raw memory / no real record found (sources: char) |
| Scene_s29840 | game/mission5/mission_5.h:713 | ? | recs | 1 (mission_5.cpp) | yes | - | 1/0/0 | high | overlays raw memory / no real record found (sources: ?) |
| EditorTerrainView | game/agent5/editor_dialog.cpp:98 | ? | frame, rx, ry, rz +1 | 0 | yes | - | - | high | overlays raw memory / no real record found |
| MapEditor_s2cf60 | game/mission5/mesh_3.h:160 | ? | view, drag_frame, yaw | 0 | yes | - | - | high | overlays raw memory / no real record found |
| mat3_s2f250 | game/mission5/mesh_3.h:515 | ? | s, f, u | 5 (mesh_3.h, mesh_poly_ray.cpp) | no | reach:1 | 1/4/0 | high | overlays raw memory / no real record found (sources: ?) |
| SceneRec48 | game/world_data/obj_tbl.h:34 | ? | name, flags, px, py +7 | 4 (obj_tbl.cpp) | likely | reach:4 | 4/0/0 | medium | overlays raw memory / no real record found (sources: void) |
| rec34 | include/rec34.h:10 | ? | f00, name, f14, body | 3 (game_2.cpp, rec34.h) | likely | reach:2 | 2/0/1 | low | overlays raw memory / no real record found (sources: void) |
| Node24 | game/mission5/game_2.h:176 | ? | f00, f04, f08, f0c +1 | 2 (game_2.cpp) | likely | reach:2 | 0/2/0 | medium | overlays raw memory / no real record found (sources: char) |
| HudOuter | game/config/widget_mgr.cpp:379 | ? | 1 methods | 1 (widget_mgr.cpp) | no | doc,reach:1 | 1/0/0 | low | overlays raw memory / no real record found (sources: define) |
| MeshRec | game/script/script_interpreter.h:323 | ? | name | 1 (script_interpreter.cpp) | likely | reach:1 | 0/1/0 | medium | overlays raw memory / no real record found (sources: void) |
| RecK | include/rec_dtors.h:31 | ? | head, name, s | 1 (gui_edit.cpp) | likely | - | 0/0/1 | low | overlays raw memory / no real record found (sources: void) |
| RenderXform | game/cls_map/progress_bar.cpp:734 | ? | m, f30, f34, f38 | 1 (progress_bar.cpp) | no | - | 0/1/0 | low | overlays raw memory / no real record found (sources: char) |
| elem48 | rgl/mesh2/rec_tables.h:158 | ? | s0, w10, s14 | 1 (rec_tables.cpp) | likely | reach:1 | 1/0/0 | low | overlays raw memory / no real record found (sources: void) |
| frame60 | game/char_sheet/dust.h:98 | ? | xy, type_index | 1 (dust.cpp) | likely | - | 0/1/0 | medium | overlays raw memory / no real record found (sources: char) |
| frustum_view | game/terrain_b/render_batch.h:41 | ? | 1 methods | 1 (render_batch.h) | no | - | 0/0/1 | high | overlays raw memory / no real record found (sources: void) |
| http_result | game/config/options_dlg.h:22 | ? | url, headers, body, body_len | 1 (options_dlg.cpp) | likely | - | 0/1/0 | low | overlays raw memory / no real record found (sources: int) |
| obj_c0 | include/mesh_parts.h:37 | ? | out, in | 1 (entity.cpp) | likely | - | 1/0/0 | low | overlays raw memory / no real record found (sources: void) |
| AgentFrame_s2b5c0 | game/mission5/mission_5.h:1054 | ? | m | 0 | likely | - | - | high | overlays raw memory / no real record found |
| Mark_s29130 | game/mission5/mission_5.h:607 | ? | - | 0 | no | - | - | high | overlays raw memory / no real record found |
| Resource_s27c90 | game/mission5/mission_5.h:358 | ? | - | 0 | no | - | - | high | overlays raw memory / no real record found |
| TroopRec_s257f0 | game/mission5/mission_5.h:119 | ? | - | 0 | no | - | - | high | overlays raw memory / no real record found |
| TroopRec_s26130 | game/mission5/mission_5.h:174 | ? | - | 0 | no | - | - | high | overlays raw memory / no real record found |
| contact_s29ec0 | game/mission5/mission_5.h:796 | ? | pos, point | 0 | no | - | - | high | overlays raw memory / no real record found |
| grid_view | rgl/core/frustum_grid.h:10 | ? | n, lo, step | 0 | likely | - | - | high | overlays raw memory / no real record found |
| mat4_ro | game/renderer.h:46 | ? | 1 methods | 0 | no | - | - | high | overlays raw memory / no real record found |

### D. Std container stand-ins (68)

| view | file:line | overlays | fields named | casts (files) | 64? | path | rows e/n/- | conf | notes |
|---|---|---|---|---|---|---|---|---|---|
| I64Vec | include/i64_vec.h:7 | std::vector<__int64> | _Myfirst, _Mylast, _Myend | 5 (game.cpp, script_interpreter.cpp) | yes | doc | 3/0/2 | high | vector-shaped (_Myend/_Myfirst/_Mylast) |
| conv_rec | rgl/mesh2/gui_edit.h:429 | E68 (include/e68.h:15) | begin, end, cap | 2 (module_load.cpp, world_app.cpp) | yes | reach:1 | 1/0/1 | high | vector-shaped (begin/cap/end) |
| tvec | rgl/mesh2/mesh_2.h:75 | xlen_vec (include/vec_xlen.h:10) | begin, end, cap | 0 | yes | reach:1 | - | high | vector-shaped (begin/cap/end) |
| ResourceOwner_s27c90 | game/mission5/mission_5.h:363 | entity (rgl/scene/scene_entity.h:33) | id, first, last | 2 (mission_5.cpp) | yes | - | 2/0/0 | high | vector-shaped (first/last) |
| Vec1c8 | game/agent/agent.h:99 | std::vector<PlacedObj> | first, last | 2 (agent_logic.cpp) | yes | - | 0/2/0 | high | vector-shaped (first/last) |
| Vec5c | game/inventory_panel/game_tick.h:24 | Rec5c (game/rec5c.h:9) | _Myfirst, _Mylast, _Myend | 1 (game_tick.h) | yes | - | 0/0/1 | high | vector-shaped (_Myend/_Myfirst/_Mylast) |
| vec44 | game/options_panel/vec44.h:7 | RagBone (game/options_panel/ragdoll_models.cpp:31) | begin, end, cap | 1 (ragdoll_models.cpp) | yes | - | 0/0/1 | medium | vector-shaped (begin/cap/end) |
| BigArray | game/char_sheet/facegen_misc.cpp:119 | ? | begin, end, cap | 0 | yes | - | - | low | vector-shaped (begin/cap/end) |
| RawVec | include/raw_vec.h:9 | ? | first, last, end | 0 | yes | - | - | low | vector-shaped (end/first/last) |
| Vec | game/prop_ent.h:17 | ? | first, last, cap | 0 | yes | - | - | medium | vector-shaped (cap/first/last) |
| Vec2c | game/agent/agent.h:98 | ? | first, last | 0 | yes | - | - | medium | vector-shaped (first/last) |
| Vec38 | game/agent/agent.h:97 | ? | first, last | 0 | yes | - | - | medium | vector-shaped (first/last) |
| Vec68 | game/ui/mission.h:89 | ? | begin, end | 0 | yes | - | - | medium | vector-shaped (begin/end) |
| item_modifier | game/options_panel/module_load.h:21 | ? | name, block, last, end | 0 | yes | - | - | low | vector-shaped (end/last) |
| shape_vec | rgl/dynamic/world_grid.h:38 | ? | first, last, end | 0 | yes | - | - | low | vector-shaped (end/first/last) |
| vec3_holder | rgl/mesh2/mesh_2.h:57 | ? | begin, end, cap | 0 | yes | - | - | low | vector-shaped (begin/cap/end) |
| vec_s2da20 | game/mission5/mesh_3.h:219 | ? | first, last, end | 0 | yes | - | - | high | vector-shaped (end/first/last) |
| EntVec | game/agent4/agent_logic.h:71 | Mission (game/mission.h:73) | alloc_pad, first, last, end | 75 (agent_logic.cpp, entity.cpp +19) | no | doc,reach:21 | 28/38/9 | high | vector-shaped (end/first/last) |
| PropVec | game/prop_vec.h:11 | std::vector<PlacedObj> | 3 methods | 43 (agent.cpp, agent_logic.cpp +8) | no | reach:11 | 18/25/0 | medium | vector-shaped (std:: base) |
| xlen_vec | include/vec_xlen.h:10 | vec (rgl/shapes/shapes.h:86) | 2 methods | 9 (dynamic_entity.h, e96_vec.h +6) | no | reach:7 | 8/0/1 | low | vector-shaped (std:: base) |
| vec_cfg_pub | game/inventory_panel/widget_text.cpp:227 | vec_model (game/inventory_panel/widget_text.cpp:40) | - | 3 (widget_text.cpp) | no | reach:3 | 3/0/0 | high | vector-shaped (std:: base) |
| MeshPtrVecPub | game/terrain_b/mission.cpp:81 | MeshVec (game/terrain_b/tmesh.h:10) | - | 2 (mission.cpp) | no | reach:1 | 1/0/1 | high | vector-shaped (std:: base) |
| rawvec | rgl/scene/world_items.cpp:78 | std::vector<entity> | al, first, last, end | 2 (world_items.cpp) | no | reach:2 | 2/0/0 | low | vector-shaped (end/first/last) |
| vec32 | game/char_sheet/dust.h:63 | ? | alloc, first, last, end | 2 (dust.h) | no | - | 0/0/2 | low | vector-shaped (end/first/last) |
| E92VecPub | game/party/game.cpp:941 | TrackRec (game/party/game.h:97) | - | 1 (game.cpp) | no | - | 1/0/0 | high | vector-shaped (std:: base) |
| IntStdVec | game/int_vec_insert.h:9 | IntVec (game/int_vec.h:16) | 1 methods | 1 (int_vec_insert.h) | no | reach:1 | 1/0/0 | medium | **std::vector<int>::_Insert_n made public over IntVec; keep the specialization declaration in int_vec_insert.h (its placement is codegen-relevant, see the header comment), make IntVec a std::vector<int>** |
| IntVecTidy | game/world_data/world_data.cpp:135 | std::vector<int> | - | 1 (world_data.cpp) | no | reach:1 | 1/0/0 | medium | vector-shaped (std:: base) |
| LightVec | game/char_sheet/facegen_init.cpp:33 | std::vector<Light> | 1 methods | 1 (facegen_init.cpp) | no | - | 0/0/1 | medium | vector-shaped (std:: base) |
| MeshPush | game/terrain_b/mission.cpp:510 | std::vector<mesh> | 2 methods | 1 (mission.cpp) | no | reach:1 | 0/1/0 | high | vector-shaped (std:: base) |
| PartyVecTidy | game/world_data/world_data.cpp:134 | std::vector<T> | - | 1 (world_data.cpp) | no | reach:1 | 1/0/0 | medium | vector-shaped (std:: base) |
| SceneObjVec | game/char_sheet/cs_common.h:75 | ? | - | 1 (cs_common.h) | no | - | 0/0/1 | medium | vector-shaped (std:: base) |
| vec96 | game/char_sheet/dust.h:54 | ? | alloc, first, last, end | 1 (dust.h) | no | - | 0/0/1 | low | vector-shaped (end/first/last) |
| BufTriple | rgl/mesh2/gui_edit.h:408 | ? | first, last, end | 0 | likely | - | - | low | vector-shaped (end/first/last) |
| EventVec | game/party2/party_2.h:129 | Party (include/party.h:43) | al, first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| MeshVec_s2f250 | game/mission5/mesh_3.h:491 | MeshPtrStdVec (game/std_vec_insert_n.h:11) | alloc, first, last, end | 0 | likely | reach:1 | - | high | vector-shaped (end/first/last) |
| ObjPtrVecPub | game/terrain_b/mission.cpp:84 | ? | - | 0 | no | - | - | high | vector-shaped (std:: base) |
| PVec | game/script/script_interpreter.cpp:200 | ? | f0, first, last, end | 0 | likely | - | - | low | vector-shaped (end/first/last) |
| V4List | game/ui/map_list.h:14 | ? | alloc, first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| VecPtr3 | game/agent2/agent_ai.h:22 | ? | first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| buy_view | rgl/shapes/shapes.h:113 | ? | 1 methods | 0 | no | - | - | high | vector-shaped (std:: base) |
| contact_vec | rgl/scene/scene_entity.h:95 | Contact (rgl/scene/scene_entity.h:94) | alloc, first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| owned_block | include/owned_block.h:6 | ? | f0, begin, end, cap | 0 | no | - | - | low | vector-shaped (begin/cap/end) |
| str_vec | rgl/mesh2/mesh_2.h:162 | ? | first, last, end | 0 | likely | - | - | low | vector-shaped (end/first/last) |
| tstring_vec_pub | rgl/mesh2/mesh_2.h:198 | ? | - | 0 | no | - | - | high | vector-shaped (std:: base) |
| vec | game/char_sheet/e96_vec.h:21 | ? | al, first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| vec | game/options_panel/ragdoll_models.cpp:13 | ? | al, b, e, cap | 0 | likely | - | - | low | vector-shaped (b/cap/e) |
| vec | rgl/mesh.h:168 | ? | alloc, first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| vec | rgl/shapes/shapes.h:86 | ? | al, first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| vec100 | game/terrain_a/misc.h:43 | ? | _Alval, first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| vec12 | game/agent2/agent_types.h:98 | PolyMesh (game/terrain_b/poly_mesh.h:120) | first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
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
| vecE1c8 | game/agent2/agent_types.h:109 | allocE1c8 (game/agent2/agent_types.h:97) | first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| vecMRow36 | game/terrain_b/records.h:52 | alloc24 (game/terrain_a/misc.h:35) | first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| vec_e1f8 | game/triggers/trigger_vec.h:46 | GameKeys (game/game_keys.h:7) | n, begin, end | 0 | no | - | - | low | vector-shaped (begin/end) |
| vec_e50 | game/triggers/trigger_vec.h:45 | GameKeys (game/game_keys.h:7) | n, begin, end | 0 | no | - | - | low | vector-shaped (begin/end) |
| vec_eb0 | game/triggers/trigger_vec.h:47 | GameKeys (game/game_keys.h:7) | n, begin, end | 0 | no | - | - | low | vector-shaped (begin/end) |
| vec_model | game/inventory_panel/widget_text.cpp:40 | Skin (include/skin.h:45) | alloc, first, last, end | 0 | likely | reach:2 | - | high | vector-shaped (end/first/last) |
| vecint | game/agent2/agent_types.h:108 | alloc_int (game/alloc_construct.h:14) | first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |
| vecstr | game/agent2/agent_types.h:103 | alloc_tstring (game/alloc_construct.h:10) | first, last, end | 0 | no | - | - | low | vector-shaped (end/first/last) |

### E. Fake vtable / slot views (11)

| view | file:line | overlays | fields named | casts (files) | 64? | path | rows e/n/- | conf | notes |
|---|---|---|---|---|---|---|---|---|---|
| text_slot | game/inventory_panel/note_window.cpp:199 | MissionUi (game/ui/mission_ui.h:30) | 6 methods | 20 (note_window.cpp) | no | - | 20/0/0 | medium | virtual-slot view: calls through it stand for the real virtual interface |
| VObj_s25160 | game/mission5/mission_5.h:79 | ? | 46 methods | 12 (mission_5.cpp) | no | - | 12/0/0 | high | virtual-slot view: calls through it stand for the real virtual interface |
| WVt | game/config/widget_mgr.h:26 | UiA (game/ui/ui_widgets.h:14) | 51 methods | 5 (world_data.cpp) | no | reach:5 | 5/0/0 | high | **mostly folded (lane V2, 2026-11-20): cls_map / widget_mgr / widget_node / party2 / obj_tbl / mission_4 call MissionUi's virtuals by name; left: world_data.cpp 0x494110 (5 calls) and widget_mgr::scene (WVt*, slot 50 = ContainerBase::add_child), both waiting on that file. Next: the slot signatures are pointer stand-ins for references / by-value vec2 returns (MeshWidgetVt, text_slot, text_store show const V2& / const int& / text_size by value)** |
| AccessStream | game/inventory_panel/widget_text.cpp:69 | Map10007 (include/map10007.h:6) | 20 methods | 1 (module_files.cpp) | no | reach:1 | 1/0/0 | medium | virtual-slot view: calls through it stand for the real virtual interface |
| Prop_s2f250 | game/mission5/mesh_3.h:476 | entity (rgl/scene/scene_entity.h:33) | 24 methods | 1 (mesh_poly_ray.cpp) | no | - | 0/1/0 | high | virtual-slot view: calls through it stand for the real virtual interface |
| RefRecord | rgl/mesh2/gui_edit.h:120 | tstream3 (rgl/stream.h:204) | 22 methods | 1 (module_files.cpp) | no | reach:1 | 1/0/0 | low | virtual-slot view: calls through it stand for the real virtual interface |
| Src | game/inventory_panel/loading_screen.cpp:14 | tstream3 (rgl/stream.h:204) | 22 methods | 1 (loading_screen.cpp) | no | - | 1/0/0 | low | virtual-slot view: calls through it stand for the real virtual interface |
| SrcVtbl_s261e0 | game/mission5/mission_5.h:305 | ? | 50 methods | 1 (mission_5.cpp) | no | - | 0/1/0 | high | virtual-slot view: calls through it stand for the real virtual interface |
| EntVt | game/world_data/world_data.h:47 | MissionUi (game/ui/mission_ui.h:30) | 25 methods | 0 | no | - | - | high | virtual-slot view: calls through it stand for the real virtual interface |
| ItemD_vt | game/options_panel/mission_item_recs.h:204 | item_base (game/options_panel/mission_item_recs.h:16) | panel, back_alloc, blk0, fwd_alloc +8 | 0 | likely | - | - | high | virtual-slot view: calls through it stand for the real virtual interface |
| ScrollVt | game/world_data/mesh_hud.h:68 | WItem (game/config/widget_mgr.h:116) | 28 methods | 0 | no | - | - | high | virtual-slot view: calls through it stand for the real virtual interface |

### F. Byte-match blocked (2)

| view | file:line | overlays | fields named | casts (files) | 64? | path | rows e/n/- | conf | notes |
|---|---|---|---|---|---|---|---|---|---|
| contact_ops | rgl/dynamic/dynamic_entity.h:37 | contact_list_m (rgl/scene/scene_entity.h:103) | 12 methods | 2 (world_collide.cpp) | no | reach:1 | 0/2/0 | high | **D-shaped (VC7.1 vector<Contact> members written out) and load-bearing: the stock <vector> does not give 0x427450 / 0x427540 / 0x427590; the 64-bit build can use std::vector<Contact> under a non-MSVC branch** |
| ElemVt | game/ui/mission_ui.h:148 | FxElem (include/fx.h:48) | 2 methods | 0 | no | doc | - | high | **declaration-only polymorphic class kept because cl 13.10 swaps two loads in Recf0::frame_update (0x4d63b0) without it; test removing it with run_all on video_page.cpp** |

### Incomplete real types (257 records with pads that are not views)

Name (file:line) pads/pad bytes, by pad count. These are the classes the views above fold into, or records reached
only directly; their pads are the members still to name.

- `game` (38): Agent (agent_obj.h:72) 13/0x2c, Mission (mission.h:73) 10/0x64e, CharSheet (char_sheet_panel.h:22) 7/0x9a, Missile (missile.h:13) 7/0x36, RagBody (rag_body.h:9) 4/0xd, MissionSource (mission_source.h:11) 3/0xc, Rec1208 (conv_screen.h:17) 3/0x2b, AnimBox (ptype.h:15) 2/0x8, AnimDef (agent_obj.h:56) 2/0x20, C4c3050 (screen_records.h:124) 2/0x7, C4c31c0 (screen_records.h:75) 2/0x8, C4c3290 (screen_records.h:100) 2/0x1c, Event (event.h:23) 2/0x8, HitInfo (blow.h:12) 2/0x34, MarkRec (mark_rec.h:13) 2/0xc, PartyScreen (party_screen_class.h:20) 2/0x7, Rec5c (rec5c.h:9) 2/0x44, ResultWindow (result_window.h:17) 2/0xb, SubAnim (agent_obj.h:46) 2/0x18, music_player (music_player.h:21) 2/0x7, node_groups (skeleton.h:22) 2/0x38, CampaignScreen (campaign_screen.h:17) 1/0x10b, EmitDef (emit.h:18) 1/0x8, Font (font.h:40) 1/0x4, GlyphMap (font.h:29) 1/0x4, InputRec (agent_ai.h:19) 1/0x14, MapFlags (map_flags.h:7) 1/0x1, Rec118 (screen_records.h:154) 1/0x8, Rec1bc (screen_records.h:295) 1/0x3, Slot (slot.h:16) 1/0x7, SlotRec (screen_records.h:67) 1/0x1, SpawnRec (mode_rec.h:19) 1/0x4, StrBucket (str_table.h:19) 1/0x4, TerrainPage (terrain_page.h:18) 1/0x28, head_rec (head_rec.h:44) 1/0x4, ini_entry (ini_file.h:11) 1/0xc, junk_tag (junk_tag.h:7) 1/0x4, mesh_inst (renderer.h:34) 1/0x2c
- `game/char_sheet` (17): GuiCtx (gui_ctx.h:17) 11/0x1689, MapScreen (save_game.h:81) 10/0x845d91b, dmesh (dust.h:76) 5/0x19e, scene_row (pick_entity.h:44) 4/0x144, ConvScrollSrc (conv_refresh.cpp:52) 3/0x164, SaveWriter (save_game.h:38) 3/0x2b, TextRow (editor_ui.h:76) 2/0x1f8, cell_rec (dust.h:134) 2/0x28, skin_owner (save_panel.h:37) 2/0x1a8, skin_rec (save_panel.h:28) 2/0xcc, EditorApp (editor_ui.h:64) 1/0xe8, OuterGame (save_records.h:50) 1/0x13a00, SaveCtx (save_game.h:61) 1/0xc, ScrollHost (cs_update.h:63) 1/0x484, UiDsv (cs_update.h:52) 1/0xf0, cPairs96 (save_records.h:35) 1/0x300, skin_data (save_panel.h:33) 1/0x118
- `include` (17): GameData (game_data.h:101) 12/0x64, Party (party.h:43) 7/0x17, ItemKind (item_kind.h:44) 4/0x10, Elem (dialog_rec.h:11) 3/0xc, EnvRec (fx.h:92) 3/0xb, TriggerRec (trigger_rec.h:11) 3/0xc, renderer (renderer.h:16) 3/0xf, OwnedRec48 (rec_dtors.h:40) 2/0x24, SceneEditor (game_data.h:50) 2/0x7, ScriptState (script_state.h:33) 2/0x7, FactionRec (game_recs.h:62) 1/0x3, QuestRec (game_recs.h:17) 1/0x8, RecJ (recj.h:10) 1/0x4, Tail2 (tail2.h:9) 1/0xc, Timer (timer.h:16) 1/0x4, Troop (troop.h:20) 1/0x10, frustum (frustum.h:21) 1/0x2
- `rgl/mesh2` (15): MapDialog (gui_edit.h:284) 5/0x6660, MapRecord (gui_edit.h:251) 5/0x1a8, csv_row (mesh_2.h:170) 5/0x4c, MapEditorDialog (gui_edit.h:344) 4/0x362, graph_face (graph_obj.h:44) 4/0x2b, ConfigDialog (gui_edit.h:320) 3/0x378, edge (graph_obj.h:29) 3/0x17, edit_vert (gui_edit.h:447) 2/0x7, screen_gadget (screen_cfg.h:37) 2/0x20, BlockMat (gui_edit.h:394) 1/0x64, MapObject (gui_edit.h:229) 1/0x10, Rec0xb0Tail (gui_edit.h:414) 1/0x84, RecB (gui_edit.h:155) 1/0xc, item38_vec (gui_edit.h:470) 1/0x8, node (graph_obj.h:21) 1/0x3
- `game/terrain_a` (11): Terrain (terrain.h:150) 7/0x5f0, RenderScene (misc.h:163) 6/0x34, view_t (misc.h:207) 6/0x15a, Prop (misc.h:51) 4/0xcc, light_src (misc.h:137) 4/0x20, FireAgent (misc.h:125) 2/0x58, TerrainParticle (terrain.h:94) 2/0x8, light_t (misc.h:196) 2/0x40, TEntity (terrain.h:87) 1/0xc0, flag_obj (misc.h:203) 1/0x10, fog_rec (misc.h:151) 1/0x7
- `game/options_panel` (19): QuestPage (quest_page.h:7) 4/0x1f0, VideoPage (video_page.cpp:98) 4/0x80, options_panel (options_panel.h:58) 4/0x1c, ValueSource (hud.h:66) 3/0x164, ItemA (mission_item_recs.h:120) 2/0x8, ItemB (mission_item_recs.h:148) 2/0x8, ItemC (mission_item_recs.h:168) 2/0x8, KeysPage (keys_page.cpp:137) 2/0x3e4, E50 (elem_calls.h:8) 1/0x50, Elem50 (world_app.cpp:577) 1/0x50, Elem68 (world_app.cpp:578) 1/0x68, ElemB0 (world_app.cpp:579) 1/0xb0, RagBone (ragdoll_models.cpp:31) 1/0x4, TreeW (world_app.cpp:264) 1/0x8, ground_spec (module_load.h:40) 1/0x3c, list_node (world_app.cpp:142) 1/0x4, rec50 (mission_items.cpp:31) 1/0x50, rec68 (mission_items.cpp:32) 1/0x68, recb0 (mission_items.cpp:33) 1/0xb0
- `game/party` (5): Game (game.h:149) 20/0x285, NavNode (nav_graph.h:10) 2/0x50, PartyWindow (party_window_class.h:39) 2/0x6, TrackRec (game.h:97) 2/0x44, GameTimer (game.h:49) 1/0x4
- `game/mesh_slot` (10): MissileU (mesh_scene.cpp:251) 8/0x2b, PropRecF (mesh_game.cpp:365) 7/0x94, SndSetF (mesh_game.cpp:385) 2/0x50, SndSlot (mesh_start.cpp:96) 2/0x74, GeomPt (mesh_pass.cpp:228) 1/0x10, PolyGeom (mesh_pass.cpp:257) 1/0x14, SndBank (mesh_start.cpp:100) 1/0x4, SndSlotS (mesh_game.cpp:40) 1/0x78, TimeKey (slot.cpp:36) 1/0x44, vec_i4 (mesh_terrain.cpp:673) 1/0x4
- `game/mission5` (13): StmtRec (game_2.h:61) 4/0x84, MapNav (game_2.h:145) 3/0x14, Rec64 (game_2.h:94) 3/0x3c, MapEdge (game_2.h:138) 2/0x1c, MapNavVert (game_2.h:125) 2/0x8, MapStore (game_2.cpp:82) 2/0x3c, ground_g136 (game_2.cpp:63) 2/0x58, FactionFile (data_files.h:102) 1/0x3, MapNode (game_2.h:132) 1/0x60, TriggerStmt (game_2.h:72) 1/0x23, V0 (game_2.h:92) 1/0xc, V1 (game_2.h:93) 1/0x20, ground_v36 (game_2.cpp:62) 1/0x14
- `game/inventory_panel` (11): MissionCtx (mission_ctx_rec.h:7) 4/0x150, TutorialMenuRec (char_sheet.cpp:91) 4/0x4c, PartyCellRec (map_tooltip.cpp:33) 3/0x9c, BigRec (map_panel.cpp:79) 2/0x84, GroundItem (map_panel.cpp:87) 2/0x48, PartyInfo (map_tooltip.cpp:27) 2/0x54, note_window (note_window.cpp:215) 2/0x28, FleetRec (map_panel.cpp:664) 1/0x4c, PartyTplRec (module_files.cpp:89) 1/0x80, row138 (widget_text.cpp:167) 1/0x40, sel_list (widget_text.cpp:97) 1/0x16f8
- `game/script` (16): SceneObj (script_interpreter.cpp:2539) 4/0x2c4, SubOrder (script_interpreter.cpp:699) 3/0x24, Leader (script_interpreter.cpp:712) 2/0x24, SiteRec (script_interpreter.cpp:2557) 2/0x1cc, EntryPoint (script_interpreter.h:335) 1/0xf0, MTimer (script_interpreter.h:346) 1/0x4, PickArg (script_interpreter.cpp:2556) 1/0x400, Presentation (script_interpreter.h:210) 1/0x28, RefSlot (script_interpreter.cpp:536) 1/0x2c, Reinf (script_interpreter.cpp:555) 1/0x44, Shape (script_interpreter.cpp:539) 1/0x18, Sprite (script_interpreter.cpp:535) 1/0x64, TabCurrent (script_interpreter.cpp:1508) 1/0x36c, TableauEntityV (script_interpreter.cpp:1721) 1/0x54, TableauOverrides (script_interpreter.h:182) 1/0x344, World (script_interpreter.cpp:2587) 1/0x344
- `game/config` (7): InputSet (config.h:55) 6/0x72c, GameApp (core.h:18) 4/0x1b, OptionsDlg (options_dlg.h:39) 3/0x378, widget_mgr (widget_mgr.h:126) 2/0xc, Config (config_block.h:13) 1/0x4, CreateGameOptionsPage (config.h:42) 1/0x374, psys_record (config.h:101) 1/0x9c
- `game/cls_map` (8): Shape (progress_bar.h:68) 4/0x54, MissionScreen (cls_map.h:124) 3/0x128, ShapeFace (progress_bar.h:58) 3/0x3f, ShapeSeg (progress_bar.h:50) 3/0x29, NameRec48 (cls_map.cpp:542) 2/0x1c, E20b (cls_map.h:99) 1/0xc, ShapePt (progress_bar.h:44) 1/0x3, TearObj (cls_map.h:78) 1/0x78
- `game/ui` (9): SelRec (mission.h:64) 5/0x3c, MissionUi (mission_ui.h:30) 3/0x8, NamedRec (named_rec.h:12) 2/0x14, NamedSelRec (world.h:23) 1/0x1a7, NodeA (container.h:45) 1/0x7, SortVal (item_ref.h:39) 1/0x4, UiA (ui_widgets.h:14) 1/0x2, UiC (ui_widgets.h:58) 1/0x3, snd_slot (item_ref.h:19) 1/?
- `game/agent4` (6): AgentFx (agent_logic.h:146) 3/0xc4, Obj3Place_ (agent_logic.cpp:233) 3/0x134, SubV (agent_logic.cpp:11672) 3/0x2c, VoiceRec (agent_logic.h:43) 2/0x1f4, AnimInfo (agent_logic.cpp:9568) 1/0x3c, SceneObjV (agent_logic.cpp:11196) 1/0x88
- `game/world_data` (9): CarryKind (world_data.h:197) 3/0x22f, EmitterCtl (world_data.h:237) 2/0x59f0, CarryMesh (world_data.h:205) 1/0x8c, I64VecOp (world_data.h:123) 1/0x4, IntVecW (world_data.h:131) 1/0x4, ResSlot (world_data.h:157) 1/0x5c, SceneObject (world_data.h:252) 1/0x14, ScrollHolder (mesh_hud.h:78) 1/0x17c, TableauEntity (obj_tbl.h:24) 1/0x54
- `game/terrain_b` (5): light (render_batch.h:89) 4/0x38, SceneFile (scene_file.h:38) 3/0x83, MEdge (poly_mesh.h:35) 2/0xb, MPoly (poly_mesh.h:57) 1/0x3, MVert (poly_mesh.h:27) 1/0x4
- `game/mission_ui2` (7): ForceLabel (mission_3.h:55) 3/0x11c, ArrElem (mission_3.h:38) 2/0x1c8, ListHolder (mission_3.h:23) 2/0x80, ArrHolder (mission_3.h:46) 1/0x4, ArrInner (mission_3.h:34) 1/0xb0, FieldGroup (mission_3.h:53) 1/0x68, TabCurrent (mission_4.h:20) 1/0x36c
- `rgl/xmlcfg` (4): ActorItemRec (game_util.h:25) 3/0x3c, Obj3 (game_util.h:85) 3/0x134, RecF (records.h:37) 2/0x78, RecP (records.h:53) 1/0x4
- `game/strat` (1): gui_manager (gui_manager.h:54) 7/0x1b
- `rgl` (2): scene (scene_rec.h:41) 4/0x7, mesh_pool (psys.h:17) 2/0x2fc
- `game/triggers` (3): ItemRec (item_rec.h:27) 2/0x8, ItemRec2 (item_rec.h:54) 2/0x8, MouseRec (trigger_vec.h:110) 1/0x2c
- `rgl/dynamic` (2): ground_g136 (world_grid.h:66) 3/0x30, shape_list (world_grid.h:48) 1/0x4
- `rgl/core` (3): buf3 (frustum_grid.h:20) 2/0xcc, CapDef (bone.h:12) 1/0x8, skeleton (resource_io.h:36) 1/0x4
- `game/party2` (4): DetailVert (party_2.cpp:2374) 1/0x14, ScriptStack (camera.h:32) 1/0x4, TabWorld (party_2.h:117) 1/0x20, event_rec (party_2.cpp:2119) 1/0x40
- `game/agent5` (4): EditorBrushRec (editor_dialog.cpp:111) 1/0x14, EditorMeshObj (editor_dialog.cpp:93) 1/0x118, MenuLayout (agent_hud.h:134) 1/0x1, MeshFlag (editor_dialog.cpp:92) 1/0x4c
- `rgl/scene` (2): strategic_entity (scene_entity.h:141) 2/0x2c, lod_mesh2 (scene.h:29) 1/0x34
- `game/agent` (3): Agent_f8 (agent.h:67) 1/0xf8, KeyHolder (agent.h:90) 1/0x28, Rec20 (agent.h:25) 1/0x10
- `game/agent2` (3): Rec1f8 (agent_ai.h:10) 1/0x1f0, Table1f8 (agent_ai.h:11) 1/0x18, V32 (agent_types.h:67) 1/0x14
- `game/agent3` (2): Poser (agent_fx_sound.h:46) 1/0x30, Psys (agent_fx_sound.h:64) 1/0xcc
- `game/map_editor` (1): TargetW (exchange_screens.h:99) 1/0x30c

<!-- end generated -->

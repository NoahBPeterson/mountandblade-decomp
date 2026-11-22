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
| A | 22 | 0 | 7 | 78 |
| B | 0 | 0 | 0 | 0 |
| C | 0 | 0 | 0 | 0 |
| D | 3 | 0 | 0 | 0 |
| E | 1 | 0 | 0 | 0 |
| F | 1 | 0 | 1 | 0 |
| all | 27 | 0 | 8 | 78 |

### A. Fold into the real type (22)

| view | file:line | overlays | fields named | casts (files) | 64? | path | rows e/n/- | conf | notes |
|---|---|---|---|---|---|---|---|---|---|
| scene_ms2 | game/mesh_slot/mesh_slot.h:38 | scene (rgl/scene_rec.h:41) | - | 54 (mesh_char.cpp, mesh_pass.cpp) | no | - | 18/36/0 | high | fold into scene (pads cover the fields) |
| EV | game/options_panel/mesh_edit.cpp:131 | std::vector<MEdge> | v | 6 (mesh_edit.cpp) | no | - | 6/0/0 | medium | fold into std::vector<MEdge> (pads cover the fields) |
| sel_list | game/inventory_panel/widget_text.cpp:81 | std::vector | v | 4 (note_window.cpp) | likely | doc | 0/4/0 | medium | fold into std::vector (pads cover the fields) |
| EquipSet | game/mesh_slot/mesh_appear.cpp:32 | ItemRef (game/item_ref.h:14) | weapon, helmet, body, boots | 3 (agent_hud.cpp, mesh_game.cpp +1) | no | reach:1 | 1/1/1 | high | fold into ItemRef (real type has no offsets there: check) |
| vec4n | game/agent4/agent_logic.cpp:7229 | vector4 (rgl/rgl_math.h:15) | 2 methods | 3 (agent_logic.cpp) | no | reach:3 | 0/3/0 | medium | method twin: declare the 2 method(s) on vector4 |
| Profile | game/char_sheet/cs_update.h:54 | ? | troop | 2 (cs_update.cpp) | likely | - | 2/0/0 | medium | a screen in g_windows.arr[n] (slot_as<T>(n)): fold into that slot's screen class (head_rec family, game/screen_records.h) |
| Blk2dd0 | game/options_panel/module_load.h:29 | EnvRow (game/env_row.h:13) | 1 methods | 1 (module_load.cpp) | no | - | 1/0/0 | medium | method twin: declare the 1 method(s) on EnvRow |
| Team | game/inventory_panel/mission_ctx.h:39 | TeamRec (game/mission.h:59) | f0, leader, cls, relation | 1 (mission_ctx.cpp) | no | - | 1/0/0 | medium | fold into TeamRec (pads cover the fields) |
| TroopIO | game/party/game.h:75 | Troop (include/troop.h:20) | 1 methods | 1 (game.cpp) | no | - | 1/0/0 | high | method twin: declare the 1 method(s) on Troop |
| TroopX | game/mission_ui2/stat_panel_x.cpp:38 | Troop (include/troop.h:20) | 1 methods | 1 (stat_panel_x.cpp) | no | reach:1 | 1/0/0 | high | method twin: declare the 1 method(s) on Troop |
| key_clip | rgl/resource.h:159 | named_object (rgl/resource.h:19) | tracks, extra | 1 (resmgr.cpp) | likely | reach:1 | 0/1/0 | low | fold into named_object (real type has no offsets there: check) |
| mesh_anim_x | game/mesh_slot/mesh_scene.cpp:430 | mesh (rgl/mesh.h:48) | 1 methods | 1 (mesh_scene.cpp) | no | reach:1 | 1/0/0 | high | method twin: declare the 1 method(s) on mesh |
| BlowInit | game/blow.h:39 | Agent (game/agent_obj.h:52) | 2 methods | 0 | no | doc | - | high | method twin: declare the 2 method(s) on Agent |
| MissionV | game/inventory_panel/mission_ctx.h:14 | Mission (game/mission.h:78) | 2 methods | 0 | no | - | - | high | method twin: declare the 2 method(s) on Mission |
| PartyV | game/inventory_panel/mission_ctx.cpp:121 | Party (include/party.h:51) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on Party |
| PartyX | game/party_x.h:11 | Party (include/party.h:51) | 16 methods | 0 | no | - | - | high | method twin: declare the 16 method(s) on Party |
| V2_s29130 | game/mission5/mission_5.cpp:1551 | V2 (include/vec.h:35) | 3 methods | 0 | no | - | - | high | method twin: declare the 3 method(s) on V2 |
| V2r | game/config/widget_mgr.cpp:236 | V2 (include/vec.h:35) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on V2 |
| mat3_value | rgl/identity_value.h:8 | mat3 (rgl/rgl_math.h:62) | - | 0 | no | - | - | high | fold into mat3 (pads cover the fields) |
| mat4_ro | game/renderer.h:34 | rgl::mat3 (rgl/rgl_math.h:62) | 1 methods | 0 | no | - | - | high | fold into rgl::mat3 (pads cover the fields) |
| mesh_calls | game/terrain_b/render_batch.h:33 | mesh (rgl/mesh.h:48) | 2 methods | 0 | no | - | - | high | method twin: declare the 2 method(s) on mesh |
| vec2c | game/strat/strat_ext.h:33 | vec2 (include/vec.h:13) | 1 methods | 0 | no | - | - | high | method twin: declare the 1 method(s) on vec2 |

### B. Real type exists but conflicts (0)

| view | file:line | overlays | fields named | casts (files) | 64? | path | rows e/n/- | conf | notes |
|---|---|---|---|---|---|---|---|---|---|

### C. Real type not yet defined (0)

| view | file:line | overlays | fields named | casts (files) | 64? | path | rows e/n/- | conf | notes |
|---|---|---|---|---|---|---|---|---|---|

### D. Std container stand-ins (3)

| view | file:line | overlays | fields named | casts (files) | 64? | path | rows e/n/- | conf | notes |
|---|---|---|---|---|---|---|---|---|---|
| vec_e1f8 | game/triggers/trigger_vec.h:46 | GameKeys (game/game_keys.h:7) | n, begin, end | 0 | no | - | - | low | vector-shaped (begin/end) |
| vec_e50 | game/triggers/trigger_vec.h:45 | GameKeys (game/game_keys.h:7) | n, begin, end | 0 | no | - | - | low | vector-shaped (begin/end) |
| vec_eb0 | game/triggers/trigger_vec.h:47 | GameKeys (game/game_keys.h:7) | n, begin, end | 0 | no | - | - | low | vector-shaped (begin/end) |

### E. Fake vtable / slot views (1)

| view | file:line | overlays | fields named | casts (files) | 64? | path | rows e/n/- | conf | notes |
|---|---|---|---|---|---|---|---|---|---|
| ItemD_vt | game/options_panel/mission_item_recs.h:177 | item_base (game/options_panel/mission_item_recs.h:17) | panel, back_pages, fwd_pages, back_sel_a +6 | 0 | likely | - | - | high | virtual-slot view: calls through it stand for the real virtual interface |

### F. Byte-match blocked (1)

| view | file:line | overlays | fields named | casts (files) | 64? | path | rows e/n/- | conf | notes |
|---|---|---|---|---|---|---|---|---|---|
| ElemVt | game/ui/mission_ui.h:150 | FxElem (include/fx.h:36) | 2 methods | 0 | no | doc | - | high | **declaration-only polymorphic class kept because cl 13.10 swaps two loads in Recf0::frame_update (0x4d63b0) without it; test removing it with run_all on video_page.cpp** |

### Incomplete real types (55 records with pads that are not views)

Name (file:line) pads/pad bytes, by pad count. These are the classes the views above fold into, or records reached
only directly; their pads are the members still to name.

- `game` (24): Missile (missile.h:16) 4/0xe, Agent (agent_obj.h:52) 3/0xc, Mission (mission.h:78) 3/0x638, Rec1208 (conv_screen.h:18) 3/0xf, C4c31c0 (screen_records.h:74) 2/0x8, C4c3290 (screen_records.h:99) 2/0x1c, Event (event.h:23) 2/0x8, HitInfo (blow.h:12) 2/0x30, ResultWindow (result_window.h:17) 2/0xb, C4c3050 (screen_records.h:123) 1/0x4, CampaignScreen (campaign_screen.h:17) 1/0x10b, EmitDef (emit.h:19) 1/0x8, Font (font.h:16) 1/0x4, InputRec (agent_ai.h:19) 1/0x14, MapFlags (map_flags.h:7) 1/0x1, PartyScreen (party_screen_class.h:20) 1/0x4, Rec118 (screen_records.h:152) 1/0x8, Rec1bc (screen_records.h:302) 1/0x3, Slot (slot.h:16) 1/0x7, SlotRec (screen_records.h:66) 1/0x1, SpawnRec (mode_rec.h:19) 1/0x4, TerrainPage (terrain_page.h:19) 1/0x28, ini_entry (ini_file.h:11) 1/0xc, junk_tag (junk_tag.h:7) 1/0x4
- `include` (7): Elem (dialog_rec.h:11) 3/0xc, GameData (game_data.h:103) 1/0x40, ScriptState (script_state.h:28) 1/0x4, Tail2 (tail2.h:9) 1/0xc, Timer (timer.h:16) 1/0x4, frustum (frustum.h:21) 1/0x2, renderer (renderer.h:16) 1/0xc
- `game/config` (2): InputSet (config.h:46) 6/0x72c, Config (config_block.h:13) 1/0x4
- `game/inventory_panel` (2): MissionCtx (mission_ctx_rec.h:7) 4/0x150, BigRec (map_panel.cpp:79) 2/0x84
- `game/ui` (5): MissionUi (mission_ui.h:31) 1/0x5, NodeA (container.h:45) 1/0x7, PairStack (mission_ui.h:155) 1/0x4, UiA (ui_widgets.h:14) 1/0x2, UiC (ui_widgets.h:58) 1/0x3
- `game/script` (3): SiteRec (script_interpreter.cpp:2307) 2/0x1cc, EntryPoint (script_interpreter.h:231) 1/0xf0, PickArg (script_interpreter.cpp:2306) 1/0x400
- `game/terrain_b` (2): SceneFile (scene_file.h:39) 2/0x80, MEdge (poly_mesh.h:36) 1/0x8
- `game/terrain_a` (2): TerrainParticle (terrain.h:79) 2/0x8, Terrain (terrain.h:129) 1/0x5dd
- `game/char_sheet` (2): cell_rec (dust.h:62) 2/0x28, SaveCtx (save_game.h:51) 1/0xc
- `game/triggers` (3): ItemRec (item_rec.h:27) 1/0x4, ItemRec2 (item_rec.h:49) 1/0x4, TimedVal (trigger_vec.cpp:1187) 1/0x4
- `game/agent` (1): Rec20 (agent.h:25) 1/0x10
- `game/agent2` (1): V32 (agent_types.h:61) 1/0x14
- `rgl/scene` (1): strategic_entity (scene_entity.h:135) 1/0x28

<!-- end generated -->

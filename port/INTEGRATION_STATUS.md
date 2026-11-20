# Native port integration status (macOS arm64)

Lane: integration (docs/port_lanes.md). Updated 2026-11-20 (eleventh session, lane 11).

## Where it gets to

- `build/port/macos-arm64/bin/mb` (target `mb_native`) **links with nothing trapped**.
- Runs from `game/`: device, mb.fx, fonts, loading screen, all of load_module_files, then **the main menu renders correctly**
  (work/port_shots/asan_asan_menu.png, menu_menu.png: background, logo, knight, the seven entries, hover highlight).
- **Input works**: run_native.py clicks reach the menu. "Options" (`--at 14 click 0.4 0.775`) opens the options screen
  (work/port_shots/opt_opt.png): **now correct** (sliders at their values, "Damage to friends" label, battle size "51" beside
  its slider). The drop-down values "Reduced to 1/4", "Poor", "Slower" are the original's defaults (g_options' image data).
- Sound: **plays** (MBA_LOG=1 logs FSOUND_Init "device open", ~350 samples loaded, the title music stream and the click
  sounds; the 9 load failures are files missing from the install). Not checked by ear.
- "Start a New Game" (`--at 10 click 0.5 0.536`) runs the new-game sequence and opens character creation ~50 s after the
  click (Welcome / Male / Female / Go back). **One click advances one menu** (the earlier "one click runs through every menu"
  did not reproduce: the click at 0.334 0.669 hits the second option of each menu; every frame_update emits once).
  Clicking 0.334 0.669 every 5 s from 52 s runs gender, father, childhood, adulthood, the reason to adventure and "Become an
  adventurer" (cc_c6.png). The add_gold crash on the way (SoundMgr::play with garbage variation indices) is fixed.
- After "Become an adventurer" the game menu returns to the **character sheet** (cs_f.png: name box, attributes, skills
  with their levels, proficiencies, points), first with the saving-policy question (both buttons), then the tutorial box
  with its text. Recipe (times from launch): `--at 10 click 0.5 0.536`, then `click 0.334 0.669` at 52, 57, 62, 67, 72,
  77 (gender, father, childhood, adulthood, reason, begin), policy `click 0.494 0.642` at 88, tutorial OK
  `click 0.483 0.638`, name box `click 0.5 0.05`, keys (`key 0` = A), Done `click 0.917 0.952`. Run with MB_CRASH_BT=1.
- Fifth session: the tutorial box closes (WindowMgrW::apply called dev_item's slots 1/8: the closing screen's deleting
  destructor), the name is kept (UiE name box), skill clicks set the right rank (Troop::nib), Done enables once the points
  are spent (original rule: attr/skill points 0, weapon points <= 5, a name) and opens the face generator.
  Recipe after the tutorial OK (plain build, times from launch): name `click 0.5 0.05` at 102, `key 0/1/2` at 104-106,
  STR + `click 0.224 0.690` x5 from 110, skills `click 0.644 0.363` x7 and `click 0.644 0.339` x3, weapon
  `click 0.946 0.483` x10 (one per second), Done `click 0.917 0.952` at ~138 (t5_face2.png: the sheet with "asd").
  **ASan recipe** (macos-arm64-asan, same steps, slower start): New Game at 40, the six menu clicks at 105..145 (every
  8 s), policy 165, tutorial OK 180, name 185, keys 187-189, points from 192, Done 222. The plain build dies silently at
  Done (libmalloc abort_with_reason bypasses signal handlers); use ASan. `dsymutil build/port/macos-arm64-asan/bin/mb -o
  <dir>/mb.dSYM` + `llvm-symbolizer --obj=<dir>/mb.dSYM/Contents/Resources/DWARF/mb 0x<pc>` gives the source line.
- Sixth session: **the face generator renders** (work/port_shots/l6b_fg2.png: the head with hair, the sliders, Skin /
  Beard / Hair, Age, Hair Color, Randomize / Reset / Done, its tutorial box), the tutorial OK closes it, **Done** leaves it
  and the game menu shows the intro text ("You arrive at Calradia ... Continue...", l6c_map3.png).
  ASan recipe after the sheet's Done at 221: face-gen tutorial OK `click 0.482 0.637` at 240, Done `click 0.4376 0.901`
  at 252, intro Continue `click 0.333 0.67` at 275.
- Seventh session: **g_mission is a real `new Mission`** (Mission::Mission 0x4d3880, Mission::~Mission 0x4d51d0, both
  byte-exact): AgentTable is the Pool<Agent> (Pool has its constructor; PartyMgr builds its pool unzeroed), poolJ the
  Pool<RecJ>, MarkRec has the constructor 0x4c2d10 (was C4c2d10), TerrainDesc is the 0x14c-byte description with the
  constructor / destructor 0x4be6b0 / 0x4be6f0 (was clip_table; the mission fields after it are Mission's own), PolyMesh
  has 0x4d1360 / 0x4cb780 (was growable_array / graph_obj::clear_vecs), the timers are clock2_timers. Continue now runs
  MissionV::init / init_scene (raw MI offsets gone). That mission start was itself a 64-bit bug: CampaignScreen::frame_update
  read GameData::start_map_conversation at a raw 32-bit offset. With the map path's raw offsets gone (frame_update,
  GameG::tick's trigger lists, camera_mark_entities' party flags, init_screen's camera target) Continue opens the map.
- The map's ground (rgl::world::ground, the NavData every party move uses) was overwritten by ShadowMap::setup through its
  LightCam view of the world (found with an ARM hardware watchpoint: thread_set_state ARM_DEBUG_STATE64 on the main thread,
  SIGTRAP handler; a debugging trick worth keeping for the next "who wrote this" bug). ShadowMap::cam is the rgl::world now
  and every user reads world->view (ViewParams). MapPanel is CampaignScreen (its MapView vtable view called the wrong
  slot), the screen ray is MapWidget::screen_to_world (was the camera_ray view).
- **The world map renders** (work/port_shots/l7j_c3.png: terrain, trees, the compass, "Paused", the map bar Camp / Reports
  / Quests / Inventory / Character / Party, the date). ASan recipe: the intro Continue at 275, the map is up by ~300.
- Teardown: GameData::world_free (0x492f40) destroys GameData's members by name (byte-exact); FinalCleanup deletes
  g_mission through ~Mission. PartyMgr::clear freed its pages through a hand-spelled 4-byte array cookie (ASan bad-free
  at exit); it is `delete[]` now (still MATCH).

- Eighth session: **the map is alive** (work/port_shots/l8d_m0.png: the party icon on its horse with its count, towns,
  castles and villages with their labels, a looter party, the camera at its normal height). The map tick never ran:
  GameG::tick (inventory_panel/game_tick.cpp) and the track code (party_tracks.cpp) read GameData through
  `#define x FLD(T, 0x...)` raw 32-bit offsets, so `f94` (stop) read garbage and the tick returned every frame (no party
  ticks, no time). They are GameData's members now (f94, rest_period, f14058 the time speed, timer1a298, game_clock, the
  fog1/fog2/fog3 colours, wind, ents, map_obj, map_track, track_entity); the event pool cursor is EventPool::next / first
  (game/event_pool.h). Party::model is the real rgl::strategic_entity (the PartyModel vtable/pad view put `visible` in a
  pad and called slots off by one); the click's plane hit is face::intersect (PlaneTrace view: +0x20 / +0xa0 where face has
  w / normal at +0x28 / +0xa8 at 64 bits). With those, a terrain click picks the ground and issues the move order
  (MAPDBG trace: best face found, shares = 1, action 6), time runs (speed 8.6), and the path search ran.

- Ninth session: **save and load work.** Restore a Saved Game -> slot -> Restore loads the save onto the map
  (work/port_shots/l9b_a.png: the party at the Training Field, the date, the map bar); Esc -> Save & Exit writes the save
  (same size as before; reloading the re-saved file works, l9e_a.png). The note block is one `SoaBlock notes` member
  (include/note_block.h: times[8], texts[8], tableau) in Troop, FactionRec, QuestRec, Party and mission5/data_files.h's
  file records; read_soa / write_soa take `&notes`. Game::scenes is a SceneFile* (load_parties read the slots at
  32-bit offsets); scene_file.h no longer includes rgl/stream_inline.h (its includers do), so party/game.cpp can include it.
  Recipe (ASan, sandboxed saves): `MB_USER_DIR=<dir>` (port/platform/fs.cpp: Documents and AppData under <dir>; copy
  ~/Documents/Mount&Blade Savegames and ~/Library/Application Support/Mount&Blade there, the user's saves stay untouched),
  Restore `click 0.54 0.615` at 45, slot 1 `click 0.18 0.18` at 57, Restore `click 0.78 0.93` at 60, the map is up by
  ~90; Esc `key 53`, Save & Exit `click 0.30 0.575`.

- Tenth session: **quitting exits cleanly** ("Exit to Windows" from the main menu: exit code 0 under ASan, FinalCleanup and
  every static destructor run). Fixed on the way: RecD / RecI folded into TriggerRec (include/trigger_rec.h) / menu_item;
  ~MissionTemplate / ~MenuRec + `delete[]` (cl emits ??_E 0x4c3430 / 0x4c4f40, MATCH); trigger_entry's clock is a Timer
  (GameVec::stamp wrote it at 32-bit offsets into the block text); Terrain::reset and PanelMesh::build_panel on members
  (they wrote 1.0f into the layer names and 1 into the gui meshes' kids vectors); resmgr::~resmgr deletes each block
  through its own class (MSVC's virtual `delete[]` through named_object* traps under clang); the native link runs the
  static constructors / destructors in the original's TU order (tools/port/tu_order.py -> port/link/tu_order.txt).
  Debugging aid: `MB_WATCH=<global>+<hex off>` (port/main.cpp) sets a hardware write watchpoint and prints the writer's
  backtrace (found both stray writes in one run each); MB_CRASH_BT also catches SIGTRAP.

- Eleventh session (lane 11): **Quick Battle -> Skirmish 1 -> Start gets through the scene setup** (scene record loaded,
  the .sco props spawned with their meshes / bodies / triggers, the terrain generated and tessellated, the nav mesh
  built, the sun / precip / cursor entities made, the prop triggers run) **into the agent spawns** (Mission::run_spawns ->
  spawn_one -> spawn_full -> Mission::spawn). The empty `get_object failed for:` was rebuild_nav reading the scene's
  body name through game_data.h's Level494 view (32-bit pads): GameData::scenes is a SceneFile* now. Then, in order:
  MissionObj is the one prop kind (Rec78 / PropKind views gone: g_propkinds strided wrong), Terrain::tessellate fills
  NavData / NavCell (geom_elem's int at +0x84 was the face pointer; the layer walk read raw +0x258), start_setup's
  entities are rgl::entity (the Entity view's plain slot 0 called the wrong virtuals), the trigger entity of
  Operation::execute is rgl::strategic_entity, GameW's event-context accessors read GameData::event_ctx, AnimBox
  (ptype's frame) has shape_entry's members (vector of meshes, body), spawn_bodies / gen_set_init walk the
  TerrainParticle records, PlacedObj::ent / ent2 are rgl::entity*. MB_CRASH_BT also logs a message box's text and
  caller (port/platform/window.cpp: the RGL ERROR boxes).

## Next leads

- Exit teardown left: g_party_templates (conv_item_array_dtor 0x4c8980, PartyTemplate) is still a hand-written `??_E`
  reached through aliases.ld with a mismatched ABI (cdecl/2 vs fastcall/3): it takes the scalar path and leaks. Give
  PartyTemplate its destructor (`union { I64Vec list; SlotArray slots; }` becomes a SlotVec) and `delete[]` it, as
  g_propkinds is now (MissionObj: ??_EMissionObj 0x4c7ee0 / ??1MissionObj 0x4c65d0 are the compiler's, MATCH).
- Save diff (lane 9's 134 bytes): a load -> Save & Exit of sg00 differs from the loaded file in 136 bytes, same size, no shift.
  Most are float last-bit differences (e.g. 0x1c66c f055 -> ec55, 0x2ea16 facb -> f9cb: arm64 float arithmetic vs the
  original's x87 on the frames run before the save), the header's 0x58..0x63 (camera / map position) and a few small
  counters (0xc5527 2 -> 5, 0xc60c7 0x173 -> 0): state, not a writer bug. Loading it back in the original is still untested.
- **Current blocker (battle start)**: Mission::spawn (mesh_slot/mesh_game.cpp 0x4832b0) faults in FxDef::init. The
  spawner sees the agent record as AgentS (a third 0x5928-byte view beside Agent and MAgent); at 64 bits the three
  disagree from +0x258 on (view_layout.py cmp terrain_b/poly_mesh.cpp:MAgent agent/agent.cpp:Agent: 247 members, 8 bytes
  apart: MAgent 0x5a08, Agent 0x5a10), so the pool's MAgent pages and the Agent / AgentS readers stride differently.
  The fold (Agent the one class; MAgent's constructor 0x477a30 / reset 0x472f90 as Agent members; AgentS / EntityG
  views gone; PoolS -> Pool<Agent>) is the next step and blocks everything after the scene setup.
- Views met on the way, still open: ptype vs mesh_2.h's ObjectList / shape_entry (AnimBox now has the same members,
  but two classes remain; module_load reads through `(ObjectList*)&g_ptypes[i]`), TimerList vs GameVec (one class
  {count, 0x30-byte trigger_entry/TimerChild, spare}; MissionObj::read and MapIcon::read cast), PolyGeom (mesh_pass.cpp)
  vs NavData (NavCell is an incomplete GeomFace), EntVec / Mission::v63c0 (the prop entity list is a vector<int> holding
  entity pointers; EntVec::insert_n goes through vector<int>::_Insert_n: truncates at 64 bits), rgl::world::
  collect_boxed's std::vector<unsigned> of entity pointers (agent_logic.cpp 0x442180), misc.h's Prop / prop_ent.h's
  PropEnt / mission_3's ent_lods (PlacedObj views), script_interpreter's ArrHolder / Obj3 (FxDef / psys views).
- Quick Battle recipe (ASan): main menu `click 0.45 0.695` at 50, Skirmish 1 `click 0.334 0.669` at 80 (the scenario text
  with Start / Cancel), Start `click 0.334 0.669` at 110.
- **Towns and battles are missions**: the agent body is rgl::strategic_entity seen through AgentBody (agent_obj.h: id,
  frame +0x58, skel at fx +0x304), AgentBodyV / SkelBodyV / SunBody (vtable views with slot 0 a plain virtual), EntityG /
  AgentS (mesh_game.cpp: the spawner's 32-bit views of the entity and the agent, with `operator new(0x7c)` / `0x64` raw
  sizes for FxDef / capsule), SceneEnt, TEntity, TableauEntity(V). One pass folding them into strategic_entity / Agent is
  the next big step (Mission::spawn 0x4832b0 is the first function a town or battle runs). The agent record itself is two
  classes: the pool allocates MAgent (terrain_b/agent_rec.h, an IfaceB with a real vptr: 8 bytes at 64 bits) while
  agent_obj.h's Agent started with `char pad0[4]` for the vptr. Lane 10: Agent is an IfaceB now (used / id are in_use /
  pool_id), slot_vars a SlotVec, MAgent's Timed / ClockStamp Timers, AppearanceLods shared: `view_layout.py cmp
  src/game/terrain_b/poly_mesh.cpp:MAgent src/game/agent/agent.cpp:Agent` agrees up to +0x250; 303 members still apart from
  MAgent's ViewState view at +0x258 on (Agent's body / goal / path_face ...). Fold MAgent into Agent (one class, the
  constructor 0x477a30 and reset 0x472f90 as Agent members) before anything else on the mission path. Quick Battle (main menu
  0.45 0.695) reaches a mission without character creation: the fastest test.
- Other ODR copies with user-written copy constructors (the hnode / OpenItem pattern: a type trivial in one unit and
  non-trivial in another is passed by value differently on arm64) would crash the same way; search for them when a
  by-value struct argument arrives as garbage.
- note_window (Reports): keys_scroll / data_scroll are TargetW, a pad view (`pad178[0x484 - 0x178]`, then src) and its
  ValueSource (hud.h) is UiD seen through a numbered vtable; PartyScreen::frame_update 0x531800 indexes member_list by
  `((TargetW**)this)[slot]`. Fold to Camera::follow[1] / UiD members (Reports, Party, the exchange screens).

- Save path (lane 7): GameData::save_state, write_missions, QuestRec::write (was Rec138), FactionRec::write300 (was Rec300),
  PartyTemplate::write (was RecB0), Troop::write_save, TroopIO::read, the save panel (C4c31c0) are members now; the
  Rec78 / Rec5c / Table (Pool<Event>) / cPairs96 writers are still views (layout-compatible at 64 bits, check them).
- mission_5.cpp (the battle HUD, MissionBlocker::frame_update 0x5261e0 and its slices) still has ~400 `_s2xxxx` view uses:
  Agent_s284a0 / _s29840 / _s29ec0 / _s2b5c0 index the agent pages with 32-bit strides and pads. Fold them like
  s29130 (update_marks): real Agent / Mission / GameData members. Battles will need it.
- `rgl::ptr_list::operator=` / MeshVec::operator= are aliased to vector<int>::operator= (port/link/aliases.ld): the map
  widget copies the world's light list through it every frame (render_shape.cpp); give them typed definitions.
- Vtable views whose slot 0 is a plain virtual (agent_hud.h ScreenOps was one: at 64 bits the virtual destructor takes two
  Itanium slots, so `begin()` (slot 9) called the wrong function and the menu stayed blank): call the real class's virtual.
- **Find view mismatches with tools/port/view_layout.py** (new): `view_layout.py cmp A.cpp:RealClass B.cpp:View` lays both
  classes out with clang for linux-i686 and macos-arm64 and prints every member of the view whose 64-bit offset differs from
  the real class's member at the same 32-bit offset. Game vs GameData now agree completely (0 MOVED). Known open pairs:
  Event vs party2's BattleRec (pad0[4] instead of IfaceB's vptr: every field moves), SceneFile vs game_data.h's Level494,
  the 12 game-object views in mission5/mission_5.h (GameData_s29130 / s29840 / ..., pads from +0: battles), Poser::lists
  (+0x34) as the lod mesh, PropEnt.
- Raw strides / offsets still to convert: `* 0x5ac8` became `* sizeof(Party)` everywhere (22 sites); search the same way for
  0x98 (Event), 0x508 (Troop), 0x5928 (Agent), 0x494 (scene), 0x68 (menu_item) strides; save_state.cpp:20 `(char*)this + 4`.
- `rgl::cell::b` / grid_cell::objs is a `std::list<int>` holding shape pointers (world_cells.cpp, the folded 0x41f640).
- **Raw allocation sizes**: `operator new(0x..)` with a 32-bit class size overruns at 64 bits (make_editor_list's Camera
  did). Fixed for the widgets; still raw: mesh_game.cpp (0x7c, 0x64), mesh_start.cpp psys 0x1bc, mesh_appear.cpp lod2_a
  0xe0, gui_edit.cpp list node 0x28 (`rg -n "operator new\(0x" src`).
- **Views whose slot 0 is a plain virtual** call the wrong slot at 64 bits (UiE's TextSizeView called set_font):
  `rg -l "virtual void (s00|d00|v00|r0)\(\)" src` lists ~20 files; view_open.cpp's Entity (3D portrait branch only) is one.
- Header-count quirk (cl 13.10), resolved for idiom: Recd8::update_days and C4c3050::init are members now, message_window.h
  and config.h's music_res are gone. Cost: video_page.cpp's 0x4d63b0 moved to its .nonmatching.txt (two g_windows deque
  loads swapped, 581/584; eq WEAK). chk.py/check.py mask relocation addends and call it MATCH: keep it out of the manifest.
- Troop serializers still walk the record raw (party/game.cpp 0x51e... `p + 0x3c0 + i * 8`, char_sheet/save_records.cpp
  `p + 0x3c0`, `p + 0xa4` ...): save / load will need them on Troop members.
- check.py / chk.py mask relocated bytes, so a row whose only difference is a relocation addend (0x4d63b0: g_windows+0x84
  vs +0x88) reads as MATCH and check.py moves it back to the manifest; run_all.py reports it (DIFF n bytes, reloc addend).
- mesh_appear.cpp, mesh_slot.h, terrain_b/tmesh.h: lod_group seen as lod2_a / tmesh / lod_mesh_x / Poser views; the
  layouts agree at 64 bits today (view_layout.py) but they should become lod_group's members.
- QuestPage (options_panel/quest_page.h) is still a view of Hud for fill() 0x4d5ba0 (w44/w48/sel): convert like Recd8.
- ItemA::frame_update (map_editor/exchange_screens.cpp) keeps `int slot = (int)this` for src().pick / g_map_flags.commit.
- `GameMapView::scan_arrays` is now `Game::scan_arrays` (port/asan_ignore.txt updated): it stores one float past
  map_arrays::scan_a, as the original does.
- The ASan build sat on "Launching Game..." for 100 s where the plain build reaches the menu in ~40 s: give it longer.

## How to iterate

- Build: `cmake --build build/port/macos-arm64 --target mb_native -j6` (the build dir has `-g`; a full build is ~25 s).
  After src edits that change which symbols a TU defines: `.venv/bin/python tools/port/link_tables.py --target macos-arm64`,
  rebuild, run it again (fixpoint).
- ASan build (finds most 64-bit layout bugs directly, with the allocation site):
  `build/port/macos-arm64-asan` (configured with `-g -fsanitize=address -fsanitize-ignorelist=$PWD/port/asan_ignore.txt`), run with
  `ASAN_OPTIONS=detect_odr_violation=0:detect_leaks=0 .venv/bin/python tools/port/run_native.py --target macos-arm64-asan --gain 0 --time 40 --tag asan`;
  the report is in work/port_shots/asan.log.
- `tools/port/run_native.py`: bounded run from game/ with `--at <s> shot <name>` / `click <fx> <fy>` / `key <code>`,
  kills the game at the end, prints the macOS crash report's faulting thread on a signal. MBA_OUTPUT_GAIN (port/audio)
  keeps the sound quiet (`--gain`).
- tools/relink/build.py's `resolve_map ... 242 errors` line is not new: every build.json since 2026-11-15 reports 242
  (160 on 11-05, 173 on 11-06). They are resolve_map.py's "static symbol" / "symbol not found" entries (marker VAs whose
  symbol is a file-static or not a function start), listed in <out>/resolve_map.json "errors".
- lldb does not work here (process launch / attach hang: debugger authorisation); use ASan or temporary fprintf.
  `tools/port/lldb_crash.sh` is kept for a machine where it does.

## Fixed in this lane (commits a9f7050a .. )

Eighth session (37e960f3 ..): GameG::tick / track_parties / update_tracks on GameData members (the FLD offset macros
removed), EventPool (game/event_pool.h), Party::model = rgl::strategic_entity (strategic_entity::set_fade 0x4b1800),
face::intersect 0x51a2d0, battle_rec.h folded into Event (+ Event::read 0x51e970, SpriteTable::at an Event*),
count_units on Party::stacks (0x490ce0 191 -> 327, eq PASS), the hnode / OpenItem by-value ABI mismatch (copy
constructors only under cl), Troop / faction strides in the inventory / party screens, the compass via
entity::set_local_dirty.

Sixth session (2693ab79 ..): FaceTexture (the real Skin::face_textures element, was E176 / Ramp / MissionRec2 views;
hair_materials vector<int>), ViewParams' shadow texture* + fog names (render_scene's sky_t view folded; 0x46e590 203 ->
222, eq WEAK), texture::create's `new IDirect3DTexture9*[1]`, CampaignScreen / PartyScreen call MissionUi's real virtuals
(PsOps / PsOpsW vtable views removed: set_text had landed in set_font).

Fifth session (463d49b9 ..): Recd8::update_days / C4c3050::init members (0x4d63b0 to nonmatching), WindowMgrW::apply
through ScreenObj's reset / cursor slots, Troop::nib and UiE in the sheet, Troop::face_keys[4], TroopRec key/name in
CharSheet::init_screen, make_slider's tstrings (0x4f5fb0 to nonmatching), Animator::get_text (FaceKey), FaceGen's
Camera / frustum / entity members, lod_group sizes (tmesh::clone, build_appearance), FaceKey indexing in blend_indexed,
lod_mesh::name; MB_CRASH_BT traps SIGABRT and takes a file path.

Fourth session (e0e968b0, 2eae9f66, 0dc4fe00, 298ccaf1): sound_record::load members (SoundMgr::play crash), MusicTrack as
the real record, slider knobs (mesh local_frame.o), Recd8's update_days / update_difficulty (options captions), screen
w14 / w18 through slot_as<head_rec> (the game menu's return to the character sheet; was the empty box), ini_file value
reads, CharSheet::load_troop / fill_skills / open_view, MessageWindowSetup::init, skill names via SkillRec, UiE's set_text,
widget allocation sizes; port/audio MBA_LOG=1, port/main.cpp MB_CRASH_BT=1 (backtrace on a fault signal: the macOS crash
report is often not written).

Third session: Game has the real map members (map_track, track_entity, `ents` std::vector<rgl::mesh*>, map_obj, fog1..3,
`wind` Light*, compass, ScriptDebugger debugger, the editor's `sel` vector, EventContext event_ctx; TriggerTag / SimpleTag shared
in include/trigger_tags.h, ScriptDebugger / EventContext in include/game_parts.h; PartyMgr ends at +0x139dc), GameMapView is gone;
add_order_arrow / spawn_battle_icon are GameData members (`new sphere`), entity::add_mesh (0x442110, was Poser::add_extra),
build_legend_mesh(rgl::entity*), one MapIcon (game/map_icon.h), FactionRec's rank vector, SceneFile as the script's scene
(visitors, slots), PartyTemplate::load (0x4bdff0, was sound_def::save with raw offsets) and its stacks, spawn_around /
add_sprite take a Vec2, the script block's locals are `__int64[128]` (step_less / pop_try take `__int64*`), the interpreter's
c_str view is a tstring, MenuRec / menu_item for g_menus, HudOuter / FieldPanel's view widget on Camera / MissionUi,
entity::sync's scope bug, Event::entity.


Link: Pool<T>::~Pool (0x4c6e30) instead of MapDialog::clear_objects/ListObj; SceneFile[] in free_arrays; the 0x40f8a0 pointer
fill as std::_Uninitialized_fill_n (link_tables.py instantiates it); duplicate g_river_mask definitions; weak _fltused;
link_tables.py Mach-O fixes (weak defs, __const data, underscores in weaken.cmake: weakening never applied before).
Run: subsystem libraries force-loaded ahead of src/ objects (static-init order: GameApp's constructor calls the registry and
FSOUND_Init); platform globals constructed on first use; .fxo -> .fx; _findfirst handle intptr_t; HashTab::insert returns
its HOut*; WVt virtual destructor slot; and the layout64 items listed in docs/layout64_inventory.md.

- Finding the record behind a crash: the plain build's objects carry DWARF; `dwarfdump --debug-info <obj>.o` prints
  member offsets of a struct at 64 bits (compare a view against the real class), atos gives no lines here.

## Known hazards still in the tree (not yet hit)

- aliases.ld binds some names to instances of another element type: `MeshVec::operator=` and `rgl::ptr_list::operator=`
  -> `std::vector<int>::operator=` (copies 4-byte ints where the vectors hold pointers), `IntVec::insert_n` -> vector<int>.
  At 64 bits these need real typed definitions.
- Calls through casted function-pointer types with a struct return (the IDENTITY_VALUE pattern) break on arm64 (x8).
- Padded "views" of real classes (`char padNN[...]` before a pointer or tstring) and raw `(char*)p + 0xNN` accesses
  (ticket 022) are the bulk of what is left; ASan finds the heap ones.
- Views whose slot 0 is a plain virtual where the real class has a virtual destructor (Itanium uses two slots).
- Teardown paths (Pool<T>::~Pool, GameData::world_free) are untested.

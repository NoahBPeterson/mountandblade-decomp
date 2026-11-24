# Native port integration status (macOS arm64)

Lane: integration (docs/port_lanes.md). Updated 2026-11-20 (fourteenth session, lane 14).

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
  GameG::tick (game/campaign/game_tick.cpp) and the track code (party_tracks.cpp) read GameData through
  `#define x FLD(T, 0x...)` raw 32-bit offsets, so `f94` (stop) read garbage and the tick returned every frame (no party
  ticks, no time). They are GameData's members now (f94, rest_period, f14058 the time speed, timer1a298, game_clock, the
  fog1/fog2/fog3 colours, wind, ents, map_obj, map_track, track_entity); the event pool cursor is EventPool::next / first
  (game/campaign/event_pool.h). Party::model is the real rgl::strategic_entity (the PartyModel vtable/pad view put `visible` in a
  pad and called slots off by one); the click's plane hit is face::intersect (PlaneTrace view: +0x20 / +0xa0 where face has
  w / normal at +0x28 / +0xa8 at 64 bits). With those, a terrain click picks the ground and issues the move order
  (MAPDBG trace: best face found, shares = 1, action 6), time runs (speed 8.6), and the path search ran.

- Ninth session: **save and load work.** Restore a Saved Game -> slot -> Restore loads the save onto the map
  (work/port_shots/l9b_a.png: the party at the Training Field, the date, the map bar); Esc -> Save & Exit writes the save
  (same size as before; reloading the re-saved file works, l9e_a.png). The note block is one `SoaBlock notes` member
  (game/ui/note_block.h: times[8], texts[8], tableau) in Troop, FactionRec, QuestRec, Party and game/campaign/data_files.h's
  file records; read_soa / write_soa take `&notes`. Game::scenes is a SceneFile* (load_parties read the slots at
  32-bit offsets); scene_file.h no longer includes engine/io/stream_inline.h (its includers do), so game/campaign/game_state_load.cpp can include it.
  Recipe (ASan, sandboxed saves): `MB_USER_DIR=<dir>` (port/platform/fs.cpp: Documents and AppData under <dir>; copy
  ~/Documents/Mount&Blade Savegames and ~/Library/Application Support/Mount&Blade there, the user's saves stay untouched),
  Restore `click 0.54 0.615` at 45, slot 1 `click 0.18 0.18` at 57, Restore `click 0.78 0.93` at 60, the map is up by
  ~90; Esc `key 53`, Save & Exit `click 0.30 0.575`.

- Tenth session: **quitting exits cleanly** ("Exit to Windows" from the main menu: exit code 0 under ASan, FinalCleanup and
  every static destructor run). Fixed on the way: RecD / RecI folded into TriggerRec (game/data/trigger_rec.h) / menu_item;
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

- Twelfth session (lane 12): **the agent record is one class.** Step 1: the AI block at +0x258 is `AgentAi Agent::ai`
  (game/agent/ai/agent_ai_game.h: constructor 0x475e50, init 0x463370, both still byte-exact; was ViewState / ViewBody and Agent's flat
  fields with 32-bit pads). Its timers are clock2_timer (the constructor's five timer starts are the members' own),
  `input_q` is a real `std::deque<InputRec>` (Tail2 is gone from the agent; deque<InputRec>::_Tidy is byte-identical to
  deque<int>'s 0x4c4d10, so ICF folds them), InputState is {bits, f4, move}. Agent code reads `ai.goal`, `ai.path_pos`...
  (the renames were applied at clang's error locations: scratch tool, not kept). Figures: ai_update_control 0x448eb0
  4682 -> 4675, Operation::execute 0x49fa10 19731 -> 19670 (the recorded 19067 was stale), render_scene 0x46e590
  222 -> 176 (the anonymous-union count quirk), start_setup 0x484590 5296 (same).
  Step 2: **MAgent, AgentS, PoolS, AgentEq, BigObj are gone**: Agent has MAgent's constructor 0x477a30 and reset
  0x472f90 (byte-exact / same figure), the members with constructors (clock2_timer t40 / tm / timer / t5800 / t5810 /
  weave_timer / t5918, gui_timer snd_stamp, clock2_active_timer mount_timer, BlowInit blow, SlotVec slot_vars), the spawn
  words (flags +0x18, class_rank +0x1cc, slot_flag[10] +0x220, override_flags / override_items / num_override_items
  +0x56f0.., meters[] are node_groups*), facing is a vector4. The pool is Pool<Agent> (??_EAgent / ??0Agent /
  Pool<Agent>::alloc / reset rows renamed; symbols/slices: ??_7Agent@@6B@ is 0x591848, 0x590b84 is PlacedObj's).
  Agent::reset_path is 0x432080 (was BigObj::reset), Agent::agent_class 0x433140 (was AgentX's), Mission::pick_slots
  takes Agent* (was AgentEq, every field off at 64 bits: it wrote -1 into the item-mesh pointers). mesh_game's spawner
  and Mission::frame use agents directly (PtrTableF gone). build_appearance reads the troop as Troop
  (face_keys / face_keys_b +0x3e0, was `chars_c + troop * 0x508`), append_mesh FaceTexture::color.
  objcmp: only helper symbols change in the agent TUs; header-count figures moved: MapPanel::update 0x51abd0
  53 -> 67, MapPanel::draw_party_line 0x51c090 now chk MATCH (not moved: run_all first).

- Thirteenth session (lane 13): **the agent body and its skeleton are the real classes.** Agent::body is
  `rgl::strategic_entity*` (AgentBody gone: body->m is `local`, body->skel is `fx`), the skeleton is FxDef (Skeleton /
  Region / BoneRec / skeleton.h's controller gone; Skeleton's methods are FxDef's: ensure_updated 0x43e740, rec 0x43f260
  (returns `&elems[i].m84`), set_visible 0x452f80 (`bool`), remove_group 0x439ec0, start_anim 0x439fc0, all_groups
  0x43a130; env_curve.cpp's reset_rec / rec_shaped_phase / set_rec_phase), the channels are EnvRec (Channel / Curve / SkelX
  gone: channel.h deleted; EnvRec::busy 0x433840 / unshape / set_phase, `gui_timer tm` at +0x20 (was clock / t0; needed
  for reset_rec's byte match), `KeyPair wt` {w[2], offs[2]} at +0xc0 (Channel::rate is offs[1], an int), pose_src is an
  int flag). Raw `recs + i * 0x1dc + 0x84` reads are `elems[i].m84` (aim bone 9, head 9's origin, 3 / 6 / 19 / 12).
  The skeleton resource is frame_set with engine/physics/bone.h's Bone (moved out of resource_io.h; cone / twist limits are floats,
  `bodies` is `std::vector<CapDef>`): SkelData / ChanRec gone (capsule_hit, start_ragdoll, ragdoll_update).
  world_data's CarryBody / CarrySkel / CarryElem and agent_hud's Skeleton / SunBody views are gone too.
  MSVC: every touched manifest byte-exact under run_all (agent_logic 141/141, curve, agent_pick, agent_stats,
  terrain_render, world_data, agent_hud, script_interpreter, resmgr); update_anims 0x43d040 2697 -> 2555 aligned.
  Then, one ASan run each (Quick Battle, ASan recipe below), in order: E68W::purge (tableau materials: E68::v is
  `std::vector<skin*>`, find_or_create / purge / quarter / make_material are E68's; ResSlot / BlockMat / MissionView's
  material half gone), Agent::camera_params (writes the body's capsule; AgentBodyV / CamParams gone),
  Terrain::compute_normals (a casted hidden-result-pointer call to tri_normal broke on arm64; row to nonmatching, 25,
  equiv PASS), Mission::prop_ents (`std::vector<rgl::entity*>`, was vector<int> v63c0 fed by EntVec), Terrain::ents
  (rgl::entity*, TEntity gone), the animations (engine/fx/anim_set.h: AnimSet / AnimDef / SubAnim; DefList, RecH, AnimDefV /
  W, PartV, SubV, Record / ActorItemRec, Group gone; psys_record::read is SubAnim::read), lod_group::set_material
  0x48cfc0 (ListHolder / Owned gone), Mission::copy_slot on MarkRec members (MarkRec is 0x138 bytes at 64 bits), the
  prop tween on PlacedObj (animate 0x467c50 / animate_to 0x4897a0 / set_position 0x489820; Prop, PropAnim,
  MissionView, ViewObj gone). Every touched manifest stays byte-exact under run_all.
  **Quick Battle now spawns every agent and runs the mission's first frame** (start_setup -> Mission::frame).
  Lane 13's blocker (fixed in the fourteenth session): Mission::frame -> Agent::update -> update_attack -> FxAnim::update_poses2 faults: fx_animation.h's
  AnimRec / FxBone / KeyObj are 32-bit views (AnimRec's pads put `table` / `ref` at the wrong 64-bit offsets; FxElem::c
  holds KeyObj pointers in `int c[2]`; KeyObj pads skel_anim's keys / tail). Fold: AnimRec -> EnvRec (f18 ->
  shaped_phase, cur -> cur_key, clock / start -> tm, wt -> wt.w, offs -> wt.offs + step_clock, v8c -> v7c[1], phase
  0x4231f0 / shape 0x4232b0 become EnvRec's; env_curve.cpp's inline shape must stay inline under another name), EnvRec::ref
  -> quaternion* (agent_logic 0x43d040 builds those quaternions as vector4 today), FxElem::c -> skel_anim* c[2],
  KeyObj -> skel_anim (KeyTable vs key_array / vec_array are one record), FxAnim's methods -> FxDef's.
  ASan Quick Battle recipe (machine busy with other lanes): main menu `click 0.45 0.695` at 90, Skirmish 1
  `click 0.334 0.669` at 130, Start `click 0.334 0.669` at 170, `--time 330`; the plain build's 40 / 60 / 80 was too
  early for the plain build too (80 / 110 / 140 worked).
  Left from the brief (SceneEnt and the Anim / KeyHolder / Elem12 views are done, see below): Wielder, the lod2_a /
  node_groups / lod_mesh2 casts of the spawner (`(lod_mesh2**) a->slot_mesh`: the 22 pointers from +0x13c are one array
  in the original, build_appearance writes ap[21] = the name label), EmptySlot (PlacedObj::set_position's direct call of
  0x477b70: IfaceB::slot1 is inline in iface.h, so a qualified call emits nothing; it needs an out-of-line definition).

- Fourteenth session (lane 14): **the mission's first frame completes** (every agent's update and tick, the animation
  pass, world::step). fx_animation.h's views are folded: AnimRec is EnvRec (phase 0x4231f0 / shape 0x4232b0 are EnvRec's;
  env_curve.cpp's inline copy is EnvRec::eased), FxAnim's methods are FxDef's, FxBone is FxElem (quat4 gone: q / abs_q / qe4 /
  qa / qb are quaternion, `skel_anim* c[2]`, t[i][j] instead of the raw +0x104 slot), KeyObj is skel_anim, KeyTable /
  KeyFrames / Elem12 are key_array (game/input/key_table.h: find 0x423a50, sample 0x436ae0; KeyHolder::sample is
  skel_anim::sample 0x436b90; Anim gone, SubAnim::anim is skel_anim*), EnvRec::table / keys_a / keys_b / ref are
  skel_anim* / quaternion*. value() and chain() return by value (the original ABI; aliases.ld's FxAnimQ::qvalue ->
  FxAnim::value alias bound a by-value call to the out-parameter function, cdecl/4 vs /5).
  Then, one ASan run each: Agent::draw_name (AgentFx was a pad view of the "agent_name" lod mesh: `node_groups*
  name_label` +0x190, build_appearance's ap[21]; its origin is a vector4: layout_text reads w), SceneEnt folded
  (spawn_missile / spawn_prop / move_step / ai_update_control on rgl::entity, rgl::dynamic_entity (engine/physics/rigid_body_rgl_dynamic.h,
  split out of rigid_body.h so agent_logic does not see that header's vector specializations) and
  rgl::strategic_entity; world::collect_boxed fills a std::vector<rgl::entity*> (was vector<unsigned>: the pointers were
  truncated); Missile::ent is rgl::entity*), Agent::pick / fill_weights / AgentX 0x432470 / agent_hud's rebuild on
  members (raw +0xd8 / +0x130 / +0x100 / +0x240), Mission::water (+0x65a8 was `float f65a8` read as a SkyF* view: the
  water rgl::mesh; create_water / regenerate_water take rgl::mesh**), rgl::pool::grow's last link (`((int*)blk)[count*3-3]`
  missed the last node at 64 bits: the free list ran into the heap). Mission::prop_ents is
  std::vector<rgl::strategic_entity*>: init_scene's resize binds to agent_logic's 0x43f310 again (target_gate), since
  vector<entity*>::resize is the rgl objects' 0x41fa00.
  Rows: fx_keys value 0x423540 681 -> 777 (PASS), chain 0x423b60 70 -> 200 (PASS), update_poses2 0x422130 838 -> 907
  (PASS), draw_name 0x4552c0 221 -> 179 (WEAK), spawn_prop 0x44c210 1607 (WEAK-FP; the recorded 1634 / quoted 580 were
  older measures), ai_update_control 0x448eb0 4675 -> 4716, move_step 0x45d820 5695 -> 5758 (WEAK).
  Second change set: snd_slot / snd_bank were a second class for TriggerRec / TriggerVec (the same 0x78-byte record:
  intervals, state, the two DialogBlocks, three gui_timers): update 0x4625d0 is TriggerRec's, update_all 0x462760 /
  randomize 0x462790 TriggerVec's, and agent_spawn_and_tick.cpp / mission_start_setup.cpp walk the template's TriggerVec (the SndSlotS /
  SndSlot / SndBank / SndSetF pad views strode 0x78 bytes; snd_slot / snd_bank stay typedefs for mission_5.cpp, V4's).
  The rain emitter is `new rgl::psys(0)` (start_setup 5296 -> 5264; the operator new(0x1bc) stand-in left it short and
  uninitialised), mesh_pool (particle_system.h's pad view of the render scene's owner) is rgl::world (get_mesh 0x413d30 is
  world's, scene::owner without the union). ASan ignores ItemKind::is_ranged: choose_action reads g_items[-1] as the
  original does (cmp edi,-1; jl at 0x44d4d9).
  With both sets **Quick Battle runs start_setup to the end** (spawns, the setup frames with every agent's update and
  tick, the triggers, the shadow-map render) and reaches the battle screen's first frame.
  **Current blocker**: MissionBlocker::frame_update -> update_status 0x4fb350 (game/ui/hud/agent_hud.cpp) dereferences
  status_group (+0xc4, +0x138 at 64 bits) while it is still null: init_screen 0x4f7f00 (slot 2, which builds it) never
  ran. A trace of WindowMgrW::open shows screens 3 and 23 opened, never 14, before the mission screen's first
  frame_update; so screen 14 reaches the top of g_windows.dq, or gets ticked, some other way (custom_battle_go ->
  start_setup -> MapFlags::commit). Next: find that path (likely a raw-offset or view read of the screen id).
  Reproduce in 3 s: `run_native.py --start quick_battle:1 --user-dir <sandbox>` (Release; ASan ~8 s).
  game/ui/hud/agent_hud.cpp still has raw offsets in init_screen's grow_health branch (troop +0x58 / +0x7a) and
  agent_hud.h a raw `* 0x5928` agent lookup.
  TriggerTag (game +0x139e8) is the same {count, array, one} record as TriggerVec (game_tick / troop_rec cast it);
  the missile pool is still four views of one record (RecJ / Missile / MissileU / PropRecF, plus Wielder for its item
  pair; Mission::v5dfc is the retire list of their entities as vector<int>): fold them into one Missile : IfaceB
  (ctor 0x4377d0, reset 0x437810; `clock2_timer` at +0x70) and Pool<Missile> before arrows fly.
  Recipe: the ASan Quick Battle runs as before (90 / 130 / 170, `--time 330`; the mission starts ~115-125 s after
  launch); MB_USER_DIR=<copied sandbox>, MB_CRASH_BT=1.
  Physics (lane P1): newton_jolt.cpp follows Newton 1.53 within 1-2% on traced scenes (work/physics_traces/,
  physics_trace_replay). When a ragdoll misbehaves natively, replay a trace first (a mismatch there is the shim, else the
  game side); Newton's joint limits are soft and Jolt's hard, so poses past a limit snap on the first step.

## Next leads

- Exit teardown left: g_party_templates (conv_item_array_dtor 0x4c8980, PartyTemplate) is still a hand-written `??_E`
  reached through aliases.ld with a mismatched ABI (cdecl/2 vs fastcall/3): it takes the scalar path and leaks. Give
  PartyTemplate its destructor (`union { I64Vec list; SlotArray slots; }` becomes a SlotVec) and `delete[]` it, as
  g_propkinds is now (MissionObj: ??_EMissionObj 0x4c7ee0 / ??1MissionObj 0x4c65d0 are the compiler's, MATCH).
- Save diff (lane 9's 134 bytes): a load -> Save & Exit of sg00 differs from the loaded file in 136 bytes, same size, no shift.
  Most are float last-bit differences (e.g. 0x1c66c f055 -> ec55, 0x2ea16 facb -> f9cb: arm64 float arithmetic vs the
  original's x87 on the frames run before the save), the header's 0x58..0x63 (camera / map position) and a few small
  counters (0xc5527 2 -> 5, 0xc60c7 0x173 -> 0): state, not a writer bug. Loading it back in the original is still untested.
- (Lane 12's battle-start blocker, the AgentBody / Skeleton / Channel / Curve fold, is done: see the thirteenth session.)
- Views met on the way, still open: ptype vs poly_mesh_edge_face_ops.h's ObjectList / shape_entry (AnimBox now has the same members,
  but two classes remain; module_load reads through `(ObjectList*)&g_ptypes[i]`), TimerList vs GameVec (one class
  {count, 0x30-byte trigger_entry/TimerChild, spare}; MissionObj::read and MapIcon::read cast), PolyGeom (render_passes.cpp)
  vs NavData (NavCell is an incomplete GeomFace), EntVec / Mission::v63c0 (the prop entity list is a vector<int> holding
  entity pointers; EntVec::insert_n goes through vector<int>::_Insert_n: truncates at 64 bits), rgl::world::
  collect_boxed's std::vector<unsigned> of entity pointers (agent_logic.cpp 0x442180), misc.h's Prop / prop_ent.h's
  PropEnt / mission_3's ent_lods (PlacedObj views), script_interpreter's ArrHolder / Obj3 (FxDef / psys views). spawn_prop (agent_logic.cpp 0x44c210, the prop spawner
  that now runs) still builds its entities through scene_ent.h's SceneEnt (a padded rgl::entity view) and
  mesh_start's rain emitter is `operator new(0x1bc)` (rgl::psys).
- Quick Battle recipe (ASan): main menu `click 0.45 0.695` at 50, Skirmish 1 `click 0.334 0.669` at 80 (the scenario text
  with Start / Cancel), Start `click 0.334 0.669` at 110. Plain build (lane 11): 40 / 60 / 80, with MB_CRASH_BT=1 (the
  mission setup runs inside the Start click's frame, ~0.3 s).
- **Towns and battles are missions**: the agent body is rgl::strategic_entity seen through AgentBody (agent_obj.h: id,
  frame +0x58, skel at fx +0x304), AgentBodyV / SkelBodyV / SunBody (vtable views with slot 0 a plain virtual), EntityG /
  AgentS (agent_spawn_and_tick.cpp: the spawner's 32-bit views of the entity and the agent, with `operator new(0x7c)` / `0x64` raw
  sizes for FxDef / capsule), SceneEnt, TEntity, TableauEntity(V). One pass folding them into strategic_entity / Agent is
  the next big step (Mission::spawn 0x4832b0 is the first function a town or battle runs). The agent record itself is two
  classes: the pool allocates MAgent (game/terrain/agent_rec.h, an IfaceB with a real vptr: 8 bytes at 64 bits) while
  agent_obj.h's Agent started with `char pad0[4]` for the vptr. Lane 10: Agent is an IfaceB now (used / id are in_use /
  pool_id), slot_vars a SlotVec, MAgent's Timed / ClockStamp Timers, AppearanceLods shared: `view_layout.py cmp
  src/game/terrain/poly_mesh.cpp:MAgent src/game/agent/agent_queries.cpp:Agent` agrees up to +0x250; 303 members still apart from
  MAgent's ViewState view at +0x258 on (Agent's body / goal / path_face ...). Fold MAgent into Agent (one class, the
  constructor 0x477a30 and reset 0x472f90 as Agent members) before anything else on the mission path. Quick Battle (main menu
  0.45 0.695) reaches a mission without character creation: the fastest test.
- Other ODR copies with user-written copy constructors (the hnode / OpenItem pattern: a type trivial in one unit and
  non-trivial in another is passed by value differently on arm64) would crash the same way; search for them when a
  by-value struct argument arrives as garbage.
- note_window (Reports): keys_scroll / data_scroll are TargetW, a pad view (`pad178[0x484 - 0x178]`, then src) and its
  ValueSource (mission_hud_buttons.h) is UiD seen through a numbered vtable; PartyScreen::frame_update 0x531800 indexes member_list by
  `((TargetW**)this)[slot]`. Fold to Camera::follow[1] / UiD members (Reports, Party, the exchange screens).

- Save path (lane 7): GameData::save_state, write_missions, QuestRec::write (was Rec138), FactionRec::write300 (was Rec300),
  PartyTemplate::write (was RecB0), Troop::write_save, TroopIO::read, the save panel (C4c31c0) are members now; the
  Rec78 / Rec5c / Table (Pool<Event>) / cPairs96 writers are still views (layout-compatible at 64 bits, check them).
- mission_5.cpp (the battle HUD, MissionBlocker::frame_update 0x5261e0 and its slices) still has ~400 `_s2xxxx` view uses:
  Agent_s284a0 / _s29840 / _s29ec0 / _s2b5c0 index the agent pages with 32-bit strides and pads. Fold them like
  s29130 (update_marks): real Agent / Mission / GameData members. Battles will need it.
- `rgl::ptr_list::operator=` / MeshVec::operator= are aliased to vector<int>::operator= (port/link/aliases.ld): the map
  widget copies the world's light list through it every frame (map_widget_render.cpp); give them typed definitions.
- Vtable views whose slot 0 is a plain virtual (agent_hud.h ScreenOps was one: at 64 bits the virtual destructor takes two
  Itanium slots, so `begin()` (slot 9) called the wrong function and the menu stayed blank): call the real class's virtual.
- **Find view mismatches with tools/port/view_layout.py** (new): `view_layout.py cmp A.cpp:RealClass B.cpp:View` lays both
  classes out with clang for linux-i686 and macos-arm64 and prints every member of the view whose 64-bit offset differs from
  the real class's member at the same 32-bit offset. Game vs GameData now agree completely (0 MOVED). Known open pairs:
  Event vs party2's BattleRec (pad0[4] instead of IfaceB's vptr: every field moves), SceneFile vs game_data.h's Level494,
  the 12 game-object views in game/mission/mission_frame_update.h (GameData_s29130 / s29840 / ..., pads from +0: battles), Poser::lists
  (+0x34) as the lod mesh, PropEnt.
- Raw strides / offsets still to convert: `* 0x5ac8` became `* sizeof(Party)` everywhere (22 sites); search the same way for
  0x98 (Event), 0x508 (Troop), 0x5928 (Agent), 0x494 (scene), 0x68 (menu_item) strides; save_state.cpp:20 `(char*)this + 4`.
- `rgl::cell::b` / grid_cell::objs is a `std::list<int>` holding shape pointers (world_cells.cpp, the folded 0x41f640).
- **Raw allocation sizes**: `operator new(0x..)` with a 32-bit class size overruns at 64 bits (make_editor_list's Camera
  did). Fixed for the widgets; still raw: agent_spawn_and_tick.cpp (0x7c, 0x64), mission_start_setup.cpp psys 0x1bc, agent_appearance.cpp lod2_a
  0xe0, editor_selection_and_translation.cpp list node 0x28 (`rg -n "operator new\(0x" src`).
- **Views whose slot 0 is a plain virtual** call the wrong slot at 64 bits (UiE's TextSizeView called set_font):
  `rg -l "virtual void (s00|d00|v00|r0)\(\)" src` lists ~20 files; character_sheet_view.cpp's Entity (3D portrait branch only) is one.
- Header-count quirk (cl 13.10), resolved for idiom: Recd8::update_days and C4c3050::init are members now, message_window.h
  and config.h's music_res are gone. Cost: video_page.cpp's 0x4d63b0 moved to its .nonmatching.txt (two g_windows deque
  loads swapped, 581/584; eq WEAK). chk.py/check.py mask relocation addends and call it MATCH: keep it out of the manifest.
- Troop serializers still walk the record raw (game/campaign/game_state_load.cpp 0x51e... `p + 0x3c0 + i * 8`, game/save/save_records.cpp
  `p + 0x3c0`, `p + 0xa4` ...): save / load will need them on Troop members.
- check.py / chk.py mask relocated bytes, so a row whose only difference is a relocation addend (0x4d63b0: g_windows+0x84
  vs +0x88) reads as MATCH and check.py moves it back to the manifest; run_all.py reports it (DIFF n bytes, reloc addend).
- agent_appearance.cpp, mesh_slot.h, terrain_b/tmesh.h: lod_group seen as lod2_a / tmesh / lod_mesh_x / Poser views; the
  layouts agree at 64 bits today (view_layout.py) but they should become lod_group's members.
- QuestPage (options_panel/quest_page.h) is still a view of Hud for fill() 0x4d5ba0 (w44/w48/sel): convert like Recd8.
- ItemA::frame_update (game/ui/screens/mission_menu_and_exchange_screens.cpp) keeps `int slot = (int)this` for src().pick / g_map_flags.commit.
- `GameMapView::scan_arrays` is now `Game::scan_arrays` (port/asan_ignore.txt updated): it stores one float past
  map_arrays::scan_a, as the original does.
- The ASan build sat on "Launching Game..." for 100 s where the plain build reaches the menu in ~40 s: give it longer.

## Test runs (state-driven; tools lane T1, 2026-11-20)

The fixed-time recipes in the sessions above (main menu at 50-90 s, Skirmish 1 at 80, New Game at 40, map by ~300) are
obsolete: they waited for the worst case. Measured on this Mac (launch -> state, Release / ASan):

| target | Release | ASan | old recipe |
|---|---|---|---|
| main menu (`menu_ready`) | 1.6-2.3 s | 2.6-7.7 s | 40 / 50-100 s |
| Quick Battle: Start pressed, Mission::start_setup | 2.3-2.8 s | 2.6 s | 80-170 s |
| map from a save (`screen 7 map`) | 2.9 s | 8.7 s | ~90 s |
| character creation's first menu (`new_game`) | 2.4 s | | ~60 s |

Startup split (Release, MBSTATE times): process start -> main() ~0.4 s (dyld), main() -> first frame 0.85 s (window,
device, mb.fx, launcher), the seven loading steps 0.7-1.2 s (parse_module_info + load_pack ~0.5 s, load_module_files
0.2 s, textures 0.35 s). Nothing there is worth lazy-loading yet. The time that was real: Game::start_new_game's
TroopRec::level_up read the attributes at a raw +0x58 (inside the tstrings at 64 bits), so raise_stats spun on rand()
for 0-60 s in every new game / quick battle; it reads Troop::attributes now (objcmp SAME). Variance comes from machine
load (other lanes' builds): wait on states, never on times.

- **States**: port/mb_start.cpp writes `MBSTATE <s> <event> [args]` lines to stderr (the run log): `main`, `first_frame`,
  `load_step N`, `screen <id> <name>` (4 main_menu, 7 map, 14 mission, 21 save_load, 23 game_menu ...), `menu_ready`,
  `menu <game menu id>`, `mission_frame N` (1, 2, 3, 5, 10, 20, 30, 50, 100, ... frames on the mission screen),
  `start <step>`, `start_done`, `start_failed <why>`. MB_STATE=0 turns them off.
- **Direct launch**: `MB_START` (run_native.py `--start`) presses the menus' own buttons (the 0x10000 pressed flag a
  click sets, after the GUI's input pass, so the screens' original handlers run): `quick_battle[:N]` (scenario N of
  start_game_3, then Start), `new_game`, `tutorial`, `load[:N]` / `map[:N]` (sg0N.sav, default the first save), or
  steps `main:<row>`, `option:<mno id>`, `option:<menu>#<n>`, `slot:<n>`, `wait:<screen>` (comma-separated).
  Nothing of it is in src/; the hook is FSOUND_Update (port/audio), which GameApp::FrameMove calls once a frame.
- **run_native.py**: `--wait <state>`, `--on <state> <action>`, `--do <action>` run in order (actions: shot / click / key /
  sleep / end), `--state-timeout` (120 s), `--user-dir DIR` (MB_USER_DIR sandbox, seeded with copies of your saves).
  `--at` still works as before. Recipes:
  - Quick Battle, shot at the 30th mission frame: `run_native.py --tag qb --start quick_battle --on "mission_frame 30"
    shot b --do end --time 120` (add `--target macos-arm64-asan` with the ASAN_OPTIONS below).
  - Map from a save: `--user-dir work/port_user/<lane> --start map --on "screen 7" sleep 1 --do shot map --do end`.
  - Main menu only: `--wait menu_ready --do shot menu --do end`. Character creation: `--start new_game --wait start_done`,
    then `--do click ...` / `--start new_game,option:start_male` style steps.
  - Clicks still work state-driven: `--wait "menu menu_custom_battle_2" --do sleep 0.5 --do click 0.334 0.669`.

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
removed), EventPool (game/campaign/event_pool.h), Party::model = rgl::strategic_entity (strategic_entity::set_fade 0x4b1800),
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
in include/trigger_tags.h, ScriptDebugger / EventContext in game/data/game_parts.h; PartyMgr ends at +0x139dc), GameMapView is gone;
add_order_arrow / spawn_battle_icon are GameData members (`new sphere`), entity::add_mesh (0x442110, was Poser::add_extra),
build_legend_mesh(rgl::entity*), one MapIcon (game/campaign/map_icon.h), FactionRec's rank vector, SceneFile as the script's scene
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

## 64-bit port targets (2026-11-22)

The same reconstruction, built and run for three more targets; byte-exactness stays a 32-bit MSVC criterion — these lanes
gate on compile + link (0 trapped) + ctest + a run_native / wine smoke run.

- **macOS x86_64**: cross-built from Apple silicon with the same Apple clang (`port/cmake/macos-x86_64.cmake`), runs under
  Rosetta. Dependencies are static per-target builds from `tools/port/deps.py` (pinned tarballs in `work/port_deps/src/`,
  install prefix `work/port_deps/macos-x86_64/install`; brew's are arm64-only). Tables at fixpoint in `port/link/macos-x86_64/`
  (55 duplicates, 80 weakened TUs, 0 trapped). ctest **5/5**; quick_battle reaches mission_frame 30 and renders the battle
  (work/port_shots/qx86_b.png). The ragdoll test's one failure was not physics: Jolt's PUBLIC ISA flags (-mavx2 -mfma ...)
  leaked through mb_physics into the test, and Rosetta mistranslated the VEX codegen of its hoisted fmin update
  (pelvisMinZ corrupted to 0). Jolt is now a PRIVATE dep of mb_physics — only newton_jolt.cpp includes Jolt headers;
  details in docs/c2scope_evaluation.md.
- **Linux x86_64**: native clang 18 + lld inside an OrbStack ubuntu 24.04 amd64 machine
  (`port/cmake/linux-x86_64-native.cmake`; the Mac home is mounted at identical paths, so the Mac-side table tools read the
  same build dir; deps built there with `deps.py --native`). Tables at fixpoint in `port/link/linux-x86_64/` (54 duplicates,
  12 weakened TUs, 25 aliased, 3 STL instances, 7 -fno-inline TUs, 0 trapped). ctest **5/5**; quick_battle reaches
  mission_frame 30 and renders the battle (work/port_shots/qlx86_b.png). The run gate first died inside Device::init:
  ELF's flat namespace bound glslang's out-of-line `std::vector<int>::resize` (libstdc++ does not inline it; libc++ does,
  so macOS never hit this) to port/stl's implementation, which writes the MSVC 32-byte vector layout into a 24-byte
  libstdc++ one, leaving `_M_start` null — `memberRemapper[...]` wrote to nil. Exactly one symbol collides (comm of the
  src/ and dep symbol sets); native_link.cmake now renames it in every src object (`--redefine-sym`, originals untouched),
  so src/ keeps port/stl's resize and the deps bind to libstdc++'s. Details in docs/c2scope_evaluation.md.
- **Windows x86_64** (2026-11-23): zig's bundled clang + mingw-w64 sysroot (`port/cmake/windows-x86_64.cmake`,
  `x86_64-windows-gnu`, Itanium ABI like the other ports — not MSVC), cross-built from the Mac; deps are the same
  per-target static builds (`tools/port/deps.py --target windows-x86_64`), minus the `.res` members (zig's lld-link
  rejects them); the game's `.rc` compiles with zig-rc (resinator). Runs under Wine. `-mcpu=x86_64_v3` (zig's spelling
  of clang's `-march=x86-64-v3`: AVX2/BMI/FMA) is set on this lane only — the OrbStack linux-x86_64 VM exposes no AVX2,
  and Rosetta miscompiles VEX codegen on macos-x86_64 (826de59e). Tables at fixpoint in
  `port/link/windows-x86_64/` (54 duplicates, 41 weakened TUs, 24 aliased, 4 STL instances, 7 -fno-inline emissions,
  0 trapped), via COFF surgery: llvm-objcopy has no `--weaken-symbol` for COFF, so tools/port/coff.py does the
  equivalent (local copy, references rebound to the owner); lld-link takes whole-archive per archive but refuses
  `/alternatename` on the command line (aliases.ld's PROVIDEs go in as a `.drectve` object), and SDL3's dllimport
  `__imp_X` references collide with mb_platform's pseudo-Win32 definitions until tools/port/winimports.py emits
  partial DATA-only import archives for exactly the symbols mb_platform defines. ctest **5/5**; mb_gfx_test 19/19
  (incl. fx_all's 76 techniques); quick_battle reaches mission_frame 30 and renders the battle
  (work/port_shots/qb_b.png; re-verified with `-mcpu=x86_64_v3` as winv3_b.png, ctest 5/5 again). Wine specifics: its
  64-bit directory-search handles are raw pointers that callers keep
  in 32-bit longs (port/platform/fs.cpp wraps `_findfirst/_findnext` with a small-handle table), the registry store
  rewrites need MoveFileExA(MOVEFILE_REPLACE_EXISTING) where msvcrt's rename fails, and d3dapp's narrow-cast window
  title is detected in CreateWindowExW. The Wine packaging trap (a GCenx `.app`'s Contents/MacOS/wine is an AppKit
  launcher that eats arguments; the real loader is Contents/Resources/wine/bin/wine) and the loader split — tests on
  the cask 11.22 loader (the 11.21 loader dies with a fatal Rosetta translation error mid-mbp_kernel), the Vulkan run
  gate pinned to 11.21 (11.22's vkCreateInstance faults) — are in docs/c2scope_evaluation.md.
- **Linux arm64** (2026-11-23): native clang 18 + lld inside the OrbStack ubuntu 24.04 arm64 machine `mbarm`
  (`port/cmake/linux-arm64-native.cmake`; baseline armv8-a, no `-mcpu`). Same recipe as linux-x86_64: `deps.py --target
  linux-arm64 --native` (SDL3's X11 build also needs libxtst-dev), compile, then `link_tables.py --target linux-arm64
  --allow-traps` + rebuild to a fixpoint in 2 passes (`port/link/linux-arm64/`: 54 duplicates, 12 weakened TUs, 25
  aliased, 3 STL instances, 7 -fno-inline emissions, 0 trapped). Nothing in port/ or src/ needed an arm64 change.
  `bin/mb` is an aarch64 PIE needing only libm/libstdc++/libgcc_s/libc (max GLIBC_2.38). ctest **5/5**; quick_battle
  reaches mission_frame 30/50 under Xvfb + lavapipe and renders the battle (work/port_shots/arm64_qb_b.png). Recipe:
  `ssh mbarm@orb`, then `xvfb-run -a -s '-screen 0 1280x800x24' python3 tools/port/run_native.py --target linux-arm64
  --start quick_battle --wait 'mission_frame 30' --do shot a --do end`.
- **Install rules** (2026-11-23, `port/cmake/native_link.cmake`): `cmake --install <build> --prefix <dir>` puts `mb` in
  `<prefix>/libexec/mountandblade/` and a launcher (`port/install/mountandblade.in`: `MB_GAME_DIR` or the Steam library
  paths, then cd + exec) in `<prefix>/bin/`; Windows installs `mb.exe` to `bin/`. Jolt's own install rules are off
  (`ENABLE_INSTALL=OFF`), so nothing but those two files is installed on any lane. Build-from-source guide for users on
  distros that are not the latest Ubuntu LTS: docs/BUILDING_PORT.md.

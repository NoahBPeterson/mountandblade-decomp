# Native port integration status (macOS arm64)

Lane: integration (docs/port_lanes.md). Updated 2026-11-19 (fourth session).

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
- **Current blocker**: not yet seen past the sheet. In the last run the tutorial box was still up at 145 s (the OK click
  at 92 s may have come before it opened) and Done looked disabled (no name typed: the earlier key-press crashes were
  heap overruns, fixed). Next: click OK later (~100 s), type a name, Done -> face generator -> world map.

## Next leads

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
- **Header-count quirk (cl 13.10)**: one more member declaration in screen_records.h (or a sizeof(tstring) there) swaps two
  g_windows deque loads in video_page.cpp's Recf0::frame_update 0x4d63b0; config.h's unused music_res declaration likewise
  holds PartyScreen::setup_skills 0x5059c0. Hence update_days is a __fastcall free function and C4c3050's fill-in is
  MessageWindowSetup::init (message_window.h). objcmp the includers after any header edit.
- cam_view.cpp (face generator done path) still copies `g_game->chars_c + w14 * 0x508 + 0x3c0` raw.
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
- lldb does not work here (process launch / attach hang: debugger authorisation); use ASan or temporary fprintf.
  `tools/port/lldb_crash.sh` is kept for a machine where it does.

## Fixed in this lane (commits a9f7050a .. )

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

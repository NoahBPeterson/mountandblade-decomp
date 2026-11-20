# Native port integration status (macOS arm64)

Lane: integration (docs/port_lanes.md). Updated 2026-11-19.

## Where it gets to

- `build/port/macos-arm64/bin/mb` (target `mb_native`) **links with nothing trapped**.
- Runs from `game/`: device, mb.fx, fonts, loading screen, all of load_module_files, then **the main menu renders correctly**
  (work/port_shots/asan_asan_menu.png, menu_menu.png: background, logo, knight, the seven entries, hover highlight).
- **Input works**: run_native.py clicks reach the menu. "Options" (`--at 14 click 0.4 0.775`) opens the options screen
  (work/port_shots/menu_options.png). Leads there: every slider is at its left end, the choices read "Reduced to 1/4",
  "Poor", "Slower", and a stray "51" sits under the logo: the option values are probably read from a 32-bit view
  (rgl/options.h g_options / the config record) - not checked yet.
- Sound: not verified by ear (the log is silent; FSOUND runs through port/audio). Check with `--gain 0.05`.
- "Start a New Game" (`--at 10 click 0.5 0.536`) now runs the whole new-game sequence: load_game_data, start_new_game,
  build_map, Game::build_map_trees, Game::build_scene (party / battle icons, the map entities, the start scripts), and opens
  the first game menu: **character creation renders** ("Welcome, adventurer ... select your character's gender", Male /
  Female / Go back; work/port_shots/ng_ng50.png, ~50 s after the click). Next: click "Male" (~0.33 0.67 of the window) and
  follow the creation menus to the world map.

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
- Options screen values (sliders at the left, "Reduced to 1/4" ...): not looked at yet.
- Sound: not verified yet (run with `--gain 0.05`).
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

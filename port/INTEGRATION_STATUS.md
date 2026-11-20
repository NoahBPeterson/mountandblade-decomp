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
- "Start a New Game" (`--at 10 click 0.5 0.536`) runs the new-game sequence through load_game_data, start_new_game
  (troops, auto_equip), build_map / read_map / PolyGeom::build, init_map / world::build_faces and into
  **Game::build_map_trees, which is the current crash** (the loading screen "Loading Map File..." shows meanwhile,
  work/port_shots/asan_newgame1.png).

## Current crash and next leads

- `Game::build_map_trees` (game_2.cpp 0x5213d0) ends with `((GameMapView*)this)->ents.push_back(cmv)`: GameMapView
  (mission5/game_2.h) is a 32-bit padded view of Game (party/game.h): map_track +0x2c, track_entity +0x30,
  `std::vector<rgl::mesh*> ents` +0x34, map_obj +0x44, fog1..3 +0x48..+0x78, `wind` +0x139dc, `compass` +0x1a2a8.
  Game itself spells the same words `int f2c`, `pad30`, `Zero3 f38` (the vector's three pointers), `f44`, `pad48`.
  Real fix: give Game these members with their types (map_track, track_entity, ents as the vector, map_obj, fog1..3;
  wind / compass where they really live) and drop GameMapView; Game's constructor 0x51cfa0 zeroes the vector with
  three stores (Zero3), so check its row (check.py) when the member becomes a std::vector.
- `rgl::cell::b` / grid_cell::objs is a `std::list<int>` that stores shape pointers (world_cells.cpp inserts through
  list<int>::_Insert, the folded 0x41f640): the pointers are truncated at 64 bits; removal (world_cells.cpp:78) compares
  them. Needs a list of pointers whose MSVC call still binds 0x41f640 (an alias row, as aliases.ld does for others).
- `GameMapView::scan_arrays` (byte-exact) stores one float past map_arrays::scan_a, as the original does: ignored for ASan
  by port/asan_ignore.txt (the ASan build is configured with `-fsanitize-ignorelist=$PWD/port/asan_ignore.txt`); in the
  plain build it lands on whatever clang puts after the frame's map_arrays.

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

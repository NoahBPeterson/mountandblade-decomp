# Native port integration status (macOS arm64)

Lane: integration (docs/port_lanes.md). Updated 2026-11-19.

## Where it gets to

- `build/port/macos-arm64/bin/mb` (target `mb_native`) **links with nothing trapped**: link_tables.py reports 3 "trapped"
  names, but none is referenced by the linked image (ld64 links without traps.cpp; `nm` has none of them).
- Runs from `game/`: static init, WinMain, the D3D9 device over SDL_GPU/Metal, mb.fx compiled (from the .fx beside mb.fxo),
  fonts, the trigger records, the first frames of the loading screen (C4c3290), then module loading
  (`C4c3290::advance` -> load_data_files / load_module_files) up to `Skin::read` (skins.txt).
- **Current crash** (ASan build): `tstring::equals` reading a garbage buffer in `Skin::read`
  (src/game/cls_map/progress_bar.cpp, the face-texture loop comparing `hair_textures[t]` with the read name). The
  hair_textures vector<tstring> elements hold a buffer pointer whose high half is uninitialised: something writes these
  tstrings (or the Skin around them) with a 32-bit layout. Suspects: the Skin array views of mesh_scene::read
  (inventory_panel/widget_text.cpp: vec_model / vec_cfg_pub / BigArray, std::vector<Skin>::_Insert_n 0x4fe520 from
  agent_vec.cpp), and the Skin copy constructor path.
- Main menu not reached yet; screenshots in work/port_shots/ show the window (white/black: nothing composed yet).

## How to iterate

- Build: `cmake --build build/port/macos-arm64 --target mb_native -j6` (the build dir has `-g`; a full build is ~25 s).
  After src edits that change which symbols a TU defines: `.venv/bin/python tools/port/link_tables.py --target macos-arm64`,
  rebuild, run it again (fixpoint).
- ASan build (finds most 64-bit layout bugs directly, with the allocation site):
  `build/port/macos-arm64-asan` (configured with `-g -fsanitize=address`), run with
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

## Known hazards still in the tree (not yet hit)

- aliases.ld binds some names to instances of another element type: `MeshVec::operator=` and `rgl::ptr_list::operator=`
  -> `std::vector<int>::operator=` (copies 4-byte ints where the vectors hold pointers), `IntVec::insert_n` -> vector<int>.
  At 64 bits these need real typed definitions.
- Calls through casted function-pointer types with a struct return (the IDENTITY_VALUE pattern) break on arm64 (x8).
- Padded "views" of real classes (`char padNN[...]` before a pointer or tstring) and raw `(char*)p + 0xNN` accesses
  (ticket 022) are the bulk of what is left; ASan finds the heap ones.
- Views whose slot 0 is a plain virtual where the real class has a virtual destructor (Itanium uses two slots).
- Teardown paths (Pool<T>::~Pool, GameData::world_free) are untested.

# src/game/triggers migration: module-file record loaders, option dialogs, device-object handlers (0x4be742..0x4c2c50, 0x4ce870..0x4ce960)

Region of the TU map (`work/migrate/tu_map.csv` TUs 96 trigger_vec, 97 game_vec, 98 trigger_entry_vec, 99 big_obj and 105
item_rec). The staging slices in `match/slices/` (s004be310 tail, s004beb80, s004bf170, s004bf870, s004c03b0, s004c05b0,
s004c17c0, s004c2170, s004c2b20 head, s004cdfd0 tail) are unchanged and stay the reference. Both modules build with the default
flags `/O2 /MT /Gy /TP` (the slices' `/EHa` rows are not needed: cl 13.10 emits the try/catch of 0x4c1b60 without `/EH`).

## Summary

| module | range | rows | exact before (slices) | exact after | nonmatching | partial |
|---|---|---|---|---|---|---|
| trigger_vec.cpp | 0x4be742..0x4c2c50 | 56 (55 functions + the catch funclet 0x4c1bcc) | 30 | 40 | 16 | 0 |
| item_rec.cpp | 0x4ce870..0x4ce960 | 3 | 1 | 1 | 2 | 0 |
| **total** | | **59** | **31** | **41** | **18** | **0** |

(The map lists 59 rows in the two ranges: 42 + 5 + 6 + 3 + 3. The slices had 5 exact rows per the map's `exact` column but their manifests
list 31; the map's counts were stale.) Regressions: none, the 31 byte-exact slice rows are byte-exact here. No row was partial and none is.

Improved (slice status -> module status), 10 rows, all now MATCH: 0x4bf5f0 `options_dlg_proc` (a nested `switch (LOWORD(wParam))` gives
the original's `dec eax` form), 0x4bf640 `CreateGameOptionsPage::update_checks` (the original sets the radio buttons with
`BM_SETSTATE`, 0xf3, not `BM_SETCHECK`; ends in the tail jump to `refresh` 0x4b9e70), 0x4c0520 `load_game_variables`, 0x4c17c0
`build_shader` (rewritten from the disassembly: `FindClose(handle)`, `D3DXCreateEffectFromFileA`, the gamma vector copied through
an integer register), 0x4c1940 `Core::RestoreDeviceObjects` and 0x4c1a60 `Core::InvalidateDeviceObjects` (the slice had the right
frame but the wrong calls: `ShowCursor`, `SetCursorProperties`, `GetDeviceCaps`; the buffer-pair loop compares as signed ints),
0x4c1be0 `ini_file::ini_file`, 0x4c20c0 `MapIcon::read` (the slice read one value; the function reads eight fields), 0x4c2300
`MissionTemplate::read`, 0x4c23f0 `PresentationRec::read` (the slice had placeholder "???%s" strings and wrong reads; the real strings
are "Mission Template %s", "Presentation %s", "Map icon %s", "Game menu %s", "Error: Unable to load %s").
Nonmatching rows that got much closer: 0x4c03b0 301 -> 2 bytes, 0x4bf170 335 -> 218, 0x4c2b20 280 -> 117 (now `MenuRec::read`),
0x4c05b0 3432 -> 2891, 0x4bf870 2377 -> 2183 (the dialog is a different function from the slice's, see below).

## What the slices got wrong (verified against the disassembly)

The slice sources invented layouts and sometimes whole behaviours; every function was re-derived from the disassembly where the
slice's account did not hold:

- **0x4bf870** is not a "script debugger" with a `GameDebugState` laid out by sequence. It is `Core::script_debugger_proc` (0x4c3330
  jumps to it with `ecx = g_app_obj`, dialog handle in the object at +0x378). Its state sits in the game-config object
  `g_config` (0x5e2930): +0x1a060 file slot, +0x1a068 pause toggle, +0x1a06c selected script text (tstring), +0x1a07c selected index,
  +0x1a080 the module_scripts.py text (the slice's "debugger_file at +0x1a08c" is that string's length). The handled messages are
  WM_INITDIALOG, WM_COMMAND and WM_SIZING (0x214, not WM_MOVE); the viewer is a rich-edit control (EM_FINDTEXTEX 0x438, EM_EXSETSEL
  0x437, EM_SETCHARFORMAT 0x444: the slice used EM_SETSEL for all three); the script list for control 0x46c is parsed out of the
  `[ ... ( ... ) ... ]` body (CB_INSERTSTRING 0x14a / CB_SETITEMDATA 0x151).
- **0x4c20c0** reads eight fields (name, flags, mesh name, four numbers) and then a `GameVec`; the slice read one value.
- **0x4c25e0** is the music monitor (`music_player::update`): two stream slots, FMOD open-state / play / length / time, track selection by situation mode and
  last-played time, random retrigger. The slice's `BigObj` put the 0x100 track records at +0x18 and called `GetTickCount` where the call is `rand` (0x536a9f).
- **0x4c2250** is cdecl (the stream is the first stack argument, the vector the second) and reads the count into `this+4`; **0x4c2170** and **0x4c2200** work on the
  0x30-byte `RecH` (name, pointer / count / flags / name pair, level), not on an invented `TriggerEntry`.
- **0x4c1b60** is `std::uninitialized_copy` with the `Alloc204` allocator (0x4c1b40 is `Alloc204::construct`, 0x4c1b20 `Alloc2c::construct`); its
  catch funclet is 0x4c1bcc.
- **0x4c1c90 / 0x4c1e10** take a log-name argument (`resize(stream, name[, extra])`, `ret 8` / `ret 0xc`), passed on to `trigger_rec::read` / `trigger_entry::read`.

## Boundaries

Forward-call rule re-checked on `work/migrate/facts.json` over 0x4be700..0x4c2c60 and 0x4ce860..0x4ce970: **the only forward call that
leaves a function inside the region is 0x4be730 -> 0x4be742, the jump over an 18-byte WinLicense marker (`EB 10 "WL  " ...`) at the entry of
`MetaMission::init`**; the other "forward" targets are the FMOD / d3dx9 import thunks at 0x5342e2..0x53438a and the marker jump at
0x4be7bc. So the map's `high` boundary at 0x4be742 is a false positive (fixed here: 0x4be730 / 0x4be742 are one function, listed as two
nonmatching rows as in the slice), and there is no call evidence for the other three map boundaries (0x4c1e10 TriggerVec|GameVec,
0x4c2170 MouseVec|TriggerEntryVec, 0x4c24c0 TriggerEntryVec|BigObj; class changes only, "low"). The B regime holds in the region (helpers
precede their callers: 0x4c1c20 before 0x4c1c90, 0x4c1ee0 before its users, 0x4c2170 before 0x4c2250).

**TUs 96-99 are one module, `trigger_vec.cpp`.** The class names the map used as boundary signals fail here the same way as in
rgl/shapes: `TriggerVec`, `GameVec`, `MouseVec`, `TriggerEntryVec`, `BigObj` are the slices' guesses for classes that are other, existing ones
(`loaded_psys_table`, `RecH`, `music_player`, `DialogMgr`, `ini_file`, ...), and the region interleaves unrelated helper groups (copy loops,
registry helpers, two dialog procedures, the module_info parser) that no call separates. The start is exactly the marker function and the end
is 0x4c2c50, where `C4c2c50` (src/rgl/mesh2/rec_tables.cpp) starts; the last function here, 0x4c2b20 (`MenuRec::read`), has the map's
window 0x4c2b20..0x4c2c50 for the start of the next TU, so it could belong to either side.
**0x4ce870..0x4ce960 is a separate module, `item_rec.cpp`** (three functions, 0x4ce960 `RecBlocker` starts the next one in src/game/options_panel):
the range is 0x4ce870, far from the first one; the map's boundaries are class changes only.

## Classes: canonical headers used, extended and worked around

Included: `src/game/tstring.h`, `src/game/strat/gui_manager.h` (streams, `g_log`, `g_time`, `mb_error`), `src/game/config/core.h` (`Core`),
`src/game/config/config.h` (`GameConfig`, `CreateGameOptionsPage`, `trigger_rec`, `trigger_entry`, `music_player`, `loaded_psys_table`, `ini_file`,
`menu_item`, `resource_manager_x`), `src/game/party/game.h` (`TriggerRec`), `src/rgl/xmlcfg/records.h` (`RecH`), `src/rgl/mesh2/rec_tables.h`
(`Alloc2c`, `Alloc204`, `clamp_float3`, `g_music_volume`), `src/rgl/mesh2/head_rec.h` (item_rec.cpp), `src/rgl/resource.h` (`texture`),
`src/game/shadow_map.h` (`ShadowMap` for the object at 0xa70248), `src/game/render_batch.h` / `dev_buffer.h`.

Pure additions to canonical headers (offsets unchanged; each cites the function that proves it):
- `core.h` `Core`: the pad +0x348..+0x590 is split into `font` (+0x348: 0x4bed90, 0x4c1940), `page_hwnd` (+0x374: 0x4bf640), `script_dlg`
  (+0x378: 0x4bf870), `options_hwnd` (+0x380: 0x4bf170, 0x4bee10), `module_names[0x200]` (+0x390: the 0x20 tstrings 0x4bed90 destroys);
  declared members `RestoreDeviceObjects` / `InvalidateDeviceObjects` / `DeleteDeviceObjects` (vtable 0x5948e0 slots 6, 9, 10: 0x4c1940, 0x4c1a60, 0x4c1ad0),
  `read_options` 0x4bf170, `show_options` 0x4bee10, `script_debugger_proc` 0x4bf870. (`mode.Format` at +0x59c is the `mode` the slice called an int.)
- `config.h`: `GameConfig` pad split (+0x1a060 debug_file, +0x1a068 paused, +0x1a06c script_text, +0x1a07c script_index; +0x1a098 / +0x1a0cc / +0x1a0d0 check
  sources of 0x4bf640); `CreateGameOptionsPage::update_checks` 0x4bf640; `trigger_entry` gets the clock pair at +0x20 and the constructor 0x4c1de0 (size 0x30 is the
  record the loaders allocate); `loaded_psys_table::clear` 0x4c1f20 / `resize` 0x4c1f70; `ini_file` constructor 0x4c1be0 and destructor 0x4be920 (the slices' "small
  table" / "big_table": the 0x1000-entry key/value tables at 0xa18238 and 0xa44240); `music_player` constructor 0x4c24c0 and `update` 0x4c25e0;
  `resource_manager_x` gets `get_texture`, `reload_textures`, `reset_effect_sets`, `restore`, `invalidate`, `linearise_meshes`, `load_pack` (declarations only).
- `game.h` `TriggerRec`: the placeholders become the real 0x78-byte layout (`f0 f4 f8`, `f0c`, two `DialogBlock`s at +0x10 / +0x2c, three clock pairs at +0x48 / +0x58 / +0x68,
  `f50 f60 f70` kept) and the constructor 0x4c1c20 is declared; game.cpp still compiles and is byte-exact (22 of 22).
- `tstring.h`: `read_from` 0x4151e0 (declared with `struct _iobuf*`, the type of `FILE`).

New classes in this directory (`trigger_vec.h`): `MetaMission` (0x4be742), `code_table` (0x4be7c0), `vec_e50 / vec_e1f8 / vec_eb0` (the element-count
helpers), `clip_pair`, `blk2c / blk204`, `TriggerVec`, `GameVec`, `MapIcon`, `MissionTemplate`, `PresentationRec`, `MenuRec`, `TriggerEntryVec`, `SpawnRec` (a view for
the 0x4c-byte records `load_extras` 0x4bdd50 reads; config.h declares that method on `psys_record`), `MouseRec` (a second view of `psys_record` with the members its
array constructor and destructor touch: the exact bytes of 0x4c1f20 / 0x4c1f70 need `mstr`, a tstring whose constructor / destructor stay out of line). `item_rec.h`:
`item_head` (the head record with its virtual destructor 0x4c4c90; `head_rec` of mesh2 only models the vptr store and stores +4 before the derived vptr, which
costs the deleting destructor 0x4ce930 its exact bytes), `ItemRec`, `ItemRec2`, `owned_block`.

Worked around, not changed (found while verifying):
- `config.h` declares `psys_record::read` (0x4bdc00) as a thiscall member; the original is cdecl `(stream, record)` (this file declares `read_psys_rec`).
  `psys_record::load_extras` (0x4bdd50) is applied to 0x4c-byte records here, not 0xc0-byte ones.
- `dialog_manager.h` cannot be included with the stream.h-based headers (own `stream_base`, `g_game`): the `DialogMgr` constructor 0x4c2470 is written on the raw object.
- Several headers define their own `resmgr` view; `rgl/core/resmgr.h` cannot be included with `rec_tables.h` (`g_log`).
- `head_rec` also exists in `src/game/options_panel/trigger_records.h` (another agent's in-progress header) with a different base constructor.

## Tricks kept / removed

Removed: the slices' hand-declared Win32 and D3D prototypes (real headers now), the `tstring` / `stream` / `logger` look-alikes, the `__fastcall` wrappers around tstring members,
the `Block44` / `Block204` assignments (now `Alloc2c` / `Alloc204`), `_atol`, the explicit `EditorPanel` / `GameApp` / `Application` classes (the object is `Core`).
Kept: `core_dtor` writes the vtable pointers 0x5948e0 / 0x549c60 and calls `~CD3DEnumeration` by hand (the base destructor of CD3DApplication is out of line, the original
inlined it); the `(stream_base&)g_log` cast, which keeps cl from devirtualising the log calls; `V2() {}` / `mstr` for the inline array constructors; `ini_file::~ini_file`
is the empty destructor (the member destructors generate the original's loop).
Not reproducible from C++: the 18-byte WinLicense markers in `MetaMission::init` (0x4be730), `music_player::music_player` (0x4c2542, 0x4c25c1) and `parse_module_info` (0x4c05ff).

## Verification

See the report in the commit message: `chk.py` on every VA, `run_all.py` (40 / 40 and 1 / 1), `compile_all.py` (src/game: 70 of 71, the failing file is
src/game/world_data/world_data.cpp of another agent; src/rgl: 32 of 32), the resolution audit (41 agree), and the equivalence sweep of the 18 nonmatching rows.
Dependent modules re-checked after the header additions: src/game/config (48 + 9 + 2 + 7 + 3 rows) and src/game/party/game (22) are still byte-exact.

## Equivalence sweep results (nonmatching rows)

PASS: 0x4be742, 0x4c1c20, 0x4c24c0, 0x4c2200. WEAK (no mismatch): 0x4bf870, 0x4c03b0, 0x4c2250.
UNSUPPORTED (imports or vtables not modelled): 0x4bf170 (GetDlgItem), 0x4c1b60/0x4c1bcc, 0x4c1c90, 0x4c1e10,
0x4c2170, 0x4c2b20 (allocator/EH shapes), 0x4c25e0 (FMOD), 0x4ce870/0x4ce8d0 (own-copy vtable).
FAIL: 0x4c05b0, 5/341 inputs, ours reaches _purecall after the tstring::equals chain; recorded in the row's reason.

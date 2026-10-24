# src/game/config migration: UI tail, widget manager, options dialog, config/ini (0x48baa0..0x48be00, 0x499830..0x49b1c0, 0x4b8f40..0x4be730)

Three runs of the TU map (`work/migrate/tu_functions.csv` TUs 76 (tail), 88-90, 94, 95) are moved from `match/slices/`
staging into five translation units. The slices are unchanged and stay the reference. Every module builds with
`/O2 /MT /Gy /TP` (one flag set, no per-row flags, no `/EH*`). The ranges between them (0x48be00..0x499830,
0x49b1c0..0x4b8f40) belong to other migrations.

## Summary

| module | range | functions | exact before (slices) | exact after | nonmatching | partial |
|---|---|---|---|---|---|---|
| ui_widgets_c.cpp | 0x48baa0..0x48be00 | 4 | 2 | 2 | 1 | 1 |
| widget_mgr.cpp | 0x499830..0x49ad20 | 10 | 6 | 7 | 3 | 0 |
| widget_node.cpp | 0x49ad20..0x49b1c0 | 4 | 1 | 3 | 1 | 0 |
| options_dlg.cpp | 0x4b8f40..0x4b9e70 | 12 | 8 | 9 | 3 | 0 |
| config.cpp | 0x4b9e70..0x4be730 | 62 | 45 | 48 | 12 | 2 (+1 not-code row) |
| **total** | | **92** | **62** | **69** | **20** | **3 (+1)** |

(Row counts of the txt files: 69 manifest rows, 20 nonmatching, 4 partial; 69 + 20 + 3 = 92 functions, the fourth partial
row is 0x4be520, which is string data and no function.)

Regressions: none (the 62 VAs that are byte-exact in some slice are all rows of the manifests; checked by diffing the VA sets).
Improved to byte-exact (7): 0x49ab80 `apply_preset_b`, 0x49b120 `NodeVec::push_back`, 0x49b170 `NodeVec::insert`, 0x4b9a30
`container_prop_string_impl`, 0x4badf0 `Core::choose_display_mode`, 0x4bdff0 `sound_def::save`, 0x4be310 `menu_item::read`.
Reasons: 0x4badf0 is the SDK `FindBestFullscreenMode` with the configured display mode standing in for the desktop mode,
written against `dxframework/d3denumeration.h` (the slice used an invented `carray`/`Module`/`MeshObject` view; the
depth-format list, not a "shader type list", feeds the rank); 0x4be310 had its two printf argument orders swapped and its
`tstring` temporaries unscoped; 0x4bdff0 walks its six entries from `this+0x48`; 0x4b9a30 is a four-argument `__stdcall`
(`ret 0x10`, not "a never-read fourth argument") with external linkage.
Bugs found in the previous source and fixed (all in nonmatching rows): `choose_dx_settings` called `GlobalMemoryStatus(NULL)`,
used an uninitialised `WCHAR*` as the property buffer, wrote globals where the original writes through its `Config*`
argument (the signature is `(Config*, bool)`, `ret 8`), and returned on failure without releasing the COM chain;
`layout_text_to` swapped the text bounds in one branch (the branches differ only in the font object); `apply_preset`
called the `refresh` slot (+0x90) where the original calls slot +0xc; `menu_item::read` swapped printf arguments.

## Boundaries

Forward-call rule re-checked on `work/migrate/facts.json` (calls and tail jumps; WinLicense marker jumps are not calls):
- 0x48baa0 start: `UiD::key` (0x48b960) calls `set_value` forward (high). The map's TU 76 `ui_d_3` is cut at 0x48be00 (the
  stat panel starts there, another migration).
- 0x49ad20 start: `DropDown::on_event` 0x49aca0 calls `create` forward (high). `create` -> `push_back` 0x49b120 -> `insert`
  0x49b170 -> `_Insert_n` 0x49b1c0 are further forward calls, but all of them are `std::vector<NodeA*>` members (template
  COMDATs land after their first caller); 0x49b1c0 and later belong to the next range. widget_mgr (0x499830..0x49ad20) is
  split from widget_node at the proven edge.
- 0x499830 start: class change only (obj_tbl | widget_mgr), low.
- options_dlg | config at 0x4b9e70: the map cuts at 0x4b9b90 (class change OptionsDlg | Core, low). `choose_dx_settings`
  0x4b9b90 is an OptionsDlg method, so the dialog object runs to 0x4b9e70 (`CreateGameOptionsPage::refresh`); kept.
- 0x4b9e70..0x4be730: no call leaves the range forward (the only forward calls are the FMOD/d3d9 import thunks), so it is
  one module although the content changes class many times (Core probes, option block, cursor, DirectInput, render batch
  buffer table, ambient particles, camera ray, ini file, record readers). Weak evidence; kept as one larger module as asked.
  Within it 0x4bd830..0x4be730 (record readers) could be a separate object (no call evidence either way).
- 0x4be730 end: the WinLicense marker function (`meta_mission::init`, jmp +0x10 over `WL  ` data) starts the next object.
- 0x4bc5b0 (`init_formats`) contains a WinLicense marker pair (18 bytes each end) that cl cannot emit; that is its diff.

## Headers (one per class)

| header | classes | verified by |
|---|---|---|
| core.h | `Core : CD3DApplication` (g_app_obj 0x8ee3c48, `mode` +0x590) | 0x4bace0, 0x4bad70, 0x4badf0, 0x4b99c0 |
| config.h | `Config` (g_cfg 0x5a9f98), `GameConfig`, `CreateGameOptionsPage`, `Cursor`, `DisplayState`, `InputSet`, `dev_buffer`/`buffer_pair`/`render_batch`, `weather`, `camera_ray`, `ini_file`, `Elem`/`Block`, `trigger_rec`/`trigger_entry`, `psys_record`, `prsnt_record`, `sound_def`, `sound_record`, load tables, `music_player`, `menu_item`, `rec34`, `rec20`, `clip_table`/`clip_slot` | each field cites its function |
| options_dlg.h | `OptionsDlg`, `byte_buf`, `http_result`, HTTP/registry helpers | 0x4b8f40..0x4b9af0 |
| widget_mgr.h | `widget_mgr`, `DropDown`, `NodeA`, `UiAInline`, buttons, `MissionUiText`, `Font` | 0x499830..0x49b0f0 |
| ui_widgets_c.h | `UiEBase`, `UiE` | 0x48bb10..0x48bda0 |

Existing headers reused by inclusion: `../tstring.h`, `../strat/gui_manager.h` (streams, `g_log`, `mb_error`, mouse
globals), `../../rgl/{rgl_math.h,resource.h}`, `../../dxframework/*` (through core.h), `../ui/{ui_widgets.h,mission_ui.h}`.

### Conflicts with headers outside this directory (not edited, reported)

- `Block`/`Elem`: `src/game/dialog_manager.h` defines both, but also its own `stream_base`, which collides with
  `rgl/stream.h` (used by gui_manager.h for `g_log`); config.h repeats the 0x1c-byte `Block` with the copy constructor of
  0x4bd7a0 (`Elem::Elem` out of line, 0x4607d0). Merge into one header before the skeleton.
- `dev_buffer`/`buffer_pair`: `src/game/renderer.h` has the first 0x48 bytes of the same record (no `name` at +0x18, no
  `decl` at +0x48); `render_batch` here is a fuller view of the object `src/game/terrain_b/render_batch.h` models (`vf` +0x14,
  `cur_*` +0x3b4; this file adds `decls` +0, the index pair +0x30c, shader slots +0x418818).
- `Config`: `src/rgl/resource_manager.h` declares `rgl_config` (global 0x5a9f98) opaque with only `save()`; config.h
  names all 0x1b dwords. Same object.
- `weather` is a view of the UI root object of `strat/gui_manager.h` (`meshA/meshB[32]` at +0x1728/+0x6328 are `first/second`).
- `GameConfig` (pointer at 0x5e2930) is another partial view of the game object (settings text +0x1a080, mode +0x1a094,
  list state +0x1a0b8, sub-option +0x1a1d0); other modules declare their own.
- `Settings` (0x42f190 `get_float`): `src/rgl/mesh2/gui_edit.h` has a different `Settings`; config.h only needs the member.
- Names changed against the slices: `Core` (was an invented layout) is `CD3DApplication`-derived; slice symbols `renderer`
  (0x4bc830/0x4bcfb0/0x4bd0c0) and `render_batch` (0x4bc5b0) are one class `render_batch`; the slice's `sound_record::save`
  (0x4bdff0) is `sound_def::save` (a different record from the 0x98-byte `sound_record` that 0x4be200/0x4be270 use).
  The slices' stream method names in 0x4bd830..0x4bdf60 were off by one slot: the "r_bool" reads at +0x38 are `r_float`
  (the fields are floats; 0x4bd990 multiplies one by pi/180).

## Verification

- `chk.py` on every `// @ 0x...` VA: 69 MATCH (rows in `<module>.manifest.txt`).
- `run_all.py --manifest`: 2/2, 7/7, 3/3, 9/9, 48/48 byte-exact.
- Resolution audit (`resaudit_all.py --slices src:...`, `work/migrate/scratch_game_config_audit.jsonl`): 69 rows, 69
  agree, 0 disagree. Six "local copy" notes: the `std::vector::_Insert_n` instances called by `widget_mgr` factories
  (original 0x4927d0) and by `NodeVec::insert` (0x49b1c0) live in other objects.
  The audit found one wrong global in the slices (0x4bb260 reads 0x8ee3fbc, not 0x8ee3fc0: `g_hwnd_page`).
- Equivalence on the nonmatching rows (`equiv.py <module> <va> --src ... --flags "/O2 /MT /Gy /TP"`), 0 FAIL among rows kept as nonmatching:
  - PASS: 0x4bba90, 0x4bc5b0, 0x4bd120.
  - WEAK (no mismatch): 0x4bd170, 0x49abc0 (93 valid inputs), 0x49aca0.
  - UNSUPPORTED (harness limits, reasons in the txt): 0x48bda0, 0x49a300, 0x49ad20, 0x4b8ff0, 0x4b9110, 0x4b9b90, 0x4bb3f0,
    0x4bc830, 0x4bd990, 0x4bdb60, 0x4bdc00, 0x4bdec0, 0x4be0e0, 0x4be270.
  - FAIL, hence partial: 0x48bb50 (custom-ABI `decode_utf8` fake), 0x4bd2f0 (uninitialised locals read when ortho == 0),
    0x4bd4d0 (stale upper bytes of a dword-passed char). 0x4be520 is string data.
- Symbol lines for the audit/equiv resolver: `symbols/slices/{config,options_dlg,ui_widgets_c,widget_mgr,widget_node}.txt`
  (globals outside sibling headers, the DirectInput/DXDiag GUIDs, `tstring_ctor/dtor`).

## Kept slice-local tricks, with reason

- `static` `user_path` / `screenshots_path` plus the keeper `keep_paths` (not in the binary, never link it): the originals take
  their argument in ESI, a convention cl only picks for internal-linkage functions, yet other objects call them.
- `tstring_ctor`/`tstring_dtor` as `__fastcall` externs on the raw `clip_slot_raw` array (0x4be6b0/0x4be6f0).
- The 6-instruction `decode_utf8` asm helper (custom register ABI of 0x487b00), as in `../ui/ui_widgets_a.cpp`.
- `((stream_base&)g_log).w_cstr(...)`: stops cl devirtualising the call on the global.
- `Config::value_as_int` / `split_line` are thiscall members (`this` unused): callers load ECX before each call.

## Addendum (M4 fix-up): 0x4be730
The last entry of the TU (0x4be730..0x4be742) is not a function: it is the 18-byte WinLicense entry marker (`jmp +0x10` over
`"WL  "` data) of `MetaMission::init`, whose body and row are 0x4be742 in `src/game/triggers/trigger_vec.cpp`. Added to
`symbols/not_functions.txt`; `config.cpp` is unchanged.

# src/game/cls_map migration: 0x4d92c0..0x4e87d0 (TU map 107 cls_map, 108 progress_bar)

The region (117 functions, T regime) is moved from the `match/slices/` slices (s004d8c20 tail, s004d9450, s004d9aa0,
s004da410, s004dada0, s004db660, s004dbf7e, s004dc630, s004dcef0, s004dd830, s004de1b0, s004de6f0, s004dfd50, s004e05c0,
s004e25a0, s004e2ea0, s004e3780, s004e40f0, s004e4790, s004e4c70, s004e55f0, s004e5f80, s004e65e0, s004e6a70, s004e75a0,
s004e7b30, s004e85d0 head) into two translation units. The slices are unchanged. Flags `/O2 /MT /Gy /TP` for both.

## Summary

| module | range | functions | exact before (slices) | exact after | nonmatching rows | partial |
|---|---|---|---|---|---|---|
| cls_map.cpp | 0x4d92c0..0x4e3eb0 | 76 (incl. 8 EH funclets) | 29 | 34 | 42 (8 funclet rows, 0 diff) | 0 |
| progress_bar.cpp | 0x4e3eb0..0x4e87d0 | 41 (incl. 2 EH funclets) | 14 | 22 | 19 (2 funclet rows, 0 diff) | 0 |
| **total** | | **117** | **43** | **56** | | |

No regressions: every VA that was byte-exact in a slice manifest is byte-exact in a module (VA sets diffed). Every one of the
117 VAs had slice source and still has source (the two 0x4e85d0 / 0x4e8720 functions had slice source although the TU map says
"missing"; the char_sheet module dropped them as out of range). Nothing became partial.

Newly exact (13): 0x4d9370, 0x4d9390, 0x4d93e0, 0x4d9400, 0x4d9420, 0x4e3f20, 0x4e4060, 0x4e40f0, 0x4e41e0, 0x4e42b0,
0x4e42f0, 0x4e4340, 0x4e4690 (plus 0x4e4320, 0x4e5f50, 0x4e5f80, 0x4e5fc0, 0x4e51e0, 0x4e5230, 0x4e5270, 0x4e54e0, 0x4e5570 now
being real library instances or faithful ports; they were already exact). The key discovery: explicit whole-class
instantiations (`template class std::vector<E50>; <E68>; <E176>; <int>; <E20b>; <RagModel>; <loading_config>`) reproduce the
std::vector members the linker placed in these objects byte-for-byte (`_Insert_n`, `_Ufill`, `operator=`, `push_back`,
`resize`, `_Uninit_copy` with its EH unwinder, `_Uninit_fill_n`, `fill`, ...). The slices had hand-written look-alikes.
Faithful ports that became exact: 0x4e4690 (`RagModel::load`, with the external E50 constructor 0x4cd7a0), 0x4e5270
(`SaveWindow::list_saves`, with the real stream/reader view), 0x4e3f20.

## Boundaries (evidence)

| boundary | evidence |
|---|---|
| 0x4d92c0 (start) | `GlyphMap::at` follows the previous module's last function (options_panel, TU 106); the TU map class change. Not touched. |
| 0x4e3eb0 (cls_map | progress_bar) | class change only (low confidence): `progress_bar` starts here (its constructor stores the vtable 0x597f20); no forward call crosses it. The forward call 0x4da900 -> 0x4e43e0 (`gui_manager::rebuild` -> `draw_string`, both gui_manager members) is a TU-107 function calling a TU-108-range function, i.e. it argues the split may be later (T regime: forward calls prove nothing); the split is kept at the class change. |
| 0x4e87d0 (end) | `MapScreen::buttons` (char_sheet module) starts the next TU. The slice s004e85d0 straddles it; only 0x4e85d0 and 0x4e8720 are taken here. |

T regime: helpers defined after their caller are kept out of line (`__declspec(noinline)` on the declarations of
`new_sized_button`, `make_short_button`, and on `decode_utf8`), functions in binary order. WinLicense markers (`EB 10 'WL  '`
18-byte records, which cl cannot emit) are inside 0x4d9450 (WinMain) and 0x4e7a70 (MapLoader::read); both stay nonmatching.

## Headers

New / extended canonical headers (pure additions; each is reported to the other agents through this list):

| header | change | proving function |
|---|---|---|
| `src/include/vec_elems.h` (NEW) | `R44`, `b16`, `E50` (+ `E50::load` 0x4d3330), `E176`, `E516`, `NamePtrs56`, moved out of `src/rgl/mesh2/mesh_2.h` (which now includes it; mesh_2.cpp compiles, 57/57 exact) | 0x4d9840.., 0x4e4690 |
| `src/game/font.h` | `Glyph` union key/f0, `GlyphMap` (`count`, `buckets`, `i20`, `i24`, `insert` 0x4d3c00, `at` 0x4d92c0), `Font` named fields (replacing `pad0[0x28]`), `Font()` 0x4d99d0, `load` 0x4d9aa0 (already in HEAD through another agent's commit) | 0x4d99d0, 0x4d9aa0 |
| `src/game/strat/gui_manager.h` | `line_wrap[0x50]` +0xcd0, `line_height[0x50]` +0xd20, `line_arg[0x50]` +0xe60 (replacing `padcd0`), `draw_string` 0x4e43e0, `rebuild` 0x4da900, `build_scene` 0x4e6050, `add_root` 0x49b120 | 0x4da900, 0x4e43e0 |
| `src/game/config/widget_mgr.h` | `WVt` slots `d11(int)`, `d30(const int*)`, `d31(int, rgl::mesh*)`, `d37/d38(const void*)`, `d43(void*, const void*)`, `d46()` | 0x4dada0, 0x4e05c0, 0x4dcef0 |
| `src/game/config/config.h` | `ini_file::get_hex(int*)` 0x4e51e0, `get_float(int*)` 0x4e5230, `hex_field(const tstring&)` 0x4e5d30 | the three exact functions |
| `src/include/msg_log.h` | `MsgLog::reset_geometry` 0x4e64b0, `update(float)` 0x4e6510 (already in HEAD through another agent's commit) | both exact |

Included, never redefined: `tstring.h`, `font.h`, `gui_manager.h`, `mesh2_common.h` (`LangTables`, `g_strtable`), `e68.h`,
`vec_elems.h`, `ptr_tag.h`, `timer.h`, `troop.h`, `party.h`, `game_data.h`, `config/{config,core,widget_mgr,ui_widgets_c}.h`,
`ui/{mission_ui,ui_widgets}.h`, `rgl/{mesh,resource,stream}.h`, `msg_log.h`, `tinyxml.h`.
`party/party_window.h` and `rgl/mesh2/mesh_2.h` cannot be included here (conflicts on `g_game` / `g_resmgr`).

Local views, reported (the header they mirror cannot be included):

* `UiFrame` (cls_map.h): vtable 0x5924a0 as in `party/party_window.h`.
* `OptionsMenu` (0xa73730, cls_map.h) overlaps `options_panel`'s `ControlKeyTable` / `KeysPage` and `GameKeys`.
* `MapScreen` (progress_bar.h): partial view of `char_sheet/save_game.h`; `Shape` (the map mesh at MapScreen+0x6698) is
  the class of the three builders 0x4e65e0 / 0x4e6a70 / 0x4e75a0.
* `RagModel` (name + `vector<E50>`) is what the slices called `vector16_pair` / `row20`; `E50` is options_panel's `RagBone`
  and mesh_2's `Obj7a0` (0x50 bytes).
* `FrameGui` (a view of gui_manager for 0x4dc350), `LoadingScreen` (0x4daa10), `SaveWindow`, `SavePanel`, `MapLoader`,
  `MapScene` (0x4e7b30, a semantic model).
* `MenuKindRec` has stride 0x24 (the sibling `hud.h` says 0x1c, which is wrong).

## Slice defects found while porting (fixed from the disassembly)

`make_hint(o, text&, x, y, w, h)`, `new_medium_button(parent, text*, a, b, c)`, `new_long_button`, `new_sized_button(parent,
text&, a, b, c, name&, down&, k1, k2, k3)`, `make_slider(parent, x, y, k)`, `make_dropdown(parent, name*, x, y)` had guessed
prototypes in the skeleton slices; `d43` takes (text*, pos[2]). 0x4daa10: window scale (1.002, 0.752), logo (0.4, 0.1), hint
constants. 0x4e4790 / 0x4e55f0: `SaveWindow::slot_button(parent, slot, out)` (the slice used `this->win`), `label_refresh` takes
its prefix by value. 0x4e43e0 is `gui_manager::draw_string(tstring*, float, int, char)` (argument order was wrong).
0x4db440: `" (%d/%d)"` takes (count - wounded, count) (the slice had them swapped).

**Caveats kept honest:** 0x4e05c0 (`OptionsWin::init`) is a skeleton whose constants do not match the disassembly (first
make_hint pushes 0.52, 0.7, 0.3, 0.035) and which calls placeholder helpers; it stays nonmatching with no equivalence claim.
0x4de6f0 (`OptionsScreen::open`) was a skeleton too (1949 of 5721 bytes); it was rebuilt from the disassembly on 2026-10-30.

## Verification

* `chk.py` on both modules (34/76 and 22/41 MATCH; every non-MATCH row is a nonmatching row).
* `run_all.py --manifest`: 34/34 and 22/22 byte-exact. `compile_all.py --filter src/game/cls_map`: 2/2.
* Whole COMDAT checks (`cmpobj.py`) of the parents 0x4d9ed0, 0x4da1f0, 0x4da300, 0x4db9d0, 0x4dbcc0, 0x4e4340, 0x4e5fc0 are
  byte-identical including their EH funclets, which therefore carry 0-diff rows.
* Rows whose manifest symbol is one of several byte-identical instances (ICF candidates) were named by the callers' original
  call operands in the resolution audit: 0x4d9370 `_Destroy<E50>`, 0x4d9390 `erase<E68>`, 0x4d93e0/9400/9420
  `_Ucopy<E50*/E68*/E176*>`, 0x4da840 `clear<E50>`, 0x4e4320 `allocator<loading_config>::construct`.
* Symbols: `symbols/slices/cls_map.txt`, `progress_bar.txt` (definitions, the original addresses of the instantiated helpers,
  `??_7progress_bar@@6B@` 0x597f20).
* Other modules re-verified after the header edits: compile_all for rgl/mesh2, game/config, triggers, agent3, xmlcfg, mesh_slot,
  char_sheet all compile; run_all for config, widget_mgr, ui_widgets_c, gui_manager, gui_edit, mesh_2, agent_combat,
  agent_fx_sound all byte-exact. `src/game/strat/tstring.cpp` currently fails on another agent's uncommitted
  `tstring.h` edit (duplicate `split`), not on anything here.

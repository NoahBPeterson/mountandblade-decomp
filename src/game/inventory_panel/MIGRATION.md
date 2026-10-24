# src/game/inventory_panel migration: tu112 (0x502cc0..0x51c5a0)

The 118 functions of the T-regime run `tu112` / `inventory_panel` of the TU map, taken from 38 slices of `match/slices/`
(`s00502cc0` .. `s0051be20`) into 17 modules. The slices are unchanged and stay the reference. All modules build with
the default flags `/O2 /MT /Gy /TP` (no per-row flags).

## Summary

| module | range | rows | byte-exact before (slices) | byte-exact after | improved | regressed | nonmatching | partial | form |
|---|---|---|---|---|---|---|---|---|---|
| party_screen.cpp | 0x502cc0..0x507400 | 11 | 2 | 2 | 0 | 0 | 9 | 0 | merged onto canonical headers |
| char_sheet.cpp | 0x507400..0x5093d0 | 1 | 0 | 0 | 0 | 0 | 1 | 0 | slice source carried over |
| char_window.cpp | 0x5093d0..0x509980 | 5 | 4 | 4 | 0 | 0 | 1 | 0 | carried over |
| view_open.cpp | 0x509980..0x50a1c0 | 2 | 1 | 1 | 0 | 0 | 1 | 0 | carried over |
| note_window.cpp | 0x50a1c0..0x50cc60 | 1 | 0 | 0 | 0 | 0 | 1 | 0 | carried over |
| widget_text.cpp | 0x50cc60..0x50d5e0 | 12 (10 + 2 catch-funclet index rows) | 4 | 4 | 0 | 0 | 8 | 0 | carried over |
| module_files.cpp | 0x50d5e0..0x50ec40 | 1 | 0 | 0 | 0 | 0 | 1 | 0 | carried over |
| game_app.cpp | 0x50ec40..0x50f440 | 3 | 2 | 2 | 0 | 0 | 1 | 0 | carried over |
| loading_screen.cpp | 0x50f440..0x50fc60 | 6 | 4 | 4 | 0 | 0 | 2 | 0 | carried over |
| skeleton_load.cpp | 0x50fc60..0x50fe20 | 1 | 0 | 0 | 0 | 0 | 1 | 0 | carried over |
| inventory_panel.cpp | 0x50fe20..0x513710 | 17 | 9 | 9 | 0 | 0 | 8 | 0 | merged |
| mission_ctx.cpp | 0x513710..0x516ac0 | 27 | 8 | 8 | 0 | 0 | 19 | 0 | merged |
| game_tick.cpp | 0x516ac0..0x5191e0 | 19 (18 + 1 padding row) | 7 | 7 | 0 | 0 | 12 | 0 | merged |
| map_update.cpp | 0x5191e0..0x51a2d0 | 1 | 0 | 0 | 0 | 0 | 1 | 0 | carried over |
| map_picker.cpp | 0x51a2d0..0x51abd0 | 7 | 2 | 2 | 0 | 0 | 5 | 0 | carried over |
| map_info.cpp | 0x51abd0..0x51be20 | 1 | 0 | 0 | 0 | 0 | 1 | 0 | carried over |
| map_tooltip.cpp | 0x51be20..0x51c5a0 | 3 | 1 | 1 | 0 | 0 | 2 | 0 | carried over |
| **total** | | **118** | **44** | **44** | 0 | 0 | **74** | 0 | |

The TU map's count (118) is reproduced: every address of `facts.json` in 0x502cc0..0x51c5a0 has a row in one of the
`*.manifest.txt` / `*.nonmatching.txt` files (0x51776f is the last int3 of the padding after `sun_color`, kept as a
score-nothing row, as in its slice; 0x50d3d9 and 0x50d473 are the two `catch` funclets of the `_Insert_n` COMDAT).
No improvement and no regression among the 44 byte-exact rows. Among the nonmatching rows the sizes and diff counts
equal the slice's, except: `place_slot_mesh` 0x510fa0 4241 -> 4210, `train_troops` 0x518d70 2332 -> 2347 (ours 1867
bytes vs 1883 in the slice; the original is 1126).

## Two kinds of modules

**Merged (party_screen, inventory_panel, mission_ctx, game_tick; 73 rows).** The slices defined the same real objects
under many slice-local struct names; they were rewritten onto the canonical headers (`game_data.h`, `party.h`,
`troop.h`, `mission.h`, `item_ref.h`, `tstring.h`, `str_table.h`, `msg_log.h`, `ui/mission_ui.h`, `config/config.h`).
`inv_common.h` holds the pure-addition VIEWS these modules need (`GameDataV : GameData`, `PsOps` / `PsFill` widget
vtable views, `GuiRoot`, `MainMenu`, `MapFlags`, `party_at`); each cites the function that proves it. Where a canonical
class lacks a field the modules read it by offset (`GD_I` / `GD_C` / `GD_F`, or the `FLD()` macros of game_tick.cpp).
The slice-era class names were wrong in several places and are renamed:

| slice name | what it is |
|---|---|
| `Mission` (0x502cc0..0x507310) | the party / prisoner management screen: `PartyScreen` (NOT `game/mission.h` `Mission`) |
| `InventoryPanel` / `SlotWindow` / `InvWindow` / `Panel` | one window object (= `PartyWindow`, `party/party_window.h`); `party_window.h` cannot be included together with `game_data.h` (both declare `g_game`), so `inventory_panel.h` has its own layout |
| `MissionCtx` / `Unit` / `MetaMission` / `BattleW` / `g_mission_ui` | the object at 0xa73300 (`MissionCtx`) |
| `Scene` | the canonical `Mission` (`g_mission` 0x5e292c); `Team` = `TeamRec` seen 4 bytes earlier; `Sub3` = `ClassInfo` |
| `Game` / `GameData` / `World` | `g_game` (0x5e2930) |
| 0x513710 | `MapFlags::commit` (the block at 0x5e291c) |

Slice-era mistakes found while porting (corrected from the disassembly and the `ret N` values): 0x5063e0 had the wrong
vtable layout and is `ret 0`; 0x50fe20 and 0x5108d0 are `ret 4`; 0x512960 `compute` leaves the price in eax (declared
`void` plus a declaration-only `compute_price` for the callers that read it); 0x510fa0 is a thiscall
`place_slot_mesh(slot, pt, page, index)`, not `__fastcall`; `Mission::refresh_party_window` takes no argument. Two
parameter / member shadowing bugs (`compute(mode)`, `can_transfer(mode)`) are renamed away. `0x516ac0` is
`GameG::tick`.

**Carried over (the other 13 modules, 45 rows).** Their slice sources define their own ad-hoc layouts (`rgl.h` of
`match/include`, plus `map_info_tstring.h` / `skeleton_tinyxml.h`, the copies of the slice-local `tstring.h` /
`tinyxml.h`, renamed so they cannot shadow the canonical headers). They are byte-for-byte the slice sources, so all
44 byte-exact rows stay exact and no number changed. This is the part of the region that is NOT yet on the canonical
headers: the classes `MapPanel` / `MapScreen` / `Camera` (one world-map screen class), `CharSheet` / `char_window`
(the existing `src/game/char_sheet/*.h` are partial and disagree with each other), `note_window`, `EditorPanel` /
`EditorDialog` (0x8ee3c48), `LoadingScreen` / `GameApp` are defined locally per module. Whoever owns the headers of
those classes should merge them; the module split below makes that a mechanical change.

## Boundaries

All 17 start addresses are slice starts. This is a T-regime run (callee after caller), so a forward call proves
nothing, and the only evidence is class changes between neighbouring functions, so the boundaries are WEAK (the
`tu112` run is probably a single object of about 110 KB): the module cuts are the class changes of the slices
(`PartyScreen` | `CharSheet`/`char_window` | `note_window` | editor helpers | `GameApp`/`LoadingScreen` | `SkeletonBody::load` | inventory window |
`MissionCtx`/`Screen` | `GameG` | `MapPanel` | picker | map tooltip). Modules were kept separate where merging
means reconciling incompatible slice layouts of the same object (the carried-over ones) rather than where there is
evidence of a translation-unit boundary. Free gaps (int3 padding) sit between all neighbours, as for any COMDAT
sequence, so they prove nothing either.

- **0x51c5a0 is not in this region.** The TU map ends at 0x51c5a0, so `Panel::update` (0x51c5a0, 377 bytes, the last
  function of `s0051be20`) belongs to the next object. It is removed from `map_tooltip.cpp` (and its row from the
  nonmatching file); the slice `s0051be20` still holds it. Whoever migrates 0x51c5a0.. takes it from there (the
  slice reports it nonmatching, 119 bytes).
- **0x50d3d9 / 0x50d473** are catch funclets of the `_Insert_n` instantiation at 0x50d260 (index artifacts, not
  functions): `widget_text.nonmatching.txt` keeps them as in the slice.
- `GameDataV::f516ac0` of `inv_common.h` is `GameG::tick` (0x516ac0, game_tick.cpp), declared there for 0x4xxxxx callers.
  `mission_ctx.cpp` still calls the 0x5191e0 function `f5191e0()` (the map-screen `update_target`, `map_update.cpp`).

## Canonical headers

- Included unchanged: `game/tstring.h`, `game/str_table.h`, `game/item_ref.h`, `include/{game_data,party,troop,msg_log,
  item_kind,shape,game_recs}.h`, `game/mission.h`, `game/ui/mission_ui.h`, `game/config/config.h`.
- **One pure addition**: `src/game/item_ref.h` gets `void name(char* out);   // 0x512730` (`ItemRef::name`, proved by
  the byte-exact function in `inventory_panel.cpp`). Nothing else in a canonical header was touched.
- Not included, with the reason: `party/party_window.h` (the same window object as `InventoryPanel`; the two
  headers disagree on `g_game`), `char_sheet/*.h` (partial / conflicting), the `Mission` of `mission.h` (not the
  party screen).
- `rg "struct <Name>\b" src/` was run for `PartyScreen`, `InventoryPanel`, `MissionCtx`, `GameG`, `MenuWidget`: each is
  defined once, here. **Exception**: `Screen` (the world-map screen, `mission_ctx.h`, also redefined by the slice-copied
  `loading_screen.cpp` / `game_app.cpp`) shares its name with unrelated `Screen` structs in `cls_map/cls_map.h` and
  `char_sheet/screen*.h`; the layouts differ and nothing links them together yet. The owner of the world-map screen
  class should unify them (the methods `frame_update` 0x514d40 and `camera_move` 0x515d80 are nonmatching, so
  renaming costs no byte-exact row). Reported, not worked around.

## Tricks removed / kept

- T-regime order: helpers defined after their caller are `__declspec(noinline)` (macro `MB_NOINLINE`), binary order is kept
  in every module.
- Slice-local `#define`s and the register-pinning spellings that made `money_label`, `show_selected_character`,
  `data_ready` (64-bit flags test, locals `clk` / `start` for the load order), `pop_key` byte-exact are kept as is.
- Kept deliberately: `troop_name()` inline in party_screen.cpp (restores 0x507310), the `compute` / `compute_price`
  pair, the cast views `GameDataV`, `MissionV`, `PartyV` (derive from the canonical class only to add methods).
- Resolution: `symbols/slices/inventory_panel.txt` names the globals the audit could not resolve from the headers
  (`g_strtable`, `g_tick_scale`, `g_editor_dialog` / `g_panel` 0x8ee3c48, `label_refresh`, `ObjTable::make_mesh_from`).

## Verification

- `run_all.py --manifest` on every module: all rows byte-exact (44 / 44).
- `compile_all.py --filter src/game/inventory_panel`: 17 / 17 compile.
- Resolution audit of the 44 exact rows: 42 agree, 2 unverified: 0x50d250 `dialog_thunk` (a 3-instruction forwarder to
  `EditorPanel::terrain_dialog`, whose address is not in `symbols/`), 0x50d260 `_Insert_n` (five helper callees not
  verifiable; the slice has the same two unverified rows).
- Equivalence sweep of the 74 nonmatching rows: see the end of this file.

### Equivalence sweep (`sweep_nonmatching.py`, `--fp-tol`, full mismatch tally) of the 74 nonmatching rows

PASS 10, WEAK 17, UNSUPPORTED 46, FAIL 1 (each row's verdict is appended to its line in the `*.nonmatching.txt` files).

- The port found one real bug in this work: `GameG::train_troops` / `skill_plus_8` / `update_*` read the party through
  `party_at(&parties, id)` without the `PartyMgr::index_of(id)` call the original makes (FAIL, 191/568 inputs
  faulting at the first page lookup). Fixed (`party_at(&parties, parties.index_of(id))`, game_tick.cpp); the module now
  gets WEAK/UNSUPPORTED only.
- **0x5191e0 `update_target` (map_update.cpp) is FAIL under the sweep's seed and unresolved**: the source is the
  slice's, verbatim, which is WEAK (0 mismatches over 480 and 537 valid inputs) with two other seeds. One signature,
  `call #16 (depth 1)`: the original reaches `PartyMgr::index_of` where the source reaches `Party::reissue_order`;
  `--replay` of the saved input does not reproduce it. Recorded in the row; the owner of the map screen class
  should recheck the retarget branches (0x519945..0x519fdd).
- UNSUPPORTED rows are those the harness cannot emulate (unmodelled imports, giant functions); their reasons in the
  slices still apply. The per-row reasons of the merged modules are the slice's, quoted unchanged.

### Not done / open

- Modules `char_sheet`, `char_window`, `view_open`, `note_window`, `widget_text`, `module_files`, `game_app`,
  `loading_screen`, `skeleton_load`, `map_update`, `map_picker`, `map_info`, `map_tooltip` (45 rows) are slice sources
  carried over, not merged onto the canonical headers. Their classes (`MapPanel` / `MapScreen` / `Camera` = one
  world-map screen, `CharSheet` / `char_window`, `EditorPanel` at 0x8ee3c48, `GameApp`, `LoadingScreen`) remain
  defined locally; merging them with `Screen` of `mission_ctx.h` and with `src/game/char_sheet/` is future work.
- No nonmatching function was improved except `place_slot_mesh` (4241 -> 4210); `train_troops` is 3 bytes worse
  than its slice (2335 vs 2332; the code is 16 bytes smaller).
- 0x51c5a0 (`Panel::update`) belongs to the next object (see Boundaries).

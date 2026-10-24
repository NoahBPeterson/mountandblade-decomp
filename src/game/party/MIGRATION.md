# src/game/party migration

Regions: 0x501a20..0x502c30 and 0x51c5a0..0x51eaa0 (T regime, original TUs). Flags `/O2 /MT /Gy /TP`.
Verified: `run_all` 22/22 and 9/9 byte-exact, resolution audit 31/31 agree, equiv: no FAIL on any
nonmatching row except `NavGraph::solve` (moved to partial).

## Before / after
| module | range | functions | exact before | exact after | nonmatching | partial |
|---|---|---|---|---|---|---|
| game | 0x51c5a0..0x51ea10 | 38 | 20 | 22 (+`Game::init_map`, `read_soa`) | 15 | 1 (`NavGraph::solve`) |
| party_window | 0x501a20..0x502c30 | 13 | 8 | 9 (+`place_slot`, `add_strip_icon` kept) | 4 | 0 |

## Boundary decisions
Evidence is class names only; in the T regime forward calls prove nothing. Two modules: `party_window`
(the equipment/inventory window, one class family `PartyWindow`/`MissionUi`) and `game` (the `Game` object
load/save/reset, `NavGraph`, and the loading panel at 0x51c5a0/0x51c720, merged rather than split because
the panel is two functions with no data of its own). `refresh_stash` (0x502c30) is extern, owned by the neighbour.
NavGraph (`g_nav` 0xa732b8, nodes of 0x88 bytes) owns 0x51d3a0..0x51db70; the slices filed these under `Game`.

## Tricks
Kept: `__declspec(noinline)` helpers defined after callers, binary order, `volatile float` + cast in
`place_slot` (cl drops the store of an unused by-address argument), `*(volatile int*)&kind` in `add_strip_icon`
(only form that reproduced register order in the module), local annotated `g_item_count // 0x5e29d4`
(mangled-name collision in the audit), `symbols/slices/party_game.txt` for `g_troop_extra`.
Removed: slice-local `Panel`/`Rec78Tag`/`Rec508`/`Rec300` ad-hoc types.

## Canonical names
`Panel` -> `LoadingPanel`, `Rec78Tag` -> `TriggerTag`, `Rec78` -> `TriggerRec` (local), `Rec508` -> `TroopIO`,
`Rec300` -> `Faction`, nav functions -> `NavGraph`. Widget calls go through `WidgetOps` (local pure-interface view).

## Header conflicts (nothing outside src/game/party edited)
- `Game` is also `World` (ui/world.h) and `GameData` (dialog_manager.h, terrain_b/game_data.h): the 0x5e2930 object.
- `MissionUi` vtable slot names differ from `WidgetOps`; `ItemRef` lacks `stat_200`/`max_amount`; `ItemKind` lacks +0x208 and the sound sub-object at +0x228; `Troop` lacks name/read and +0x3c0/+0x400/+0x4f4; `rgl::mesh` lacks `translate`.
- PartyWindow's real base is the gui `widget` but its fields overlap that base's pad, so it is standalone.
- `Rec78` in troop_inv.h (sprite table) differs from `TriggerRec`.
- `?g_item_count@@3HA` collides with another slice's mapping to 0x8ed4f88.
- `tools/difftest/resaudit_all.py --help` is ignored, runs the whole audit and overwrote the shared `work/resaudit/all.jsonl` once (accidentally).

## Notes
- WinLicense markers (`EB 10 "WL  "`, 18 bytes) in `Game::Game`, `load_state`, `load_clock` cannot be emitted from C++; those stay nonmatching.
- Matching is fragile: `add_strip_icon` flips 0/3 bytes with unrelated header content.
- `make_inventory_menu` leaves `pos[7]` uninitialised, as the original does.
- Slice corrections: `build_page` scale is (0.264, height); slice omitted `label_refresh("inv_im_equipment_menu")`.

## Addendum (M4 fix-up): `PartyWindow::refresh_stash` 0x502c30 and `Game::load_scene_parties` 0x51eaa0
- `party_window.cpp` (now 0x501a20..0x502cc0): `refresh_stash` 0x502c30 added as the last function (byte-exact, 10 manifest rows);
  it was declared "next object" because the slice s00502300 listed it in the TU map's next range, but the TU ends at 0x502cc0.
- `game.cpp` (now 0x51c5a0..0x51eb90): `Game::load_scene_parties` 0x51eaa0 added as the last function. Nonmatching (177 bytes, 236 vs 233:
  the original keeps `this` in ebp with a 0xc-byte frame, ours uses edi/ebx). The slice's `r_int`/`r_raw` were slots 0x38 / 0x4c, which are
  `stream_base::r_float` (14) and `r_int` (19); the troop-flag gate is `troops[id].flags30 & 0x10` (a 64-bit test). equiv PASS (531 valid, 8/8 edges, bit identical).

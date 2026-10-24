# M4 agent5: `src/game/agent5` (tu110 "agent_8", 0x4f7d70..0x501a20, 42 functions, T regime)

Moved from the slices `s004f7d70 .. s00501940` (`match/slices` is unchanged) into four original translation units. 16 of 42 functions are
byte-exact (slices: 10); no function that was byte-exact in a slice lost its status, none became partial.

## Before / after

| module | range | functions | exact before | exact after | nonmatching |
|---|---|---|---|---|---|
| `agent_hud` | 0x4f7d70..0x4fe060 (end of the V4List helpers) | 27 | 5 (0x4f8bc0, 0x4f8c20, 0x4fd0e0, 0x4fd3b0, 0x4fd3d0) | 10 | 17 |
| `agent_vec` | 0x4fe0b0..0x4fe860 | 12 (5 of them funclets) | 5 (0x4fe120, 0x4fe190, 0x4fe4b0, 0x4fe520, 0x4fe840) | 5 | 7 (2 functions, 5 EH funclets) |
| `editor_dialog` | 0x4fe860..0x500ad0 | 1 | 0 | 0 | 1 |
| `party_create` | 0x500ad0..0x501a20 | 2 | 0 | 1 | 1 |
| total | | 42 | 10 | 16 | 26 |

Newly byte-exact: `Mission::init_agent` 0x4f7d70, `Mission::refresh_agent` 0x4f7e00, `MissionHud::update_equipment` 0x4f8b40,
`LogScreen::build_thunk` 0x4fc810, `IntList::buy` 0x4fd490, `PartyWindow::money_label` 0x501940 (the slice was 3 bytes off).
Fewer differing bytes than the slice (all still nonmatching): 0x4fdff0 158 -> 60, 0x4fe060 75 -> 66, 0x4fbfa0 1784 -> 1320, 0x4fc820 1966 -> 1724, 0x4fd1d0 403 -> 384.
More differing bytes than the slice (both nonmatching; the slice's body was a shortcut that is not faithful to the disassembly, this one is): 0x4fd700 2014 -> 2520,
0x4fb180 234 -> 382, 0x4fd150 21 -> 25, 0x4fd4f0 444 -> 449.

## Why four modules

`docs/tu_map.md` has one TU for the whole range, but the sources cannot be one translation unit:

* The HUD half needs the "rgl world (b)" headers (`rgl/rgl_math.h`, `rgl/mesh.h`, `mission.h`, `ui/mission_ui.h`). `agent_vec` needs the real
  VC7.1 `<vector>` headers with `/EHsc`-shaped instantiations (the slice's own conditions), so its element types cannot share a TU with the
  class headers (`CfgRec` / `RowRec` stand for `loading_config` and `UiRow`, see below).
* `editor_dialog` is one 8.4 KB dialog procedure over `windows.h` / `commctrl.h` / `d3d9.h` and `match/include/rgl.h` (world (a)); world (a) and
  world (b) define `rgl::vector4` twice and cannot be included together.
* `party_create` is the tail of the `PartyWindow` object family (`party/party_window.h`, g_game seen as `Game`). Evidence: `0x500ad0` calls
  `0x501a20`, `0x502320`, `0x5024e0`, `0x5025a0` (all `party_window.cpp`) and writes the offsets `party_window.h` documents (`pages` +0x964, `rows` +0x968,
  `container` +0x96c, `armor_labels` +0xa64, ...). The TU map puts the boundary at 0x501a20, so these two functions may really belong to
  the same TU as `party_window.cpp`; the evidence for moving the boundary is weak (forward calls prove nothing in T regime), so the TU map was
  left alone and they are a separate module that includes the party header.

Within each module the binary order is kept; helpers that follow their caller are `__declspec(noinline)` (`IntList::buy` before its caller
0x4fd150 in the file order of the binary, `V4List::insert`, the static `decode_utf8`).

## Boundary / layout decisions with evidence

* `MWidget` (the game-log / map-text list widget, vtable 0x5995c0, size 0x1d0) is NOT a C++ subclass of `MissionUi`. Its destructor 0x4fd3d0 ends in
  `jmp 0x487c00` (the out-of-line base destructor body) where a derived class of the canonical `MissionUi` inlines the implicit one
  (`lea ecx,[esi+0x128]; jmp ~tstring`). It is an opaque body (`MissionUi` size) with `U()` giving the `MissionUi*` view, built by
  `MissionUiBody::init` (0x48ae20) and torn down by `MissionUiBody::dtor` (0x487c00). That keeps 0x4fd3d0 and 0x4fd3b0 exact.
* `IntList::buy` 0x4fd490 is the `vector<int>` buffer allocator that `Game::Scene::list` (0x51d250) also calls, and the constructor of
  `MWidget` (0x4fd150) calls it for `list_c`; the slice modelled it as a free function over an empty-allocator class.
* The slices were wrong in many places; every one below was fixed against the disassembly (`disasm.py`, `cmpdis.py`):
  `Mission::init_agent` takes the agent as a stack argument (slice: `View`), `ShadowMap::sample` is a `ShadowMap` member on the global at
  0xa70248, `rand() % 15817` (not 15785), the 22-dword (not 23) clear in `refresh_agent`, the mesh arrays at +0xa0 / +0xac, `scene->m30c = -eye->u`,
  the listener position pointer `&g_listener.o` (0x5aa168), `camera_to` traces through `CellGrid::trace`, 0x4ee910's first argument 0.95 and the bar
  offset +0.001f, `add_image` slot +0x80 (layer 2) vs +0x7c (`meeting_window`), `0x4dd920` takes `(frame, game+0xac)`.
  `Font::layout_map` (0x4fd700): the glyph advance scale comes from the scale argument (not the extent), `measure(buf, total, &row_start, &word_end)`,
  the tag test is `find(..) == 0`, `substr(pos + 1, len)`, the three colour pointers (text / link / hot), the rectangles go to `rects_a` at the
  tag open and `rects_b` at its close (the slice mixed them up). `PartyWindow::create`: the list height is `((n + 2) / 3) * 0.088`, both
  position and size use `min(height, visible)`, the leave button uses `new_sized_button` (0x4dcc50) with `0.072f / 0.085f`, the armour key is
  `inv_t_armor_y_a`, the strip icons use `add_strip_icon` (0x5024e0), the tutorial box is the first screen of the window manager.
* `PartyWindow` (`party/party_window.h`) got the members above as the former padding: `page_widget[2]` +0x2c, `view[2]` +0x34, `gold[2]` +0x44,
  `name[2]` +0x4c, `hinge[4]` +0x54, `total_cost` +0xa60, `f_a8c`, `leave` +0xa90, `f_a98`, `f_ab0/4/8`, `f_abc`, `equip_bg` +0xac8, and
  the methods `create()` 0x500ad0 and `money_label(int)` 0x501940. `party_create.cpp` pins them with `offsetof` checks; `party_window.cpp` still compiles.

## Canonical headers: included / extended

Included (not redefined): `rgl/rgl_math.h`, `rgl_string.h`, `rgl/mesh.h`, `rgl/core/resmgr.h`, `tstring.h`, `item_ref.h`, `include/item_kind.h`, `mission.h`,
`agent_obj.h`, `mode_rec.h`, `game_data.h`, `fx.h`, `shape.h`, `frustum.h`, `ui/mission_ui.h`, `font.h`, `include/dialog_rec.h`, `shadow_map.h`,
`terrain_b/emit.h`, `terrain_b/cell_grid.h`, `party/party_window.h`.

Pure additions (new members / methods at unused offsets, each proven by the function cited):

* `agent_obj.h`: `slot_mesh[4]` +0x13c (replaces `pad134`; 0x4f7e00, 0x4f8b40), `slot_mesh4` +0x17c, `hide()` 0x4f8bc0, `show()` 0x4f8c20, `set_slot_value(int,int)` 0x470d80.
* `mission.h`: `init_agent` 0x4f7d70, `refresh_agent` 0x4f7e00, `camera_to` 0x4fa790, `apply_sun_light` 0x4fb180.
* `rgl/mesh.h`: `append_skinned` 0x44eb00, `scale` 0x44f0c0, `set_color(vector4)` 0x47af30, `set_bone_index` 0x47b360, `transform` 0x495ba0,
  `calc_bounds` 0x438d80, `set_offset` 0x4fb8e0, `darken_vertices` 0x4fbef0.
* `font.h`: `layout_map(...)` 0x4fd700. `shadow_map.h`: `ShadowMap::sample` 0x4faee0. `tstring.h`: `split(const char&, int&)` 0x42c9b0.
* `party/party_window.h`: see above.

Workarounds (a view of the same object, because the canonical header cannot be used together with the module's world):

* `agent_hud.h`: `HudUi` (pure slot interface over `MissionUi`, as `WidgetOps` / `DrawObj`), `SkelView` (the skeleton fields 0x4f7e00 touches; `skeleton.h`
  needs world (a)), `GameClock`, `SunBody`, `CamSlot`, `FrameNode`, `GuiMgr` (`g_gui` 0x5d7080), `UiVec`, `InputKeys`, and the widget classes of this region
  (`GroupWidget`, `WindowBase`, `FrameWidget`, `FlagWidget`, `ToggleWidget`, `ListWidget`, `NameWidget`, `MWidget`). `GameData` is used for g_game here,
  `Game` in `party_create`: three incompatible views of the object at 0x5e2930 (`GameData`, `World`, `Game`) exist in the tree.
* `agent_vec.cpp`: `CfgRec` (0x1f8 bytes) and `RowRec` (0x1c8 bytes) stand for `loading_config` (`cls_map/cls_map.h`) and `UiRow` (`char_sheet/screen.h`). Their
  headers pull in `match/include/rgl.h` (world (a)) and a record with real members would inline its own copy constructor, which the original keeps out of line
  (0x4dc030, 0x4e4340, 0x4d3720, 0x4e42f0), so only sizes and the out-of-line entry points are modelled. This is a second *spelling* of those records under other names,
  not a second definition of the class; if a later wave unifies the worlds they should be replaced by the canonical classes (the exact rows 0x4fe520 and 0x4fe840 then need
  re-checking).
* `editor_dialog.cpp`: `EditorGameView` / `EditorSceneView` / `EditorTerrainView` (the slice's `GameState`, `EditorWorld`, `Terrain`; the canonical `GameState`
  (`terrain_a/game_state.h`) and `Terrain` (`terrain_a/terrain.h`) cover other offsets of the same objects). `EditorPanel` keeps its name because the caller
  (`inventory_panel/widget_text.cpp`) declares `EditorPanel::terrain_dialog`.
* `party_create.cpp`: `InvList` (the 0x4a8 scrolling list, ctor 0x49deb0; `inventory_panel/party_screen.h` `PsScrollList` is the same class but that header is not
  committed yet), `InvWindow` (ctor 0x4931a0, vptr 0x592848), `GuiHost` (0x5d7080), `GameStrings` (`string_value` 0x48e9e0 with the hidden pointer spelled out).
  The list's virtual +0xc8 (set row count) is called through the widget view's `add_child` slot, which takes one stack word as well.

## Flags

All modules `/O2 /MT /Gy /TP` (the manifest default). `agent_vec` uses the real `<vector>` headers; the 5 exact rows were exact in the slice with the same flags.
`#pragma intrinsic(tan, atan2, atan, sqrt)` in `agent_hud.cpp` (the camera code uses the inlined x87 forms).

## Tricks kept / removed

Kept (they reproduce the original):

* `__declspec(noinline)` on `IntList::buy` (called, not inlined, by 0x4fd150), `V4List::insert` and the static `decode_utf8` (custom register convention: text in EDI, `&out_len` in EDX).
* `__forceinline` for `param_value` (7 unrolled copies in 0x4fc820), the `res_clone(tstring(...))` temporary (a named `tstring` plus a scope destructor emitted two destructor calls).
* The two-level hierarchy `WindowBase : MissionUi` / `FrameWidget : WindowBase` (a direct derivation produced a spurious `MissionUi()` call and a duplicate vptr store).
* `char raw[sizeof(tstring)]` for the by-pointer result of 0x48e9e0 in `money_label` (the callee constructs the string; the original runs no default constructor).
  This also exposed the stack-slot order that the slice could not get (3 bytes off): declare the raw buffer before the key.
* `agent_by_id()` spelled `&g_mission->agents.pages[id >> 4][id & 0xf]` (this form made 0x4f8b40 exact).

Removed: the slices' hand-spelled `__usercall` free-function models of 0x4fd700 (`Font::layout` with a result-block first argument), `vector_E100_buy` as a free function,
and the local `tstring` / `Widget` / `InventoryPanel` class copies of `s00500ad0` and `s00501940`.

## Known limits

* 0x4f7f00 and 0x500ad0 contain WinLicense marker blobs and can never be byte-exact.
* The large nonmatching bodies (0x4f8c80, 0x4f7f00, 0x4fbfa0, 0x4fc820, 0x4fd700, 0x4fe860, 0x500ad0) are complete, faithful ports; the remaining difference is x87 scheduling and
  register allocation.
* Equivalence (difftest): see the audit in the commit message / report; the `agent_vec` rows keep "local copy" entries because the vector instantiations are compiled here.

# Vtables for the standalone build

Goal: build.py's vtable fix-up has nothing to do. Every vtable our compiler emits for a class the original has a table for
equals the original's slot for slot (the same function, or the same code, which the original's `/OPT:ICF` folded to one
address), so there are 0 rewritten slots, 0 vptr stores moved to the original's table, 0 forwarding stubs, 0 placeholder
virtuals bound by the glue and 0 `vtbl_*` externs. `tools/relink/vtable_audit.py` is the work list; `build.json` has
the counters (`vtable_report`).

```
.venv/bin/python tools/relink/vtable_audit.py [--module STEM] [--vt REGEX] [--json FILE]
```

It prints, per original vtable we match, the original's slot function (with our name for it) beside what our compiler put
in the slot, and classifies the difference: `stub` (a bare `jmp` forwarding stub), `placeholder` (`s00`, `d10`, `slotN`,
`__purecall`), `foreign` (a real function that is not the original's), `dtor` (slot 0, a different deleting destructor),
`beyond` / `short` (our table ends early: base slots never declared), `long`, `suspect` (fewer than half the slots agree).

## The rule: the class that owns the table owns its virtuals

A table slot that holds a function our source defines under another class's name (`ResultWindow::init`,
`screen_text::refresh`, `MissionScreen::init_*`) almost never means the original had two classes: the slice tooling named
each function by the screen it believed it belonged to, and only the vtable ties them. Compare the layouts the functions
use (the same tstrings at the same offsets, the same pointer fields). If they agree there is one class, and the functions
are its members:

1. Declare the class once, in a header both TUs include, `: head_rec` (or whatever the real base is), with the **virtuals
   in the original's slot order and the overrides declared on the class**. The first non-inline virtual (or the
   constructor) makes cl emit the vtable.
2. Define each override in the TU where the original has it (a class's members may be spread over TUs: constructor and
   destructors with the record family, the window maker with the window code, ...), annotated `// @ 0xADDR`.
3. Delete the forwarding stub (`void C::init_screen() { ((Other*)this)->f(); }`), the `Other` view struct, and any
   `extern char vtbl_X[]` the hand destructor used to store the vptr.
4. Rename the manifest / slice rows to the new mangled names.

```cpp
// src/game/result_window.h: one class, vtable 0x595740 (was C4c2c50 + ResultWindow + screen_text)
struct ResultWindow : head_rec {
    ContainerBase* win;  /* +0x2c ... */  int* flag; /* +0x4c */  tstring* text; /* +0x50 */  tstring s54; /* ... */
    ResultWindow();                          // 0x4c2c50
    __declspec(noinline) ~ResultWindow();    // 0x4c2cd0, called (not inlined) by the deleting destructor 0x4c2cb0
    virtual void reset_screen();             // 0x4c30b0  g_gui.f1709 = 1
    virtual void init_screen();             // 0x4dc630  (cls_map.cpp)
    virtual void frame_update(const float*);      // 0x4ca550  (gui_edit.cpp)
    virtual char pure_7() { return 0; }      // 0x4280e0
};
// rec_tables.cpp
ResultWindow::ResultWindow() { flag = 0; text = 0; }            // base ctor + vptr store come from the compiler
ResultWindow::~ResultWindow() {}                                // members destroyed in reverse, vptr stores: the compiler's
// cls_map.cpp   // @ 0x4dc630
void ResultWindow::init_screen() { if (w04 == 0) { ... } ... }
```

## Destructors

Do not write `dtor()` + `destroy_X(self, int, flags)` + `*(void**)this = vtbl_X`. A real `~X()` (with
`__declspec(noinline)` where the original calls it out of line from the deleting destructor) makes cl emit the vptr
stores, the reverse member destruction and `??_GX@@UAEPAXI@Z` itself; the manifest rows become `??1X@@UAE@XZ` and
`??_GX@@UAEPAXI@Z` and stay byte-exact (ResultWindow: 32/32 rows of rec_tables unchanged).

## Slots the original's linker folded (ICF)

The original was linked with `/OPT:ICF`: two classes' slot 1 may be one function (`g_gui.f1709 = 1` is at 0x4c30b0 and
in the tables of four classes). Each class defines its own copy of the body; the audit counts the slot as equal because
the ICF trial link folds them. Do not write a free function and a stub per class.

```cpp
void C4c3050::reset_screen()   { g_gui.f1709 = 1; }   // @ 0x4c30b0
void ResultWindow::reset_screen() { g_gui.f1709 = 1; }
void Rec34::reset_screen()     { g_gui.f1709 = 1; }
```

## Head records: what was done and what the rest needs (lane V1)

`game/screen_records.h` holds the head_rec records, one class each, shared by every TU that holds one of their members
(`MB_ZERO_TAIL`, the opaque `RecX` creation types and the C4c / Rec* classes). Rec1208 lives in `game/conv_screen.h`, ItemRec /
ItemRec2 in `game/triggers/item_rec.h`, C4c3160 + `Slot20` in screen_records.h, the old views are `typedef`s of the record
(`OptionsScreen`/`OptionsWin`/`MeetingWindow`/`XferWin`/`LogScreen`/`quest_window`/`ConvScreen`) where one view was the
whole record. Their overrides are defined in the TU of the original function (`// @ 0xADDR`), their destructors are the
compiler's (`~X()` for the part the original calls out of line, implicit `??_GX`; the plain records need none: the
compiler-emitted deleting destructors fold with the base's under ICF), slot 3 is `frame_update(float)` (the per-frame / event
handler; the parameter is the frame time, a handler that forwards its address, as `pick(&arg)` does, takes
`(int*)&arg`), slot 1 is `g_gui.reset()` once per class (`g_gui.f1709 = 1` for the 0x4c30b0 group, `reset(); overlay = 0` for 0x4c6e10).

Done (slot 1 + 2 + 3 unless noted): C4c2fb0, C4c3050, C4c31c0, C4c3290, Rec38 (debrief report / leave), Rec44 (mission window),
Rec34 (debrief), Rec118 (meeting window / mission start menu), Rec3c (done window), Rec454 (quest log), Rec48 (import /
export window), Rec58 (game log), Rec1208 (conversation), Rec1bc (key bindings), Recd8 (options), Recf0 (video options),
ItemRec (main menu), ItemRec2 (escape menu; slot 2 EscapeMenu::create, slot 3 buttons), C4c3160 slot 3, TerrainPage / FaceGen slot 1.

Lessons that cost time:

* Inside a member of a class that has `MB_ZERO_TAIL` (a class-scope `operator new`), an unqualified `operator new(0x190)`
  is the class's zeroing allocator: `NEW_OVERLAY_WINDOW` and quest_log's window maker write `::operator new`.
* The slice tooling's "views" often spanned SEVERAL records (Hud = main menu / Rec38 / Rec44 / Rec454, MissionScreen = Rec44 /
  Rec118 / Rec3c, DebriefMenu = Rec34 / Rec38, KeysPage / QuestPage / VideoPage = pages of Rec1bc / Recd8 / Recf0). Give each
  record the fields its own functions use, move the function, keep the helper members that remain shared on the view
  (`((MissionScreen*)this)->fill_mission_grid()`).
* A parameter the original forwards by address (`pick(&arg)`, `m(&code)`) stays the function's own parameter (`float`),
  taken as `(int*)&arg`; an `int arg` copy changes the code.
* ConvScreen::open had `(int dialog, int* out)` in the slices; the original is a plain `ret` with `refresh(-1, 0)`.

## What is left in the V1 scope (and why)

* TerrainPage (Rec23c, now one class with facegen_win) slot 2 `init_screen` 0x4f1460, slot 3 `frame_update` 0x4f2d70 (no longer stubs); FaceGen (Rec428, the face generator; game/face_gen.h) slots 2 / 3 `init_screen` 0x4f49d0 and `frame_update` 0x4ea8e0: one class now (no longer stubs).
* Recacc (party / inventory window): done. `PartyWindow` (party/party_window_class.h) is the one class, a head_rec: slot 1
  `slot_empty_1`, slot 2 `slot_empty_2` (0x500ad0, the window maker), slot 3 `scale_3` (0x50fe20, the trade engine's per-frame
  step) and slot 7 are its own virtuals; InventoryPanel's methods (inventory_panel/inventory_panel.cpp, built with the GameData
  header) are its members, so there is no forwarding stub left for this record.
* C4c3160 slot 2 `PartyScreen::open_party_window` 0x500ad0.. (party_screen.cpp, a 1,300-line view with its own helpers).
* mission_items: ItemA / ItemB / ItemC / ItemD / MissionBlocker. Their deleting destructor in the original is 0x4ce930, the
  ItemRec one: ItemA..C start with the same `owned_block` at +0x30 as ItemRec (their `f34 f38 f3c` are its begin/end/cap), so
  the item family has a shared base class (head_rec + owned_block + clock / stamp), ItemRec / ItemRec2 / ItemA..C derive from
  it and the base's destructor is the folded one. The slot 2 / 3 targets are `GameScreen::build` / `EventScreen::on_event`
  (ItemA), `SaveWinCtl::create` / `SourceScreen::hook_a` (ItemB: its slot 2 0x4e5d80 is a `jmp` thunk to create),
  `create_flagged` / `hook_b` (ItemC), `view_obj::reset` / `InputScreen::on_input` (ItemD), `MissionHud::refresh` /
  `Screen_s261e0::tick` (MissionBlocker, slot 4 `MissionParams::init` is in the TU). Their views (SourceScreen = MissionSource at
  +0x30 + interval, ScreenBase) are the same record.
* head_cam and head_obj2 are now CampaignScreen and CharSheet, each one class with its screen (game/campaign_screen.h, game/char_sheet_panel.h).

## Still open (see the audit)

Base classes whose placeholder virtuals (`s00()`, `d10()`) the derived classes override in the original; records whose tables
are shorter than the original's. The stream families are done: `src/rgl/stream.h` has the real chain (`text_stream` ->
`outfile_b`/`outfile`; `text_stream2` -> `text_reader_base` -> `tstream3`; `mem_stream` -> `bin_reader_base` -> `bin_reader`;
`bin_writer`), every class defines its own slot bodies (the `STREAM_FWD_*` macros and `stream_fwd.h` are gone), and the
constructors/destructors the original inlines into its loaders are in `src/rgl/stream_inline.h`. An audit "long" on the log
file's 0x595618 is a tool artifact: the dtor calls slot 26 by absolute address, which `vtable_starts` reads as a table start.

Widget family: hand-built widgets of these classes in other families' sources (`*(void**)w = vtbl_...`): agent5/agent_hud.h
(`GroupWidget`, `TextGroupWidget`, `FrameWidget`, `FlagWidget`), inventory_panel/party_screen.h (`PsBar`,
`PsListWidget`), note_window.cpp, char_sheet/quest_log.cpp, char_sheet/editor_ui.cpp, char_sheet/facegen_win.cpp,
inventory_panel/char_sheet.cpp, party2/camera.cpp (`ContainerBase`, `UiB`, `UiA`, `UiE`, `NodeA`, `text_box`: use
`new <class>` from ui/container.h / ui/ui_widgets.h / config/ui_widgets_c.h), party/party_window.h (`PartyPanel` 0x599bf8,
`PartySlot` 0x599d28) and the drop-down (ListPanelW / DropDownW / DropDown, 0x592570: a UiA with a row vector). The widget
call sites still go through `WVt` (config/widget_mgr.h), a view of MissionUi's slots with the call sites' argument types.

Re-run the audit after each family: the
`stub` / `placeholder` / `dtor` counts per module are the remaining work.

## The widget family (ui/, config/, cls_map/, world_data/)

The widget tables are one hierarchy, not one class per table:

```
MissionUi (0x591c80, 49 virtuals; ui/mission_ui.h)
  UiA (0x591d58, the toggle button; inline ctor)   <- UiA (same table), MediumButton (0x591e20), XyzButton (0x591ee8), UiE (0x592208)
  UiB (0x591fb0, slider) <- UiC (0x592078, + update_children);   UiD (0x592140)
  ContainerBase (0x5924a0, ui/container.h) <- NodeA (0x592848), text_box (0x597050)
  progress_bar (0x597f20)
```

* Every base slot has its real name and signature, and its body is defined in the TU of the original function
  (`MissionUi::animate` in mesh_slot/slot.cpp, `update_bounds` in mission_ui2/mission_3.cpp, `normalize_uv` in
  world_data/mesh_hud.cpp, `layout_text_to` / `get_xy` in config/widget_mgr.cpp ...). The slices' names (`Effect`, `FieldPanel`,
  `MeshX`, `HudBar`, `PObj`, `ContainerW`, `FrameBox`, `UiFrame`, `gui_window`, `text_box_base`) were views of these classes.
* A derived widget overrides a slot by declaring it `virtual` and defining it where the original function is
  (`UiB::animate` is HudBar's 0x497880 in mesh_hud.cpp, `ContainerBase::update_children` is FrameBox::release 0x48ef40 in
  mission_4.cpp). No `d10` / `d35` names, no `jmp` stubs, no `vtbl_*` stores for these classes: `new UiB`, `new NodeA`,
  `new text_box`, `new UiA`.
* Constructors the original inlines into its callers (UiA's, ContainerBase's) are in-class definitions; cl then inlines them
  at the sites the original did and keeps the out-of-line copy where it needs one. `typedef ContainerBase UiFrame` keeps the
  old name for the sources that still use it.
* A deleting destructor is the compiler's. `NodeA` declares an inline `~NodeA() {}` so cl expands the base destructor call in
  its own `??_G` (0x49b0f0, byte-exact). `text_box` (0x597050) is the one table of the family still `dtor`: its slot 0 is 0x5021f0,
  the container's `??_G` as party_window.cpp's object compiled it (the derived vptr store dead, the vector freed through the
  shared out-of-line `vec<mface>::clear`); our `??_Gtext_box` is a different instance of the same source. `ContainerBase::destroy`
  keeps 0x5021f0 as a function of its own for PartyPanel's table (0x599bf8, still hand-stored: party/party_window.h).
* The `long` the audit printed for UiB / NodeA was `vtable_starts` taking a code immediate for the start of another table inside
  them; the audit now keeps the table whole when the pointer run is as long as ours.
* The functions the slices called `gui_window::refresh_all / wheel_step / wheel_focus / mouse_event / handle_event` (0x42cd60, 0x42ce50,
  0x42cec0, 0x42cf50, 0x42d170) are the widget classes' own virtuals (`MissionUi::key / refresh`, `ContainerBase::refresh / key`,
  `NodeA::key`), defined once in ui/widget_slots.cpp. `gui_window`, `widget` (widget.h) and `WidgetOps` no longer exist.

## Entity tail and agent record (lane V4): what the compiler does with derived deleting destructors

* A derived class with no destructor of its own still gets `??_GD` from cl 13.10. Its body is `call <nearest destructor function>; delete`
  (one level only: `D : C : E` with an inline `~C` gives `??_GD = call ??1C`, while `??_GC` has `~C` expanded). The original's
  `/OPT:ICF` then folds it with the base's deleting destructor when the bytes are equal, which is why `mesh_entity` (slot 0 =
  `??_Gentity` 0x4214e0) is *equal* in the audit although we emit `??_Gmesh_entity`. Micro tests (`/Ob0 /Ob1 /Ob2`, `__forceinline`
  on either dtor, an empty user `~D`) never make cl expand the base destructor into `??_GD`.
* `dynamic_entity` (slot 0 = `??_Gcollision_entity` 0x427d20, which has `~collision_entity` expanded) therefore stays the one
  `dtor` row of the entity tail: our `??_Gdynamic_entity` is `call ~rigid_base` (two implicit levels). Folding `rigid_base` into
  `dynamic_entity` (one level) makes it `call ~collision_entity`, still not the expanded body, and loses the constructor row
  0x425370 (11 bytes), so it was not kept. The gate already reports such slots separately (`dtor_slots`, not `slots_differing`).
* A class that has no destructor in source but destroys members (the agent record 0x5928 bytes: `Tail2` at +0x53c0, the id
  vector at +0x1c) gets its original code only when the members are real members: the implicit destructor stores no vptr of its
  own, destroys the members in reverse order and ends with the base's vptr store. Writing `~MAgent() { ((Tail2*)...)->~Tail2(); ... }`
  stores the class's own vptr first and is 144 bytes off.
* `vtable_starts` (build.py, vtable_audit.py) took `call [0x54b4dc]` (the entity factory calling slot 1 directly) and a byte
  coincidence inside another instruction (0x4444cc reads as 0x54b800) for table starts, so mesh_entity read as a 1-slot table and
  static_entity as 18 slots. A start is now an operand that stores or loads the address (`mov r, imm`, `push imm`, `mov [m], imm`).
  Not filtered: a `.data` byte table that contains 0x00590900 (0x5d1854) makes IfaceB (0x5908f8, 3 slots) read as 2.

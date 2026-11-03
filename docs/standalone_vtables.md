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
3. Delete the forwarding stub (`void C::slot_empty_2() { ((Other*)this)->f(); }`), the `Other` view struct, and any
   `extern char vtbl_X[]` the hand destructor used to store the vptr.
4. Rename the manifest / slice rows to the new mangled names.

```cpp
// src/game/result_window.h: one class, vtable 0x595740 (was C4c2c50 + ResultWindow + screen_text)
struct ResultWindow : head_rec {
    ContainerBase* win;  /* +0x2c ... */  int* flag; /* +0x4c */  tstring* text; /* +0x50 */  tstring s54; /* ... */
    ResultWindow();                          // 0x4c2c50
    __declspec(noinline) ~ResultWindow();    // 0x4c2cd0, called (not inlined) by the deleting destructor 0x4c2cb0
    virtual void slot_empty_1();             // 0x4c30b0  g_gui.f1709 = 1
    virtual void slot_empty_2();             // 0x4dc630  (cls_map.cpp)
    virtual void scale_3(const float*);      // 0x4ca550  (gui_edit.cpp)
    virtual char pure_7() { return 0; }      // 0x4280e0
};
// rec_tables.cpp
ResultWindow::ResultWindow() { flag = 0; text = 0; }            // base ctor + vptr store come from the compiler
ResultWindow::~ResultWindow() {}                                // members destroyed in reverse, vptr stores: the compiler's
// cls_map.cpp   // @ 0x4dc630
void ResultWindow::slot_empty_2() { if (w04 == 0) { ... } ... }
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
void C4c3050::slot_empty_1()   { g_gui.f1709 = 1; }   // @ 0x4c30b0
void ResultWindow::slot_empty_1() { g_gui.f1709 = 1; }
void Rec34::slot_empty_1()     { g_gui.f1709 = 1; }
```

## Head records: what was done and what the rest needs (lane V1)

`game/screen_records.h` holds the head_rec records, one class each, shared by every TU that holds one of their members
(`MB_ZERO_TAIL`, the opaque `RecX` creation types and the C4c / Rec* classes). Rec1208 lives in `game/conv_screen.h`, ItemRec /
ItemRec2 in `game/triggers/item_rec.h`, C4c3160 + `Slot20` in screen_records.h, the old views are `typedef`s of the record
(`OptionsScreen`/`OptionsWin`/`MeetingWindow`/`XferWin`/`LogScreen`/`quest_window`/`ConvScreen`) where one view was the
whole record. Their overrides are defined in the TU of the original function (`// @ 0xADDR`), their destructors are the
compiler's (`~X()` for the part the original calls out of line, implicit `??_GX`; the plain records need none: the
compiler-emitted deleting destructors fold with the base's under ICF), slot 3 is `scale_3(float)` (the per-frame / event
handler; the parameter is the frame time, a handler that forwards its address, as `pick(&arg)` does, takes
`(int*)&arg`), slot 1 is `g_gui.reset()` once per class (`g_gui.f1709 = 1` for the 0x4c30b0 group, `reset(); overlay = 0` for 0x4c6e10).

Done (slot 1 + 2 + 3 unless noted): C4c2fb0, C4c3050, C4c31c0, C4c3290, Rec38 (debrief report / leave), Rec44 (mission window),
Rec34 (debrief), Rec118 (meeting window / mission start menu), Rec3c (done window), Rec454 (quest log), Rec48 (import /
export window), Rec58 (game log), Rec1208 (conversation), Rec1bc (key bindings), Recd8 (options), Recf0 (video options),
ItemRec (main menu), ItemRec2 (escape menu; slot 2 EscapeMenu::create, slot 3 buttons), C4c3160 slot 3, Rec23c / Rec428 slot 1.

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

* Rec23c (terrain page) slot 2 `facegen_win::init` 0x4f1460, slot 3 `TerrainPage::update` 0x4f2d70; Rec428 (face generator)
  slots 2 / 3 `FaceGen::init` 0x4f49d0 and `CamView::update_model` 0x4ea8e0: two views with different names and widget types
  of one record (char_sheet/facegen_init.h and cam_view.h); merge their fields into Rec428, as was done for Rec58 / Rec454.
* Recacc (party / inventory window) slots 2 / 3: `PartyWindow` (party/party_window.h, 36 placeholder virtuals) and
  `InventoryPanel` (inventory_panel/inventory_panel.h) are two views of it; PartyWindow's placeholder virtuals have to go with
  the merge.
* C4c3160 slot 2 `PartyScreen::open_party_window` 0x500ad0.. (party_screen.cpp, a 1,300-line view with its own helpers).
* mission_items: ItemA / ItemB / ItemC / ItemD / MissionBlocker. Their deleting destructor in the original is 0x4ce930, the
  ItemRec one: ItemA..C start with the same `owned_block` at +0x30 as ItemRec (their `f34 f38 f3c` are its begin/end/cap), so
  the item family has a shared base class (head_rec + owned_block + clock / stamp), ItemRec / ItemRec2 / ItemA..C derive from
  it and the base's destructor is the folded one. The slot 2 / 3 targets are `GameScreen::build` / `EventScreen::on_event`
  (ItemA), `SaveWinCtl::create` / `SourceScreen::hook_a` (ItemB: its slot 2 0x4e5d80 is a `jmp` thunk to create),
  `create_flagged` / `hook_b` (ItemC), `view_obj::reset` / `InputScreen::on_input` (ItemD), `MissionHud::refresh` /
  `Screen_s261e0::tick` (MissionBlocker, slot 4 `MissionParams::init` is in the TU). Their views (SourceScreen = MissionSource at
  +0x30 + interval, ScreenBase) are the same record.
* head_cam / head_obj2 (rec_slots_hc.cpp): `Screen::init_screen` / `frame_update`, `CharSheet::init` / `update`, `rec_2e4::release`.

## Still open (see the audit)

Base classes whose placeholder virtuals (`s00()`, `d10()`) the derived classes override in the original; the stream
families built from `STREAM_FWD_*` forwarding macros (each class should define its own real slot bodies, which the
ICF trial link folds); records whose tables are shorter than the original's. Re-run the audit after each family: the
`stub` / `placeholder` / `dtor` counts per module are the remaining work.

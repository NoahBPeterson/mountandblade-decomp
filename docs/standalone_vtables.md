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

## Head records: what was done and what the rest needs (lane 1)

`game/screen_records.h` holds the head_rec records that are screens, one class each, shared by every TU that holds one of
their members. Done: C4c2fb0 (the "creating game" panel: slot 2 `cls_map.cpp` 0x4daa10, slot 3 `party/game.cpp` 0x51c5a0, plus
`sub` 0x51c720), C4c3050 (message window: 0x4ddf80, 0x4ca6c0), C4c31c0 (save / load window: 0x4e4790, 0x4ea290, `list_saves`,
`slot_button`, `select`), C4c3290 (start-up loading screen: 0x4e3aa0, 0x50f390, `advance` 0x50f440). Their destructors are
the compiler's (`~X()` for the part the original calls out of line, implicit `??_GX`), `head_rec_delete` is gone (the
base's `??_Ghead_rec` is compiler-emitted: the derived classes' deleting destructors differ from it only in the vptr they
store, which is the same `??_7head_rec` now), slot 3 is `scale_3(float)` (it is the per-frame update), and the fake classes
the slices had invented for the halves (`LoadingScreen`, `LoadingPanel`, `MessageMenu`, `dev_list`, `SaveWindow`,
`SavePanel`, `MeetingWindow::init2`) are gone.

Two things made the remaining ~20 tables expensive and are why they are not done:

* The slice tooling's screen classes (`Hud`, `MissionScreen`, `QuestPage`, `options_panel`, `DebriefMenu`, `PartyScreen` +
  `XferWin`, `TerrainPage`, `PartyWindow` + `InventoryPanel`) are views that span SEVERAL records: `Hud::handle_buttons` is the
  main-menu record (ItemRec), `Hud::on_leave` Rec38, `Hud::on_mission_start` Rec44, `Hud::on_update` Rec454. Each function has to
  be given the fields of its own record (the offsets agree with the record sizes: Rec38 0x38, Rec44 0x44, Rec454 0x454) and
  the helper members it calls have to move to that record too.
* The screens are written against two widget universes: `ui/mission_ui.h` (MissionUi / UiB / UiAInline, what `cls_map` and
  `progress_bar` use) and `ui_widget.h` (Widget, what `char_sheet` / `options_panel` use). A record shared by both can only
  declare its buttons as one of them (C4c31c0 uses `Widget*` and `cls_map/progress_bar.cpp` casts at the three creation
  sites); `char_sheet` cannot include `mission_ui.h` (`rgl_mesh.h` redefines `rgl::mesh_list`), `mesh_2.cpp` cannot either
  without the `resource_manager` merge done here. Unifying Widget and MissionUi would remove both.

## Still open (see the audit)

Base classes whose placeholder virtuals (`s00()`, `d10()`) the derived classes override in the original; the stream
families built from `STREAM_FWD_*` forwarding macros (each class should define its own real slot bodies, which the
ICF trial link folds); records whose tables are shorter than the original's. Re-run the audit after each family: the
`stub` / `placeholder` / `dtor` counts per module are the remaining work.

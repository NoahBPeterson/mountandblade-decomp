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

## Still open (see the audit)

Base classes whose placeholder virtuals (`s00()`, `d10()`) the derived classes override in the original; the stream
families built from `STREAM_FWD_*` forwarding macros (each class should define its own real slot bodies, which the
ICF trial link folds); records whose tables are shorter than the original's. Re-run the audit after each family: the
`stub` / `placeholder` / `dtor` counts per module are the remaining work.

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

//-----------------------------------------------------------------------------
// File: item_rec.cpp  (0x4ce870..0x4ce960)
//
// Desc: constructors of the two "item" head records and their shared deleting destructor. Names provisional.
//       Flags: /O2 /MT /Gy /TP. Boundary: see MIGRATION.md (class change only; no call evidence either way).
//-----------------------------------------------------------------------------
#include "./item_rec.h"

// A compiler fence (no code): pins the store order of the base ctor's id store, the derived vptr store and
// the zero/load stores, which cl 13.10 otherwise schedules differently from the original (docs/codegen.md 10.9).
extern "C" void _ReadWriteBarrier(void);

// @ 0x4ce870
ItemRec::ItemRec()
{
    _ReadWriteBarrier();
    w04 = 0;
    owned.begin = 0;
    owned.end = 0;
    owned.cap = 0;
    clock = 0;
    stamp = g_ticks[0];
    _ReadWriteBarrier();
    f5c = 0;
    f58 = 1.0f;
    f40 = -1;
}

// @ 0x4ce8d0
ItemRec2::ItemRec2()
{
    _ReadWriteBarrier();
    w04 = 0;
    owned.begin = 0;
    owned.end = 0;
    owned.cap = 0;
    clock = 0;
    stamp = g_ticks[0];
    _ReadWriteBarrier();
    f5c = 0;
    f58 = 1.0f;
    f40 = -1;
}

// 0x4ce930 is the compiler-generated deleting destructor of ItemRec (ICF also serves ItemRec2): head_rec's virtual
// destructor, the owned block's destructor 0x4c8bf0 and operator delete.
//
// Slots 2 and 3 (screen_build / screen_buttons) are declared in item_rec.h; their bodies live with the functions the
// image's tables name (MainMenuScreen::create in cls_map.cpp, Hud::handle_buttons in hud.cpp, ...). Nothing in the
// binary constructs these records, so no caller reaches those slots (docs/Papercuts.md).


// Slot 1 of both records is the free function 0x4c6e00 (reset_gui); the body is a tail call to it, which is what the
// image's slot holds. Slot 7 is always_true (declared inline above).
void ItemRec::slot_empty_1()  { reset_gui(); }
void ItemRec2::slot_empty_1() { reset_gui(); }

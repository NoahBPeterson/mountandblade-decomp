//-----------------------------------------------------------------------------
// File: item_rec.cpp  (0x4ce870..0x4ce960)
//
// Desc: constructors of the two "item" head records and their shared deleting destructor. Names provisional.
//       Flags: /O2 /MT /Gy /TP. Boundary: see MIGRATION.md (class change only; no call evidence either way).
//-----------------------------------------------------------------------------
#include "./item_rec.h"
#include "../../include/clock_table.h"
#include "../strat/gui_manager.h"     // g_gui

// The member initialisers are small inline helpers: written out flat, cl schedules the base ctor's id store and the
// derived vptr store after the zero stores, which the original does not.

static __forceinline void start_timer(Timer& t) { t.clock = 0; t.start = g_ticks[0]; }
static __forceinline void clear_block(owned_block& b) { b.begin = 0; b.end = 0; b.cap = 0; }
// @ 0x4ce870
ItemRec::ItemRec() : head_rec(no_w04())
{
    w04 = 0;
    clear_block(owned);
    start_timer(tm);
    f5c = 0;
    f58 = 1.0f;
    f40 = -1;
}

// @ 0x4ce8d0
ItemRec2::ItemRec2() : head_rec(no_w04())
{
    w04 = 0;
    clear_block(owned);
    start_timer(tm);
    f5c = 0;
    f58 = 1.0f;
    f40 = -1;
}

// 0x4ce930 is the compiler-generated deleting destructor of ItemRec (ICF also serves ItemRec2): head_rec's virtual
// destructor, the owned block's destructor 0x4c8bf0 and operator delete.
//
// Slots 2 and 3 (init_screen / frame_update) are declared in item_rec.h; their bodies live with the functions the
// image's tables name (MainMenuScreen::create in cls_map.cpp, Hud::handle_buttons in hud.cpp, ...). Nothing in the
// binary constructs these records, so no caller reaches those slots (docs/Papercuts.md).


// Slot 1 of both records is the free function 0x4c6e00 (reset_gui); the body is a tail call to it, which is what the
// image's slot holds. Slot 7 is always_true (declared inline above).
void ItemRec::reset_screen()  { g_gui.reset(); }
void ItemRec2::reset_screen() { g_gui.reset(); }

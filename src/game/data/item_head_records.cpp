//-----------------------------------------------------------------------------
// File: item_head_records.cpp  (0x4ce870..0x4ce960)
//
// Desc: constructors of the two "item" head records and their shared deleting destructor. Names provisional.
//       Flags: /O2 /MT /Gy /TP. Boundary: see MIGRATION.md (class change only; no call evidence either way).
//-----------------------------------------------------------------------------
#include "game/data/item_head_records.h"
#include "engine/core/clock_table.h"
#include "game/ui/gui_manager.h"     // g_gui

// The member initialisers are small inline helpers: written out flat, cl schedules the base ctor's id store and the
// derived vptr store after the zero stores, which the original does not.

// @ 0x4ce870
ItemRec::ItemRec()
{
}

// @ 0x4ce8d0
ItemRec2::ItemRec2()
{
}

// 0x4ce930 is the compiler-generated deleting destructor of ItemRec (ICF also serves ItemRec2): head_rec's virtual
// destructor, the owned block's destructor 0x4c8bf0 and operator delete.
//
// Slots 2 and 3 (init_screen / frame_update) are declared in item_head_records.h; their bodies live with the functions the
// image's tables name (MainMenuScreen::create in winmain_and_menu_screens.cpp, Hud::handle_buttons in mission_hud_buttons.cpp, ...). Nothing in the
// binary constructs these records, so no caller reaches those slots (docs/Papercuts.md).


// Slot 1 of both records is the free function 0x4c6e00 (reset_gui); the body is a tail call to it, which is what the
// image's slot holds. Slot 7 is always_true (declared inline above).
void ItemRec::reset_screen()  { g_gui.reset(); }
void ItemRec2::reset_screen() { g_gui.reset(); }

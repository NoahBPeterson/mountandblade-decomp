// record_slots_screen_records.cpp: the slot 1/2/3 overrides of the head_rec record classes declared by module_option_records.h (vtables 0x595740..0x596c68).
// The image's tables hold there functions that our source defines under other classes' names (MissionScreen::init_*,
// Hud::on_*, ResultWindow::init, ...), whose `this` is a different view of the record than the 0x30..0x1200-byte record
// that owns the table. Each override is a forwarding stub: it compiles to a bare `jmp` to the function the image's
// slot names (declared by a minimal local view of its class; the class name must match the mangled name). Slot 3's
// base signature is frame_update(const float*) while the real functions take int/float: the stub forwards the 4-byte
// argument unchanged.
#include "game/data/module_option_records.h"
#include "game/ui/gui_manager.h"     // g_gui
#include "game/ui/screens/party_screen_class.h"   // PartyScreen

struct XferWin {
    void update(int);
};

// slot 1 of C4c2fb0 / C4c3290: 0x4c6e00 (editor_selection_and_translation.cpp reset_gui), once per class (the original's linker folded the copies)
void C4c2fb0::reset_screen() { g_gui.reset(); }
void C4c3290::reset_screen() { g_gui.reset(); }

// PartyScreen (game/ui/screens/party_screen_class.h)
void PartyScreen::reset_screen() { g_gui.reset(); }

void C4c31c0::reset_screen() { g_gui.reset(); }



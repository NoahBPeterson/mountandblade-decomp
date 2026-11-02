// rec_slots_c4c.cpp: the slot 1/2/3 overrides of the head_rec record classes declared by rec_tables.h (vtables 0x595740..0x596c68).
// The image's tables hold there functions that our source defines under other classes' names (MissionScreen::init_*,
// Hud::on_*, ResultWindow::init, ...), whose `this` is a different view of the record than the 0x30..0x1200-byte record
// that owns the table. Each override is a forwarding stub: it compiles to a bare `jmp` to the function the image's
// slot names (declared by a minimal local view of its class; the class name must match the mangled name). Slot 3's
// base signature is scale_3(const float*) while the real functions take int/float: the stub forwards the 4-byte
// argument unchanged.
#include "../../rgl/mesh2/rec_tables.h"

struct PartyScreen {
    void open_party_window();
};
struct XferWin {
    void update(int);
};

// slot 1 of C4c2fb0 / C4c3290: 0x4c6e00 (gui_edit.cpp reset_gui), once per class (the original's linker folded the copies)
void C4c2fb0::slot_empty_1() { g_gui.reset(); }
void C4c3290::slot_empty_1() { g_gui.reset(); }

// C4c3160
void C4c3160::slot_empty_1() { reset_gui(); }
void C4c3160::slot_empty_2() { ((PartyScreen*)this)->open_party_window(); }

void C4c31c0::slot_empty_1() { g_gui.reset(); }



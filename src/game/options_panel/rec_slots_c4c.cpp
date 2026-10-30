// rec_slots_c4c.cpp: the slot 1/2/3 overrides of the head_rec record classes declared by rec_tables.h (vtables 0x595740..0x596c68).
// The image's tables hold there functions that our source defines under other classes' names (MissionScreen::init_*,
// Hud::on_*, ResultWindow::init, ...), whose `this` is a different view of the record than the 0x30..0x1200-byte record
// that owns the table. Each override is a forwarding stub: it compiles to a bare `jmp` to the function the image's
// slot names (declared by a minimal local view of its class; the class name must match the mangled name). Slot 3's
// base signature is scale_3(const float*) while the real functions take int/float: the stub forwards the 4-byte
// argument unchanged.
#include "../../rgl/mesh2/rec_tables.h"

void set_flag_5d8789();     // 0x4c30b0 (rec_tables.cpp)

struct ResultWindow {
    void init();
};
struct screen_text {
    void refresh(int);
};
struct LoadingScreen {
    void init();
    void update(int);
};
struct LoadingPanel {
    void update(int);
};
struct MessageMenu {
    void create_message();
};
struct dev_list {
    void select_windowed(int);
};
struct PartyScreen {
    void open_party_window();
};
struct XferWin {
    void update(int);
};
struct SaveWindow {
    void init();
};
struct SavePanel {
    void update(int);
};
struct MeetingWindow {
    void init2();
};

// C4c2c50
void C4c2c50::slot_empty_1() { set_flag_5d8789(); }
void C4c2c50::slot_empty_2() { ((ResultWindow*)this)->init(); }
void C4c2c50::scale_3(const float* a) { ((screen_text*)this)->refresh((int)a); }

// C4c2fb0
void C4c2fb0::slot_empty_1() { reset_gui(); }
void C4c2fb0::slot_empty_2() { ((LoadingScreen*)this)->init(); }
void C4c2fb0::scale_3(const float* a) { ((LoadingPanel*)this)->update((int)a); }

// C4c3050
void C4c3050::slot_empty_1() { set_flag_5d8789(); }
void C4c3050::slot_empty_2() { ((MessageMenu*)this)->create_message(); }
void C4c3050::scale_3(const float* a) { ((dev_list*)this)->select_windowed((int)a); }

// C4c3160
void C4c3160::slot_empty_1() { reset_gui(); }
void C4c3160::slot_empty_2() { ((PartyScreen*)this)->open_party_window(); }
void C4c3160::scale_3(const float* a) { ((XferWin*)this)->update((int)a); }

// C4c31c0
void C4c31c0::slot_empty_1() { reset_gui(); }
void C4c31c0::slot_empty_2() { ((SaveWindow*)this)->init(); }
void C4c31c0::scale_3(const float* a) { ((SavePanel*)this)->update((int)a); }

// C4c3290
void C4c3290::slot_empty_1() { reset_gui(); }
void C4c3290::slot_empty_2() { ((MeetingWindow*)this)->init2(); }
void C4c3290::scale_3(const float* a) { ((LoadingScreen*)this)->update((int)a); }

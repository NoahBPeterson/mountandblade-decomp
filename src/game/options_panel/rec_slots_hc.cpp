// rec_slots_hc.cpp: the slot 1/2/3 overrides of the head_rec record classes declared by mesh2/head_rec.h (vtables 0x595740..0x596c68).
// The image's tables hold there functions that our source defines under other classes' names (MissionScreen::init_*,
// Hud::on_*, ResultWindow::init, ...), whose `this` is a different view of the record than the 0x30..0x1200-byte record
// that owns the table. Each override is a forwarding stub: it compiles to a bare `jmp` to the function the image's
// slot names (declared by a minimal local view of its class; the class name must match the mangled name). Slot 3's
// base signature is scale_3(const float*) while the real functions take int/float: the stub forwards the 4-byte
// argument unchanged.
#include "../../rgl/mesh2/head_rec.h"

void set_flag_5d8789();     // 0x4c30b0 (rec_tables.cpp)

struct Screen {
    void init_screen();
    void frame_update(float);
};
struct rec_2e4 {
    void release();
};
struct CharSheet {
    void init();
    void update(float);
};

// head_cam
void head_cam::slot_empty_1() { reset_gui(); }
void head_cam::slot_empty_2() { ((Screen*)this)->init_screen(); }
void head_cam::scale_3(const float* a) { ((Screen*)this)->frame_update(*(float*)&a); }
void head_cam::scale_5(const float* a) { write_stream((w_stream*)a); }
void head_cam::scale_6(const float* a) { read_stream((w_stream*)a); }

// head_obj2
void head_obj2::slot_empty_1() { ((rec_2e4*)this)->release(); }
void head_obj2::slot_empty_2() { ((CharSheet*)this)->init(); }
void head_obj2::scale_3(const float* a) { ((CharSheet*)this)->update(*(float*)&a); }

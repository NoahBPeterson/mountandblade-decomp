//-----------------------------------------------------------------------------
// File: text_box_slots.cpp
//
// Desc: the 50 virtual slots of text_box_base (text_box_base.h, vtable 0x597050): the widget base's slots, the container's
//       overrides (ContainerW::update_all ..), the mouse handler (gui_window::mouse_event) and the frame's destroying
//       destructor. Each is a leaf the original's slot also holds or a stub /O2 compiles to a bare `jmp` to the function the
//       slot names (see ../ui/widget_slots.cpp for the scheme). Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "../tstring.h"
#include "../ui/ui_common.h"
#include "../ui/item_ref.h"
#include "../ui/mission_ui.h"
#include "../ui/ui_frame.h"
#include "./text_box_base.h"

// ---- the owners of the functions the original's slots name (definitions elsewhere) ------------------------------
struct Effect { void animate(float dt); };
struct FieldPanel { void update_bounds(); };                            // 0x48d020 (mission_ui2/mission_3.cpp)
struct MeshX { void normalize_uv(MeshX* src); };                        // 0x495fa0 (world_data/mesh_hud.cpp)
struct ContainerW {                                                     // world_data/world_data.cpp
    void place(int* a);                                                 // 0x490570
    void track_bounds(void* p);                                         // 0x4905c0
    void activate_all();                                                // 0x4906b0
    void reset_all();                                                   // 0x4906e0
    void bounds_of(int* a, float* p3, float* p4, float* p5);            // 0x490710
    void update_all(int p);                                             // 0x4908c0
};
struct gui_window {                                                     // strat/gui_manager.cpp
    char mouse_event(unsigned mask);                                    // 0x42cf50
    void wheel_focus();                                                 // 0x42cec0
};
namespace c49aca0 {
struct R8 { int a, b; };
struct PObj { R8 text_rect(); };                                        // 0x49b4f0 (party2/camera.cpp)
}
struct MissionUiText : MissionUi { void layout_text_to(const tstring* text, const float* pos); };   // 0x49abc0 (config/widget_mgr.cpp)
struct FrameBox { void release(); };                                    // 0x48ef40 (mission_ui2/mission_4.h)
typedef void (c49aca0::PObj::*TextRectPF)(float*);

#define M ((MissionUi*)this)
void* text_box_base::destroy(unsigned flags) { return ((UiFrame*)this)->destroy(flags); }
bool text_box_base::d01() { return false; }
void text_box_base::d02(int a) { ((ContainerW*)this)->update_all(a); }
void text_box_base::d03() { ((FieldPanel*)this)->update_bounds(); }
void text_box_base::d04(float a, int b) { M->set_a(a, b); }
void text_box_base::d05(float a, int b) { M->set_b(a, b); }
void text_box_base::d06(float a, int b) { M->set_c(a, b); }
void text_box_base::d07(float a, int b) { M->set_d(a, b); }
void text_box_base::d08(float a, float* p) { M->set_e(a, p); }
void text_box_base::d09(float a, float* p) { M->set_f(a, p); }
void text_box_base::d10(rgl::scene* sc, float* count, int c, int d) { ((ContainerW*)this)->bounds_of((int*)sc, count, (float*)c, (float*)d); }
void text_box_base::d11(int v) { M->set15c(v); }
int text_box_base::d12() { return M->get15c(); }
void text_box_base::d13(int v) { M->set168(v); }
float text_box_base::d14() { return M->get168(); }
bool text_box_base::d15(int m) { return M->test_c8(m); }
void text_box_base::d16() { ((ContainerW*)this)->reset_all(); }
bool text_box_base::d17(int m) { return M->clear_mask(m); }
void text_box_base::set_key_flags(int f) { M->or_flags(f); }
void text_box_base::d19(int v) { M->set_d0(v); }
void text_box_base::d20(int v) { M->set_d4(v); }
void text_box_base::d21(int m) { M->or_d4(m); }
void text_box_base::d22(void* p) { ((ContainerW*)this)->place((int*)p); }
void text_box_base::d23(void* child) { ((ContainerW*)this)->track_bounds(child); }
void text_box_base::d24() { ((ContainerW*)this)->activate_all(); }
void text_box_base::set_xy(float* v) { M->move_f0(v); }
void text_box_base::d26(float* v) { M->move_f8(v); }
void text_box_base::set_size(float* v) { M->set_text_pos(v); }
void text_box_base::set_offset(float* v) { M->set138(v); }
void text_box_base::d29(float* v) { M->set148(v); }
void text_box_base::d30(float* v) { M->or148(v); }
void text_box_base::d31(int a, int b) { M->run(a, b); }
void text_box_base::d32(int a, int b) { M->set_track(a, (rgl::mesh*)b); }
void text_box_base::d33(int a) { ((MeshX*)this)->normalize_uv((MeshX*)a); }
void text_box_base::d34(void* a) { (void)a; }
char text_box_base::d35(unsigned mask) { return ((gui_window*)this)->mouse_event(mask); }
void text_box_base::refresh() { ((gui_window*)this)->wheel_focus(); }
void text_box_base::get_xy(float* v) { (((c49aca0::PObj*)this)->*(TextRectPF)&c49aca0::PObj::text_rect)(v); }
void text_box_base::set_color(unsigned* c) { M->set14c((float*)c); }
void text_box_base::d39(void* p) { M->set_target(p); }
void text_box_base::d40(int p) { M->set_pos(p); }
void text_box_base::d41(float* out, const tstring* s) { M->set_text(out, s); }
void text_box_base::d42(tstring* out) { M->put(out); }
void text_box_base::d43(const tstring* s, const float* p) { ((MissionUiText*)this)->layout_text_to(s, p); }
void text_box_base::d44() { M->hide(); }
void text_box_base::d45() { M->show(); }
void text_box_base::d46() { M->flag2_set(); }
void text_box_base::d47() { M->flag2_clear(); }
int text_box_base::d48() { return 0; }
void text_box_base::d49() { ((FrameBox*)this)->release(); }

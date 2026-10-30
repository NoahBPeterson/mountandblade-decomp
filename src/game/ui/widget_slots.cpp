//-----------------------------------------------------------------------------
// File: widget_slots.cpp
//
// Desc: the virtual slots of the widget base MissionUi (vtable 0x591c80) and of the widgets derived from it in
//       ui_widgets.h (UiA 0x591d58, UiB 0x591fb0, UiC 0x592078, UiD 0x592140). The original's tables hold, per slot,
//       a function that our source defines under another class (the slices split the widget code between
//       "Effect", "MissionUi", "UiA" ...) or a one-instruction leaf. A slot here is either that leaf (the linker folds
//       it with the original's: `xor al,al; ret`, `ret 4` ...) or a forwarding stub that /O2 compiles to a bare `jmp`
//       to the function the slot names (same `this`, same stack arguments). The owner classes are declared
//       minimally below; the class names and signatures must match the names of the definitions.
//       A pointer-to-member cast stands in where the stub's parameter has another type than the callee's (an int
//       slot argument, a float parameter): the stack words are passed on unchanged, which is what the jmp does.
//       Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "../tstring.h"
#include "./ui_common.h"
#include "./item_ref.h"
#include "./mission_ui.h"
#include "./ui_widgets.h"

// ---- the owners of the functions the original's slots name (definitions elsewhere) ------------------------------
struct WidgetDtor { void* destroy(unsigned char flag); };               // 0x502300 (party/party_window.h)
struct Effect { void animate(float dt); };                              // 0x487c40 (mesh_slot/slot.cpp)
struct FieldPanel { void update_bounds(); };                            // 0x48d020 (mission_ui2/mission_3.cpp)
struct MeshX { void normalize_uv(MeshX* src); };                        // 0x495fa0 (world_data/mesh_hud.cpp)
struct gui_window {                                                     // strat/gui_manager.cpp
    char refresh_all(unsigned mask);                                    // 0x42cd60
    void wheel_step();                                                  // 0x42ce50
};
namespace c49aca0 {
struct R8 { int a, b; };
struct PObj { R8 text_rect(); };                                        // 0x49b4f0 (party2/camera.cpp)
}
struct MissionUiText : MissionUi { void layout_text_to(const tstring* text, const float* pos); };   // 0x49abc0 (config/widget_mgr.cpp)
struct HudBar {                                                         // world_data/mesh_hud.cpp
    void update_4(int arg);                                             // 0x497880
    void update_5(int arg);                                             // 0x497e30
    void rebuild_frame();                                               // 0x497d10
    void scale_lane(float arg);                                         // 0x498390
};

// ---- MissionUi, vtable 0x591c80 ------------------------------------------------------------------------------
typedef void (WidgetDtor::*DtorPF)(int);
typedef void (Effect::*AnimatePF)(int);
typedef void (c49aca0::PObj::*TextRectPF)(float*);
typedef void (MissionUi::*SetTextPF)(float*, const tstring*);

void MissionUi::d00(int release) { (((WidgetDtor*)this)->*(DtorPF)&WidgetDtor::destroy)(release); }
bool MissionUi::d01() { return false; }
void MissionUi::d02(int a) { (((Effect*)this)->*(AnimatePF)&Effect::animate)(a); }
void MissionUi::d03() { ((FieldPanel*)this)->update_bounds(); }
void MissionUi::d04(float a, int b) { set_a(a, b); }
void MissionUi::d05(float a, int b) { set_b(a, b); }
void MissionUi::d06(float a, int b) { set_c(a, b); }
void MissionUi::d07(float a, int b) { set_d(a, b); }
void MissionUi::d08(float a, float* p) { set_e(a, p); }
void MissionUi::d09(float a, float* p) { set_f(a, p); }
void MissionUi::d10(rgl::scene* sc, float* count, int c, int d) { render(sc, count, c, d); }
void MissionUi::d11(int v) { set15c(v); }
int MissionUi::d12() { return get15c(); }
void MissionUi::d13(int v) { set168(v); }
float MissionUi::d14() { return get168(); }
bool MissionUi::d15(int m) { return test_c8(m); }
void MissionUi::d16() { clear_c8(); }
bool MissionUi::d17(int m) { return clear_mask(m); }
void MissionUi::set_key_flags(int f) { or_flags(f); }
void MissionUi::d19(int v) { set_d0(v); }
void MissionUi::d20(int v) { set_d4(v); }
void MissionUi::d21(int m) { or_d4(m); }
void MissionUi::d22(void* p) { move_a((float*)p); }
void MissionUi::d23(MissionUi* src) { (void)src; }
void MissionUi::d24() {}
void MissionUi::set_xy(float* v) { move_f0(v); }
void MissionUi::d26(float* v) { move_f8(v); }
void MissionUi::set_size(float* v) { set_text_pos(v); }
void MissionUi::set_offset(float* v) { set138(v); }
void MissionUi::d29(float* v) { set148(v); }
void MissionUi::d30(float* v) { or148(v); }
void MissionUi::d31(int a, int b) { run(a, b); }
void MissionUi::d32(int a, int b) { set_track(a, (rgl::mesh*)b); }
void MissionUi::d33(int a) { ((MeshX*)this)->normalize_uv((MeshX*)a); }
void MissionUi::d34(void* a) { (void)a; }
char MissionUi::d35(unsigned mask) { return ((gui_window*)this)->refresh_all(mask); }
void MissionUi::refresh() { ((gui_window*)this)->wheel_step(); }
void MissionUi::get_xy(float* v) { (((c49aca0::PObj*)this)->*(TextRectPF)&c49aca0::PObj::text_rect)(v); }
void MissionUi::set_color(unsigned* c) { set14c((float*)c); }
void MissionUi::d39(void* p) { set_target(p); }
void MissionUi::d40(int p) { set_pos(p); }
void MissionUi::d41(float* out, const tstring* s) { set_text(out, s); }
void MissionUi::d42(tstring* out) { put(out); }
void MissionUi::d43(const tstring* s, const float* p) { ((MissionUiText*)this)->layout_text_to(s, p); }
void MissionUi::d44() { hide(); }
void MissionUi::d45() { show(); }
void MissionUi::d46() { flag2_set(); }
void MissionUi::d47() { flag2_clear(); }
int MissionUi::d48() { return 0; }

// ---- UiA, vtable 0x591d58 ------------------------------------------------------------------------------------
bool UiA::d01() { return true; }
void UiA::d10(rgl::scene* sc, float* count, int c, int d) { update(sc, count, c, d); }
char UiA::d35(unsigned mask) { return key(mask); }
void UiA::d44() { flag1(); }
void UiA::d46() { flag2(); }
int UiA::d48() { return type(); }

// ---- UiB, vtable 0x591fb0 ------------------------------------------------------------------------------------
void UiB::d02(int a) { ((HudBar*)this)->update_4(a); }
void UiB::d10(rgl::scene* sc, float* count, int c, int d) { show(sc, count, c, d); }
char UiB::d35(unsigned mask) { return key(mask); }
int UiB::d48() { return type2(); }

// ---- UiC, vtable 0x592078 ------------------------------------------------------------------------------------
void UiC::d02(int a) { ((HudBar*)this)->update_5(a); }
void UiC::d10(rgl::scene* sc, float* count, int c, int d) { show(sc, count, c, d); }
char UiC::d35(unsigned mask) { return ((UiB*)this)->key(mask); }
int UiC::d48() { return type(); }
void UiC::update_children() { ((HudBar*)this)->rebuild_frame(); }

// ---- UiD, vtable 0x592140 ------------------------------------------------------------------------------------
void UiD::d10(rgl::scene* sc, float* count, int c, int d) { show(sc, count, c, d); }
char UiD::d35(unsigned mask) { return key(mask); }
void UiD::update_children(float a) { ((HudBar*)this)->scale_lane(a); }

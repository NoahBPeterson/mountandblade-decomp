//-----------------------------------------------------------------------------
// File: widget_slots.cpp
//
// Desc: the virtual slots the widgets declared in widget_mgr.h (MediumButton 0x591e20, XyzButton 0x591ee8, UiAInline
//       0x591d58, NodeA 0x592848) and ui_widgets_c.h (UiE 0x592208) override. Each is a leaf the original's slot
//       also holds, or a stub /O2 compiles to a bare `jmp` to the function the original's slot names (see
//       ../ui/widget_slots.cpp for the scheme); the owner classes are declared minimally below.
//       Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "./widget_mgr.h"
#include "./ui_widgets_c.h"

struct HudBar {                                                         // world_data/mesh_hud.cpp
    void update_2(int arg);                                             // 0x4966b0
    void update_full(int arg);                                          // 0x4985f0
};
struct ContainerW {                                                     // world_data/world_data.cpp
    void place(int* a);                                                 // 0x490570
    void track_bounds(void* p);                                         // 0x4905c0
    void activate_all();                                                // 0x4906b0
    void reset_all();                                                   // 0x4906e0
    void bounds_of(int* a, float* p3, float* p4, float* p5);            // 0x490710
    void update_all(int p);                                             // 0x4908c0
    void add_child(MissionUi* child);                                   // 0x493a30
};
struct FrameBox { void release(); };                                    // 0x48ef40 (mission_ui2/mission_4.h)
struct gui_window {                                                     // strat/gui_manager.cpp
    char handle_event(unsigned mask);                                   // 0x42d170
    void wheel_focus();                                                 // 0x42cec0
};
typedef void (MissionUi::*MakeTextPF)(float*, const tstring*);

// ---- MediumButton, vtable 0x591e20 ------------------------------------------------------------------------------
bool MediumButton::d01() { return true; }
void MediumButton::d10(rgl::scene* sc, float* count, int c, int d) { ((UiA*)this)->update(sc, count, c, d); }
char MediumButton::d35(unsigned mask) { return ((UiA*)this)->key(mask); }
void MediumButton::d41(float* out, const tstring* s) { (this->*(MakeTextPF)&MissionUi::make_text)(out, s); }
void MediumButton::d44() { flag1(); }
void MediumButton::d46() { flag2(); }
int MediumButton::d48() { return ((UiA*)this)->type9(); }

// ---- XyzButton, vtable 0x591ee8 ---------------------------------------------------------------------------------
bool XyzButton::d01() { return true; }
void XyzButton::d02(int a) { ((HudBar*)this)->update_2(a); }
void XyzButton::d10(rgl::scene* sc, float* count, int c, int d) { ((UiA*)this)->update(sc, count, c, d); }
char XyzButton::d35(unsigned mask) { return ((UiA*)this)->key(mask); }
void XyzButton::d44() { flag1(); }
void XyzButton::d46() { flag2(); }
int XyzButton::d48() { return ((UiA*)this)->type8(); }

// ---- UiAInline, vtable 0x591d58 ---------------------------------------------------------------------------------
bool UiAInline::d01() { return true; }
void UiAInline::d10(rgl::scene* sc, float* count, int c, int d) { ((UiA*)this)->update(sc, count, c, d); }
char UiAInline::d35(unsigned mask) { return ((UiA*)this)->key(mask); }
void UiAInline::d44() { flag1(); }
void UiAInline::d46() { flag2(); }
int UiAInline::d48() { return ((UiA*)this)->type(); }

// ---- UiE, vtable 0x592208 ---------------------------------------------------------------------------------------
bool UiE::d01() { return true; }
void UiE::d02(int a) { ((HudBar*)this)->update_full(a); }
void UiE::d10(rgl::scene* sc, float* count, int c, int d) { update(sc, count, c, d); }
char UiE::d35(unsigned mask) { return key(mask); }
void UiE::d44() { flag1(); }
void UiE::d46() { flag2(); }
int UiE::d48() { return 6; }

// ---- NodeA, vtable 0x592848 -------------------------------------------------------------------------------------
void NodeA::d00(int release) { destroy(release); }
void NodeA::d02(int a) { ((ContainerW*)this)->update_all(a); }
void NodeA::d10(rgl::scene* sc, float* count, int c, int d) { ((ContainerW*)this)->bounds_of((int*)sc, count, (float*)c, (float*)d); }
void NodeA::d16() { ((ContainerW*)this)->reset_all(); }
void NodeA::d22(void* p) { ((ContainerW*)this)->place((int*)p); }
void NodeA::d23(MissionUi* child) { ((ContainerW*)this)->track_bounds(child); }
void NodeA::d24() { ((ContainerW*)this)->activate_all(); }
char NodeA::d35(unsigned mask) { return ((gui_window*)this)->handle_event(mask); }
void NodeA::refresh() { ((gui_window*)this)->wheel_focus(); }
void NodeA::update_children() { ((FrameBox*)this)->release(); }
void NodeA::d50(MissionUi* child) { ((ContainerW*)this)->add_child(child); }

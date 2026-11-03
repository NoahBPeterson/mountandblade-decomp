// ui_widgets_c.h: the single-line text editor widget UiE (vtable 0x592208), a MissionUi (../ui/mission_ui.h).
// Names provisional. Layout read off the constructor 0x48bb10, key 0x48bb50 and update 0x48bda0.
//
// The class is a UiA (ui/ui_widgets.h) with an edit state. Two constructors, because the binary has two code shapes for it:
//   - UiE()   the out-of-line one at 0x48bb10 (defined in ui_widgets_c.cpp);
//   - UiE(0)  the shape that widget_mgr.cpp (0x49a0b0, add_party) inlines: UiA's inline constructor stores f178/b121, the vptr
//             follows, then f180/f181/f184 (the order in the disassembly of 0x49a0d5..0x49a0f3).
#ifndef GAME_CONFIG_UI_WIDGETS_C_H
#define GAME_CONFIG_UI_WIDGETS_C_H
#include "../ui/ui_widgets.h"

// g_last_char: the UI root's f1718 (g_gui, 0x5d7080), read through g_gui in ui_widgets_c.cpp

struct UiE : UiA {                          // vtable 0x592208: the toggle button with an edit state (f178 "not editing", f180 editing, f181 selection, f184 capacity in code points)
    UiE();                                  // 0x48bb10
    UiE(int) { f180 = 0; f181 = 0; f184 = 0x1c; }   // inlined by 0x49a0b0
    // slots of vtable 0x592208 that differ from the base's
    virtual void animate(int a);            // +0x08 0x4985f0 (world_data/mesh_hud.cpp)
    virtual void render(rgl::scene* sc, float* count, int c, int d);   // +0x28 0x48bda0 (ret 0x10)
    virtual char key(unsigned mask);        // +0x8c 0x48bb50
    virtual int kind();                     // +0xc0 widget type code 6 (0x4171f0)
};

#endif

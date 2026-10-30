// ui_widgets_c.h: the single-line text editor widget UiE (vtable 0x592208), a MissionUi (../ui/mission_ui.h).
// Names provisional. Layout read off the constructor 0x48bb10, key 0x48bb50 and update 0x48bda0.
//
// Two constructors, because the binary has two code shapes for the same class:
//   - UiE()   the out-of-line one at 0x48bb10 (stores f180/f181 before the vptr; defined in ui_widgets_c.cpp);
//   - UiE(0)  the shape that widget_mgr.cpp (0x49a0b0, add_party) inlines: the intermediate base UiEBase stores
//             f178/b121, the vptr follows, then f180/f181/f184 (the order in the disassembly of 0x49a0d5..0x49a0f3).
#ifndef GAME_CONFIG_UI_WIDGETS_C_H
#define GAME_CONFIG_UI_WIDGETS_C_H
#include "../ui/ui_widgets.h"

// g_last_char: the UI root's f1718 (g_gui, 0x5d7080), read through g_gui in ui_widgets_c.cpp

struct UiEBase : MissionUi {
    int f178;                               // +0x178 "not editing" (0x48bb10 clears, 0x48bda0 sets)
    char pad17c[4];
    char f180;                              // +0x180 editing (0x48bb50)
    char f181;                              // +0x181 has selection (0x48bb50)
    char pad182[2];
    int f184;                               // +0x184 capacity in code points (0x1c, 0x48bb10)
    UiEBase() {}
    UiEBase(int) { f178 = 0; b121 = 0; }
};

struct UiE : UiEBase {                      // vtable 0x592208
    UiE();                                  // 0x48bb10
    UiE(int) : UiEBase(0) { f180 = 0; f181 = 0; f184 = 0x1c; }   // inlined by 0x49a0b0
    char key(unsigned p);                   // 0x48bb50
    void update(void* a, float* b, int c, int d);   // 0x48bda0 (ret 0x10)
    // slots of vtable 0x592208 that differ from the base's (config/widget_slots.cpp)
    bool d01();                             // +0x04 returns 1 (0x48b170)
    void d02(int a);                        // +0x08 -> HudBar::update_full 0x4985f0
    void d10(rgl::scene* sc, float* count, int c, int d);   // +0x28 -> update
    char d35(unsigned mask);                // +0x8c -> key
    void d44();                             // +0xb0 -> MissionUi::flag1 0x48b1a0
    void d46();                             // +0xb8 -> MissionUi::flag2 0x48b180
    int d48();                              // +0xc0 widget type code 6 (0x4171f0)
};

#endif

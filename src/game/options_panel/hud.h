// hud.h: the mission screen ("Hud") and its mission-item source, the widgets and the menu they drive
// (0x4d5290..0x4d5ba0). Offset stubs from the functions of hud.cpp (cited per field); names provisional.
// The slices s004d4b80 (handle_buttons) and s004d5410 (the callbacks) each declared a Hud; this is the union.
#ifndef OPTIONS_PANEL_HUD_H
#define OPTIONS_PANEL_HUD_H
#include <windows.h>
#include <math.h>
#include "../tstring.h"

// 0x495700 (thiscall, ret 4) on 0xa73980: pick a main-menu option.
#include "../main_menu.h"                         // MainMenu (0xa73980): the one definition

// the widgets the menu options write back into (each int* is a widget: +0x4 = option index, +0x18 = a second
// index, +0x8c = value)
extern int* g_menu_w1;                              // 0xa7398c
extern int* g_menu_w3;                              // 0xa739ac
extern int* g_menu_w4;                              // 0xa739c8
extern int* g_menu_w5;                              // 0xa739d4
extern int* g_menu_w6;                              // 0xa739f0
extern char g_saved;                               // 0x5e2a08

struct MenuScreen {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    char pad04[0x18 - 4];                           // (the vptr is at +0; the screen base fills up to +0x18; 0x4d59b9 writes +0x18)
    int  f18;                                      // +0x18
};
extern MenuScreen* g_screen;                        // 0xa739b8

#include "../mission.h"   // canonical: g_mission (0x5e292c)
extern int   g_u32;                                 // 0xa735c0
extern float g_char_scale;                          // 0x590d7c

// the strings the mission items write and the numbers their sounds depend on
#include "../dialog_manager.h"   // canonical: g_script (0xa70440)
extern __int64 g_spawn_lo;                          // 0xa70448
extern __int64 g_spawn_hi;                          // 0xa70450
extern float g_mouse_x;                             // 0x5d7094
extern float g_mouse_y;                             // 0x5d7098
extern float g_tick_scale;                          // 0x54a6d0 (1e-5f: fmul dword)
extern float g_day_scale;                           // 0x591834 (fmul dword)
extern double g_day_period;                         // 0x54a5d8
extern float g_hour_scale;                          // 0x590c90
extern float g_hour_base;                           // 0x54ab28
extern int g_ticks_[2];                             // 0x5e2948 (the 64-bit tick clock)
extern int g_mission_clock;                         // 0x5d8790 (mission time; 0 = no mission)
extern int g_a170d8;                                // 0xa170d8
extern int g_a174d8;                                // 0xa174d8

extern "C" double __cdecl fmod(double x, double y); // 0x536a6a
#include "../../include/crt.h"   // canonical: ftol2 (0x535a64)

#define ABS(x) ((float)fabs(x))

// sound holder inside a MenuKind (s0048add0): count of 0x30-byte children at +0, the child array at +4.
// update(0xc3 / 0xc2 / 0xc1) advances the children with that flag.
#include "../ui/timer_list.h"                    // TimerList: count +0, items +4 (the ui module declares the children)
struct MenuKind {                                   // 0x24 bytes (table at 0xa75f2c; cls_map.h MenuKindRec)
    char      pad0[0x14];
    TimerList sound;                                // +0x14
    char      pad1c[0x24 - 0x1c];
};
extern MenuKind* g_presentations;                      // 0xa75f2c (menu-kind table, element stride 0x24: 0x4d546c lea eax,[eax+eax*8] / [ecx+eax*4+0x14])
inline void play_family_sound(int family, int id)
{
    g_presentations[family].sound.update(id);
}

// ---- widgets -------------------------------------------------------------------------------------------
// control widget: "activate" at vtable +0x2c, "value" at +0x30, "float" at +0x38, "used" at +0x44,
// "key flags" at +0x48, "kind" at +0xc0 (slots cross-checked with s004cf7b0 / s004dd830).
#include "../ui_widget.h"                 // Widget (the UI widget interface)

// the progress-bar data source (see s0048b760): +0x168..+0x170 the current value and its range,
// +0x178 the slot index, +0x17c the step, +0xf0 the per-slot screen values.
// UiD (ui/ui_widgets.h, whose g_tick_scale clashes with this header's): the progress bar (vtable 0x592140) that ValueSource views
// (+0x178 index, +0x17c step); the two members the sources call.
struct UiD {
    float value();   // 0x48b930
    void set_value(float x);   // 0x48baa0
};
struct ValueSource {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09(); virtual void v0a(); virtual void v0b();
    virtual void v0c(); virtual void v0d(); virtual void v0e(); virtual void v0f();
    virtual void v10(); virtual void v11();
    virtual void set_key_flags(int f);              // +0x48
    char  pad04[0xf0 - 0x04];
    float slot[1];                                  // +0xf0
    char  pad1[0x168 - 0xf4];
    float f168;                                     // +0x168 (current value)
    float f16c;                                     // +0x16c (range low)
    float f170;                                     // +0x170 (range high)
    char  pad2[0x178 - 0x174];
    int   idx;                                      // +0x178
    float step;                                     // +0x17c
    __forceinline float value() { return ((UiD*)(void*)this)->value(); }                    // UiD::value 0x48b930
    __forceinline void  set_value(float v) { ((UiD*)(void*)this)->set_value(v); }           // UiD::set_value 0x48baa0
    void  on_value_changed(float arg);              // 0x4d5940
};
// value source reached through an item pointer (Hud+0x30 sources): only the vtable slots matter
typedef ValueSource ValueWidget;

// an item record: a Widget with the offset vector at +0xe0 (x0,y0,w,h), the mesh animation nodes at
// +0x108, a name at +0x128 (string) and two item flags at +0x180/+0x184.
struct MissionItem : Widget {
    char         pade0[0xe0 - sizeof(Widget)];
    float        org[2];                            // +0xe0 x, +0xe4 y
    char         pade8[0xf0 - 0xe8];
    float        ext[4];                            // +0xf0 x far, +0xf4 y far, +0xf8 x near, +0xfc y near (offsets
                                                    // from org; 0x4d54f5..0x4d55d3 test org+near <= mouse < org+far)
    char         pad100[0x108 - 0x100];
    void*        nodes[5];                          // +0x108 .. +0x118
    char         pad11c[0x128 - 0x11c];
    tstring      name;                              // +0x128
    char         pad138[0x180 - 0x138];
    char         f180;                              // +0x180
    char         pad181[3];
    int          f184;                              // +0x184
};

// the per-item value source is a pointer at item+0x484
inline ValueWidget* item_value(MissionItem* it) { return *(ValueWidget**)((char*)(void*)it + 0x484); }

// the item source (0x4d5410's `this`, Hud+0x30): an STL vector of MissionItem* at +0x4/+0x8/+0xc, the
// item family at +0x10 (its sound is the family-th ItemSound), the current clock slot at +0x18 and the
// 64-bit tick the slot was entered at (+0x20/+0x24).
#include "../mission_source.h"                  // MissionSource (pick 0x4d5410)


// ---- the screen object owning the callbacks -------------------------------------------------------------
struct Hud {
    char    pad00[0x4];
    int     f04;                                    // +0x4 (page mode)
    char    pad08[0x10 - 8];
    int     f10;                                    // +0x10
    char    pad14[0x18 - 0x14];
    int     f18;                                    // +0x18
    char    pad1c[0x30 - 0x1c];
    Widget* f30;                                    // +0x30 (menu button)
    Widget* f34;                                    // +0x34
    Widget* f38;                                    // +0x38
    MissionItem* f3c;                               // +0x3c (the clock item; its value source is at +0x484)
    char    pad40[0x60 - 0x40];
    Widget* f60;                                    // +0x60 (handle_buttons 0x4d5290: used flags open menu options)
    Widget* f64;                                    // +0x64
    Widget* f68;                                    // +0x68
    Widget* f6c;                                    // +0x6c (may be null)
    Widget* f70;                                    // +0x70
    Widget* f74;                                    // +0x74
    Widget* f78;                                    // +0x78
    char    pad7c[0x88 - 0x7c];
    int     f88;                                    // +0x88
    char    pad8c[0x24c - 0x8c];
    int     f24c;                                   // +0x24c
    int     f250;                                   // +0x250
    Widget* wi[1];                                  // +0x254 (page list)

    void handle_buttons(int arg);                   // 0x4d5290
    void on_mission_start(int arg);                 // 0x4d5980
    void on_leave(int arg);                         // 0x4d59f0
    void on_update(int arg);                        // 0x4d5a20
};

// A page object of the list at +0x254 (0x4d5ba0 fills it; only the call matters here).
#endif

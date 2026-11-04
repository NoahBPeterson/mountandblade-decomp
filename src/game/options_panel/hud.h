// hud.h: the mission screen ("Hud") and its mission-item source, the widgets and the menu they drive
// (0x4d5290..0x4d5ba0). Offset stubs from the functions of hud.cpp (cited per field); names provisional.
// The slices s004d4b80 (handle_buttons) and s004d5410 (the callbacks) each declared a Hud; this is the union.
#ifndef OPTIONS_PANEL_HUD_H
#define OPTIONS_PANEL_HUD_H
#include <windows.h>
#include <math.h>
#include "../tstring.h"

// 0x495700 (thiscall, ret 4) on 0xa73980: pick a main-menu option.
#include "../window_mgr.h"                         // g_windows (0xa73980)
#include "../key_names.h"

// the widgets the menu options write back into are slots of the window manager (window_mgr.h): slot 3 (0xa7398c), 11 (0xa739ac),
// 18 (0xa739c8), 21 (0xa739d4), 28 (0xa739f0); each is read as an int record: +0x4 = option index, +0x18 = a second index, +0x8c = value.
#include "../window_mgr.h"
#include "../strat/gui_manager.h"
#include "../shared_vars.h"                      // g_saved (0x5e2a08)

struct MenuScreen {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    char pad04[0x18 - 4];                           // (the vptr is at +0; the screen base fills up to +0x18; 0x4d59b9 writes +0x18)
    int  f18;                                      // +0x18
};

#include "../mission.h"   // canonical: g_mission (0x5e292c)
extern float g_char_scale;                          // 0x590d7c

// the strings the mission items write and the numbers their sounds depend on
#include "../dialog_manager.h"   // canonical: g_script (0xa70440)

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
#include "../game_globals.h"                        // g_presentations (0xa75f2c, canonical): its first dword points at the menu-kind table
inline void play_family_sound(int family, int id)
{
    ((MenuKind*)g_presentations.records)[family].sound.update(id);   // element stride 0x24: 0x4d546c lea eax,[eax+eax*8] / [ecx+eax*4+0x14]
}

// ---- widgets -------------------------------------------------------------------------------------------
// control widget: "activate" at vtable +0x2c, "value" at +0x30, "float" at +0x38, "used" at +0x44,
// "key flags" at +0x48, "kind" at +0xc0 (slots cross-checked with s004cf7b0 / s004dd830).
#include "../ui_widget.h"                 // MissionUi (the UI widget interface)
#include "../ui_globals.h"      // UI / core globals

// the progress-bar data source (see s0048b760): +0x168..+0x170 the current value and its range,
// +0x178 the slot index, +0x17c the step, +0xf0 the per-slot screen values.
// UiD (ui/ui_widgets.h): the progress bar (vtable 0x592140) that ValueSource views (+0x178 index, +0x17c step).
#include "../ui/ui_widgets.h"
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

// the per-item value source is a pointer at item+0x484
inline ValueWidget* item_value(MissionUi* it) { return *(ValueWidget**)((char*)(void*)it + 0x484); }

// the item source (0x4d5410's `this`, Hud+0x30): an STL vector of MissionItem* at +0x4/+0x8/+0xc, the
// item family at +0x10 (its sound is the family-th ItemSound), the current clock slot at +0x18 and the
// 64-bit tick the slot was entered at (+0x20/+0x24).
#include "../mission_source.h"                  // MissionSource (pick 0x4d5410)
#include "../../include/clock_table.h"


// A page object of the list at +0x254 (0x4d5ba0 fills it; only the call matters here).
#endif

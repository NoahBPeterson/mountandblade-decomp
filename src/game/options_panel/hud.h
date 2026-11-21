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

// the running presentation's triggers (PresentationRec::triggers): update(0xc3 / 0xc2 / 0xc1) runs the presentation's events
#include "../trigger_vecs.h"   // SimpleTriggerVec: the simple-trigger list
#include "../game_globals.h"                        // g_presentations (0xa75f2c): the PresentationRec records
inline void play_family_sound(int family, int id)
{
    g_presentations.recs()[family].triggers.update(id);   // element stride 0x24: 0x4d546c lea eax,[eax+eax*8] / [ecx+eax*4+0x14]
}

// ---- widgets -------------------------------------------------------------------------------------------
// control widget: "activate" at vtable +0x2c, "value" at +0x30, "float" at +0x38, "used" at +0x44,
// "key flags" at +0x48, "kind" at +0xc0 (slots cross-checked with s004cf7b0 / s004dd830).
#include "../ui_widget.h"                 // MissionUi (the UI widget interface)
#include "../ui_globals.h"      // UI / core globals

// A widget of kind 7 is a scrolling list (Camera, party2/camera_class.h); its scroll bar, the UiD in follow[1], is the
// value the mouse wheel steps (UiD::on_value_changed 0x4d5940).
#include "../ui/ui_widgets.h"
#include "../party2/camera_class.h"
inline UiD* item_value(MissionUi* it) { return static_cast<Camera*>(it)->follow[1]; }

#include "../presentation.h"                    // widget_mgr: the presentation (pick 0x4d5410 is its per-frame handler)
#include "../../include/clock_table.h"


// A page object of the list at +0x254 (0x4d5ba0 fills it; only the call matters here).
#endif

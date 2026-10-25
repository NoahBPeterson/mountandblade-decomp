#ifndef SCREEN_INIT_H
#define SCREEN_INIT_H
// Slice s004ec4d0: the campaign screen's UI construction and the map-camera entity pass.
//  0x4ec4d0  Screen::init_screen  rebuilds the whole campaign UI: resets the GUI, creates the map bar
//                                overlay (+0x2c), the button overlay (+0x34) with its terrain map and the six
//                                translated menu buttons, two hint widgets, the paused banner (+0x5c), the
//                                wait-hint widget (+0x58) and the three-dimensional map widget (+0x30).
//  0x4ecce0  camera_mark_entities scales the camera zoom into g_game+0x13acc and walks the party list,
//                                setting +0x8b5 = 1 on every party whose scene object (+0x59c0) exists and
//                                whose flags (+0xa0) have 0x400 but not 0x80000.
// flags /O2 /MT /Gy /TP
#include <new>
#include <string.h>
#include "rgl.h"

// ---- engine string (16 bytes, see s0042a1f0/tstring.h) --------------------------------------------------
#include "../tstring.h"

// ---- widgets and overlays (src/game/ui_widget.h, overlay.h, map_widget.h) -----------------------------------
#include "../ui_widget.h"
#include "../overlay.h"
#include "../map_widget.h"

// ---- the campaign screen (`this` of 0x4ec4d0 / 0x4ecce0) -----------------------------------------------
#include "../campaign_screen.h"            // Screen (the campaign map screen)

// ---- game objects -------------------------------------------------------------------------------------
struct Rect {                                   // the 0x10-byte region copied into Screen+0x290
    int x, y, w, h;
};
#include "../../include/game_data.h"            // GameData (g_game, 0x5e2930): the one definition (was a Game view here)

#include "../gui_mgr.h"
struct ResMgr { void* get_object(const tstring& n); };
#include "../str_table.h"                      // StrTable (0x5e2a18; was StrTab here: find(name) was lookup 0x43a1e0)

#define g_gui     (*(GuiManager*)0x5d7080)
#define g_resmgr  (*(ResMgr*)0x5d6fa0)
#define g_strtab  (*(StrTable*)0x5e2a18)

extern char g_5e212c;                           // 0x5e212c
extern char g_5d8788;                           // 0x5d8788
extern int g_5a9fb0;                            // 0x5a9fb0 (no terrain camera)
#include "../renderer.h"
extern int g_level;                            // 0xa732b4 (current scene)
extern int g_a70438;                            // 0xa70438 (last-loaded scene)
extern float g_590bb8;                          // 0x590bb8
extern float g_549cdc;                          // 0x549cdc (1.0f)
extern float g_54b3b4;                          // 0x54b3b4

// ---- callees -------------------------------------------------------------------------------------------
void ctor_4931a0(void* p);                      // 0x4931a0 (ecx = object)
void __fastcall stop_all(void* p);           // 0x494310 (ecx = 0x5aa128: the campaign UI root)
void* clone(void* m);                    // 0x41c6d0
void* __fastcall make_short_button(Overlay* o, void* name);   // 0x4e5570
Widget* new_medium_button(Overlay* o, void* text, float x, float y, float z);   // 0x4dce70
#include "../../include/vec.h"                      // vector2 (= Vec2)
WidgetObj* make_hint(const tstring& text, int a, Overlay* o, vector2 p, vector2 s);   // 0x4dada0
void __fastcall ctor_4ed630(MapWidget* w);       // 0x4ed630

// the party manager at g_game+0xf8 (include/party.h)
#include "../../include/party.h"

// (the 0x5ac8-byte party record is Party, src/include/party.h; the slice view PartyRec was unused here)
struct GameScene {                              // g_psys (0xa70248)
    void attach_camera(int scene);                     // 0x4711e0
    void extend(void* p);                // 0x468c20
    void fit_bounds();                               // 0x463cc0
    void render();                               // 0x47fef0
};
#define g_psys (*(GameScene*)0xa70248)

static Widget* new_widget(int size)
{
    Widget* w = (Widget*)operator new(size);
    if (w != 0)
        new (w) Widget();
    else
        w = 0;
    return w;
}

#endif

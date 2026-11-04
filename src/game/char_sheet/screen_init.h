#ifndef SCREEN_INIT_H
#define SCREEN_INIT_H
// Slice s004ec4d0: the campaign screen's UI construction and the map-camera entity pass.
//  0x4ec4d0  CampaignScreen::init_screen  rebuilds the whole campaign UI: resets the GUI, creates the map bar
//                                overlay (+0x2c), the button overlay (+0x34) with its terrain map and the six
//                                translated menu buttons, two hint widgets, the paused banner (+0x5c), the
//                                wait-hint widget (+0x58) and the three-dimensional map widget (+0x30).
//  0x4ecce0  camera_mark_entities scales the camera zoom into g_game+0x13acc and walks the party list,
//                                setting +0x8b5 = 1 on every party whose scene object (+0x59c0) exists and
//                                whose flags (+0xa0) have 0x400 but not 0x80000.
// flags /O2 /MT /Gy /TP
#include "../../include/image_addrs.h"
#include <new>
#include <string.h>
#include "../../rgl/rgl_math.h"

// ---- engine string (16 bytes, see s0042a1f0/tstring.h) --------------------------------------------------
#include "../tstring.h"

// ---- widgets and overlays (src/game/ui_widget.h, ui/container.h, map_widget.h) -----------------------------------
#include "../ui_widget.h"
#include "../ui/container.h"
#include "../map_widget.h"
#include "./cs_common.h"
#include "../res_mgr.h"                                 // g_resmgr
#include "../../rgl/mesh.h"                           // rgl::mesh::clone 0x41c6d0

// ---- the campaign screen (`this` of 0x4ec4d0 / 0x4ecce0) -----------------------------------------------
#include "../campaign_screen.h"            // CampaignScreen (the campaign map screen)

// ---- game objects -------------------------------------------------------------------------------------
struct Rect {                                   // the 0x10-byte region copied into CampaignScreen+0x290
    int x, y, w, h;
};
#include "../../include/game_data.h"            // GameData (g_game, 0x5e2930): the one definition (was a Game view here)

#include "../strat/gui_manager.h"
#include "../str_table.h"                      // StrTable (0x5e2a18; was StrTab here: find(name) was lookup 0x43a1e0)

#define g_strtab  g_strtable

#include "../../include/renderer.h"
extern const float g_590bb8;                        // 0x590bb8
extern const float g_54b3b4;                        // 0x54b3b4

// ---- callees -------------------------------------------------------------------------------------------
// (stop_all 0x494310, clone 0x41c6d0, make_short_button 0x4e5570, new_medium_button 0x4dce70, make_hint 0x4dada0, ctor_4ed630: cs_common.h / cs_res.h)
#include "../../include/vec.h"                      // vector2 (= Vec2)

// the party manager at g_game+0xf8 (include/party.h)
#include "../../include/party.h"
#include "../../include/level_global.h"      // g_level (0xa732b4)
#include "../ui_globals.h"      // UI / core globals
#include "../float_consts.h"

// (the 0x5ac8-byte party record is Party, src/include/party.h; the slice view PartyRec was unused here)
extern ShadowMap g_shadow_map;                                        // 0xa70248 (cs_common.h's view of the shadow-map object)
#define g_psys ((char*)&g_shadow_map)                         // the shadow / particle system (ShadowMap, ShadowMapP: cs_common.h)

static MissionUi* new_widget(int size)
{
    return (MissionUi*)new_widget_base(size);   // operator new, then the widget base constructor 0x48ae20
}

#endif

// party_screen.h: the party / prisoner management screen (code 0x502cc0..0x507400), shown from the map and from the
// mission. The slices called its class Mission, PartyWindow, party_window and party_menu; it is none of those
// (src/game/mission/mission.h Mission is the mission object, src/game/ui/screens/inventory/inventory_window.h PartyWindow the inventory window).
// One layout for all functions; every offset cites the function that proves it. Names provisional.
#ifndef GAME_INVENTORY_PANEL_PARTY_SCREEN_H
#define GAME_INVENTORY_PANEL_PARTY_SCREEN_H
#include "platform/image_addrs.h"
#include "game/mission/inv_common.h"
#include "game/data/module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in game/app/options_dialog_and_module_tables.cpp)
#include "game/ui/ui_globals.h"      // UI / core globals

#include "game/ui/screens/party_screen_class.h"      // PartyScreen: the record of vtable 0x595870 (ONE class with the exchange window), PsRow

// ---- widget classes this screen builds: game/ui/widgets/container.h (`new NodeA(0)`, `new text_box`, `new ContainerBase`) ----
#include "game/ui/widgets/container.h"
#include "game/ui/widgets/ui_widgets.h"        // UiA: the member buttons
#include "game/mission/camera_class.h"            // Camera: the 0x4a8-byte scrolling list (ctor 0x49deb0): flags +0x104, scrollbar +0x484 (follow[1]), list +0x490 (target)


// ---- objects and functions of other translation units --------------------------------------------------------------
#include "game/common/game_globals.h"      // g_module_settings (canonical)
#include "game/data/tableau.h"                    // TableauTbl g_tableaus 0xa75f40
struct UiA; struct Camera; struct NodeA;
static __forceinline void __fastcall panel_ctor(void* p) { new (p) MissionUi; }                   // 0x48ae20 (MissionUi::MissionUi)
UiA* __cdecl make_short_button(MissionUi* parent, const tstring* text, float a, float b, float h);   // 0x4e5570
UiA* __cdecl new_medium_button(MissionUi* parent, const tstring* text, float a, float b, float h);   // 0x4dce70
UiA* __cdecl new_long_button(MissionUi* parent, const tstring* text, float a, float b, float h);     // 0x4dcbd0
static __forceinline Camera* new_scroll_list()
{
    return new Camera;                                // 0x49deb0
}
struct UiD;
UiD* __cdecl make_scrollbar(ContainerBase* parent, const tstring* name);                        // 0x4f6160
Camera* __cdecl make_editor_list(ContainerBase* parent, const tstring* name, UiD* sc);              // 0x4f62f0
static __forceinline MissionUi* make_short_button(MissionUi* p, tstring* t, int a, int b, float h)
{ return (MissionUi*)make_short_button(p, (const tstring*)t, (float)a, (float)b, h); }
static __forceinline MissionUi* new_medium_button(MissionUi* p, tstring* t, int a, int b, float h)
{ return (MissionUi*)new_medium_button(p, (const tstring*)t, (float)a, (float)b, h); }
static __forceinline MissionUi* new_long_button(MissionUi* p, tstring* t, int a, int b, float h)
{ return (MissionUi*)new_long_button(p, (const tstring*)t, (float)a, (float)b, h); }
UiA* new_sized_button(MissionUi* parent, const tstring& text, float a, float b, float c,
                      const tstring& name, const tstring& down, float k1, float k2, float k3);   // 0x4dcc50
static __forceinline void __fastcall quarter_update(MissionUi* w) { ((Camera*)w)->draw(); }   // 0x488d50
int skill_level(int member, int skill);            // 0x437b20 (GameData::skill_level, called here with the global as an argument)

#endif // GAME_INVENTORY_PANEL_PARTY_SCREEN_H

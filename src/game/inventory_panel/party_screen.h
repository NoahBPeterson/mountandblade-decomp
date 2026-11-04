// party_screen.h: the party / prisoner management screen (code 0x502cc0..0x507400), shown from the map and from the
// mission. The slices called its class Mission, PartyWindow, party_window and party_menu; it is none of those
// (src/game/mission.h Mission is the mission object, src/game/party/party_window.h PartyWindow the inventory window).
// One layout for all functions; every offset cites the function that proves it. Names provisional.
#ifndef GAME_INVENTORY_PANEL_PARTY_SCREEN_H
#define GAME_INVENTORY_PANEL_PARTY_SCREEN_H
#include "../../include/image_addrs.h"
#include "./inv_common.h"
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)
#include "../ui_globals.h"      // UI / core globals

#include "../party_screen_class.h"      // PartyScreen: the record of vtable 0x595870 (ONE class with the exchange window), PsRow

// ---- widget classes this screen builds (all derive from the widget base MissionUi, ctor 0x48ae20, 0x178 bytes) ----------
struct PsWindow : MissionUi {                       // 0x190 bytes: base ctor 0x4931a0 (vptr 0x592848 / 0x597050 stored by the caller)
    std::vector<MissionUi*> panels;                 // +0x178 child vector (0x502cc0, 0x504ef0)
    char f188;                                      // +0x188
};
struct PsFrame : MissionUi {                        // 0x188 bytes, ctor 0x4931a0
    int f178;                                       // +0x178 (left uninitialised by the ctor)
    int f17c;                                       // +0x17c
    int f180;                                       // +0x180
    unsigned f184;                                  // +0x184
};
struct PsBar : PsFrame {};                          // vptr 0x597050 (implicit ctor: base call then the vptr store)
struct PsListWidget : MissionUi {                   // 0x188 bytes, vptr 0x5924a0, the ctor is inlined at the `new`
    int f178, f17c, f180;
    unsigned f184;
};
#include "../party2/camera_class.h"            // Camera: the 0x4a8-byte scrolling list (ctor 0x49deb0): flags +0x104, scrollbar +0x484 (follow[1]), list +0x490 (target)


// ---- objects and functions of other translation units --------------------------------------------------------------
#include "../game_globals.h"      // g_module_settings (canonical)
extern int g_item_sel;                              // 0xa75f6c (the selected item index; negative = nothing selected)
extern ObjTable g_tableaus;                          // 0xa75f40
struct TableauTbl {
    rgl::mesh* make_mesh_from(const int* index, const void* from);   // 0x4991d0 (game/world_data/tableau.h; ObjTable is the same table)
};
// Definitions elsewhere under their own declarations: the class stubs carry only the member called here, the forwarders keep
// the spelling of the call sites (the widget views are MissionUi pointers).
struct ContainerW {
    void* construct();   // 0x4931a0 (world_data: the list container's constructor body, returns this)
};
struct UiA; struct Camera; struct NodeA;
static __forceinline void __fastcall window_ctor(void* p) { ((ContainerW*)p)->construct(); }   // 0x4931a0
static __forceinline void __fastcall panel_ctor(void* p) { new (p) MissionUi; }                   // 0x48ae20 (MissionUi::MissionUi)
UiA* __cdecl make_short_button(MissionUi* parent, const tstring* text, float a, float b, float h);   // 0x4e5570
UiA* __cdecl new_medium_button(MissionUi* parent, const tstring* text, float a, float b, float h);   // 0x4dce70
UiA* __cdecl new_long_button(MissionUi* parent, const tstring* text, float a, float b, float h);     // 0x4dcbd0
// the `new` expressions of the widget classes: allocate, run the constructor 0x4931a0 / 0x49deb0 of the definitions, and store the
// class's vtable pointer by its address (0x597050), as the original does after the call
static __forceinline PsBar* new_ps_bar()
{
    PsBar* b = (PsBar*)::operator new(0x188);
    if (b != 0) {
        ((ContainerW*)b)->construct();
        *(void**)b = (void*)vtbl_597050;
    }
    return b;
}
static __forceinline PsListWidget* new_ps_list()      // 0x188 bytes, the constructor inlined: base 0x48ae20, three zeroed words, vptr 0x5924a0
{
    PsListWidget* w = (PsListWidget*)::operator new(0x188);
    if (w != 0) {
        new (w) MissionUi;
        w->f17c = 0;
        w->f180 = 0;
        w->f184 = 0;
        *(void**)w = (void*)vtbl_5924a0;
    }
    return w;
}
static __forceinline Camera* new_scroll_list()
{
    void* m = ::operator new(0x4a8);
    if (m != 0)
        new (m) Camera;                                // 0x49deb0
    return (Camera*)m;
}
struct UiD;
UiD* __cdecl make_scrollbar(void* parent, const tstring* name);                        // 0x4f6160
Camera* __cdecl make_editor_list(void* parent, const tstring* name, int sc);              // 0x4f62f0
static __forceinline MissionUi* make_short_button(MissionUi* p, tstring* t, int a, int b, float h)
{ return (MissionUi*)make_short_button(p, (const tstring*)t, (float)a, (float)b, h); }
static __forceinline MissionUi* new_medium_button(MissionUi* p, tstring* t, int a, int b, float h)
{ return (MissionUi*)new_medium_button(p, (const tstring*)t, (float)a, (float)b, h); }
static __forceinline MissionUi* new_long_button(MissionUi* p, tstring* t, int a, int b, float h)
{ return (MissionUi*)new_long_button(p, (const tstring*)t, (float)a, (float)b, h); }
static __forceinline MissionUi* make_member_list(MissionUi* p, const tstring* n, MissionUi* sc)
{ return (MissionUi*)make_editor_list((void*)p, n, (int)sc); }
static __forceinline MissionUi* make_editor_list(MissionUi* p, const tstring* n, MissionUi* sc)
{ return (MissionUi*)make_editor_list((void*)p, n, (int)sc); }
static __forceinline UiD* make_scrollbar(MissionUi* p, const tstring* n)
{ return make_scrollbar((void*)p, n); }
MissionUi* __cdecl new_member_button(MissionUi* parent, const tstring& name, float x, int zero, float h,
                                     const tstring& text, const tstring& pressed, float width_ratio,
                                     float height, float ratio);                              // 0x4dcc50
static __forceinline void __fastcall quarter_update(MissionUi* w) { ((Camera*)w)->draw(); }   // 0x488d50
int skill_level(int member, int skill);            // 0x437b20 (GameData::skill_level, called here with the global as an argument)

#endif // GAME_INVENTORY_PANEL_PARTY_SCREEN_H

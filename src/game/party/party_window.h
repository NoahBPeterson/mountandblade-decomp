// party_window.h: the party screen's inventory window (code 0x501a20..0x502cc0; the class is the window object that
// owns the equipment menus, the inventory pages and the armour / encumbrance labels). Names provisional.
//
// Layout (all offsets from `this`; each cites the function that proves it):
//   +0x04 f4      0x502320, 0x502920   (non-zero: the other hero is shown)
//   +0x14 troop   0x502320             (troop index of the second page / row)
//   +0x3c panels  0x501a20             (the equipment menu panel of row idx)
//   +0x64 widgets 0x502320, 0x502920   (slot widgets of the inventory pages, 0x180 bytes per page)
//   +0x364 flags  0x502920, 0x502320   (per slot state: bit 2 = needs a mesh)
//   +0x664 aux    0x502320             (0x12fd1 per slot)
//   +0x964 pages, +0x968 rows, +0x96c container                    0x5025a0, 0x5024e0, 0x501a20
//   +0x970 row_slot, +0x9c0 row_state   (ten slots per row, 0x28 bytes per row)    0x501a20, 0x502c30
//   +0xa64 armor labels (two sets of four)                           0x5025a0
//   +0xa84 / +0xa88 strip icons, +0xac0 / +0xac4 their specs         0x5029f0
#ifndef GAME_PARTY_PARTY_WINDOW_H
#define GAME_PARTY_PARTY_WINDOW_H
#include "../tstring.h"
#include "../game_globals.h"      // canonical g_disable_food_slot
#include "../../rgl/rgl_math.h"
#include "../ui/item_ref.h"
#include "../ui/troop_inv.h"
#include "../ui/ui_widgets.h"
#include "./game.h"
#include "./widget_ops.h"
#include "./externs.h"

// ---- objects of other translation units ----------------------------------------------------------------
#include "../config_table.h"               // ConfigEntry, ConfigTable, g_config, CONFIG_ENTRIES: the one definition

#include "../res_mesh.h"               // ResMesh (was ResMesh), ResMgr, ObjTable: the one definition
#include "../../rgl/resource.h"          // resource_manager g_resmgr 0x5d6fa0
#define g_resmgr_party (*(ResMgr*)&g_resmgr)   // the 0x5d6fa0 object seen as the pack-lookup view ResMgr
extern ObjTable g_tableaus;                      // 0xa75f40
namespace rgl { struct mesh; }
rgl::mesh* __cdecl make_item_mesh(const ItemRef* ref, const float* scale);   // 0x4fba40 (ret 0): the mesh of an item reference (the definition, agent5/agent_hud.cpp, returns rgl::mesh*)

void __cdecl label_refresh(void* w, tstring name);          // 0x4e4c70 (cdecl, the name by value)
#include "../item_sound.h"               // ItemSound (ItemKind +0x228): the one definition
inline float item_f208(ItemKind* k) { return *(float*)((char*)k + 0x208); }       // +0x208 base armour value   0x502620
inline ItemSound* item_sound(ItemKind* k) { return (ItemSound*)((char*)k + 0x228); }   // 0x502b80

extern int g_equip_page;                    // 0xa75f68 (negative: nothing selected)  0x5024e0
struct SpawnSliders {                       // 0xa70448: reset by 0x502b80
    int lo, hi;                             // 0xa70448, 0xa7044c
    __int64 pos;                            // 0xa70450 (lo word = troop, hi word = its sign)
};
struct ScriptState;
extern ScriptState g_script;                // 0xa70440 (dialog_manager.h; its g_game clashes with game.h)

// ---- widgets -------------------------------------------------------------------------------------------
struct UiWindow : MissionUi {               // 0x188 bytes, constructor 0x4931a0
    char pad178[0x188 - 0x178];
    UiWindow();                             // 0x4931a0
};
struct PartyPanel : UiWindow {              // vtable 0x599bf8 (the equipment menu / inventory page block)
    PartyPanel() { *(void**)this = (void*)0x599bf8; }
};
struct PartySlot : MissionUi {              // vtable 0x599d28, size 0x178 (the item slot)
    PartySlot() { *(void**)this = (void*)0x599d28; }
};
struct StripIcon : UiA {                    // vtable 0x591ee8, size 0x188 (constructor 0x48b1c0 of UiA)
    StripIcon() { *(void**)this = (void*)0x591ee8; }
};
#include "../ui/ui_frame.h"                  // UiFrame (vtable 0x5924a0): the one definition
// the widget base's destructor body (0x487c00) and the scalar deleting destructor built on it (0x502300, the shared
// slot 0 of vtable 0x591c80; mission_ui.h declares that slot as d00(int) without a body)
// 0x487c00 is Buffer::~Buffer (mesh_slot/slot.cpp, virtual; called here as the base destructor, so qualified)
struct Buffer { virtual ~Buffer(); };
struct WidgetDtor {
    void base_dtor() { ((Buffer*)this)->Buffer::~Buffer(); }      // 0x487c00
    void* destroy(unsigned char flag);      // 0x502300 (ret 4)
};
// 0x41f4b0 is vec<mface>::clear (rgl/shapes/shapes.h: the vector's out-of-line clear, shared by every pointer vector); only the member is declared
struct mface;
template <class T> struct vec { void clear(); };

// 0x42cf50 is gui_window::mouse_event (strat/gui_manager.h): the base class's handler of the window below
struct gui_window { char mouse_event(unsigned mask); };

// ---- the window ---------------------------------------------------------------------------------------
struct PartyWindow {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03(); virtual void v04();
    virtual void v05(); virtual void v06(); virtual void v07(); virtual void v08(); virtual void v09();
    virtual void v10(); virtual void v11(); virtual void v12(); virtual void v13(); virtual void v14();
    virtual void v15(); virtual void v16(); virtual void v17();
    virtual void on_input(unsigned mask);   // +0x48
    virtual void v19(); virtual void v20(); virtual void v21(); virtual void v22(); virtual void v23();
    virtual void v24(); virtual void v25(); virtual void v26(); virtual void v27(); virtual void v28();
    virtual void v29(); virtual void v30(); virtual void v31(); virtual void v32(); virtual void v33();
    virtual void v34(); virtual void v35();
    virtual void refresh();                 // +0x90
    int f4;                                 // +0x04  0x502320, 0x502920
    char pad8[0x14 - 0x8];
    int troop;                              // +0x14  0x502320, 0x501a20
    char pad18[0x2c - 0x18];
    void* page_widget[2];                   // +0x2c  0x500ad0 (the page widget build_page(i) returns)
    void* view[2];                          // +0x34  0x500ad0 (the scrolling item lists)
    void* panels[2];                        // +0x3c  0x501a20 (equipment menu panel of row idx)
    void* gold[2];                          // +0x44  0x500ad0, 0x501940 (the money labels)
    void* name[2];                          // +0x4c  0x500ad0 (the troop name labels)
    void* hinge[4];                         // +0x54  0x500ad0 (the four window hinges)
    void* widgets[2][0x60];                 // +0x64  0x502320 (page * 0x180 + 0x64 + slot * 4)
    int flags[2][0x60];                     // +0x364 0x502920 (bit 2: the slot needs a mesh)
    int aux[2][0x60];                       // +0x664 0x502320
    int pages;                              // +0x964 0x5025a0
    int rows;                               // +0x968 0x5025a0 (at most two)
    void* container;                        // +0x96c 0x5024e0, 0x501a20
    void* row_slot[2][10];                  // +0x970 0x501a20, 0x502c30
    int row_state[2][10];                   // +0x9c0 0x501a20, 0x502c30
    char pada10[0xa60 - 0xa10];
    void* total_cost;                       // +0xa60 0x500ad0
    void* armor_labels[2][4];               // +0xa64 0x5025a0 (head, body, legs, encumbrance)
    void* icon_a84;                         // +0xa84 0x5029f0
    void* icon_a88;                         // +0xa88 0x5029f0
    int f_a8c;                              // +0xa8c 0x500ad0 (cleared)
    void* leave;                            // +0xa90 0x500ad0 (the leave button)
    char pada94[0xa98 - 0xa94];
    int f_a98;                              // +0xa98 0x500ad0 (cleared)
    char pada9c[0xab0 - 0xa9c];
    int f_ab0, f_ab4, f_ab8;                // +0xab0 0x500ad0 (-1)
    char f_abc;                             // +0xabc 0x500ad0 (the other party's hero is on the list)
    char padabd[3];
    int spec_ac0, spec_ac4;                 // +0xac0 0x5029f0
    void* equip_bg;                         // +0xac8 0x500ad0 (mode 4: the equipment panel background)

    void create();                                              // 0x500ad0 (builds the window: src/game/agent5/party_create.cpp)
    void money_label(int which);                                // 0x501940 (ret 4: src/game/agent5/party_create.cpp)
    void* make_inventory_menu(int idx);                         // 0x501a20 (ret 4)
    char mouse_event(unsigned mask);                            // 0x5021b0 (ret 4)
    MissionUi* make_slot(ItemRef* slot);                        // 0x502230 (ret 4)
    char refresh_event(unsigned mask);                          // 0x5022c0 (ret 4)
    void* build_page(int page);                                 // 0x502320 (ret 4)
    void add_strip_icon(char* slot, const tstring& name, int troop, int kind);   // 0x5024e0 (ret 0x10)
    void init_window();                                         // 0x5025a0
    void refresh_armor(void** labels, int troop);               // 0x502620 (ret 8)
    void refresh_page(int page);                                // 0x502920 (ret 4)
    void place_slot(MissionUi* w, ItemRef* item, int idx, int* state, int page, char flag);   // 0x5029f0 (ret 0x18)
    ResMesh* make_mesh(ItemRef* item, float* scale, int troop); // 0x502b80 (ret 0xc)
    void refresh_stash(int row);                                // 0x502c30
    char base_mouse_event(unsigned mask) { return ((gui_window*)this)->mouse_event(mask); }     // 0x42cf50 (ret 4; the gui widget base)
};

#endif // GAME_PARTY_PARTY_WINDOW_H

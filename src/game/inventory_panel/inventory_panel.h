// inventory_panel.h: the inventory / trade screen's window object (code 0x50fe20..0x513710): per-frame step, slot grids,
// item tooltip, slot transfer. The slices called it InventoryPanel, SlotWindow, InvWindow and Panel; the party screen's
// inventory window of src/game/party/party_window.h (PartyWindow) is the same object seen through the Game header (its
// layout is the first half of this one; the two headers cannot be included together: each declares its own g_game).
// Layout: every offset cites the function that proves it. Names provisional.
#ifndef GAME_INVENTORY_PANEL_INVENTORY_PANEL_H
#define GAME_INVENTORY_PANEL_INVENTORY_PANEL_H
#include "./inv_common.h"
#include "../ui/ui_widgets.h"                           // UiD (the scroll bar's class)
#include "../tstring.h"

// the scroll bar the inventory view owns (0x48baa0 set_value; slot +0x48 set_key_flags)
struct InvBar : PsOps {
    char pad0[0xf0 - 4];
    float stops[0x1e];                                  // +0xf0 (0x50fe20)
    float thumb;                                        // +0x168
    float track_start;                                  // +0x16c
    float track_end;                                    // +0x170
    float f174;                                         // +0x174
    int stop;                                           // +0x178 current stop
    float scale;                                        // +0x17c
    __forceinline void set_value(float v) { ((UiD*)this)->set_value(v); }   // 0x48baa0 (ret 4): UiD::set_value (this view is a UiD)
};
struct InvView {                                        // the inventory view: its scroll bar is at +0x484 (0x50fe20)
    char pad0[0x484];
    InvBar* bar;                                        // +0x484
};

struct DragMesh {                                       // the mesh carried by the mouse (global g_drag 0x5d75e4); slot 0 = release(int)
    virtual void release(int flag);                     // +0x00 (0x512c80)
    __forceinline void scale(float f) { ((rgl::mesh*)this)->scale(f); }   // 0x44f0c0 (ret 4): rgl::mesh::scale
};
extern DragMesh* g_drag;                                // 0x5d75e4


// The party window (src/game/party/party_window.h, the same object seen through the Game header: the two headers cannot be
// included together) owns four of the methods this screen calls; only those are declared here.
struct PartyWindow {
    void money_label(int which);                                    // 0x501940 (ret 4)
    void init_window();                                             // 0x5025a0
    void add_strip_icon(char* slot, const tstring& name, int troop, int kind);   // 0x5024e0 (ret 0x10)
    ResMesh* make_mesh(ItemRef* item, float* scale, int t);         // 0x502b80 (ret 0xc)
};
#pragma warning(disable:4715)
struct InventoryPanel {
    int vptr;                                           // +0x00
    int mode;                                           // +0x04 (0 own, 1 loot, 2 trade, 3 / 4 exchange: 0x50fe20, 0x512960, 0x512c80)
    char pad08[0x14 - 0x08];
    int troop;                                          // +0x14 (troop of the second page)
    int next_menu;                                      // +0x18 (0x64 = none: 0x50fe20)
    char pad1c[0x2c - 0x1c];
    MissionUi* btn[2];                                  // +0x2c page buttons (0x5108d0)
    InvView* view[2];                                   // +0x34 (0x50fe20)
    MissionUi* btn2[2];                                 // +0x3c stash page buttons (0x5108d0)
    char pad44[0x64 - 0x44];
    MissionUi* equip[2][0x60];                          // +0x64 slot widgets of the equipment / inventory pages (0x5108d0)
    unsigned flags[2][0x60];                            // +0x364 per-slot state (bit 0 sold / bought, bit 1 needs a mesh: 0x510850, 0x5126e0)
    unsigned values[2][0x60];                           // +0x664 per-slot trade value (0x5132e0, 0x512620)
    int view_count;                                     // +0x964
    int view_count2;                                    // +0x968
    void* window;                                       // +0x96c
    MissionUi* grid[2][10];                             // +0x970 stash slot widgets (0x5108d0)
    unsigned fstash[2][10];                             // +0x9c0 stash slot state (pages 2 / 3: 0x510850, 0x513320)
    unsigned vstash[2][10];                             // +0xa10 stash slot value (0x5132e0)
    MissionUi* total_cost;                              // +0xa60 (0x50fe20, 0x513360)
    void* armor_labels[2][4];                           // +0xa64
    MissionUi* icon_a84;                                // +0xa84 (page 0 character icon)
    MissionUi* icon_a88;                                // +0xa88
    int pending;                                        // +0xa8c pending trade value
    MissionUi* leave;                                   // +0xa90
    int f_a94;                                          // +0xa94
    int locked;                                         // +0xa98 (holding an item / trade locked)
    ItemRef held;                                       // +0xa9c the held item (0x512c80)
    char held_flag;                                     // +0xaa4 its saved flag
    char padaa5[3];
    int held_value;                                     // +0xaa8 its saved value
    int held_page;                                      // +0xaac the page it came from
    int q_pay;                                          // +0xab0 pay question result
    int q_dump;                                         // +0xab4 dump question result
    int q_food;                                         // +0xab8 food question result
    char fabc;                                          // +0xabc
    char padabd[3];
    int spec_ac0, spec_ac4;                             // +0xac0 / +0xac4 character strip specs

    __forceinline void money_label(void* arg) { ((PartyWindow*)this)->money_label((int)arg); }   // 0x501940 (ret 4)
    __forceinline void init_window() { ((PartyWindow*)this)->init_window(); }                     // 0x5025a0
    __forceinline void add_strip_icon(char* slot, const tstring& name, int troop, int kind)       // 0x5024e0 (ret 0x10)
    { ((PartyWindow*)this)->add_strip_icon(slot, name, troop, kind); }
    void refresh_transaction(int unused);                           // 0x50fe20 (ret 4)
    void set_slot_flag(int page, int idx, unsigned mask, char on);  // 0x510850 (ret 0x10)
    void refresh_lists(int unused);                                 // 0x5108d0 (ret 4)
    __declspec(noinline) void place_slot(MissionUi* w, ItemRef* slot, int page, int index);   // 0x510f50 (ret 0x10)
    void place_slot_mesh(ItemRef* slot, float* pt, int page, int index);   // 0x510fa0 (ret 0x10)
    __declspec(noinline) char test_slot_flag(int page, int slot, unsigned mask);         // 0x5126e0 (ret 0xc)
    void compute(int* item, int mode);                              // 0x512960 (ret 8; the price stays in eax)
    __forceinline int compute_price(int* item, int mode)   // 0x512960 (the same function; the price stays in eax: compute is called through its address read as an int-returning member)
    {
        union { void (InventoryPanel::*v)(int*, int); int (InventoryPanel::*i)(int*, int); } c;
        c.v = &InventoryPanel::compute;
        return (this->*c.i)(item, mode);
    }
    void transfer_slot(ItemRef* item, int page, int slot);          // 0x512c80 (ret 0xc)
    char can_transfer(ItemRef* item, int mode, int flag);           // 0x513230 (ret 0xc)
    void set_slot_value(int page, int slot, int value);             // 0x5132e0 (ret 0xc)
    __declspec(noinline) void or_slot_flag(int page, int slot);                          // 0x513320 (ret 8)
    void add_cost(int delta);                                       // 0x513360 (ret 4)
    bool try_place(ItemRef* item, int page, int slot);              // 0x5134c0 (ret 0xc)
    __forceinline DragMesh* make_mesh(ItemRef* item, int* out, int troop)   // 0x502b80 (ret 0xc)
    { return (DragMesh*)((PartyWindow*)this)->make_mesh(item, (float*)out, troop); }
};

#endif // GAME_INVENTORY_PANEL_INVENTORY_PANEL_H

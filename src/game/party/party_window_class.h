// party_window_class.h: the party screen's inventory window (code 0x500ad0..0x502cc0, the record at slot 8 of the trigger table,
// vtable 0x595764, size 0xacc). It is a head_rec: the screen record whose slot 2 builds the window (0x500ad0), whose slot 3
// is the per-frame / event handler (the trade engine, inventory_panel/inventory_panel.cpp 0x50fe20) and whose slot 1 resets the
// gui. The members are the record's own; the methods of party_create.cpp / party_window.cpp work on them and the ones of
// inventory_panel.cpp (0x50fe20..0x513710, the trade engine) are built with the GameData header instead.
// This header only declares the class: the code that needs the Game object includes party_window.h.
//
// Layout (all offsets from `this`; each cites the function that proves it):
//   +0x04 w04  the mode (0 own inventory, 1 loot, 2 trade, 3 / 4 exchange)   0x502320, 0x502920
//   +0x14 w14  troop index of the second page / row                           0x502320
//   +0x2c page_widget, +0x34 view, +0x3c panels, +0x44 gold, +0x4c name, +0x54 hinge   0x500ad0, 0x501a20
//   +0x64 widgets (slot widgets of the inventory pages, 0x180 bytes per page), +0x364 flags, +0x664 aux
//   +0x964 pages, +0x968 rows, +0x96c container                    0x5025a0, 0x5024e0, 0x501a20
//   +0x970 row_slot, +0x9c0 row_state, +0xa10 row_value (ten slots per row, 0x28 bytes per row)    0x501a20, 0x502c30, 0x5132e0
//   +0xa60 total_cost, +0xa64 armor labels (two sets of four), +0xa84 / +0xa88 strip icons, +0xac0 / +0xac4 their specs
#ifndef GAME_PARTY_PARTY_WINDOW_CLASS_H
#define GAME_PARTY_PARTY_WINDOW_CLASS_H
#include "../head_rec.h"
#include "../tstring.h"
#include "../item_ref.h"
#include "../ui/ui_widgets.h"     // XyzButton (the strip icon)
#include "../ui/container.h"      // ContainerBase (the page panel, the window)

struct MissionUi;
struct Camera;
namespace rgl { struct mesh; }

// ---- the widgets the window's member functions build (party/party_window.cpp) ------------------------------------
struct PartyPanel : ContainerBase {         // vtable 0x599bf8 (the equipment menu / inventory page block)
    virtual char key(unsigned mask);        // +0x8c 0x5021b0
};
struct PartySlot : MissionUi {              // vtable 0x599d28, size 0x178 (the item slot)
    virtual char key(unsigned mask);        // +0x8c 0x5022c0
};
struct StripIcon : XyzButton {              // the strip icons are XyzButtons (vtable 0x591ee8, size 0x188)
};

struct PartyWindow : head_rec {   // vtable 0x595764
    ContainerBase* page_widget[2];          // +0x2c  0x500ad0 (the page widget build_page(i) returns)
    Camera* view[2];                        // +0x34  0x500ad0 (the scrolling item lists)
    ContainerBase* panels[2];               // +0x3c  0x501a20 (equipment menu panel of row idx)
    MissionUi* gold[2];                     // +0x44  0x500ad0, 0x501940 (the money labels)
    MissionUi* name[2];                     // +0x4c  0x500ad0 (the troop name labels)
    MissionUi* hinge[4];                    // +0x54  0x500ad0 (the four window hinges)
    MissionUi* widgets[2][0x60];            // +0x64  0x502320 (page * 0x180 + 0x64 + slot * 4)
    int flags[2][0x60];                     // +0x364 0x502920 (bit 2: the slot needs a mesh)
    int aux[2][0x60];                       // +0x664 0x502320
    int pages;                              // +0x964 0x5025a0
    int rows;                               // +0x968 0x5025a0 (at most two)
    ContainerBase* container;               // +0x96c 0x5024e0, 0x501a20
    MissionUi* row_slot[2][10];             // +0x970 0x501a20, 0x502c30
    int row_state[2][10];                   // +0x9c0 0x501a20, 0x502c30
    int row_value[2][10];                   // +0xa10 0x5132e0 (the trade value of a stash slot)
    MissionUi* total_cost;                  // +0xa60 0x500ad0
    MissionUi* armor_labels[2][4];          // +0xa64 0x5025a0 (head, body, legs, encumbrance)
    StripIcon* icon_a84;                    // +0xa84 0x5029f0
    StripIcon* icon_a88;                    // +0xa88 0x5029f0
    int pending;                            // +0xa8c the pending trade value (0x500ad0 clears it; 0x50fe20, 0x513360)
    MissionUi* leave;                       // +0xa90 0x500ad0 (the leave button)
    int f_a94;                              // +0xa94
    int locked;                             // +0xa98 holding an item / trade locked (0x500ad0 clears it; 0x512c80)
    ItemRef held;                           // +0xa9c the held item (0x512c80)
    char held_flag;                         // +0xaa4 its saved flag
    char padaa5[3];
    int held_value;                         // +0xaa8 its saved value
    int held_page;                          // +0xaac the page it came from
    int q_pay, q_dump, q_food;              // +0xab0 the pay / dump / food question results (0x500ad0: -1; 0x50fe20)
    char f_abc;                             // +0xabc 0x500ad0 (the other party's hero is on the list)
    char padabd[3];
    int spec_ac0, spec_ac4;                 // +0xac0 0x5029f0
    MissionUi* equip_bg;                    // +0xac8 0x500ad0 (mode 4: the equipment panel background)

    PartyWindow() { w04 = 0; }
    virtual void slot_empty_1();            // slot 1  0x4c6e00: g_gui.reset()                 (options_panel/rec_slots_tr.cpp)
    virtual void slot_empty_2();            // slot 2  0x500ad0: builds the window             (agent5/party_create.cpp)
    virtual void scale_3(float unused);     // slot 3  0x50fe20 (ret 4): the trade engine's per-frame step (inventory_panel/inventory_panel.cpp)
    virtual char pure_7() { return 1; }     // slot 7: 0x48b170

    void money_label(int which);                                // 0x501940 (ret 4: src/game/agent5/party_create.cpp)
    void* make_inventory_menu(int idx);                         // 0x501a20 (ret 4)
    MissionUi* make_slot(ItemRef* slot);                        // 0x502230 (ret 4)
    PartyPanel* build_page(int page);                           // 0x502320 (ret 4)
    void add_strip_icon(StripIcon** slot, const tstring& name, int troop, int kind);   // 0x5024e0 (ret 0x10)
    void init_window();                                         // 0x5025a0
    void refresh_armor(MissionUi** labels, int troop);          // 0x502620 (ret 8)
    void refresh_page(int page);                                // 0x502920 (ret 4)
    void place_slot(MissionUi* w, ItemRef* item, int idx, int* state, int page, char flag);   // 0x5029f0 (ret 0x18)
    rgl::mesh* make_mesh(ItemRef* item, float* scale, int troop); // 0x502b80 (ret 0xc)
    void refresh_stash(int row);                                // 0x502c30

    // 0x50fe20..0x513710 (inventory_panel/inventory_panel.cpp, built with the GameData header instead of party/game.h)
    void set_slot_flag(int page, int idx, unsigned mask, char on);  // 0x510850 (ret 0x10)
    void refresh_lists(int unused);                                 // 0x5108d0 (ret 4)
    __declspec(noinline) void place_slot(MissionUi* w, ItemRef* slot, int page, int index);   // 0x510f50 (ret 0x10)
    void place_slot_mesh(ItemRef* slot, float* pt, int page, int index);   // 0x510fa0 (ret 0x10)
    __declspec(noinline) char test_slot_flag(int page, int slot, unsigned mask);         // 0x5126e0 (ret 0xc)
    void compute(int* item, int mode);                              // 0x512960 (ret 8; the price stays in eax)
    __forceinline int compute_price(int* item, int mode)   // 0x512960 (the same function; the price stays in eax: compute is called through its address read as an int-returning member)
    {
        union { void (PartyWindow::*v)(int*, int); int (PartyWindow::*i)(int*, int); } c;
        c.v = &PartyWindow::compute;
        return (this->*c.i)(item, mode);
    }
    void transfer_slot(ItemRef* item, int page, int slot);          // 0x512c80 (ret 0xc)
    char can_transfer(ItemRef* item, int mode, int flag);           // 0x513230 (ret 0xc)
    void set_slot_value(int page, int slot, int value);             // 0x5132e0 (ret 0xc)
    __declspec(noinline) void or_slot_flag(int page, int slot);                          // 0x513320 (ret 8)
    void add_cost(int delta);                                       // 0x513360 (ret 4)
    bool try_place(ItemRef* item, int page, int slot);              // 0x5134c0 (ret 0xc)
};

typedef char party_window_size_check[(sizeof(PartyWindow) == 0xacc) ? 1 : -1];

#endif // GAME_PARTY_PARTY_WINDOW_CLASS_H

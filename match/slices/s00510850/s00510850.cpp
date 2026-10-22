// Slice s00510850: three InventoryPanel helpers.
//   0x510850  set_slot_flag(page, idx, mask, on): the per-slot state dwords (page<2: 0x80-byte rows at
//             +0x364, page>=2: 10-dword rows at +0x970)
//   0x5108d0  the full grid refresh: two equipment pages, the ten-slot inventory pages, the page buttons,
//             the click handling (transfer / place) and the closing reset of the gui state
//   0x510f50  place_slot: position one slot widget and hand its item reference to 0x510fa0
// `this` is the inventory panel of s00500ad0 (mode +0x4, troop +0x14, page buttons +0x2c/+0x3c, the
// equipment widgets at +0x64 with a 0x180 stride, the counts +0x964/+0x968, the ten-slot grids at +0x970,
// the trade-locked flag +0xa98).  Names are provisional.
#include <windows.h>
#include "rgl.h"

// ---- the engine string (0x10 bytes; ctor 0x42bbe0, dtor 0x42bc40, assign 0x42bcb0) ----
struct tstring {
    int pos;                                            // +0x00
    char* buf;                                          // +0x04
    int cap;                                            // +0x08
    int len;                                            // +0x0c
    tstring();                                          // 0x42bbd0
    tstring(const char* s);                             // 0x42bbe0 (ret 4)
    ~tstring();                                         // 0x42bc40
    tstring& assign(const char* s);                     // 0x42bcb0 (ret 4)
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

// ---- widgets (slots: +0x44 clicked_on(int), +0x48 set_key_flags(int); floats +0xe0/+0xe4) ----
struct UiWidget {
    virtual void d00(); virtual void d01(); virtual void d02(); virtual void d03(); virtual void d04();
    virtual void d05(); virtual void d06(); virtual void d07(); virtual void d08(); virtual void d09();
    virtual void d10(); virtual void d11(); virtual void d12(); virtual void d13(); virtual void d14();
    virtual void d15(); virtual void d16();
    virtual char clicked_on(int which);                 // +0x44
    virtual void set_key_flags(int flags);              // +0x48
    char  pad_[0xe0 - 4];
    float x;                                            // +0xe0
    float y;                                            // +0xe4
};

// the gui object handed to gui_manager::set_o568 (vtable slot 0 takes one int)
struct GuiObj { virtual void d0(int which); };

// ---- game data ----
struct ItemRef {
    int id;                                             // +0x00
    int count;                                          // +0x04
};

struct TroopRec {
    char pad0[0x20];
    tstring name;                                       // +0x20
    unsigned flags;                                     // +0x30
    int  f34;                                           // +0x34
    char pad38[0x3b4 - 0x38];
    int  money;                                         // +0x3b4
    char pad3b8[0x4a4 - 0x3b8];
    ItemRef grid[10];                                   // +0x4a4
    char pad54[0x508 - 0x4f4];
    int  count();                                       // 0x462b60
    int  find_free();                                   // 0x463030 (first slot with id == -1)
};
#define TROOP_STRIDE 0x508
#define TROOP(i) ((TroopRec*)((char*)g_game->troops + (i) * TROOP_STRIDE))
// the slot arrays are indexed with a 0xa1 (= 0x508 / 8) stride, exactly as the original does
#define EQUIP(t, k)  (((ItemRef*)((char*)g_game->troops + 0xb4))[(t) * 0xa1 + (k)])
#define GRID(t, k)   (((ItemRef*)((char*)g_game->troops + 0x4a4))[(t) * 0xa1 + (k)])

struct World {
    char      pad8c[0x8c];
    int       cur_troop;                                // +0x8c
    int       hero_troop;                               // +0x90
    char      pad94[0x13a00 - 0x94];
    TroopRec* troops;                                   // +0x13a00
};
extern World* g_game;                                   // 0x5e2930

// ---- services ----
struct GuiManager {
    char pad568[0x568];
    GuiObj* f568;                                       // +0x568
    void set_o568(GuiObj* o, bool clear_lines);         // 0x42cdb0
};
#define GUI ((GuiManager*)0x5d7080)

extern GuiObj* g_5d75e8;                                // 0x5d75e8 (== GUI->f568)
extern int    g_5d8760;                                 // 0x5d8760
extern int    g_5d8764;                                 // 0x5d8764
extern tstring g_5d7850[0x50];                          // 0x5d7850 .. 0x5d7d50
extern int    g_5d8020;                                 // 0x5d8020
extern int    g_5d8024;                                 // 0x5d8024
extern float  g_5d8028;                                 // 0x5d8028
extern int    g_5d8780;                                 // 0x5d8780
extern const char g_empty[];                            // 0x54a480 ("")

extern int g_a17148;                                    // 0xa17148
extern int g_a17348;                                    // 0xa17348
extern int g_a171b4;                                    // 0xa171b4
extern int g_a173b4;                                    // 0xa173b4

// ---- the panel ----
struct InventoryPanel {
    int  vptr;                                          // +0x00
    int  mode;                                          // +0x04
    char pad08[0x14 - 0x08];
    int  troop;                                         // +0x14
    char pad18[0x2c - 0x18];
    UiWidget* page_btn[4];                              // +0x2c
    UiWidget* page_btn2[4];                             // +0x3c
    char pad4c[0x64 - 0x4c];
    UiWidget* equip[2][0x60];                           // +0x64 (page stride 0x180)
    char pad364[0x964 - 0x364];
    int  view_count;                                    // +0x964
    int  view_count2;                                   // +0x968
    char pad_96c[0x970 - 0x96c];
    UiWidget* grid[0x20];                               // +0x970 (page stride 0x28)
    char pad_9f0[0xa98 - 0x9f0];
    int  f_a98;                                         // +0xa98 (trade locked)

    void slot_flag(int page, int idx, unsigned mask, char on);          // 0x510850 (ret 0x10)
    void refresh(int unused);                                           // 0x5108d0 (ret 4)
    __declspec(noinline) void place_slot(UiWidget* w, ItemRef* slot, int page, int index);   // 0x510f50 (ret 0x10)
    void place_slot_mesh(ItemRef* slot, float* pt, int page, int index);    // 0x510fa0 (ret 0x10)
    void transfer_slot(ItemRef* slot, int page, int index);             // 0x512c80 (ret 0xc)
    void init_window();                                                 // 0x5025a0
};

// @ 0x510850
void InventoryPanel::slot_flag(int page, int idx, unsigned mask, char on)
{
    if (page < 2) {
        if (on)
            ((int*)((char*)this + 0x364))[page * 0x60 + idx] |= mask;
        else
            ((int*)((char*)this + 0x364))[page * 0x60 + idx] &= ~mask;
    } else {
        if (on)
            ((int*)((char*)this + 0x970))[page * 10 + idx] |= mask;
        else
            ((int*)((char*)this + 0x970))[page * 10 + idx] &= ~mask;
    }
}

// @ 0x5108d0
void InventoryPanel::refresh(int unused)
{
    for (int page = 0; page < view_count; page++) {
        int troop = g_game->cur_troop;
        if (page != 0)
            troop = this->troop;
        int count;
        if (troop == g_game->hero_troop && mode == 0)
            count = 9;
        else
            count = TROOP(troop)->count();
        if (count > 0) {
            UiWidget** w = (UiWidget**)((char*)this + 0x64 + page * 0x180);
            for (int i = 0; i < count; i++)
                if (w[i]->clicked_on(4))
                    GUI->set_o568(0, true);
            for (int i = 0; i < count; i++)
                if (w[i]->clicked_on(2) && f_a98 == 0)
                    place_slot(w[i], &EQUIP(troop, i), page, i);
        }
    }

    for (int page = 0; page < view_count2; page++) {
        int troop = g_game->cur_troop;
        if (page != 0)
            troop = this->troop;
        UiWidget** w = (UiWidget**)((char*)this + 0x970 + page * 0x28);
        for (int i = 0; i < 10; i++)
            if (w[i] != 0 && w[i]->clicked_on(4) && f_a98 == 0)
                GUI->set_o568(0, true);
        for (int i = 0; i < 10; i++)
            if (w[i] != 0 && w[i]->clicked_on(2) && f_a98 == 0)
                place_slot(w[i], &GRID(troop, i), 2, i);
    }

    if (f_a98 != 0) {
        GuiObj* o = g_5d75e8;
        if (o != 0)
            o->d0(1);
        g_5d8760 = -1;
        g_5d8764 = 0xff101010;
        for (tstring* s = g_5d7850; s < g_5d7850 + 0x50; s++)
            s->assign(g_empty);
        g_5d8024 = 0;
        g_5d8028 = -0.03f;
        g_5d8020 = 0;
        g_5d75e8 = 0;
        g_5d8780 = 0;
    }

    for (int page = 0; page < view_count; page++) {
        if (!page_btn[page]->clicked_on(0x10000))
            continue;
        int troop = g_game->cur_troop;
        if (page != 0)
            troop = this->troop;
        int count;
        if (troop == g_game->hero_troop && mode == 0)
            count = 9;
        else
            count = TROOP(troop)->count();
        if (count <= 0)
            continue;
        UiWidget** w = (UiWidget**)((char*)this + 0x64 + page * 0x180);
        for (int i = 0; i < count; i++) {
            if (!w[i]->clicked_on(0x10000))
                continue;
            int idx = i;
            if (f_a98 != 0 && page != 0) {
                int r = TROOP(troop)->find_free();
                if (r >= 0 && !(troop == g_game->hero_troop && mode == 0 && r >= 9))
                    idx = r;
            }
            transfer_slot(&EQUIP(troop, idx), page, idx);
            place_slot(w[i], &EQUIP(troop, i), page, i);
            if ((g_a17148 != 0 || g_a17348 != 0 || g_a171b4 != 0 || g_a173b4 != 0) &&
                f_a98 != 0 && view_count == 2) {
                int other = (page == 0) ? this->troop : g_game->cur_troop;
                int r = TROOP(other)->find_free();
                if (r >= 0 && !(other == g_game->hero_troop && mode == 0 && r >= 9))
                    transfer_slot(&EQUIP(other, r), 1 - page, r);
            }
        }
    }

    for (int page = 0; page < view_count2; page++) {
        if (!page_btn2[page]->clicked_on(0x10000))
            continue;
        UiWidget** w = (UiWidget**)((char*)this + 0x970 + page * 0x28);
        for (int i = 0; i < 10; i++) {
            if (w[i] == 0)
                continue;
            if (!w[i]->clicked_on(0x10000))
                continue;
            int troop = (page != 0) ? this->troop : g_game->cur_troop;
            ItemRef* slot = &GRID(troop, i);
            transfer_slot(slot, page + 2, i);
            place_slot(w[i], slot, 2, i);
            if ((g_a17148 != 0 || g_a17348 != 0 || g_a171b4 != 0 || g_a173b4 != 0) &&
                f_a98 != 0 && view_count2 == 2 && page == 1) {
                int t = g_game->cur_troop;
                int count = TROOP(t)->count();
                int idx = -1;
                bool found = false;
                ItemRef* it = &EQUIP(t, 0);
                for (int k = 0; k < count && !found; k++, it++) {
                    if (it->id == -1) {
                        idx = k;
                        found = true;
                    }
                }
                if (idx >= 0)
                    transfer_slot(&EQUIP(t, idx), 0, idx);
            }
        }
    }

    init_window();
}

// @ 0x510f50
void InventoryPanel::place_slot(UiWidget* w, ItemRef* slot, int page, int index)
{
    float pt[2];
    if (slot->id >= 0) {
        pt[0] = w->x + 0.044f;
        pt[1] = w->y + 0.044f;
        place_slot_mesh(slot, pt, page, index);
    }
}

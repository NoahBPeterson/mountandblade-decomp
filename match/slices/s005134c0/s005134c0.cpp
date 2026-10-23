// Slice s005134c0: the inventory screen's trade/commit glue.
//
//   0x5134c0  the "can this item be taken back from that page" scan (try_place)
//   0x513710  the screen's per-frame commit / hotkey dispatcher
//   0x513b10  pull one accumulated key flag out of the screen's key mask
//   0x513b40  flush the screen state into the two parties shown on the two pages
//
// `this` of 0x5134c0 / 0x513b10 / 0x513b40 is the inventory screen object at 0xa73300:
//   +0x00  party slot shown on page 0 / page 1 (entry [1] is the trade "mode": 2, 3 or 4)
//   +0x08  packed party handle used when detaching the party
//   +0x14  troop row of the other side (index into g_game->chars)
//   +0x3c  accumulated key mask (see 0x513b10)
//   +0x40  "state is dirty" flag (0x513b40 runs once per set flag)
//   +0x138 the page being displayed (0 or 1)
//   +0x164 flag byte (bit 0 forces a party detach)
//   +0x168 the troop-item count already announced
//   +0x2c8 "the other page also holds a party" flag
//   +0x364 page 0/1 slot state table, +0x4e4 = &state[1][0]
//   +0x9c0 page 2/3 slot state table (fstash), read as a flat 10-entry run per page
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "rgl.h"

#define MB_NOTHROW throw()

extern "C" int _fltused = 0;

// ---------------------------------------------------------------------------------------------------------
// the engine string (0x10 bytes; ctor 0x42bbe0, copy ctor 0x42c110, dtor 0x42bc40)
struct tstring {
    int pos;                                            // +0x00
    char* buf;                                          // +0x04
    int cap;                                            // +0x08
    int len;                                            // +0x0c
    tstring() MB_NOTHROW;
    tstring(const char* s) MB_NOTHROW;                  // 0x42bbe0 (ret 4)
    tstring(const tstring& o) MB_NOTHROW;               // 0x42c110 (ret 4)
    ~tstring() MB_NOTHROW;                              // 0x42bc40
};

// translation table at 0x5e2a18
struct StrTable {
    tstring* lookup(const tstring& key) MB_NOTHROW;     // 0x43a1e0 (ret 4)
};
extern StrTable g_strtable;                             // 0x5e2a18

// item kind record, 0x238 bytes (array at 0x5e29d0)
struct ItemKind {
    char pad0[0x1c8];
    union {
        unsigned flags;                                 // +0x1c8
        struct { unsigned char type; char tpad[3]; };
    };
    char pad1cc[0x238 - 0x1cc];
};
extern ItemKind* g_items;                               // 0x5e29d0

// item specifier (2 dwords; the low byte of +4 is the amount)
struct ItemRef {
    int id;                                             // +0x00
    int f4;                                             // +0x04
};

// 0x508-byte character/troop row of the table at g_game->chars
struct Agent {
    char pad0[0xb4];
    ItemRef items[1];                                   // +0x0b4 (count() of them)
    char padbc[0x4a4 - 0xbc];
    ItemRef stash[10];                                  // +0x4a4 (the 10 carried goods)
    char pad4f4[0x508 - 0x4f4];
    int count();                                        // 0x462b60
};

// 0x5ac8-byte party record (resolved out of the party manager's block pool)
struct Party {
    char pad0[0x88];
    int f88;                                            // +0x088
    char pad8c[0xac - 0x8c];
    int fac;                                            // +0x0ac (banner/faction id)
    char padb0[0x8b4 - 0xb0];
    char f8b4;                                          // +0x8b4 ("party changed")
    char pad8b5[0x918 - 0x8b5];
    int f918;                                           // +0x918
    char pad91c[0x5ac8 - 0x91c];
};

// the party manager living at g_game+0xf8
struct PartyMgr {
    char pad0[0x138d0];
    char** blocks;                                      // +0x138d0 (16 parties per block)
    char pad138d4[0x13908 - 0x138d4];
    Party* get(int slot);                               // 0x51ee50 (ret 4)
    char is_slot(int slot);                             // 0x4903e0 (ret 4)
    int party(int slot);                                // 0x48ad00 (ret 4, returns a handle)
    Party* resolve(int h) { return (Party*)(blocks[h >> 4] + (h & 0xf) * 0x5ac8); }
};

struct GameData {
    char pad00[0x78];
    int f78;                                            // +0x078 (the active party slot)
    char pad7c[0x10];
    int f8c;                                            // +0x08c (the player's troop row)
    int f90;                                            // +0x090 (the "hero" troop row)
    char pad94[0x14];
    int fa8;                                            // +0x0a8
    char padac[4];
    int fb0;                                            // +0x0b0
    char padb4[0x14];
    int fc8;                                            // +0x0c8
    char padcc[0x18];
    char** f_e4;                                        // +0x0e4 (block table of 0x98-byte objects)
    char pade8[0x10];
    PartyMgr parties;                                   // +0x0f8
    char* chars;                                        // +0x13a00 (0x508-byte rows)
    char pad13a04[0x13af0 - 0x13a04];
    int f13af0;                                         // +0x13af0
    int f13af4;                                         // +0x13af4
    int f13af8;                                         // +0x13af8
    int f13afc;                                         // +0x13afc

    void clear_hero_ammo(int troop);                                        // 0x513eb0 (ret 4)
    void count_party(int party, int* a, int* b, int* c, int* d);            // 0x490ce0 (ret 0x14)
    Agent* troop(int idx) { return (Agent*)(chars + idx * 0x508); }
};
extern GameData* g_game;                                // 0x5e2930

// the message log / notification sink at 0x5d7080
struct MsgLog {
    void add(tstring text, int color);                  // 0x452c00 (ret 0x14; text by value, first arg)
    void add(tstring* text, int color);                 // 0x4334b0 (ret 8)
};
extern MsgLog g_msglog;                                 // 0x5d7080

// the 0x98-byte object resolved from the handle at screen+0x08 (0x4b6180 takes it in ecx)
struct DetachTarget {
    void detach_party();                                // 0x4b6180
};

// a hotkey button: vtable at +0, state at +0x04, "highlight" counter at +0x18
struct Widget {
    virtual void v0(); virtual void v1(); virtual void v2(); virtual void v3();
    virtual void v4();                                  // +0x10
    int f4;                                             // +0x04
    char pad8[0x18 - 0x8];
    int f18;                                            // +0x18
};
extern Widget* g_w98;                                   // 0xa73998
extern Widget* g_wA0;                                   // 0xa739a0
extern Widget* g_wA8;                                   // 0xa739a8
extern Widget* g_wAC;                                   // 0xa739ac
extern Widget* g_wB0;                                   // 0xa739b0
extern Widget* g_wB8;                                   // 0xa739b8
extern Widget* g_wC0;                                   // 0xa739c0
extern Widget* g_wDC;                                   // 0xa739dc
extern Widget* g_wE8;                                   // 0xa739e8
extern Widget* g_wEC;                                   // 0xa739ec

// the main menu object at 0xa73980
struct Menu {
    void reset(int n);                                  // 0x494260 (ret 4)
    void go(int n);                                     // 0x495700 (ret 4)
};
extern Menu g_main_menu;                                // 0xa73980

// the active mission (0x5e292c)
struct Mission {
    char pad0[0x5270];
    int f5270;                                          // +0x5270
    void start_setup();                                 // 0x484590
};
extern Mission* g_scene;                                // 0x5e292c

extern int g_a735c0;                                    // 0xa735c0 (scene counter)
extern char g_a73320;                                   // 0xa73320
extern int g_a73438;                                    // 0xa73438 (page index)
extern int g_a7343c;                                    // 0xa7343c (page 1 party slot)
extern int g_a73440;                                    // 0xa73440 (page 0 party slot)

extern void autosave_notice();                          // 0x514830

// ---------------------------------------------------------------------------------------------------------
// the inventory screen object at 0xa73300
struct InventoryPanel {
    int page_party[2];                                  // +0x000 (party slot of page 0 / page 1)
    unsigned f08;                                       // +0x008 (packed handle of the detach target)
    int f0c;                                            // +0x00c
    char pad10[4];
    int f14;                                            // +0x014 (troop row of the other side)
    char pad18[0x3c - 0x18];
    unsigned f3c;                                       // +0x03c (accumulated key mask)
    char f40;                                           // +0x040 (dirty flag)
    char pad41[0x138 - 0x41];
    int f138;                                           // +0x138 (page being displayed)
    char pad13c[0x164 - 0x13c];
    char f164;                                          // +0x164
    char pad165[3];
    int f168;                                           // +0x168 (announced item count)
    char pad16c[0x2c8 - 0x16c];
    char f2c8;                                          // +0x2c8
    char pad2c9[0x364 - 0x2c9];
    unsigned flags[2][0x60];                            // +0x364 (page 0/1 slot state)
    unsigned values[2][0x60];                           // +0x664 (page 0/1 slot value)
    char pad964[0x9c0 - 0x964];
    unsigned fstash[2][10];                             // +0x9c0 (page 2/3 slot state)

    char slot_flag(int page, int slot, unsigned mask);  // 0x5126e0 (ret 0xc)

    // --- this slice ---
    bool try_place(ItemRef* item, int page, int slot);  // 0x5134c0 (ret 0xc)
    unsigned pop_key();                                 // 0x513b10
    void update_parties();                              // 0x513b40

    // --- callees defined in later slices ---
    int count_hero_items();                             // 0x513e10
    void setup_513f80(int a, int b);                    // 0x513f80 (ret 8)
    void pick_template();                               // 0x514360
    void start_scene(int a, int b, int c);              // 0x5147e0 (ret 0xc)
    void f514890(int a, int b, int c, int d);           // 0x514890 (ret 0x10)
    void start_battle(int a, int b, int c, int d);      // 0x514c00 (ret 0x10)
    void start_mission();                               // 0x514cd0
};
extern InventoryPanel g_panel;                          // 0xa73300

// the commit state passed to 0x513710 (this): two pending flags, a "busy" flag, a counter
struct CommitState233 {
    char f0;                                            // +0x00
    char f1;                                            // +0x01
    char f2;                                            // +0x02
    char pad3[5];
    int f8;                                             // +0x08
    void commit(char* out);                             // 0x513710 (ret 4)
};

// ---------------------------------------------------------------------------------------------------------
// @ 0x5134c0
bool InventoryPanel::try_place(ItemRef* item, int page, int slot)
{
    bool found = false;
    int id = item->id;

    if (id == -1)
        return found;
    if (slot_flag(page, slot, 1))
        return found;
    if (g_items[id].type != 0xb)
        return found;

    if (page == 1) {
        int idx = f14;
        int n;

        if (idx == g_game->f90 && page_party[1] == 0)
            n = 9;
        else
            n = g_game->troop(idx)->count();
        if (n <= 0)
            return found;
        for (int i = 0; i < n; i++) {
            if (g_game->troop(idx)->items[i].id == id && (flags[1][i] & 1))
                found = true;
        }
    }
    else {
        int idx = g_game->f8c;
        int n;

        if (idx == g_game->f90 && page_party[1] == 0)
            n = 9;
        else
            n = g_game->troop(idx)->count();
        if (n > 0) {
            for (int i = 0; i < n; i++) {
                if (g_game->troop(idx)->items[i].id == id && (flags[0][i] & 1))
                    found = true;
            }
        }
        for (int i = 0; i < 10; i++) {
            if (g_game->troop(idx)->stash[i].id == id && (fstash[0][i] & 1))
                found = true;
        }
    }

    if (found) {
        if (page == 1) {
            tstring msg("ui_reclaim_your_sold_goods");
            g_msglog.add(*g_strtable.lookup(msg), -1);
        } else {
            tstring msg("ui_return_your_bought_goods");
            g_msglog.add(*g_strtable.lookup(msg), -1);
        }
    }
    return found;
}

// @ 0x513710
void CommitState233::commit(char* out)
{
    if (f0 != 0)
        f0 = 0;

    if (f1 != 0) {
        if (f8 <= 0 && g_game->fb0 == 0)
            g_game->fb0 = 1;
        if (g_game->fb0 != 0) {
            g_panel.setup_513f80(g_game->fb0, 0);
            g_game->fb0 = 0;
        }
        f1 = 0;
    }

    if (g_game->fa8 != 0) {
        g_panel.setup_513f80(g_game->fa8, 0);
        g_game->fa8 = 0;
    }
    g_game->fa8 = 0;

    int key = g_panel.pop_key();
    *out = 1;

    if (key == 1) {
        f1 = 1;
        g_panel.start_mission();
    }
    else if (key == 2) {
        g_panel.f514890(g_a73440, -1, -1, 3);
    }
    else if (key == 0x40 || key == 0x20) {
        if (key == 0x20) {
            int p = g_a7343c;
            g_panel.f514890(g_game->parties.get(p)->fac, -1, p, 2);
        }
        else if (key == 0x40) {
            int p = *(int*)(0xa73304 - g_a73438 * 4);
            g_panel.f514890(g_game->parties.get(p)->fac, -1, p, 4);
        }
    }
    else if (key == 0x10) {
        g_wAC->f4 = 0;
        if (g_a73320 != 0)
            g_wAC->f4 = 2;
        g_wAC->f18 = 0x64;
        g_wAC->f4 = 0;
        return g_main_menu.reset(0xb);
    }
    else if (key == 0x80) {
        g_wA0->f4 = 1;
        g_wA0->f18 = 0x64;
        return g_main_menu.go(8);
    }
    else if (key == 0x100) {
        g_wB0->f4 = 4;
        g_wB0->f18 = 0x64;
        return g_main_menu.go(0xc);
    }
    else if (key == 4) {
        g_panel.pick_template();
        g_wA8->f18 = 0x64;
        return g_main_menu.go(0xa);
    }
    else if (key == 0x800) {
        g_panel.start_battle(g_game->f13af0, g_game->f13afc, g_game->f13af4, g_game->f13af8);
        g_wC0->f18 = 0x64;
        return g_main_menu.go(0x10);
    }
    else if (key == 0x1000) {
        g_wEC->f18 = 0x64;
        return g_main_menu.go(0x1b);
    }
    else if (key == 0x2000) {
        g_wE8->f18 = 0x64;
        return g_main_menu.go(0x1a);
    }
    else if (key == 8) {
        g_panel.start_scene(g_game->f13afc, g_game->f13af4, g_game->f13af8);
        g_scene->f5270 = g_a735c0++;
        g_scene->start_setup();
        g_wB8->v4();
        g_wB8->f18 = 0x64;
        return g_main_menu.go(0xe);
    }
    else if (key == 0x200) {
        g_panel.update_parties();
        autosave_notice();
        return g_main_menu.go(7);
    }
    else if (key == 0x400 && f8 <= 0) {
        g_w98->f18 = 0x64;
        return g_main_menu.go(6);
    }
    else if (g_game->fc8 == 1) {
        g_game->fc8 = 0;
        g_panel.update_parties();
        autosave_notice();
        return g_main_menu.go(7);
    }
    else if (f8 <= 0) {
        g_w98->f18 = 0x64;
        return g_main_menu.go(6);
    }
    else {
        f2 = 1;
        g_wDC->v4();
        g_wDC->f18 = 0x64;
        return g_main_menu.go(0x17);
    }

    g_wC0->f18 = 0x64;
    return g_main_menu.go(0x10);
}

// @ 0x513b10
unsigned InventoryPanel::pop_key()
{
    unsigned* p = (unsigned*)0x5a8e68;

    do {
        unsigned k = *p;
        if (k & f3c) {
            f3c &= ~k;
            return k;
        }
        p += 2;
    } while ((int)p < 0x5a8f68);
    return 0;
}

// @ 0x513b40
void InventoryPanel::update_parties()
{
    if (f40 != 0) {
        f40 = 0;
        g_game->clear_hero_ammo(g_game->f78);

        if (page_party[f138] >= 0 && g_game->parties.is_slot(page_party[f138])) {
            int a = 0, b = 0, c = 0, d = 0;

            g_game->clear_hero_ammo(page_party[f138]);
            g_game->parties.get(page_party[f138])->f8b4 = 1;
            g_game->count_party(page_party[f138], &a, &b, &c, &d);
        }
        if (f2c8 != 0) {
            if (page_party[1 - f138] >= 0 && g_game->parties.is_slot(page_party[1 - f138])) {
                int a = 0, b = 0, c = 0, d = 0;

                g_game->clear_hero_ammo(page_party[1 - f138]);
                g_game->parties.get(page_party[1 - f138])->f8b4 = 1;
                g_game->count_party(page_party[1 - f138], &a, &b, &c, &d);
            }
        }

        if (f14 == 3 || (f164 & 1)) {
            unsigned h = f08;
            char* blk = g_game->f_e4[h >> 4];

            ((DetachTarget*)(blk + (h & 0xf) * 0x98))->detach_party();
        }

        PartyMgr* pm = &g_game->parties;

        pm->resolve(pm->party(g_game->f78))->f88 = -1;
        pm->resolve(pm->party(g_game->f78))->f918 = 0;

        if (count_hero_items() > f168) {
            tstring msg("ui_some_soldiers_are_ready_to_upgrade");

            g_msglog.add(g_strtable.lookup(msg), 0xffbbffbb);
        }

        pm->resolve(pm->party(g_game->f78))->f8b4 = 1;
    }
}

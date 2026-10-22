// Slice s005324d0 (batch ds1): the party-stack grid helpers of the party window --
// selecting/clearing a grid cell, reading a cell, moving men between char records,
// applying the edited stack lists back to the parties, swapping two stack rows and
// compacting a party's stack list.  Names are provisional.
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <dinput.h>
#include <stdlib.h>
#include <math.h>
#include "rgl.h"

// ---- the party-window object (only the offsets used here are typed) ------------------------------
struct Party;
struct Widget {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09(); virtual void v10();
    virtual void select(int flag);                       // +0x2c
};

struct PartyMgr {
    char pad0[0x138d0];
    Party** pages;                                       // +0x138d0
    int index_of(unsigned id);                           // 0x48ad00 (thiscall, ret 4)
    Party* get(int id);                                  // 0x51ee50 (thiscall, ret 4)
};

struct Party {                                           // 0x5ac8-byte party record
    char data[0x5ac8];

    void remove_slot(int slot);                          // 0x48c330 (ret 4)
    int  total_value(unsigned flags);                    // 0x4b1cf0 (ret 4)
    void remove_row(int* out, int slot, int* table);     // 0x489540
    int  find_row(int troop, int type, int flag);        // 0x489620
    __declspec(noinline) void compact();                 // 0x532db0

    void clear_selection();                              // 0x5324d0
    void get_cell(int a, int b, int c, int* main, int* cell);   // 0x532520
    void move_men(int amount);                           // 0x532560
    void apply(int which);                               // 0x5325b0
    void swap_rows(int which);                           // 0x532740
};
typedef char assert_party_size[sizeof(Party) == 0x5ac8 ? 1 : -1];

inline Party* party_at(PartyMgr* m, int id)
{
    Party* page = m->pages[id >> 4];
    return &page[id & 0xf];
}

struct CharRec {                                         // 0x508-byte troop record
    char pad0[0x3b4];
    int f3b4;                                            // +0x3b4
    char pad3b8[0x508 - 0x3b8];
};

struct GameData {
    char pad0[0x78];
    int main_party;                                      // +0x78
    char pad7c[0x8c - 0x7c];
    int player;                                          // +0x8c
    char pad90[0x13a00 - 0x90];
    CharRec* chars;                                      // +0x13a00
};
extern GameData* g_game;                                 // 0x5e2930

inline PartyMgr* party_mgr() { return (PartyMgr*)((char*)g_game + 0xf8); }

inline Party* party_index(int id)
{
    PartyMgr* m = party_mgr();
    int idx = m->index_of(id);
    return party_at(m, idx);
}

// @ 0x5324d0
void Party::clear_selection()
{
    if (*(int*)((char*)this + 0x8e4) != -1) {
        int idx = (*(int*)((char*)this + 0x8e0) + *(int*)((char*)this + 0x8dc) * 2) * 0x40
                + *(int*)((char*)this + 0x8e4);
        Widget* w = *(Widget**)((char*)this + 0xb8 + idx * 4);
        w->select(0);
    }
    *(int*)((char*)this + 0x8e4) = -1;
    *(int*)((char*)this + 0x8e0) = -1;
    *(int*)((char*)this + 0x8dc) = -1;
}

// @ 0x532520
void Party::get_cell(int a, int b, int c, int* main, int* cell)
{
    *cell = ((int*)((char*)this + 0x4c8))[(b + a * 2) * 0x40 + c];
    *main = g_game->main_party;
    if (a != 0)
        *main = *(int*)((char*)this + 0x10);
}

// @ 0x532560
void Party::move_men(int amount)
{
    g_game->chars[g_game->player].f3b4 += amount;
    g_game->chars[*(int*)((char*)this + 0x14)].f3b4 -= amount;
}

struct Row { int v[8]; };
typedef char assert_row[sizeof(Row) == 0x20 ? 1 : -1];

// @ 0x5325b0
void Party::apply(int which)
{
    GameData* g = g_game;
    int mainp = g->main_party;
    int owner = g->player;
    if (which != 0) {
        mainp = *(int*)((char*)this + 0x10);
        if (*(int*)((char*)this + 4) == 3 || *(int*)((char*)this + 4) == 2) {
            owner = *(int*)((char*)this + 0x14);
        } else {
            owner = *(int*)((char*)party_mgr()->get(mainp) + 0xac);
        }
    }
    if (owner >= 0) {
        g->chars[owner].f3b4 = ((int*)((char*)this + 0x18f0))[which];
    }
    if (mainp >= 0) {
        *(int*)((char*)party_index(mainp) + 0x8a8) = ((int*)((char*)this + 0x18e8))[which];
        if (((int*)((char*)this + 0x18e8))[which] > 0) {
            Party* src = (Party*)((char*)this + 0x8e8 + which * 0x800);
            int i = 0;
            do {
                *(Row*)((char*)party_index(mainp) + 0xa8 + i * 0x20) = *(Row*)src;
                i++;
                src = (Party*)((char*)src + 0x20);
            } while (i < ((int*)((char*)this + 0x18e8))[which]);
        }
    }
    if (which == 0) {
        for (int i = 0; i < 64; i++)
            for (int j = 0; j < 32; j++)
                ((int*)((char*)g_game + 0x1405c))[i * 32 + j] =
                ((int*)((char*)this + 0x1908))[i * 32 + j];
    }
}

// @ 0x532740
void Party::swap_rows(int which)
{
    int slot = ((int*)((char*)this + 0x4c8))[
        (*(int*)((char*)this + 0x8e0) + *(int*)((char*)this + 0x8dc) * 2) * 0x40
        + *(int*)((char*)this + 0x8e4)];
    int owner = g_game->main_party;
    if (*(int*)((char*)this + 0x8dc) != 0)
        owner = *(int*)((char*)this + 0x10);
    Party* p = party_index(owner);

    Row tmp = *(Row*)((char*)p + 0xa8 + (slot + which) * 0x20);
    *(Row*)((char*)p + 0xa8 + (slot + which) * 0x20) =
        *(Row*)((char*)p + 0xa8 + slot * 0x20);
    *(Row*)((char*)p + 0xa8 + slot * 0x20) = tmp;

    int o1 = 0x1405c + slot * 0x80;
    int o2 = 0x1405c + (slot + which) * 0x80;
    for (int i = 0; i < 0x20; i++) {
        int t = *(int*)((char*)g_game + o2);
        *(int*)((char*)g_game + o2) = *(int*)((char*)g_game + o1);
        *(int*)((char*)g_game + o1) = t;
        o1 += 4;
        o2 += 4;
    }
}

// @ 0x532830
void __stdcall stack_remove(int party_id, int slot, int dst_id, int* was_present, int* removed)
{
    int* table;
    Row local;
    int dummy[8];
    PartyMgr* m = party_mgr();
    local = *(Row*)((char*)party_at(m, m->index_of(party_id)) + 0xa8 + slot * 0x20);

    *was_present = *(int*)((char*)party_at(m, m->index_of(party_id)) + 0xa8 + slot * 0x20 + 0xc) > 0;
    if (*was_present)
        (*(int*)((char*)party_index(party_id) + 0xa8 + slot * 0x20 + 0xc))--;
    bool had = *(int*)((char*)party_index(party_id) + 0xa8 + slot * 0x20 + 0x10) > 0;
    if (had)
        (*(int*)((char*)party_index(party_id) + 0xa8 + slot * 0x20 + 0x10))--;
    *removed = 0;
    (*(int*)((char*)party_index(party_id) + 0xa8 + slot * 0x20))--;
    if (*(int*)((char*)party_index(party_id) + 0xa8 + slot * 0x20) < 1) {        *removed = 1;
        table = (party_id == g_game->main_party)
              ? (int*)((char*)g_game + 0x1405c) : 0;
        party_index(party_id)->remove_row(dummy, slot, table);
    }

    int found = party_index(dst_id)->find_row(local.v[1], local.v[2], 1);
    if (found == *(int*)((char*)party_index(dst_id) + 0x8a8)) {
        int* row = (int*)((char*)party_index(dst_id) + 0xa8 + found * 0x20);
        row[0] = 0;
        row[2] = 0;
        row[5] = 0;
        row[3] = 0;
        row[4] = 0;
        row[6] = 0;
        row[1] = -1;
        *(int*)((char*)party_index(dst_id) + 0xa8 + found * 0x20 + 4) = local.v[1];
        *(int*)((char*)party_index(dst_id) + 0xa8 + found * 0x20 + 8) = local.v[2];
        (*(int*)((char*)party_index(dst_id) + 0x8a8))++;
    }
    if (dst_id == g_game->main_party) {
        if (*(int*)((char*)party_index(dst_id) + 0xa8 + found * 0x20) < 0x20) {
            int* row = (int*)((char*)party_index(g_game->main_party) + 0xa8 + found * 0x20);
            int rv = rand();
            ((int*)((char*)g_game + 0x1405c))[row[0] + found * 0x20] = rv;
        }
    }
    (*(int*)((char*)party_index(dst_id) + 0xa8 + found * 0x20))++;
    (*(int*)((char*)party_index(dst_id) + 0xa8 + found * 0x20 + 0xc)) += 1 - *was_present;
    if (had) {
        (*(int*)((char*)party_index(dst_id) + 0xa8 + found * 0x20 + 0x10))++;
    }
    party_index(dst_id)->compact();
    if ((local.v[2] & 1) == 0) {
        party_index(dst_id)->total_value(0x10);
    }
}

// @ 0x532db0
void Party::compact()
{
    int i = 1;
    if (*(int*)((char*)this + 0x8a8) > 1) {
        do {
            remove_slot(i);
            i++;
        } while (i < *(int*)((char*)this + 0x8a8));
    }
    *(char*)((char*)this + 0x8b4) = 1;
}

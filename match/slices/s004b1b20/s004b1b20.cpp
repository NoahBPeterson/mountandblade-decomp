// Slice s004b1b20: party inventory / morale / skill helpers (tentative names).
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "rgl.h"
using rgl::matrix;

struct tstring;

// ---- game data ----------------------------------------------------------------------------------------
struct CharRec {                                    // troop record, 0x508 bytes
    char pad0[0x30];
    unsigned __int64 flags;                         // +0x30 tf_*
    char pad38[0x50 - 0x38];
    int f50;                                        // +0x50
    char pad54[0x64 - 0x54];
    int f64;                                        // +0x64
    char pad68[0x9c - 0x68];
    int f9c;                                        // +0x9c
    float fa0;                                      // +0xa0
    char pada4[0xb0 - 0xa4];
    int level;                                      // +0xb0
    char padb4[0x4e4 - 0xb4];
    int f4e4;                                       // +0x4e4
    char pad4e8[0x508 - 0x4e8];

    int perk_value(int perk);                       // 0x4351a0 (ret 4)
    float food_factor();                            // 0x4b21f0
};
typedef char assert_charrec[sizeof(CharRec) == 0x508 ? 1 : -1];

struct GameData {
    char pad0[0x78];
    int main_party;                                 // +0x78
    char pad7c[0x8c - 0x7c];
    int player;                                     // +0x8c
    unsigned f90;                                   // +0x90
    char pad94[0x13a00 - 0x94];
    CharRec* chars;                                 // +0x13a00
    char pad13a04[0x1a068 - 0x13a04];
    int f1a068;                                     // +0x1a068

    bool cond_ok(int troop);                        // 0x4638f0 (ret 4)
    int skill_value(int troop, int skill);          // 0x437b20 (ret 8)
    bool not_afternoon();                           // 0x435740
    bool has_perk(int perk, int troop, int unused);  // 0x435790 (ret 0xc)
};
extern GameData* g_game;                            // 0x5e2930

extern float g_k6_9;                                // 0x5935d0
extern float g_k0_04;                               // 0x54ab1c
extern float g_k0_006;                              // 0x5935cc
extern float g_k30;                                 // 0x54b658
extern float g_k0_01;                               // 0x54a6f0
extern float g_k15;                                 // 0x590da0
extern float g_k0_7;                                // 0x54b3c8
extern float g_k0_6;                                // 0x590d94
extern float g_k1_2;                                // 0x590b94
extern float g_k1_25;                               // 0x590c80
extern float g_k1_05;                               // 0x590f24
extern float g_k1;                                  // 0x549cdc
extern float g_k0;                                  // 0x549ca8
extern float g_k3;                                  // 0x54a710
extern float g_k0_02;                               // 0x590cd0
extern float g_k0_4;                                // 0x54b3b8
extern int g_level_exp[];                           // 0x5934bc
extern int g_perk_a;                                // 0xa18120
extern int g_perk_b;                                // 0xa18124
extern int g_perk_c;                                // 0xa18128
extern float g_perk_d;                              // 0xa18138
extern float g_perk_e;                              // 0xa1813c
extern __int64 g_perk_result;                       // 0xa70458

// inventory slot of a party (0x20 bytes)
struct InvSlot {
    int count;                                      // +0
    int item;                                       // +4
    int flags8;                                     // +8 (bit 0)
    char pad9[4];                                   // +0xc
    int damage;                                     // +0x10
    int dmg14;                                      // +0x14
    float fac;                                      // +0x18
    char pad1c[4];
};
typedef char assert_invslot[sizeof(InvSlot) == 0x20 ? 1 : -1];

struct Party {                                      // 0x5ac8 bytes
    char pad0[8];
    int f8;                                         // +0x8 (troop id)
    char padc[0x88 - 0xc];
    int f88;                                        // +0x88
    char pad8c[0xa0 - 0x8c];
    unsigned __int64 flagsa0;                       // +0xa0
    InvSlot inv[0x40];                              // +0xa8
    int inv_count;                                  // +0x8a8
    float fac;                                      // +0x8ac
    float db0;                                      // +0x8b0
    char pad8b4[0x5978 - 0x8b4];
    int f5978;                                      // +0x5978 (type)
    char pad597c[0x5ac8 - 0x597c];

    bool update();                                  // 0x4b1b80
    int slot_value(int idx, unsigned flags) const;  // 0x4b1be0 (ret 8)
    int total_value(unsigned flags) const;          // 0x4b1cf0 (ret 4)
    int find_worth(int value, unsigned flags) const;// 0x4b1d30 (ret 8)
    void add_time(int idx, int amount);             // 0x4b1d80 (ret 8)
    void compact();                                 // 0x4b1eb0
    float morale_factor();                          // 0x4b1fa0
    float speed_factor();                           // 0x4b20b0
    void set_speed_from_stat();                     // 0x4b2120
    int skill_bonus();                              // 0x4b22d0
    int skill_bonus2();                             // 0x4b2330
    int skill(int skill);                           // 0x4b2370 (ret 4)

    void swap_slots(int a, int b);                  // 0x4b1710 (ret 8)
};
typedef char assert_party_size[sizeof(Party) == 0x5ac8 ? 1 : -1];

// 0x5c-byte item record (the vector copy helpers below)
struct ItemRec {
    char bytes[0x5c];
};
typedef char assert_itemrec[sizeof(ItemRec) == 0x5c ? 1 : -1];

// @ 0x4b1b20
void copy_items_forward(ItemRec* dst, ItemRec* end, const ItemRec* src)
{
    while (dst != end) {
        *dst = *src;
        ++dst;
    }
}

// @ 0x4b1b50
void copy_items_backward(ItemRec* stop, ItemRec* first, ItemRec* last)
{
    while (first != stop) {
        --first;
        --last;
        *last = *first;
    }
}

// @ 0x4b2160
void __stdcall copy_item(void* dst, void* src)
{
    if (dst) {
        ItemRec tmp = *(ItemRec*)src;
        *(ItemRec*)dst = tmp;
    }
}

// A container whose first member is the item-array base pointer (iterator helper).
struct ItemIter {                           // vector iterator: non-POD, so returned through a hidden pointer
    ItemRec* p;
    ItemIter(ItemRec* q) : p(q) {}
};
struct ItemArray {
    ItemRec* base;
    ItemIter at(int idx);                   // 0x4b22b0
};

// @ 0x4b22b0
ItemIter ItemArray::at(int idx)
{
    return ItemIter(base + idx);
}

__declspec(noreturn) void __stdcall throw_exception(void*, void*);  // 0x5368e6

// @ 0x4b21e6
void throw_stub()
{
    throw_exception(0, 0);
}

// @ 0x4b1b80
bool Party::update()
{
    for (int i = 0; i < inv_count; i++) {
        InvSlot& slot = inv[i];
        if (slot.count <= 0) {
            slot.count = 0;
        }
        if (!(g_game->chars[slot.item].flags & 0x10) && slot.damage > slot.count) {
            slot.damage = slot.count;
        }
    }
    return true;
}

// @ 0x4b1be0
int Party::slot_value(int idx, unsigned flags) const
{
    GameData* g = g_game;
    const InvSlot* slot = &inv[idx];
    if (flags & 0x20) {
        if (slot->item == g->player) {
            goto zero;
        }
    }
    if (flags & 1) {
        if (idx == 0) {
            if ((g->chars[slot->item].flags & 0x10) != 0) {
                goto zero;
            }
        }
    }
    if (flags & 2) {
        if ((g->chars[slot->item].flags & 0x10) != 0) {
            goto zero;
        }
    }
    if (flags & 4) {
        if ((g->chars[slot->item].flags & 0x10) != 0) {
            if (g->cond_ok(slot->item)) {
                goto zero;
            }
        }
    }
    if (flags & 8) {
        if ((g->chars[slot->item].flags & 0x10) == 0) {
            goto zero;
        }
    }
    if (flags & 0x10) {
        if (slot->flags8 & 1) {
            goto zero;
        }
    }
    int amount = slot->count;
    if (flags & 0x100) {
        if (!(g->chars[slot->item].flags & 0x10)) {
            amount -= slot->damage;
        }
    }
    if (amount < 0) {
        goto zero;
    }
    return amount;
zero:
    return 0;
}

// @ 0x4b1cf0
int Party::total_value(unsigned flags) const
{
    int total = 0;
    int n = inv_count;
    for (int i = 0; i < n; i++) {
        total += slot_value(i, flags);
    }
    return total;
}

// @ 0x4b1d30
int Party::find_worth(int value, unsigned flags) const
{
    int total = 0;
    for (int i = 0; i < inv_count; i++) {
        int n = slot_value(i, flags);
        total += n;
        if (n != 0 && value < total) {
            return i;
        }
    }
    return -1;
}

// @ 0x4b1d80
void Party::add_time(int idx, int amount)
{
    GameData* g = g_game;
    CharRec* chars = g->chars;
    InvSlot& slot = inv[idx];
    float& acc = inv[idx].fac;
    CharRec* chr = &chars[slot.item];
    if (chr->flags & 0x10) {
        chr->f9c += amount;
        return;
    }
    if (chr->f50 == 0) {
        return;
    }
    acc += (float)amount;
    if (acc >= (float)(int)(g_level_exp[chr->level] * g_k0_006 + g_k30)) {
        while (slot.dmg14 < slot.count) {
            slot.dmg14++;
            acc -= (float)(int)(g_level_exp[chr->level] * g_k0_006 + g_k30);
            if ((float)(int)(g_level_exp[chr->level] * g_k0_006 + g_k30) >= acc) {
                break;
            }
        }
    }
    if ((float)slot.dmg14 >= (float)slot.count) {
        acc = 0;
    }
}

// @ 0x4b1eb0
void Party::compact()
{
    int keep = 0;
    for (int i = 0; i < inv_count; i++) {
        if (inv[i].flags8 & 1) {
            break;
        }
        keep++;
    }
    int dst = 0;
    for (int j = 0; j < keep; j++) {
        if (g_game->chars[inv[j].item].flags & 0x10) {
            if (dst != j) {
                swap_slots(dst, j);
            }
            dst++;
        }
    }
    for (int j = keep; j < inv_count; j++) {
        if (g_game->chars[inv[j].item].flags & 0x10) {
            if (keep != j) {
                swap_slots(keep, j);
            }
            keep++;
        }
    }
}
// @ 0x4b1fa0
float Party::morale_factor()
{
    if (f88 != -1) {
        return g_k1;
    }
    int a = 0;
    int b = 0;
    for (int i = 0; i < inv_count; i++) {
        InvSlot s = inv[i];
        unsigned __int64 fl = g_game->chars[s.item].flags;
        if (fl & 0x10) {
            ++a;
        } else if (fl & 0x400) {
            a += s.count;
        } else {
            b += s.count;
        }
    }
    float v = ((float)a * g_k3 + (float)b) * g_k0_02 + g_k0_4;
    if (v < g_k0) {
        return g_k0;
    }
    if (v >= g_k1) {
        return g_k1;
    }
    return v;
}

// @ 0x4b20b0
float Party::speed_factor()
{
    if (flagsa0 & 0x200) {
        return g_k15;
    }
    float v = db0 * fac;
    int type = f5978;
    if (type == 0xb || type == 0xc || type == 0xa || type == 0xd) {
        v = g_k0_7 * v;
    }
    if (g_game->not_afternoon()) {
        return g_k0_6 * v;
    }
    return v;
}

// @ 0x4b2120
void Party::set_speed_from_stat()
{
    g_perk_result = 100;
    g_game->has_perk(0x1a, f8, 0);
    db0 = (float)g_perk_result * g_k0_01;
}

// A 12-byte element and the copier invoked by construct_items (0x436cf0, __thiscall).
struct Elem12 { int a, b, c; };
typedef char assert_elem12[sizeof(Elem12) == 0xc ? 1 : -1];
struct ElemCopier {
    void copy(Elem12* dst, Elem12* src);            // 0x436cf0 (ret 8)
};

// @ 0x4b2180
Elem12* construct_items(Elem12* first, Elem12* last, Elem12* dst, ElemCopier* ctx)
{
    try {
        while (first != last) {
            ctx->copy(dst, first);
            ++dst;
            ++first;
        }
    } catch (...) {
    }
    return dst;
}

// @ 0x4b21f0
float CharRec::food_factor()
{
    unsigned __int64 f = flags;
    if (f & 0x10) {
        if (f4e4 >= 0) {
            return (perk_value(0x18) * g_k0_04 + g_k1_25) * g_k6_9;
        }
        return (perk_value(0x19) * g_k0_04 + g_k1_05) * g_k6_9;
    }
    if (f & 0x400) {
        return (perk_value(0x18) * g_k0_04 + g_k1_2) * g_k6_9;
    }
    return (perk_value(0x19) * g_k0_04 + g_k1) * g_k6_9;
}

// @ 0x4b22d0
int Party::skill_bonus()
{
    if (g_game->has_perk(0x11, f8, 0)) {
        return (int)g_perk_result;
    }
    int item = inv[0].item;
    int bonus = g_game->chars[item].f64;
    return g_game->skill_value(item, 1) * g_perk_a + g_perk_c + bonus;
}

// @ 0x4b2330
int Party::skill_bonus2()
{
    if (g_game->has_perk(0x12, f8, 0)) {
        return (int)g_perk_result;
    }
    return g_game->skill_value(inv[0].item, 2) * g_perk_b;
}

// @ 0x4b2370
int Party::skill(int skill)
{
    int best = 0;
    if (inv_count != 0 && !g_game->cond_ok(inv[0].item)) {
        best = g_game->skill_value(inv[0].item, skill);
    }
    int found = 0;
    for (int i = 0; i < inv_count; i++) {
        int item = inv[i].item;
        if (!(g_game->chars[item].flags & 0x10)) {
            continue;
        }
        if (inv[i].flags8 != 0) {
            continue;
        }
        if (item == g_game->player) {
            if (!(g_game->chars[item].fa0 >= g_perk_e)) {
                continue;
            }
        } else {
            if (!(g_game->chars[item].fa0 >= g_perk_d)) {
                continue;
            }
        }
        int v = g_game->skill_value(item, skill);
        if (v > found) {
            found = v;
        }
    }
    int r = found;
    if (best >= 2) r = found + 1;
    if (best >= 5) r++;
    if (best >= 8) r++;
    if (best >= 0xa) r++;
    return r;
}

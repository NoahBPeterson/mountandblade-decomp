// Slice s004b2c00: the party's per-frame tick (party speed / zoom easing, food and inventory decay, then the
// main party's zoom-ease, item-grid decay and morale curve), the "leave the map" flag reset, the 0x5c-byte
// item-range construct loop and the per-item timer reset.
//
//   0x4b2c00  Party::tick
//   0x4b32f0  Party::clear_map_flag
//   0x4b3350  construct_items_n (0x5c-byte records through 0x4b2160, by-reference iterator)
//   0x4b33b4  throw stub (_CxxThrowException from the EH handler above)
//   0x4b33c0  Party::advance_clock (+0x64 milliseconds accumulator, per-item +0x6c reset)
#include <math.h>
#include <vector>
#include "rgl.h"

using rgl::vector4;

// ---- engine helpers -----------------------------------------------------------------------------------------
void __stdcall throw_exception(void*, void*);            // 0x5368e6  _CxxThrowException
struct ElemCopier {                                      // opaque context object passed as arg4
    void copy_item(void* dst, const void* src);          // 0x4b2160  0x5c-byte record copy (thiscall, ignores this)
};
int __cdecl rand_value();                                // 0x536a9f  LCG game random (0 .. 0x7fff)
struct SceneMgr {                                        // global at 0xa732b4
    void release(void* id);                              // 0x41f0d0 (thiscall; pops its argument)
};
extern SceneMgr g_scene_mgr;                             // 0xa732b4

extern float g_gk3;                                      // 0x54a710  3.0f
extern float g_gk0_05a;                                  // 0x549d70  0.5f
extern float g_gk0_1;                                    // 0x549d7c  0.1f
extern float g_gk0_6;                                    // 0x590d94  0.6f
extern float g_k0;                                       // 0x549ca8  0.0f
extern float g_k1;                                       // 0x549cdc  1.0f
extern float g_k0_1;                                     // 0x54b8b0  0.1f
extern float g_k0_04;                                    // 0x54ab1c  0.04f
extern float g_k0_02;                                    // 0x590cd0  0.02f
extern float g_k_slot;                                   // 0x590b10  0.13f (value of one slot unit)
extern float g_k_consumption;                            // 0x590c78  0.002f  morale drain
extern float g_k_zoom_step;                              // 0x54b8b4  0.03f (zoom step per tick)
extern float g_rand_k;                                   // 0x54a6bc  1/15817
extern float g_gk_skill_k;                               // 0x54a6e4
extern float g_gk_skill_k2;                              // 0x54a704
extern float g_k_item_weight;                            // 0x593640  per-item weight of the equipped grid item
extern int g_u_a181a8;                                   // 0xa181a8
extern int g_rand_mod;                                   // 0x5aa054
#define g_clamp_max (*(int*)0x8ee3ac4)                  // outside the annotation range, so a literal cast (as s005261e0)

struct Elem12 { int a, b; float c; };                    // the 0xc-byte grid element

// ---- game data ------------------------------------------------------------------------------------------------
struct ItemKind {                                        // 0x238-byte item kind
    char pad0[0x1c8];
    unsigned char f1c8;                                  // +0x1c8 (grid item marker)
    char pad1c9[0x1fc - 0x1c9];
    int f1fc;                                            // +0x1fc
    int f210;                                            // +0x210
    char pad214[0x238 - 0x214];
};
typedef char assert_itemkind[sizeof(ItemKind) == 0x238 ? 1 : 8];

struct ItemRef {                                         // 8-byte item reference
    int index;                                           // +0  (item kind)
    unsigned flags;                                      // +4  (low byte = count, top byte = tag)

    int seq();                                           // 0x434b70
    int reduce(int n);                                   // 0x437510 (ret 4)
};
typedef char assert_itemref[sizeof(ItemRef) == 8 ? 1 : 8];

struct CharRec {                                         // troop record, 0x508 bytes
    char pad0[0x30];
    unsigned __int64 flags;                              // +0x30
    char pad38[0x58 - 0x38];
    int f58;                                             // +0x58
    char pad5c[0x7a - 0x5c];
    unsigned char f7a;                                   // +0x7a (food level)
    char pad7b[0xa0 - 0x7b];
    float fa0;                                           // +0xa0 (food)
    char pada4[0xb4 - 0xa4];
    ItemRef grid[16];                                    // +0xb4 (item grid; 0x462b60 returns the used length)
    char pad134[0x4ec - 0x134];
    ItemRef slot;                                        // +0x4ec (index), +0x4f0 (flags): the first record's item slot
    char pad4f4[0x508 - 0x4f4];

    int grid_size();                                     // 0x462b60
};
typedef char assert_charrec[sizeof(CharRec) == 0x508 ? 1 : 8];

struct InvSlot {                                         // 0x20-byte inventory slot of the party (stack of one troop)
    char pad0[4];
    int troop;                                           // +4  (index of the troop record)
    char pad8[0x10 - 8];
    int count;                                           // +0x10 (decays while the troop is not a food consumer)
    char pad14[0x20 - 0x14];
};
typedef char assert_invslot[sizeof(InvSlot) == 0x20 ? 1 : 8];

struct Party;
struct PartyPool {
    char pad0[0x138cc];
    std::vector<char*> pages;                            // +0x138cc (page pointers, 16 parties per page)

    int index_of(int handle);                            // 0x48ad00 (ret 4)
    Party* get(int handle)
    {
        int k = index_of(handle);
        return (Party*)((k & 0xf) * 0x5ac8 + *(pages.begin() + (k >> 4)));
    }
};

struct GameData {
    char pad0[0x78];
    int main_party;                                      // +0x78
    char pad7c[0x8c - 0x7c];
    int player;                                          // +0x8c
    char pad90[0xf8 - 0x90];
    PartyPool parties;                                   // +0xf8 .. (pages at +0x138cc relative to it)
    char padfc[0x13a00 - 0xf8 - sizeof(PartyPool)];
    CharRec* chars;                                      // +0x13a00
    char pad13a04[0x13ad0 - 0x13a04];
    float zoom;                                          // +0x13ad0
    int f13ad4;                                          // +0x13ad4
};
extern GameData* g_game;                                 // 0x5e2930
extern ItemKind* g_items;                                // 0x5e29d0

struct Party {
    char pad0[8];
    int id;                                              // +0x8
    char padc[0x14 - 0xc];
    unsigned entries;                                    // +0x14  (0x88-byte entries)
    unsigned entries_end;                                // +0x18
    char pad1c[0x64 - 0x1c];
    int clock64;                                         // +0x64  milliseconds accumulator
    char pad68[0x78 - 0x68];
    float f78;                                           // +0x78
    float f7c;                                           // +0x7c
    float zoom;                                          // +0x80
    char pad84[0x88 - 0x84];
    int f88;                                             // +0x88
    int f8c;                                             // +0x8c
    char pad90[0xa0 - 0x90];
    unsigned __int64 flagsa0;                            // +0xa0
    InvSlot inv[0x40];                                   // +0xa8 .. +0x8a7
    int inv_count;                                       // +0x8a8
    char pad8ac[0x8c8 - 0x8ac];
    int f8c8;                                            // +0x8c8
    char pad8cc[0x59c0 - 0x8cc];
    unsigned f59c0;                                      // +0x59c0
    char pad5ad0[0x5ac8 - 0x59c4];

    int skill(int skill);                                // 0x4b2370
    int total_value(unsigned flags);                     // 0x4b1cf0
    void tick();                                         // 0x4b2c00
    void clear_map_flag();                               // 0x4b32f0
    void advance_clock();                                // 0x4b33c0
};

// @ 0x4b3350
// Copy `count` 0x5c-byte item records from `src` to `dst` (each step advances both cursors by one record).
void construct_items_n(void* dst, unsigned count, const void* src, ElemCopier* ctx)
{
    try {
        while (count > 0) {
            ctx->copy_item(dst, src);
            count--;
            dst = (char*)dst + 0x5c;
        }
    } catch (...) {
        throw;
    }
}

// @ 0x4b33b4
void throw_stub()
{
    throw_exception(0, 0);
    __assume(0);
}

// @ 0x4b32f0
void Party::clear_map_flag()
{
    if (f8c <= 0 && (flagsa0 & 0x400)) {
        return;
    }
    if (f59c0 != 0) {
        g_scene_mgr.release((void*)((unsigned*)f59c0)[2]);
    }
    unsigned __int64 flags = flagsa0 | 0x100;
    f59c0 = 0;
    flagsa0 = flags;
}
// @ 0x4b33c0
struct ClockEntry { char pad0[0x6c]; int f6c; char pad70[0x88 - 0x70]; };
typedef char assert_clockentry[sizeof(ClockEntry) == 0x88 ? 1 : 8];

void Party::advance_clock()
{
    if (clock64 > 1000000000) {
        int cnt = (int)(entries_end - entries) / 0x88;
        if (entries != 0) {
            for (unsigned i = 0; i < (unsigned)cnt; i++) {
                ((ClockEntry*)entries)[i].f6c = 0;
            }
        }
        clock64 = 0;
    }
    clock64 += 3;
}

// @ 0x4b2c00
void Party::tick()
{
    float speed = (float)skill(0xb) * 0.2f + 1.0f;
    if (id == g_game->main_party && g_game->zoom > 0.0f && g_game->f13ad4 != 0) {
        speed = speed + speed;
    } else if (f88 > 0) {
        speed = speed * 0.0f;
    } else if (f8c8 >= 0 && g_game->parties.get(f8c8)->f88 > 0) {
        speed = speed * 0.0f;
    }

    // food growth / stack decay over the party's troop stacks
    for (int i = 0; i < inv_count; i++) {
        int troop = inv[i].troop;
        CharRec* ch = &g_game->chars[troop];
        if (ch->flags & 0x10) {
            int level = ch->f7a & 0xf;
            if (level < 0) {
                level = 0;
            } else if (level > g_clamp_max) {
                level = g_clamp_max;
            }
            float limit = (float)(ch->f58 + level * 2 + 0x23);
            if (ch->fa0 < limit) {
                float add = ((float)(rand_value() % 15817) * g_rand_k + 0.5f) * 0.5f * speed;
                if (troop == g_game->player) {
                    add = add * (3 - g_rand_mod);
                }
                ch->fa0 = add + ch->fa0;
            }
            if (ch->fa0 > limit) {
                ch->fa0 = limit;
            }
        } else if (inv[i].count > 0) {
            int n = inv[i].count;
            float chance = speed * 0.04f;
            do {
                if ((float)(rand_value() % 15817) * g_rand_k < chance) {
                    inv[i].count--;
                }
            } while (--n);
        }
    }

    if (id != g_game->main_party) {
        return;
    }

    // main party: zoom easing, the first record's item slot decay, morale
    if (g_u_a181a8 == 0) {
        if (g_game->zoom > 0.0f && g_game->f13ad4 != 0) {
            zoom = zoom - 0.03f;
            if (zoom < 0.0f) {
                zoom = 0.0f;
            } else if (zoom > 1.0f) {
                zoom = 1.0f;
            }
        } else {
            ItemRef* slot = &g_game->chars->slot;
            if (slot->index < 0) {
                zoom = zoom + 0.03f;
            } else {
                float chance = (float)total_value(0x10) * 0.02f;
                if (zoom > 0.1f) {
                    chance = chance * 3.0f;
                }
                while ((float)(rand_value() % 15817) * g_rand_k < chance) {
                    if (slot->index < 0) {
                        break;
                    }
                    int count = (slot->flags & 0xff) + 1;
                    int cap = g_items[slot->index].f210;
                    if (cap != 0 && (slot->flags & 0xff000000) == 0x2a000000) {
                        float x = (float)cap * 0.13f;
                        double r = floor(x);
                        int n = (int)r;
                        if (x - r > 0.5f) {
                            n = n + 1;
                        }
                        if (n < 1) {
                            n = 1;
                        }
                        cap += n;
                    }
                    if (count > cap) {
                        count = slot->seq();
                    }
                    *(unsigned char*)&slot->flags = 0;
                    slot->flags = slot->flags | count;
                    int cap2 = 0;
                    if (slot->index >= 0) {
                        int k2 = g_items[slot->index].f210;
                        cap2 = k2;
                        if (k2 != 0 && (slot->flags & 0xff000000) == 0x2a000000) {
                            float x2 = (float)k2 * 0.13f;
                            double r2 = floor(x2);
                            int m = (int)r2;
                            if (x2 - r2 > 0.5f) {
                                m = m + 1;
                            }
                            if (m < 1) {
                                m = 1;
                            }
                            cap2 += m;
                        }
                    }
                    if (cap2 - (int)(slot->flags & 0xff) <= 0) {
                        slot->index = -1;
                    }
                    chance = chance - 1.0f;
                }
                zoom = zoom - 0.03f;
                if (zoom > 0.1f) {
                    zoom = zoom - 0.1f;
                }
            }
            if (zoom > 0.1f) {
                f7c = f7c - 0.002f;
            }
            int item = g_game->chars->slot.index;
            int value = 0;
            if (item >= 0) {
                value = g_items[item].f1fc;
            }
            f7c = (float)value * g_k_item_weight + f7c;
        }
        if (zoom < 0.0f) {
            zoom = 0.0f;
        } else if (zoom > 1.0f) {
            zoom = 1.0f;
        }
        if (f7c < 0.0f) {
            f7c = 0.0f;
        } else if (f7c > 1.0f) {
            f7c = 1.0f;
        }
    }

    // the player's item grid decay
    CharRec* pc = &g_game->chars[g_game->player];
    float grid_chance = (float)g_game->parties.get(g_game->main_party)->skill(0xb) * 0.001f;
    if (!(g_game->zoom > 0.0f)) {
        grid_chance = grid_chance * 1.5f;
    }
    for (int i = 0; i < pc->grid_size(); i++) {
        ItemRef* g = &pc->grid[i];
        if (g->index >= 0
            && g_items[g->index].f1c8 == 1
            && (g->flags & 0xff000000) == 0x1e000000
            && (float)(rand_value() % 15817) * g_rand_k < grid_chance) {
            *((unsigned char*)g + 7) = 0;
            g->reduce(0);
        }
    }
}

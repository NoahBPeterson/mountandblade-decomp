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
#include "rgl.h"

using rgl::vector4;

// ---- engine helpers -----------------------------------------------------------------------------------------
void __stdcall throw_exception(void*, void*);            // 0x5368e6  _CxxThrowException
struct ElemCopier {                                      // opaque context object passed as arg4
    void copy_item(void* dst, const void* src);          // 0x4b2160  0x5c-byte record copy (thiscall, ignores this)
};
int __cdecl rand_value();                                // 0x536a9f  LCG game random (0 .. 0x7fff)
float __cdecl grid_round(float v);                       // 0x535a64  floor(v + 0.5)
struct SceneMgr {                                        // global at 0xa732b4
    void release(void* id);                              // 0x41f0d0 (thiscall; pops its argument)
};
extern SceneMgr g_scene_mgr;                             // 0xa732b4

extern float g_gk3;                                      // 0x54a710  3.0f
extern float g_gk0_05a;                                  // 0x549d70  0.5f
extern float g_gk0_05b;                                  // 0x549d7c  0.3f
extern float g_gk0_6;                                    // 0x590d94  0.6f
extern float g_k0;                                       // 0x549ca8  0.0f
extern float g_k1;                                       // 0x549cdc  1.0f
extern float g_k0_1;                                     // 0x54b8b0  0.1f
extern float g_k0_04;                                    // 0x54ab1c  0.04f
extern float g_k0_02;                                    // 0x590cd0  0.02f
extern float g_k1_2;                                     // 0x590b10  1.2f (grid slot value per unit)
extern float g_k_consumption;                            // 0x590c78  morale / consumption unit
extern float g_k_zoom_step;                              // 0x54b8b4  10000.0f (zoom / vitality step)
extern float g_rand_k;                                   // 0x54a6bc  1/15817
extern float g_gk_skill_k;                               // 0x54a6e4
extern float g_gk_skill_k2;                              // 0x54a704
extern float g_k_item_weight;                            // 0x593640  per-item weight of the equipped grid item
extern int g_u_a181a8;                                   // 0xa181a8
extern int g_rand_mod;                                   // 0x5aa054
extern int g_clamp_max;                                  // 0x8ee3ac4

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

struct CharRec {                                         // troop record, 0x508 bytes
    char pad0[0x30];
    unsigned __int64 flags;                              // +0x30
    char pad38[0x58 - 0x38];
    int f58;                                             // +0x58
    char pad5c[0x7a - 0x5c];
    unsigned char f7a;                                   // +0x7a (food level)
    char pad7b[0xa0 - 0x7b];
    float fa0;                                           // +0xa0 (food)
    char pada4[0x4ec - 0xa4];
    int f4ec;                                            // +0x4ec
    char pad4f0[0x508 - 0x4f0];

    int perk_value(int perk);                            // 0x4351a0 (ret 4)
    int food_deadline();                                 // 0x462b60
};
typedef char assert_charrec[sizeof(CharRec) == 0x508 ? 1 : 8];

struct ItemRef {                                         // 8-byte item-grid reference
    int index;                                           // +0  (item kind)
    unsigned flags;                                      // +4  (low byte = count, top byte = tag)

    int seq();                                           // 0x434b70
    int reduce(int n);                                   // 0x437510 (ret 4)
};
typedef char assert_itemref[sizeof(ItemRef) == 8 ? 1 : 8];

struct InvSlot {                                         // 0x20-byte inventory slot
    int count;                                           // +0
    unsigned item;                                       // +4 (low byte = item id, high byte = tag)
    char pad8[0x10 - 8];
    int damage;                                          // +0x10
    int dmg14;                                           // +0x14
    float fac;                                           // +0x18
    char pad1c[4];

    int seq();                                           // 0x434b70
    int reduce(int n);                                   // 0x437510 (ret 4)
};
typedef char assert_invslot[sizeof(InvSlot) == 0x20 ? 1 : 8];

struct Agent {                                           // map agent with the 8-byte item grid at +0xb4
    char pad0[0xb4];
    struct ItemRef grid[16];                             // +0xb4
    char padb4[0xc];
    int skill(int skill);                                // 0x4b2370
    int grid_size();                                     // 0x462b60
};

struct PartyRec {                                        // one 0x5ac8-byte party record
    char pad0[0x30];
    Agent* anim;                                         // +0x30
    char pad34[0x5974 - 0x34];
    int f5974;                                           // +0x5974
    char pad5978[0x5ac8 - 0x5978];
};
typedef char assert_partyrec[sizeof(PartyRec) == 0x5ac8 ? 1 : 8];

struct PartyPool {
    char pad0[0x138d0];
    PartyRec** recs;                                     // +0x138d0

    PartyRec* get(int handle);                           // 0x48ad00 + record lookup
};

struct GameData {
    char pad0[0x78];
    int main_party;                                      // +0x78
    char pad7c[0x8c - 0x7c];
    int player;                                          // +0x8c
    char pad90[0xf8 - 0x90];
    PartyPool parties;                                   // +0xf8 .. +0x139c c
    char padfc[0x13a00 - 0x139cc];
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
    float speed = (skill(0xb) * g_gk_skill_k + 1.0f);
    if (id == g_game->main_party && g_game->zoom > 0.0f && g_game->f13ad4 != 0) {
        speed = speed + speed;
    } else if (f88 > 0) {
        speed = speed * 0.0f;
    } else if (f8c8 >= 0) {
        PartyRec* pr = g_game->parties.get(f8c8);
        if (pr->f5974 > 0) {
            speed = speed * 0.0f;
        }
    }

    // food / inventory decay over the party's stack slots
    for (int i = 0; i < inv_count; i++) {
        int item = inv[i].item;
        CharRec* ch = &g_game->chars[item & 0xff];
        if (ch->flags & 0x10) {
            int level = ch->f7a & 0xf;
            if (level < 0) {
                level = 0;
            } else if (level > g_clamp_max) {
                level = g_clamp_max;
            }
            float limit = (float)(ch->f58 + level * 2 + 0x23);
            if (!(ch->fa0 < limit)) {
                float add = (float)(rand_value() % 15817) * g_rand_k + 0.5f;
                add = add * 0.5f * speed;
                if (item == g_game->player) {
                    add = add * (float)(3 - g_rand_mod);
                }
                ch->fa0 = ch->fa0 + add;
            }
            if (!(ch->fa0 < limit)) {
                ch->fa0 = limit;
            }
        } else if (inv[i].count > 0) {
            int n = inv[i].count;
            float chance = speed * g_k0_04;
            do {
                if (!(chance <= (float)(rand_value() % 15817) * g_rand_k)) {
                    inv[i].count--;
                }
            } while (--n);
        }
    }

    if (id != g_game->main_party) {
        return;
    }

    // main party: zoom easing, item-grid decay, morale
    if (g_u_a181a8 == 0) {
        if (g_game->zoom > 0.0f && g_game->f13ad4 != 0) {
            zoom = zoom - g_k_zoom_step;
            if (!(zoom < 0.0f)) {
                if (!(zoom < 1.0f)) {
                    zoom = 1.0f;
                }
            } else {
                zoom = 0.0f;
            }
        } else if (g_game->chars->f4ec < 0) {
            zoom = zoom + g_k_zoom_step;
        } else {
            float chance = (float)total_value(0x10) * g_k0_02;
            if (!(zoom < 0.3f)) {
                chance = chance * 3.0f;
            }
            while (!(chance <= (float)(rand_value() % 15817) * g_rand_k)) {
    ItemRef* slot = (ItemRef*)((char*)this + 0xb4);      // the party's item grid lives at +0xb4
                if (slot->index < 0) {
                    break;
                }
                int kind = g_items[slot->index].f210;
                int low = (slot->flags & 0xff) + 1;
                if (kind != 0 && (slot->flags & 0xff000000) == 0x2a000000) {
                    int n = (int)grid_round(kind * g_k1_2 - chance);
                    if (!(0.5f < (float)n)) {
                        n = n + 1;
                    }
                    if (n < 1) {
                        n = 1;
                    }
                    low += n;
                }
                int kept = low;
                if (kept > (slot->flags & 0xff)) {
                    kept = slot->seq();
                }
                slot->flags = (slot->flags & 0xff000000) | (kept & 0xff);
                int cur = slot->index;
                int cap = 0;
                if (cur >= 0) {
                    int k2 = g_items[cur].f210;
                    if (k2 != 0 && (slot->flags & 0xff000000) == 0x2a000000) {
                        int m = (int)grid_round(k2 * g_k1_2 - chance);
                        if (!(0.5f < (float)m)) {
                            m = m + 1;
                        }
                        if (m < 1) {
                            m = 1;
                        }
                        cap = m;
                    }
                }
                if (cap - (slot->flags & 0xff) <= 0) {
                    slot->index = -1;
                }
                chance = chance - 1.0f;
            }
            zoom = zoom - g_k_zoom_step;
            if (!(zoom < 0.3f)) {
                zoom = zoom - 0.3f;
            }
            if (!(zoom < 0.3f)) {
                f7c = f7c - g_k_consumption;
            }
            int item = ((ItemRef*)((char*)this + 0xb4))->index;
            int value = 0;
            if (item >= 0) {
                value = g_items[item].f1fc;
            }
            f7c = (float)value * g_k_item_weight + f7c;
        }
        if (!(zoom < 0.0f)) {
            if (!(zoom < 1.0f)) {
                zoom = 1.0f;
            }
        } else {
            zoom = 0.0f;
        }
        if (!(f7c < 0.0f)) {
            if (!(f7c < 1.0f)) {
                f7c = 1.0f;
            }
        } else {
            f7c = 0.0f;
        }
    }

    // main agent's item grid decay
    CharRec* pc = &g_game->chars[g_game->player];
    Agent* ag = g_game->parties.get(g_game->main_party)->anim;
    float grid_chance = ag->skill(0xb) * g_gk_skill_k2;
    if (!(g_game->zoom > 0.0f)) {
        grid_chance = grid_chance * g_k0_1;
    }
    int n = ag->grid_size();
    for (int i = 0; i < n; i++) {
        if (ag->grid[i].index >= 0
            && g_items[ag->grid[i].index].f1c8 == 1
            && (ag->grid[i].flags & 0xff000000) == 0x1e000000
            && !(grid_chance <= (float)(rand_value() % 15817) * g_rand_k)) {
            ag->grid[i].reduce(0);
        }
    }
}

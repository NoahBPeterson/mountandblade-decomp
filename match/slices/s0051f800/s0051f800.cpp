// Mount & Blade 1.011 (mount&blade.exe) -- slice s0051f800
//   0051f800  GameState::more_start    (finish setting up a freshly started game: per-troop item
//                                       reset/refresh, then the player's inventory bookkeeping)
//   0051ff90  TroopRec::reset_items    (re-initialise the two item-slot arrays of one troop)
//
// Item slots are 8-byte records: {int kind_id; int flags;} where the low byte of flags is the stack
// count and the top byte is a quality/type tag (0x2a = a "nice" item that gets an inventory bonus).
// TroopRec has 0x60 of them at +0xb4 and 10 more (the worn equipment) at +0x4a4.

#include <windows.h>
#include <math.h>

typedef unsigned char byte;

struct tstring { char pad[16]; };

// ---- the item table (0x5e29d0) -----------------------------------------------------------------------
struct ItemRec {                                    // 0x238-byte item kinds, table at 0x5e29d0
    char pad0[0x1c8];
    int  type;                                      // +0x1c8 (low byte = the type code)
    char pad1cc[0x210 - 0x1cc];
    int  f210;                                      // +0x210 (the stack size/capacity of the kind)
};
extern char* g_items;                               // 0x5e29d0

static inline int item_type(int id)
{
    return *(int*)(g_items + id * 0x238 + 0x1c8) & 0xff;
}

// ---- one inventory slot (8 bytes) --------------------------------------------------------------------
struct ItemSlot {
    int  id;                                        // +0 (item kind, -1 = empty)
    union {
        int           flags;                        // +4 (low byte = count, high byte = quality tag)
        unsigned char num;                          // +4
    } v;
    int  free_space();                              // 0x434b70 (room left in the stack)
    int  make();                                    // 0x434df0 (fill a stack, returns the count)
    void take(int n);                               // 0x437510 (ItemStack::reduce)
};

extern float g_inv_gain;                            // 0x590b10 = 0.13f
extern float g_half;                                // 0x549d70 = 0.5f

// The stack capacity of the item in a slot: the kind's stack size plus the "nice item" bonus that a
// quality tag of 0x2a grants (one more unit per 13%, rounded to nearest with ties going down).
static __forceinline int slot_cap(ItemSlot* s)
{
    if (s->id < 0)
        return 0;
    int v = *(int*)(g_items + s->id * 0x238 + 0x210);
    if (v != 0 && (s->v.flags & 0xff000000) == 0x2a000000) {
        float f = (float)v * g_inv_gain;
        double d = floor(f);                        // 0x536b10
        int q = (int)d;
        if (f - d > g_half)
            q++;
        if (q < 1)
            q = 1;
        v += q;
    }
    return v;
}

static inline int slot_count(ItemSlot* s) { return s->v.flags & 0xff; }

// ---- the game state ----------------------------------------------------------------------------------
struct TroopRec {                                   // 0x508 bytes
    char   name[16];                                // +0x00
    char   pad10[0x30 - 0x10];
    int    flags;                                   // +0x30
    char   pad34[0x58 - 0x34];
    int    level_at;                                // +0x58
    char   pad5c[0x68 - 0x5c];
    unsigned n0 : 4;                                // +0x68
    unsigned n1 : 4;
    unsigned n2 : 4;
    unsigned n3 : 4;
    unsigned n4 : 4;
    unsigned n5 : 4;
    unsigned n6 : 4;
    unsigned n7 : 4;
    char   pad6c[0x7a - 0x6c];
    unsigned char rank;                             // +0x7a
    char   pad7b[0x9c - 0x7b];
    int    f9c;                                     // +0x9c
    float  weapon_pts;                              // +0xa0
    int    hp;                                      // +0xa4
    int    hp_max;                                  // +0xa8
    int    xp_next;                                 // +0xac
    int    f_b0;                                    // +0xb0
    ItemSlot inv[0x60];                             // +0xb4 (0xb4 + 0x300 = 0x3b4)
    int    f3b4;                                    // +0x3b4
    char   pad3b8[0x4a4 - 0x3b8];
    ItemSlot worn[10];                              // +0x4a4
    char   pad4f4[0x508 - 0x4f4];
    int    count();                                 // 0x462b60 (slot capacity of this troop)
    void   reset_items(int all);                    // 0x51ff90
    void   apply_items();                           // 0x520110
};

struct GameState {                                  // s00463790
    char  pad0[0x8c];
    int   player;                                   // +0x8c (index of the current agent)
    char  pad90[0x13a00 - 0x90];
    TroopRec* troops;                               // +0x13a00
    int   troop_count;                              // +0x13a04
    void  more_start();                             // 0x51f800
};

// @ 0x51ff90 -----------------------------------------------------------------------------------------
// The original inlines two helpers here (the test and the per-slot body), which is why the slot id and
// the item type are each read twice; keep them separate so the same code shape comes out.
static __forceinline void init_slot(ItemSlot* s)
{
    int id = *(volatile int*)&s->id;
    if (id <= 0)
        return;
    int ty = item_type(id);
    if (ty == 10 || ty == 5 || ty == 6 || ty == 0x10 || ty == 0x11 || ty == 9 || ty == 0x12) {
        s->v.num = 0;
    } else if (ty == 7 || ty == 1 || ty == 0x13) {
        int r = s->make();
        *(short*)&s->v.flags = 0;
        s->v.flags |= r;
    }
}

void TroopRec::reset_items(int all)
{
    ItemSlot* s = inv;
    for (int i = 0; i < 0x60; i++, s++) {
        if (s->id <= 0)
            continue;
        if (all) {
            int ty = item_type(s->id);
            if (!(ty == 5 || ty == 6 || ty == 10 || ty == 7))
                continue;
        }
        init_slot(s);
    }

    s = worn;
    for (int i = 0; i < 10; i++, s++) {
        if (s->id <= 0)
            continue;
        if (all) {
            int ty = item_type(s->id);
            if (!(ty == 5 || ty == 6 || ty == 10 || ty == 7))
                continue;
        }
        init_slot(s);
    }
}

// @ 0x51f800 -----------------------------------------------------------------------------------------
void GameState::more_start()
{
    for (int i = 0; i < troop_count; i++) {
        troops[i].reset_items(i == player);         // 0x51ff90
        troops[i].apply_items();                    // 0x520110
    }

    TroopRec* me = troops + player;
    int pool = 0;
    int n = me->count();                            // 0x462b60

    // collect the room left in the worn equipment
    ItemSlot* s = me->worn;
    for (int i = 0; i < 10; i++, s++) {
        if (s->id > 0) {
            int ty = item_type(s->id);
            if (ty == 0x11 || ty == 0x10)
                pool += slot_count(s) + slot_cap(s) - s->free_space();
        }
    }
    // ... and in the inventory
    if (n > 0) {
        s = me->inv;
        for (int i = 0; i < n; i++, s++) {
            if (s->id > 0) {
                int ty = item_type(s->id);
                if (ty == 0x11 || ty == 0x10)
                    pool += slot_count(s) + slot_cap(s) - s->free_space();
            }
        }
    }

    int given = 0;
    // hand the collected room to the ammo stacks in the inventory ...
    if (n > 0) {
        s = me->inv;
        for (int i = 0; i < n; i++, s++) {
            if (s->id > 0 && item_type(s->id) == 0x12) {
                int need = s->free_space() - slot_count(s);
                if (pool < need)
                    need = pool;
                if (need > 0) {
                    given += need;
                    pool -= need;
                    int v = slot_count(s) + need;
                    if (v > slot_cap(s))
                        v = slot_cap(s);            // the original computes the cap twice
                    s->v.num = 0;
                    s->v.flags |= v;
                    if (s->free_space() - (s->v.flags & 0xff) <= 0)
                        s->id = -1;
                }
            }
        }
    }
    // ... and to those of the worn equipment
    s = me->worn;
    for (int i = 0; i < 10; i++, s++) {
        if (s->id > 0 && item_type(s->id) == 0x12) {
            int need = s->free_space() - slot_count(s);
            if (pool < need)
                need = pool;
            if (need > 0) {
                given += need;
                pool -= need;
                int v = slot_count(s) + need;
                if (v > slot_cap(s))
                    v = slot_cap(s);
                s->v.num = 0;
                s->v.flags |= v;
                if (s->free_space() - (s->v.flags & 0xff) <= 0)
                    s->id = -1;
            }
        }
    }

    // last pass: top the weapon stacks up with whatever is still left over, first the worn ones ...
    s = me->worn;
    for (int i = 0; i < 10; i++, s++) {
        if (s->id > 0) {
            int ty = item_type(s->id);
            if (ty == 0x11 || ty == 0x10) {
                int room = slot_count(s) + slot_cap(s) - s->free_space();
                if (room > 0) {
                    int give = given < room ? given : room;
                    if (give > 0) {
                        s->take(give);                  // 0x437510
                        given -= give;
                    }
                }
            }
        }
    }
    // ... and then the inventory
    if (n > 0) {
        s = me->inv;
        for (int i = 0; i < n; i++, s++) {
            if (s->id > 0) {
                int ty = item_type(s->id);
                if (ty == 0x11 || ty == 0x10) {
                    int free_room = slot_count(s) + slot_cap(s) - s->free_space();
                    if (free_room > 0) {
                        int give = given < free_room ? given : free_room;
                        if (give > 0) {
                            s->take(give);              // 0x437510
                            given -= give;
                        }
                    }
                }
            }
        }
    }
}

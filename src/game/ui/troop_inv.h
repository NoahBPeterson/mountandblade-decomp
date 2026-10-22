// troop_inv.h: classes of troop_inv.cpp (0x462ac0..0x4636d0): the troop record's inventory (90 slots of
// (item, modifier) pairs plus ten equipment slots), the placed-object stream reader and the 0x5374-byte view
// state. Names provisional; offsets cite the function that reads or writes them.
#ifndef GAME_UI_TROOP_INV_H
#define GAME_UI_TROOP_INV_H
#include <stddef.h>
#include <new>             // declares operator new[]/delete[]: without it cl calls operator new/delete for POD arrays
#include "../../rgl/rgl_math.h"
#include "../../rgl/stream.h"
#include "../tstring.h"
#include "./ui_common.h"
#include "./item_ref.h"

// The troop record: a 0x508-byte entry of the table behind GameData+0x13a00 (the same record as TroopRec in
// dialog_manager.h, which only declares name / id / flags; this header declares the inventory part). The slices
// called it `Agent` (0x462b60..0x462da0) and `Inv` (0x462e20..0x463100): both are this record.
struct Troop {
    char pad0[0x30];
    unsigned __int64 flags30;               // +0x30 (bit 0x10: hero / fixed stock)      0x462b60, 0x462c80, 0x462e20
    char pad38[0x50 - 0x38];
    unsigned f50;                           // +0x50 linked / leader troop index (chain end 0)   0x48a080
    char pad54[4];
    int f58[2];                             // +0x58 skill slots                          0x462c00
    char pad60[4];
    unsigned f64;                           // +0x64 (low byte = alpha / colour id)        0x489370
    char pad68[0x80 - 0x68];
    float stat[12];                         // +0x80 attribute accumulators (clamped to 0..1000 by add_stat)   0x462c00, 0x4892d0
    int fb0;                                // +0xb0 troop size   0x48a080, 0x48a290
    ItemRef inv[0x60];                      // +0xb4 inventory (inv_count() slots, at most 0x60)   0x462da0, 0x462e20, 0x463030
    int f3b4;                               // +0x3b4 (clamped at 0 by 0x489340)
    int f3b8;                               // +0x3b8 (packed by 0x489320)
    char pad3bc[0x4a4 - 0x3bc];
    ItemRef equip[10];                      // +0x4a4 equipment slots                      0x4630d0, 0x463100, 0x489c50
    char pad4f4[0x508 - 0x4f4];

    int nibble(int i);                      // 0x4351a0 (packed 4-bit field i of the skill words; not in this module)
    int inv_count();                        // 0x462b60 number of inventory slots: 90, or 30..96 by skill nibble 12 for heroes
    int slot_value(int item);               // 0x462c00
    bool fits_slot(ItemRef* ref, int slot); // 0x462c80
    bool item_matches(int idx, int slot, unsigned need, unsigned forbid, int t1, int t2, int t3);   // 0x462da0
    int pick(int seed, int slot, unsigned need, unsigned forbid, unsigned type, unsigned t2, unsigned t3, char cheapest);   // 0x462e20
    int find_free();                        // 0x463030
    bool add(ItemRef* s);                   // 0x463070
    void set_flags_low(int v);              // 0x4892a0 (replaces the low nibble of flags30)
    void add_stat(unsigned i, int amt);     // 0x4892d0
    void set_colors(int a, int b);          // 0x489320
    void sub_3b4(int amt);                  // 0x489340
    void set_alpha(int a);                  // 0x489370
    void unequip(int i);                    // 0x4630d0
    bool equip_best(int seed, int slot, unsigned need, unsigned forbid, unsigned type, unsigned t2, unsigned t3, char cheapest);   // 0x463100
};

// A 0x78-byte record of the sprite table (g_tbl78 0x5e29dc, count 0x5e29e0)
struct Rec78 {
    tstring name;                           // +0     0x4631c0
    char pad10[4];
    int f14;                                // +0x14  0x4631c0
    char pad18[0x78 - 0x18];
};
extern Rec78* g_tbl78;                      // 0x5e29dc
extern int g_tbl78_count;                   // 0x5e29e0
int find_in_table78(const tstring* name);   // 0x4352a0 (cdecl)

stream_base& r_rotation(stream_base& s, rgl::matrix* m);    // 0x415e70 (cdecl)

// A placed object (sprite-like) read from a stream. Offsets from the reader 0x4631c0.
struct Placed {
    char pad0[0x18];
    rgl::matrix m18;                        // +0x18 (origin at +0x48)
    char pad58[0x98 - 0x58];
    rgl::matrix m98;                        // +0x98 (copy of m18)
    char padd8[0x158 - 0xd8];
    float f158, f15c, f160;                 // +0x158
    char pad164[0x188 - 0x164];
    int f188, f18c;                         // +0x188 / +0x18c
    tstring name;                           // +0x190
    int f1a0, f1a4;                         // +0x1a0 / +0x1a4
    char pad1a8[0x1bc - 0x1a8];
    float f1bc;                             // +0x1bc
    void read(stream_base* s, int version); // 0x4631c0
};

// 0x5374-byte object (a camera / view-state block; fields reached by offset in 0x463370, 0x463610)
struct ViewState {
    void init();                            // 0x463370
    rgl::vector4 get_dir(int a, int b);     // 0x463610
};

#endif // GAME_UI_TROOP_INV_H

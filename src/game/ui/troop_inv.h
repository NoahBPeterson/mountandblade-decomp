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

// The troop record: a 0x508-byte entry of the table behind GameData+0x13a00 (Troop in src/include/troop.h, one definition for
// src/game and src/rgl). The slices called it `Agent` (0x462b60..0x462da0) and `Inv` (0x462e20..0x463100): both are this record.
#include "../../include/troop.h"

// A 0x78-byte record of the sprite table (g_tbl78 0x5e29dc, count 0x5e29e0): src/include/rec78.h
#include "../../include/rec78.h"
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

#include "../view_state.h"            // ViewState: the 0x5374-byte object (a camera / view-state block; fields reached by offset in 0x463370, 0x463610)

#endif // GAME_UI_TROOP_INV_H

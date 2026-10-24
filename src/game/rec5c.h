// rec5c.h: the 0x5c-byte party-track record (vector at Game+0x13a60, first/last at +0x13a64/+0x13a68). ONE definition for
// inventory_panel/game_tick (the tick that ages the ribbons) and char_sheet/save_state (the serialiser Rec5c::write 0x4e9460,
// which stores f0, f4, f8, f10, f14 with w_int and the flags word with w_raw). The two views named the same dwords as floats
// (x, y, z, f14) or ints (f0, f4, f8, f14); they overlay here in anonymous unions of plain members.
#ifndef MB_GAME_REC5C_H
#define MB_GAME_REC5C_H
#include "out_stream.h"

struct Rec5c {
    union { float x; int f0; };         // +0x00
    union { float y; int f4; };         // +0x04
    union { float z; int f8; };         // +0x08
    char pad0c[4];
    int f10;                            // +0x10 (0x4e9460 w_int)
    union {
        float f14;                      // +0x14 (age / timer)
        int i14;                        // +0x14 as an int (0x4e9460)
    };
    union {
        unsigned int flags;             // +0x18 (0x20000000 visible, 0x10000000 battle track)
        struct {
            unsigned short w18;         // +0x18
            unsigned char f1a;          // +0x1a
            unsigned char b1b;          // +0x1b
        };
    };
    char pad1c[0x5c - 0x1c];
    __declspec(noinline) void write(OutStream* s);  // 0x4e9460 (ret 4)
};

#endif // MB_GAME_REC5C_H

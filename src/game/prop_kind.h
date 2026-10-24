// prop_kind.h: the prop kind record (0x78 bytes, array at 0x5e29dc, count 0x5e29e0): per kind the flags, the bounding extent used
// by the placement passes and the sound set. ONE definition for src/game: terrain_b (spawn rules 0x472050, 0x473a60) read the
// flag byte at +0x11, mesh_slot/mesh_start.cpp (Mission::start_setup 0x484590: prop bounding box and placed-prop jitter) the
// flag word and the extent. No includes.
#ifndef GAME_PROP_KIND_H
#define GAME_PROP_KIND_H

struct PropKind {                               // 0x78 bytes (array at 0x5e29dc)
    char pad0[0x10];
    union {
        unsigned flags;                         // +0x10 (0x1000: placed props are jittered and snapped to the ground; 0x8000: agent4 swing_trace)
        struct {
            unsigned char flags_lo;             // +0x10
            unsigned char flags11;              // +0x11 (terrain_b 0x472050: bit 0x40 is the high flag byte test)
        };
    };
    char pad14[0x58 - 0x14];
    float f58, f5c, f60;                        // +0x58 extent vector (start_setup: dotted with the prop frame rows)
    char pad64[4];
    float f68;                                  // +0x68 extent radius (start_setup: grows the box by it)
    char pad6c[0x78 - 0x6c];                    // +0x6c the sound set (agent4 swing_trace)
};

extern PropKind* g_propkinds; // 0x5e29dc
#endif // GAME_PROP_KIND_H

// rec78.h: Rec78 (also named PropKind), the 0x78-byte prop kind record of the table g_propkinds (0x5e29dc, count 0x5e29e0;
// find_in_table78 0x4352a0): per kind the name, the flags, the bounding extent used by the placement passes and the sound set.
// ONE definition for src/game (agent.cpp: name lookup 0x4352a0; troop_inv.cpp: name and f14 read by 0x4631c0; terrain_b spawn rules
// 0x472050, 0x473a60: flag byte +0x11; mesh_slot/mesh_start.cpp Mission::start_setup 0x484590: flag word and extent; agent4
// swing_trace: sound set) and src/rgl/mesh2 (gui_edit.cpp: the array destructor 0x4c7ee0 destroys the tail at +0x44 through its own Rec78Tail).
#ifndef MB_REC78_H
#define MB_REC78_H
#include "../game/tstring.h"

struct OutStream;                           // char_sheet/save_state.h: the save-game writer

struct Rec78 {
    tstring name;                           // +0x00 (0x4352a0, 0x4631c0; destroyed by 0x4c7ee0)
    union {
        unsigned flags;                     // +0x10 (0x1000: placed props are jittered and snapped to the ground; 0x8000: agent4 swing_trace)
        struct {
            unsigned char flags_lo;         // +0x10
            unsigned char flags11;          // +0x11 (terrain_b 0x472050: bit 0x40 is the high flag byte test)
        };
    };
    int f14;                                // +0x14 (0x4631c0)
    char pad18[0x44 - 0x18];
    union {
        char tail[0x78 - 0x44];             // +0x44 the sub-object with the inline destructor (Rec78Tail in rgl/mesh2/gui_edit.h; 0x4c7ee0)
        struct {
            char pad44[0x50 - 0x44];
            int f50;                        // +0x50 (char_sheet/save_state.cpp 0x4e9110 writes it)
            char pad54[4];
            float f58, f5c;                 // +0x58 extent vector (start_setup: dotted with the prop frame rows)
            union {
                int f60;                    // +0x60 (0x4e9110)
                float fx60;                 // +0x60 extent vector, z
            };
            char pad64[4];
            float f68;                      // +0x68 extent radius (start_setup: grows the box by it)
            char pad6c[4];                  // +0x6c the sound set (agent4 swing_trace)
            int f70;                        // +0x70 (0x4e9110)
            char pad74[4];
        };
    };
    __declspec(noinline) void write(OutStream* out);        // 0x4e9110 (ret 4; char_sheet/save_state.cpp: the bytes from +0x0c, then f50, f60, f70)
};
typedef Rec78 PropKind;

extern Rec78* g_propkinds;                  // 0x5e29dc

#endif // MB_REC78_H

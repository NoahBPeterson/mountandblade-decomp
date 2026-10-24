// rec78.h: Rec78, the 0x78-byte named record of the table g_tbl78 (0x5e29dc, count 0x5e29e0; find_in_table78 0x4352a0).
// ONE definition for src/game (agent.cpp: name lookup 0x4352a0; troop_inv.cpp: name and f14 read by 0x4631c0) and
// src/rgl/mesh2 (gui_edit.cpp: the array destructor 0x4c7ee0 destroys the tail at +0x44 through its own Rec78Tail).
#ifndef MB_REC78_H
#define MB_REC78_H
#include "../game/tstring.h"

struct OutStream;                           // char_sheet/save_state.h: the save-game writer

struct Rec78 {
    tstring name;                           // +0x00 (0x4352a0, 0x4631c0; destroyed by 0x4c7ee0)
    char pad10[4];
    int f14;                                // +0x14 (0x4631c0)
    char pad18[0x44 - 0x18];
    union {
        char tail[0x78 - 0x44];             // +0x44 the sub-object with the inline destructor (Rec78Tail in rgl/mesh2/gui_edit.h; 0x4c7ee0)
        struct {
            char pad44[0x50 - 0x44];
            int f50;                        // +0x50 (char_sheet/save_state.cpp 0x4e9110 writes it)
            char pad54[0x60 - 0x54];
            int f60;                        // +0x60 (0x4e9110)
            char pad64[0x70 - 0x64];
            int f70;                        // +0x70 (0x4e9110)
            char pad74[4];
        };
    };
    __declspec(noinline) void write(OutStream* out);        // 0x4e9110 (ret 4; char_sheet/save_state.cpp: the bytes from +0x0c, then f50, f60, f70)
};

#endif // MB_REC78_H

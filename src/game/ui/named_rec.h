// named_rec.h: NamedRec, the 0x8-word record with three names (the main-menu / mission descriptor): setup 0x48a3a0,
// setup2 0x48a3d0. One definition for ui/world.h and inventory_panel/mission_ctx.cpp (setup called on g_main_menu.f0).
#ifndef GAME_UI_NAMED_REC_H
#define GAME_UI_NAMED_REC_H
#include "../tstring.h"

// two names and counters (0x48a3a0, 0x48a3d0)
struct NamedRec {
    int f00, f04;
    char pad08[0x1c - 0x08];
    tstring name1c;                         // +0x1c
    int f2c, f30, f34, f38, f3c, f40, f44, f48;
    int f4c;                                // +0x4c
    int f50;                                // +0x50
    char pad54[0x68 - 0x54];
    tstring name68;                         // +0x68
    tstring name78;                         // +0x78
    tstring name88;                         // +0x88
    void setup(const tstring* a, const tstring* b);                                  // 0x48a3a0 (ret 8)
    void setup2(const tstring* a, const tstring* b, const tstring* c, int v);       // 0x48a3d0
};
#endif // GAME_UI_NAMED_REC_H

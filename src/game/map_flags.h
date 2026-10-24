// map_flags.h: the commit-state block at 0x5e291c (flags f0..f2, two ints), committed by 0x513710 (thiscall, ret 4).
// ONE definition for src/game/party (game.cpp: LoadingPanel::sub 0x51c720 fills it before the commit) and
// src/game/inventory_panel (inv_common.h): both name the same global 0x5e291c and the same member 0x513710.
#ifndef GAME_MAP_FLAGS_H
#define GAME_MAP_FLAGS_H

struct MapFlags {                                           // 0x5e291c
    char f0, f1, f2;                                        // 0x5e291c, 0x5e291d, 0x5e291e
    char pad3;
    int f4, f8;                                             // 0x5e2920, 0x5e2924
    void commit(char* p);                                   // 0x513710 (ret 4)
};
extern MapFlags g_map_flags;                                // 0x5e291c

#endif // GAME_MAP_FLAGS_H

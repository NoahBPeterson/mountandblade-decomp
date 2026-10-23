// game_data.h: the game-data object (global pointer g_game, 0x5e2930) as far as terrain_b.cpp / mission.cpp read
// it: the party manager at +0xf8, the troop table (+0x13a00), the scene-object / level table (+0x13a50), the tick
// index of the mission clock and two flags. Offsets are cited at each field. Names provisional; src/game/ui,
// src/game/agent and src/game/agent3 declare their own partial views of the same object.
#ifndef TERRAIN_B_GAME_DATA_H
#define TERRAIN_B_GAME_DATA_H
#include <vector>
#include "../tstring.h"

struct Troop {                                  // 0x508 bytes, table at game+0x13a00
    char pad0[0x30];
    __int64 flags;                              // +0x30 (0x10 = hero, 0x1000000 / 0x4000000 = troop class bits) (0x473a60)
    char pad38[0xa0 - 0x38];
    float hp;                                   // +0xa0
    char pada4[0x3b8 - 0xa4];
    unsigned f3b8;                              // +0x3b8 (low 16 = party/side id, bits 16..23 = rule id)
    int used;                                   // +0x3bc
    char pad3c0[0x4e4 - 0x3c0];
    int f4e4;                                   // +0x4e4
    char pad4e8[0x508 - 0x4e8];
};
struct Stack {                                  // 0x20 bytes, array at party+0xa8
    int count;                                  // +0x00 total
    int troop;                                  // +0x04
    int flags;                                  // +0x08 (bit 0 = hero)
    int spawned;                                // +0x0c
    int wounded;                                // +0x10
    int pad14[3];
};
struct Party {                                  // 0x5ac8 bytes
    char pad0[0xa8];
    Stack stacks[64];                           // +0xa8
    int nstacks;                                // +0x8a8
    char pad8ac[0x5ac8 - 0x8ac];
    int sum_stat(int what);                     // 0x4b1cf0 (thiscall, ret 4)
};
struct PartyMgr {                               // embedded in the game data at +0xf8
    char pad0[0x138cc];
    std::vector<Party*> pages;                  // +0x138cc (first at +0x138d0; index >> 4 -> page of 16)
    int index_of(int id);                       // 0x48ad00 (thiscall, ret 4)
};
// The 0x494-byte level / scene-object record (table at game+0x13a50).
struct Level494 {
    char pad0[0x30];
    unsigned flags;                             // +0x30 (0x100: flat level, 0x1: no fog sheet) (0x479e20, 0x47a0c0, 0x4792d0)
    tstring name34;                             // +0x34 (0x47a800: the prefix find_meshes searches)
    tstring name44;                             // +0x44 (0x479e20: the body resource looked up with get_object60)
    char pad54[0x114 - 0x54];
    struct { int a, b; } pairs[1];              // +0x114 spawn pairs (0x473a60)
    char pad11c[0x494 - 0x11c];
};
struct GameData {
    char pad0[4];
    int f4;                                     // +0x04 (0x473a60)
    char pad8[0x78 - 8];
    int f78;                                    // +0x78 (default party)
    char pad7c[0x8c - 0x7c];
    int player;                                 // +0x8c (the player's troop id)
    char pad90[0xf8 - 0x90];
    PartyMgr parties;                           // +0xf8
    char pad_after_parties[0x13a00 - 0xf8 - sizeof(PartyMgr)];
    Troop* troops;                              // +0x13a00
    int ntroops;                                // +0x13a04
    char pad13a08[0x13a50 - 0x13a08];
    Level494* levels;                           // +0x13a50
    char pad13a54[0x13b18 - 0x13a54];
    int f13b18;                                 // +0x13b18 tick index (8-byte entries) (0x4792d0)
    char pad13b1c[4];
    unsigned f13b20;                            // +0x13b20 low word of the mission tick clock
    int f13b24;                                 // +0x13b24 high word
    char pad13b28[0x13b64 - 0x13b28];
    float val[4][4];                            // +0x13b64 four wind anchors: four weights / values each (0x471040)
    float pos[4][2];                            // +0x13ba4 their xy positions
    char pad13bc4[0x1405c - 0x13bc4];
    int tbl[64][32];                            // +0x1405c (0x473a60)
    char pad1505c[0x1a090 - (0x1405c + 64 * 32 * 4)];
    int f1a090;                                 // +0x1a090 (0x472050)
    char pad1a094[0x1a20c - 0x1a094];
    int f1a20c;                                 // +0x1a20c
    bool agent_low(int idx);                    // 0x4638f0 (thiscall, ret 4)
    void blend(float* out, float x, float y);   // 0x471040 (the slice's Anchors::blend: wind / sky blend over the anchors)
};
extern GameData* g_game;                        // 0x5e2930

#endif // TERRAIN_B_GAME_DATA_H

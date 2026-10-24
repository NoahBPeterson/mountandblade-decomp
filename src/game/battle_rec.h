// battle_rec.h: the table of battle records between two parties (GameData +0xcc, pages of 16 at +0xe4) and its 0x98-byte
// records. ONE definition for src/game/party2 (party_2.cpp, camera.cpp) and src/game/script (the party_*battle* operations).
// Proof that the two views are the same objects: both reach the table at game +0xcc and call BattleTable::get 0x48ef20
// (thiscall, ret 4); the script's record view has the two sides at +0x54 / +0x58 (party_get_battle_opponent 0x4a867b) as
// party_2's does (0x4b6180, 0x4b6b40) and the "next simulation time" float at +0x70, which party_2 names `next_simulation_time`
// too (it stores 1.0 in 0x4b2770 and tests / re-arms it in 0x4b71e0).
#ifndef GAME_BATTLE_REC_H
#define GAME_BATTLE_REC_H
#include <vector>
#include "../include/timer.h"

struct face;

// the scene object a battle record owns (an rgl entity: released through the scene's object table when the record is detached)
struct BattleObj {
    char pad0[8];
    int id;                                             // +0x08   0x4b6180
    char padc[0x88 - 0xc];
    float pos[4];                                       // +0x88 position (sound source)   0x4b71e0
    char pad98[0x11c - 0x98];
    int* begin;                                         // +0x11c child meshes   0x4b71e0
    int* end;                                           // +0x120
    char pad124[0x140 - 0x124];
    char visible;                                       // +0x140   0x4b71e0
};
// a 0x98-byte battle record between two parties: pages of 16 at GameData +0xe4 (battle table at +0xcc, get 0x48ef20)
struct BattleRec {
    char pad0[4];
    int live;                                           // +0x04 cleared when the record is released   0x4b6180
    int rec_id;                                         // +0x08 the record's own index   0x4b6180
    char padc[0x10 - 0xc];
    int type;                                           // +0x10 1 = live, 2 = ship battle   0x49bbb0, 0x4b2770, 0x4b6180
    char pad14[0x24 - 0x14];
    float x1, y1;                                       // +0x24 first position   0x4b2770, 0x49bbb0
    float x2, y2;                                       // +0x2c second position  0x4b2770
    char pad34[0x3c - 0x34];
    float z;                                            // +0x3c height over the face   0x4b2770
    char pad40[0x44 - 0x40];
    float opacity;                                      // +0x44 -10000.0 when bound (0x4b2770), then the icon opacity   0x4b71e0
    face* face1;                                        // +0x48 face under the first position   0x4b2770
    face* face2;                                        // +0x4c face under the second position   0x4b2770
    BattleObj* obj;                                     // +0x50 scene object   0x4b6180, 0x49bbb0
    int party_a;                                        // +0x54 leader party (0x4b6180), first side   0x4b6b40; party_get_battle_opponent 0x4a867b
    int party_b;                                        // +0x58 other side   0x4b6180; 0x4a867b
    char pad5c[0x60 - 0x5c];
    Timer born;                                         // +0x60 clock and stamp (+0x68; bind_host re-stamps it)   0x4b2770, 0x4b71e0
    float next_simulation_time;                         // +0x70 hours / seconds until the next commit   0x4b71e0 (1.0 from 0x4b2770), 0x4a8181 (party_set_next_battle_simulation_time)
    float f74;                                          // +0x74 0x4b2770
    int height;                                         // +0x78 summed heights of the two parties   0x49e9c0, 0x4b6180
    char pad7c[4];
    Timer anim;                                         // +0x80 sound clock (+0x88 start)   0x4b71e0
    int sound;                                          // +0x90 playing sound, -1 = none   0x4b2770, 0x4b71e0
    char pad94[4];
    void bind_host();                                   // 0x4b2770
    void detach();                                      // 0x4b6180
    void commit();                                      // 0x4b6870
    void update_nav_anim(int unused);                   // 0x4b71e0
};
// the table of battle records (GameData +0xcc; pages of 16 at +0xe4)
struct BattleTable {
    int count;                                          // +0x00
    std::vector<int> free_ids;                          // +0x04 (allocator word, first +8, last +0xc, end +0x10) released record ids   0x4b6180
    char pad14[4];
    BattleRec** pages;                                  // +0x18 (GameData +0xe4)   0x4b3910, 0x4b3bc0, 0x4b80d0
    char pad1c[0x2c - 0x1c];
    BattleRec* get(unsigned id);                        // 0x48ef20 (thiscall, ret 4)
};

#endif // GAME_BATTLE_REC_H

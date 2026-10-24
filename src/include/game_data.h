// game_data.h: the game-data object (GameData, global pointer g_game at 0x5e2930) as far as agent2, agent3, dialog_manager,
// terrain_b, game_util and gui_edit read it. ONE definition for those modules (src/game and src/rgl). The other headers
// that see the same object under another name (World in ui/world.h, Game in party/game.h, GameConfig in config/config.h)
// declare further fields; they cannot be included together with this one because each declares its own g_game.
// Offsets cite the function that reads them. Names that two modules gave to one offset overlay in anonymous unions of
// plain members (f78 / party_id_78, player / player_troop, troops / chars, ntroops / n_chars / f13a04,
// n_factions / f13a4c, levels / slots).
#ifndef MB_GAME_DATA_H
#define MB_GAME_DATA_H
#include "../game/tstring.h"
#include "./troop.h"
#include "./party.h"
#include "./game_recs.h"

// The 0x494-byte level / scene-object record (table at game+0x13a50).
struct Level494 {
    char pad0[0x30];
    union {
        unsigned flags;                         // +0x30 (0x100: flat level, 0x1: no fog sheet) (0x479e20, 0x47a0c0, 0x4792d0)
        unsigned char flags8;                   // +0x30 as a byte (0x4435c0)
    };
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
    union {
        int f78;                                // +0x78 (default party)   0x4428c0
        int party_id_78;
    };
    char pad7c[0x8c - 0x7c];
    union {
        int player;                             // +0x8c (the player's troop id)   0x460160, 0x460640
        int player_troop;
    };
    char pad90[0xb0 - 0x90];
    int fb0;                                    // +0xb0   0x4610b0
    char padb4[0xc8 - 0xb4];
    int fc8;                                    // +0xc8   0x460620, 0x460d80
    char padcc[0xf8 - 0xcc];
    PartyMgr parties;                           // +0xf8 .. +0x139d8 (gui_edit.cpp called it `pm`)
    char pad139d8[0x139e4 - 0x139d8];
    int f139e4;                                 // +0x139e4 (0x443aa0)
    char pad139e8[0x13a00 - 0x139e8];
    union {
        Troop* troops;                          // +0x13a00 (stride 0x508)
        TroopRec* chars;                        // +0x13a00   0x460160, 0x4428c0
    };
    union {
        int ntroops;                            // +0x13a04
        int n_chars;
        int f13a04;                             // (repeat count, mode 0x3000)
    };
    char pad13a08[0x13a1c - 0x13a08];
    int f13a1c;                                 // +0x13a1c current repeat index   0x460d80, 0x460de0
    char pad13a20[0x13a34 - 0x13a20];
    int f13a34;                                 // +0x13a34   0x4428c0
    char pad13a38[0x13a48 - 0x13a38];
    FactionRec* factions;                       // +0x13a48   0x4c8ca0
    union {
        int n_factions;                         // +0x13a4c
        int f13a4c;                             // (repeat count, mode 0x1000)
    };
    union {
        Level494* levels;                       // +0x13a50 (the scene / level table)
        Level494* slots;
    };
    char pad13a54[4];
    QuestRec* quests;                           // +0x13a58   0x4c8ca0
    int n_quests;                               // +0x13a5c
    char pad13a60[0x13b18 - 0x13a60];
    int f13b18;                                 // +0x13b18 tick index (8-byte entries) (0x4792d0)
    char pad13b1c[4];
    unsigned f13b20;                            // +0x13b20 low word of the mission tick clock
    int f13b24;                                 // +0x13b24 high word
    char pad13b28[0x13b64 - 0x13b28];
    float val[4][4];                            // +0x13b64 four wind anchors: four weights / values each (0x471040)
    float pos[4][2];                            // +0x13ba4 their xy positions
    char pad13bc4[0x1405c - 0x13bc4];
    int tbl[64][32];                            // +0x1405c (0x473a60)
    char pad1605c[0x1a090 - (0x1405c + 64 * 32 * 4)];
    int f1a090;                                 // +0x1a090 (0x472050)
    char pad1a094[0x1a20c - 0x1a094];
    int f1a20c;                                 // +0x1a20c
    char pad1a210[0x1a23c - 0x1a210];
    int f1a23c;                                 // +0x1a23c   0x4610b0

    int skill_level(int troop, int skill);      // 0x437b20 (thiscall, ret 8)
    int skill(int troop, int id);               // 0x437b20 (thiscall, ret 8) (0x4406a0)
    char emit_event(int id, int a, int b);      // 0x435790 (ret 0xc)
    void add_experience(int troop, int amount); // 0x4428c0 (ret 8) (agent_fx_sound.cpp)
    bool agent_low(int idx);                    // 0x4638f0 (thiscall, ret 4)
    void blend(float* out, float x, float y);   // 0x471040 (wind / sky blend over the anchors)
    void save_language_files(bool translated);  // 0x4c8ca0
};
extern GameData* g_game;                        // 0x5e2930

#endif // MB_GAME_DATA_H

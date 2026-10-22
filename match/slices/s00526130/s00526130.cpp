// Slice s00526130: the dialogue-listener agent lookup (0x526130).  It scans the mission's agent table for
// the used agent whose troop id (+0x88) equals DialogMgr::listener and returns the last match, or -1.
// Names provisional; every callee/global carries its VA.
#include <windows.h>

struct Agent {                                      // 0x5928 bytes
    char pad0[4];
    int used;                                       // +0x04
    char pad8[0x10 - 8];
    int f10;                                        // +0x10
    char pad14[0x2c - 0x14];
    int f2c;                                        // +0x2c
    char pad30[0x88 - 0x30];
    int troop;                                      // +0x88 (troop / character id)
    char pad8c[0x5928 - 0x8c];
};
struct AgentTable {                                 // mission+4
    int count;                                      // +0x00
    char pad4[0x14];
    Agent** pages;                                  // +0x18 (mission+0x1c)
    int first_used();                               // 0x43b1b0
    void next_used(unsigned* i);                    // 0x43b160
};
struct Mission {
    char pad0[4];
    AgentTable agents;                              // +0x04
};
extern Mission* g_mission;                          // 0x5e292c

struct TroopRec {                                   // 0x508 bytes
    char pad0[0x30];
    unsigned flags;                                 // +0x30 (0x10 = a talking troop?)
    char pad34[0x508 - 0x34];
};
struct Game {
    char pad0[0xc8];
    int player;                                     // +0xc8
    char padcc[0x13a00 - 0xcc];
    TroopRec* chars;                                // +0x13a00
};
extern Game* g_game;                                // 0x5e2930
extern int g_player_agent;                          // 0x8ed50dc (-1 when none)

struct DialogMgr {
    char pad0[0x8000];
    int listener();                                 // 0x460670
};
extern DialogMgr g_n_dialogs;                       // 0x8ed50c8

// @ 0x526130 -------------------------------------------------------------------------------------------
int find_listener_agent()
{
    int found = -1;
    int listener = g_n_dialogs.listener();
    if ((g_game->chars[listener].flags & 0x10) | (g_player_agent == -1)) {
        AgentTable* t = &g_mission->agents;
        unsigned i = t->first_used();
        Agent** pages = t->pages;
        while (i < (unsigned)t->count) {
            if ((pages[i >> 4] + (i & 0xf))->troop == listener)
                found = i;
            t->next_used(&i);
        }
    }
    return found;
}

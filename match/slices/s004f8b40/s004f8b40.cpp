// Slice s004f8b40: mission helpers that refresh the selected agent and toggle the mesh visibility of its
// held slots. 0x4f8b40 updates the selected agent's equipment display, then (re)reads the game's flag at
// +0x139e4 and either shows (0x4f8c20) or hides (0x4f8bc0) the five item meshes by writing the vis mask at
// mesh+0xcc (0xffff = hidden, 0x2000 = shown).
// Agents live in the mission's page table (id >> 4 pages of 16 agents, 0x5928 bytes each); the two helpers
// take the agent in ecx. 0x4f8b40 also works in registers only (no frame) as the original does. Field layout
// follows s0044bc30 / s00448eb0.
#include <stddef.h>
#include "rgl.h"

struct mesh {
    char pad0[0xcc];
    unsigned vis;                               // +0xcc (mesh visibility mask)
};

struct Agent;                                   // 0x5928 bytes

struct Mission {                                // 0x5e292c
    char pad0[0x1c];
    Agent** pages;                              // +0x1c
    char pad20[0x5c - 0x20];
    int player_agent;                           // +0x5c
    Agent* agent(int id) {
        Agent** page = &pages[(unsigned)id >> 4];
        return (Agent*)((char*)*page + (id & 0xf) * 0x5928);
    }
};
extern Mission* g_mission;                      // 0x5e292c

struct GameData {
    char pad0[0x139e4];
    int flag_139e4;                             // +0x139e4 (equipment display toggle)
};
extern GameData* g_game;                        // 0x5e2930

struct Agent {                                  // 0x5928 bytes
    void update_display();                      // @ 0x443aa0 (Agent::update_equipment_display)

    char pad0[0x13c];
    mesh* slot_m13c;                            // +0x13c
    mesh* slot_m140;                            // +0x140
    mesh* slot_m144;                            // +0x144
    mesh* slot_m148;                            // +0x148
    char pad14c[0x17c - 0x14c];
    mesh* slot_m17c;                            // +0x17c

    void hide();                                // @ 0x4f8bc0
    void show();                                // @ 0x4f8c20
};

// @ 0x4f8b40
void __fastcall update_selected_agent_equipment()
{
    if (g_mission->player_agent < 0)
        return;
    g_mission->agent(g_mission->player_agent)->update_display();
    if (g_game->flag_139e4 != 0)
        g_mission->agent(g_mission->player_agent)->show();
    else
        g_mission->agent(g_mission->player_agent)->hide();
}

// @ 0x4f8bc0
void Agent::hide()
{
    if (slot_m13c) slot_m13c->vis = 0xffff;
    if (slot_m140) slot_m140->vis = 0xffff;
    if (slot_m144) slot_m144->vis = 0xffff;
    if (slot_m148) slot_m148->vis = 0xffff;
    if (slot_m17c) slot_m17c->vis = 0xffff;
}

// @ 0x4f8c20
void Agent::show()
{
    if (slot_m13c) slot_m13c->vis = 0x2000;
    if (slot_m140) slot_m140->vis = 0x2000;
    if (slot_m144) slot_m144->vis = 0x2000;
    if (slot_m148) slot_m148->vis = 0x2000;
    if (slot_m17c) slot_m17c->vis = 0x2000;
}
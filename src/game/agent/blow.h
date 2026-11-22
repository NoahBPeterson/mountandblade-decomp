// blow.h: the blow record (0x58 bytes), ONE definition for src/game/agent4 (Agent::reduce_blow 0x454e30, note_blow 0x457af0,
// swing_trace 0x45a170, the Agent +0x563c last-blow copy) and src/game/script (agent_deliver_damage_to_agent). The initialiser
// 0x4353a0 (returns this) is the same function in both (game/agent/agent_queries.cpp defines it; game/terrain/agent_rec.h embeds the record at agent
// +0x563c); mission5 keeps Blow_s284a0 (its own ctor view).
// Names that two modules gave to one offset: f38 / bone (+0x38), damage / dmg_in (+0x3c, the script writes the base damage
// there; agent_logic reads it as the damage before armour), f40 / dmg_out (+0x40), f44 / absorbed, f48 / kind, f4c / item,
// f50 / item_mod, f54 / missile / hit.
#ifndef GAME_BLOW_H
#define GAME_BLOW_H
#include "engine/core/rgl_base.h"

struct HitInfo {                                    // what Blow::hit points to (0x4543a0 family)
    char pad0[0x20];
    rgl::vector4 p;                                 // +0x20 the hit point
    char pad30[0x40 - 0x30];
    rgl::vector4 d;                                 // +0x40 the hit direction
};
struct Blow {
    unsigned flags;                                 // +0 (m0; 0x20 set by agent_deliver_damage_to_agent)
    unsigned attacker;                              // +4 (m4)
    rgl::vector4 pos;                               // +8
    rgl::vector4 dir;                               // +0x18
    rgl::vector4 dir2;                              // +0x28
    int bone;                                       // +0x38 hit location / damage class
    float dmg_in;                                   // +0x3c
    float dmg_out;                                  // +0x40
    int absorbed;                                   // +0x44
    int kind;                                       // +0x48 (0 cut, 1 pierce, 2 blunt)
    int item;                                       // +0x4c
    unsigned item_mod;                              // +0x50
    union {
        struct Missile* missile;                    // +0x54
        HitInfo* hit;
    };
    Blow* init();                                   // 0x4353a0 (returns this)
};

// the blow member of the agent (Agent::blow +0x563c): its constructor is the initialiser 0x4353a0
struct BlowInit : Blow {
    BlowInit() { init(); }
    BlowInit& operator=(const Blow& b) { Blow::operator=(b); return *this; }
};

#endif // GAME_BLOW_H

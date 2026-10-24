// blow.h: the blow record (0x58 bytes), ONE definition for src/game/agent4 (Agent::reduce_blow 0x454e30, note_blow 0x457af0,
// swing_trace 0x45a170, the Agent +0x563c last-blow copy) and src/game/script (agent_deliver_damage_to_agent). The initialiser
// 0x4353a0 (returns this) is the same function in both: agent/agent.h declares the record a third time as RecZSubB (fields
// m0..m54, bound by the agent.manifest.txt rows by that name), mission5 as Blow_s284a0 (its own ctor view).
// Names that two modules gave to one offset: f38 / bone (+0x38), damage / dmg_in (+0x3c, the script writes the base damage
// there; agent_logic reads it as the damage before armour), f40 / dmg_out (+0x40), f44 / absorbed, f48 / kind, f4c / item,
// f50 / item_mod, f54 / missile / hit.
#ifndef GAME_BLOW_H
#define GAME_BLOW_H
#include "../include/rgl_base.h"

struct HitInfo {                                    // what Blow::hit points to (0x4543a0 family)
    char pad0[0x20];
    float px, py, pz;                               // +0x20
    char pad2c[0x40 - 0x2c];
    float dx, dy, dz;                               // +0x40
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

#endif // GAME_BLOW_H

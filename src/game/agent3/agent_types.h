// agent_types.h: the classes shared by agent_combat.cpp (0x43e190..0x441360) and agent_fx_sound.cpp
// (0x441ff0..0x444c28): the mission agent (Agent, 0x5928 bytes), its body / skeleton / animation slots, the
// mission object (Mission, AgentTable), the scene (world) and the small helpers both modules call. ONE definition
// of each class for this directory; every offset is read or written by a function of these two modules (the
// address is cited at the field). Names provisional.
//
// The classes shared with the other agent modules are one definition each in src/game: Agent / AgentBody / AgentTable /
// Threat (agent_obj.h), ItemRef (item_ref.h), Mission (mission.h), IfaceB (src/include/iface.h); this header includes them.
#ifndef AGENT3_TYPES_H
#define AGENT3_TYPES_H
#include <stddef.h>
#include <vector>
#include "rgl.h"
#include "../tstring.h"
#include "../agent/item_kind.h"       // ItemKind, g_items, pos2
#include "../mat4.h"                   // rgl::mat4
#include "../../include/timer.h"

using rgl::vector4;
using rgl::matrix;

// posm (rgl::mat4 + the rotation 0x40e570) is in src/game/agent_obj.h

// ---- collision shapes (vtable 0x54a4b8 shape_base, 0x54a540 capsule): src/include/shape.h ---------------------------
#include "../../include/shape.h"
extern shape_base* g_hit_shape;                         // 0x5e246c: last shape hit by trace_ray

struct Emit;                                            // particle emitter (agent_fx_sound.h)
struct cell;                                            // world grid cell (agent_fx_sound.h)
struct node_groups; typedef node_groups Mesh;                              // the lod mesh hanging off the agent (agent_fx_sound.cpp 0x443aa0)

// the scene (rgl world): first field of the mission object, defined once in ../world.h (members of other objects only declared)
#include "../world.h"

// ---- animation channels / skeleton (the animator at body+0x304) ---------------------------------------------
struct CapDef { vector4 a; vector4 b; float r; int pad[2]; };     // 0x2c bytes (0x43f6d0)
struct ChanRec {                                      // 0xdc bytes (0x43f6d0: items[i] stride)
    char pad0[0xcc];
    std::vector<CapDef> caps;                         // +0xcc (first +0xd0, last +0xd4)
};
struct SkelData {
    char pad0[0x18];
    int count;                                        // +0x18 (0x43f6d0)
    ChanRec* items;                                   // +0x1c
};
#include "../channel.h"             // Channel (shared with agent2)
#include "../skeleton.h"                // Skeleton, Region, node_groups (shared with agent / agent2)
// ---- the 0x38-byte record of the container instantiations: three words, two vector<int> (+0xc, +0x1c), three words
// (the PhysRec of src/game/agent/phys_set.h) and its allocator (agent_combat.cpp 0x43f510; agent_fx_sound.cpp 0x442060)
struct E56 {
    int a, b, c;
    std::vector<int> v1;       // +0x0c
    std::vector<int> v2;       // +0x1c
    int d, e, f;
    E56(const E56& o);         // 0x43f510
};
struct alloc_e56 {
    __declspec(noinline) void construct(E56* p, const E56& v);      // 0x442060
    void destroy(E56* p);                                           // 0x43e150
};

// ---- animation slot: 2 x 0x2c bytes at agent+0xd4 (0x4406a0 reads slot[1].cur at +0x100 and .scale at +0x118;
// 0x444370 sets cur/next/events/rate through the inlined 0x4378a0) ----------------------------------------------
#include "../anim_slot.h"          // AnimSlot (shared with agent2; set() inline here as in 0x444370)

// ---- the mission object (g_mission 0x5e292c) ------------------------------------------------------------
// ---- the agent: src/game/agent_obj.h (Agent, AgentBody, Threat, DefList; one definition for src/game) ----
#include "../agent_obj.h"
inline bool Agent::charged_0x48() const { return anim[1].cur == 0x48 && body->skel->chans->charge() > 0.1f; }   // 0x432470 inlined
// AgentTable: src/game/agent_obj.h
// Mission (g_mission 0x5e292c): src/game/mission.h
#include "../mission.h"
extern __int64 g_ticks[];                               // 0x5e2948 clock table (1e-5 s ticks)

// ---- game data: troop records, parties, slot table (g_game 0x5e2930) ---------------------------------------------
struct TroopRec {                                // 0x508 bytes (array at game+0x13a00)
    tstring id;                                  // +0 (0x4428c0)
    char pad10[0x20 - 0x10];
    tstring name;                                // +0x20 (0x4428c0)
    unsigned flags;                              // +0x30 low nibble = class (0x443870)
    char pad34[0x9c - 0x34];
    int exp;                                     // +0x9c (0x4428c0)
    char pada0[4];
    int fa4, fa8, fac;                           // +0xa4 (fac += 10 per level)
    int level;                                   // +0xb0
    char padb4[0x508 - 0xb4];
};
struct Slot494 { char pad0[0x30]; unsigned char flags; char pad31[0x494 - 0x31]; };   // 0x494 bytes (0x4435c0)
struct PartySlot { int f0; int troop; char pad8[0x18]; };      // 0x20 bytes (0x4428c0)
struct Party {                                   // 0x5ac8 bytes
    char pad0[0xa8];
    PartySlot m[64];                             // +0xa8
    int count;                                   // +0x8a8
    char pad8ac[0x5ac8 - 0x8ac];
};
struct PartyMgr {
    char pad0[0x138d0];
    Party** pages;                               // +0x138d0 (index >> 4 -> page of 16)
    int index_of(int id);                        // 0x48ad00
};
struct GameData {
    char pad0[0x78];
    int f78;                                     // +0x78 (0x4428c0)
    char pad7c[0x8c - 0x7c];
    int player;                                  // +0x8c
    char pad90[0xf8 - 0x90];
    PartyMgr parties;                            // +0xf8 (embedded; pages pointer at +0x139c8)
    char pad_x[0x139e4 - 0xf8 - sizeof(PartyMgr)];
    int f139e4;                                  // +0x139e4 (0x443aa0)
    char pad139e8[0x13a00 - 0x139e8];
    union {
        TroopRec* chars;                         // +0x13a00 (0x4428c0)
    };
    char pad13a04[0x13a34 - 0x13a04];
    int f13a34;                                  // +0x13a34 (0x4428c0)
    char pad13a38[0x13a50 - 0x13a38];
    Slot494* slots;                              // +0x13a50 (0x4435c0)
    int skill(int troop, int id);                // 0x437b20 (thiscall, ret 8) (0x4406a0)
    void add_experience(int troop, int amount);  // 0x4428c0 (ret 8) (agent_fx_sound.cpp)
};
extern GameData* g_game;                         // 0x5e2930

// the agent of an id (page table lookup, as the callers inline it)
static __forceinline Agent* agent_by_id(unsigned id)
{
    return &g_mission->agents.pages[id >> 4][id & 0xf];
}

// the same lookup with the page table read as a raw pointer array (0x43f970 compiles it so: 813 vs 837 differing bytes
// with the vector form)
static __forceinline Agent* agent_by_id_raw(unsigned id)
{
    return (Agent*)(((char**)g_mission->agents.pages._Myfirst)[id >> 4] + (id & 0xf) * 0x5928);
}

#endif // AGENT3_TYPES_H

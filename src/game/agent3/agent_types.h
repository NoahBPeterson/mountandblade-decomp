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
#include "../agent_obj.h"                              // posm
struct ChanRec {                                      // 0xdc bytes (0x43f6d0: items[i] stride)
    char pad0[4];
    int parent;                                       // +4 parent bone, -1 = root (0x44fc60)
    char pad8[4];
    posm m;                                           // +0xc local frame (0x44fc60)
    char pad4c[0x9c - 0x4c];
    float mass;                                       // +0x9c (0x44fc60)
    float lim_min, lim_max, lim_twist;                // +0xa0 / +0xa4 / +0xa8 (degrees)
    float rot_x, rot_y, rot_z;                        // +0xac / +0xb0 / +0xb4 (degrees)
    char padb8[4];
    rgl::vector4 cone;                                // +0xbc (x, y, z of the limit cone direction)
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
inline bool Agent::f100_is_0x48_and_charged() const { return anim[1].cur == 0x48 && body->skel->chans->charge() > 0.1f; }   // 0x432470 inlined
// AgentTable: src/game/agent_obj.h
// Mission (g_mission 0x5e292c): src/game/mission.h
#include "../mission.h"
extern __int64 g_ticks[];                               // 0x5e2948 clock table (1e-5 s ticks)

// ---- game data: troop records, parties, slot table (g_game 0x5e2930): the shared classes GameData / TroopRec / Party / Stack / Level494
#include "../../include/game_data.h"

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

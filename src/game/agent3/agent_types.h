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
#include "../../rgl/rgl_math.h"
#include "../tstring.h"
#include "../agent/item_kind.h"       // ItemKind, g_items, pos2
#include "../mat4.h"                   // rgl::mat4
#include "../../include/timer.h"

using rgl::vector4;
using rgl::matrix;

// rgl::matrix (rgl::mat4 + the rotation 0x40e570) is in src/game/agent_obj.h

// ---- collision shapes (vtable 0x54a4b8 shape_base, 0x54a540 capsule): src/include/shape.h ---------------------------
#include "../../include/shape.h"
extern shape_base* g_hit_shape;                         // 0x5e246c: last shape hit by trace_ray

struct node_groups; typedef node_groups Mesh;                              // the lod mesh hanging off the agent (agent_fx_sound.cpp 0x443aa0)

// the scene (rgl world): first field of the mission object (src/rgl/scene/world.h)
#include "../../rgl/scene/world.h"

// ---- the skeleton (the body's FxDef at +0x304, include/fx.h) and its resource bones -------------------------------
#include "../agent_obj.h"                              // rgl::matrix
#include "../../rgl/core/bone.h"                       // Bone / CapDef: the skeleton resource's bones (frame_set::bones) with their capsules
#include "../skeleton.h"                // node_list, node_groups (shared with agent / agent2)
// ---- the 0x38-byte record of the container instantiations: three words, the ragdoll bone and body vectors (+0xc, +0x1c),
// three words (the PhysRec of src/include/phys_set.h) and its allocator (agent_combat.cpp 0x43f510; agent_fx_sound.cpp 0x442060)
struct NewtonRagDollBone; struct NewtonBody;
struct E56 {
    int a, b, c;
    std::vector<const NewtonRagDollBone*> v1;   // +0x0c PhysRec::bones (copy constructor 0x43ed20)
    std::vector<const NewtonBody*> v2;          // +0x1c PhysRec::bodies (the same 0x43ed20: one object's identical copies fold)
    int d, e, f;
    E56(const E56& o);         // 0x43f510
};
struct VecPair2;
struct alloc_e56 {
    __declspec(noinline) void construct(E56* p, const E56& v);      // 0x442060
    void destroy(E56* p);          // 0x43e150 (src/game/agent4/agent_logic.cpp): the record's destructor (v2, then v1)
};

// Callees whose definitions are under another class name (identical code folding, or the module that defines them names the
// class differently); declared with the defining class's name so the call binds its symbol, and the object is cast at the
// call. Each says why the object is the one the definition works on.
struct AgentX {                                                     // src/rgl/xmlcfg/game_util.h: the agent record's team relation tests
    bool f100_is_0x48_and_charged() const;                          // 0x432470
};
struct Obj3 {
    void set_v184(const vector4* v);                                // 0x433740 (rgl/xmlcfg/game_util.cpp; the rgl::psys emitter's code)
    void reset_pending(int v);                                      // 0x433810
    void set_placement(const matrix* m, const vector4* a, const vector4* b, int c);   // 0x433770 (game_util.h: scene object with a placement block: the rgl::psys emitter)
};


// ---- animation slot: 2 x 0x2c bytes at agent+0xd4 (0x4406a0 reads slot[1].cur at +0x100 and .scale at +0x118;
// 0x444370 sets cur/next/events/rate through the inlined 0x4378a0) ----------------------------------------------
#include "../anim_slot.h"          // AnimSlot (shared with agent2; set() inline here as in 0x444370)

// ---- the mission object (g_mission 0x5e292c) ------------------------------------------------------------
// ---- the agent: src/game/agent_obj.h (Agent, Threat; the animations are include/anim_set.h) ----
#include "../agent_obj.h"
inline bool Agent::f100_is_0x48_and_charged() const { return anim[1].cur == 0x48 && body->fx->env[1].weight > 0.1f; }   // 0x432470 inlined
// AgentTable: src/game/agent_obj.h
// Mission (g_mission 0x5e292c): src/game/mission.h
#include "../mission.h"

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

// agent_types.h: the classes shared by agent_combat.cpp (0x43e190..0x441360) and agent_fx_sound.cpp
// (0x441ff0..0x444c28): the mission agent (Agent, 0x5928 bytes), its body / skeleton / animation slots, the
// mission object (Mission, AgentTable), the scene (world) and the small helpers both modules call. ONE definition
// of each class for this directory; every offset is read or written by a function of these two modules (the
// address is cited at the field). Names provisional.
//
// The classes shared with the other agent modules are one definition each in src/game: Agent / AgentBody / AgentTable /
// Threat (agent_obj.h), ItemRef (item_ref.h), Mission (mission.h), IfaceB (src/game/common/iface.h); this header includes them.
#ifndef AGENT3_TYPES_H
#define AGENT3_TYPES_H
#include <stddef.h>
#include <vector>
#include "engine/math/rgl_math.h"
#include "engine/core/tstring.h"
#include "game/data/item_kind.h"       // ItemKind, g_items
#include "engine/math/mat4.h"                   // rgl::mat4
#include "engine/core/timer.h"

using rgl::vector4;
using rgl::matrix;

// rgl::matrix (rgl::mat4 + the rotation 0x40e570) is in src/game/agent/agent_obj.h

// ---- collision shapes (vtable 0x54a4b8 shape_base, 0x54a540 capsule): src/engine/physics/shape.h ---------------------------
#include "engine/physics/shape.h"
extern shape_base* g_hit_shape;                         // 0x5e246c: last shape hit by trace_ray

namespace rgl { struct lod_mesh; }                                          // the lod meshes hanging off the agent (agent_fx_sound.cpp 0x443aa0)

// the scene (rgl world): first field of the mission object (src/engine/scene/world.h)
#include "engine/scene/world.h"

// ---- the skeleton (the body's FxDef at +0x304, engine/fx/fx.h) and its resource bones -------------------------------
#include "game/agent/agent_obj.h"                              // rgl::matrix
#include "engine/physics/bone.h"                       // Bone / CapDef: the skeleton resource's bones (frame_set::bones) with their capsules
#include "engine/resource/skeleton.h"                // node_list, node_groups (shared with agent / agent2)
// (the 0x38-byte record of agent4's std::vector instantiations is engine/physics/phys_set.h's PhysRec)



// ---- animation slot: 2 x 0x2c bytes at agent+0xd4 (0x4406a0 reads slot[1].cur at +0x100 and .scale at +0x118;
// 0x444370 sets cur/next/events/rate through the inlined 0x4378a0) ----------------------------------------------
#include "game/agent/anim_slot.h"          // AnimSlot (shared with agent2; set() inline here as in 0x444370)

// ---- the mission object (g_mission 0x5e292c) ------------------------------------------------------------
// ---- the agent: src/game/agent/agent_obj.h (Agent, Threat; the animations are engine/fx/anim_set.h) ----
#include "game/agent/agent_obj.h"
// AgentTable: src/game/agent/agent_obj.h
// Mission (g_mission 0x5e292c): src/game/mission/mission.h
#include "game/mission/mission.h"

// ---- game data: troop records, parties, slot table (g_game 0x5e2930): the shared classes GameData / Troop / Party / Stack / Level494
#include "game/data/game_data.h"

// the agent of an id (page table lookup, as the callers inline it)
static __forceinline Agent* agent_by_id(unsigned id)
{
    return &g_mission->agents.pages[id >> 4][id & 0xf];
}

// the same lookup with the page table read as a raw pointer array (0x43f970 compiles it so: 813 vs 837 differing bytes
// with the vector form)
static __forceinline Agent* agent_by_id_raw(unsigned id)
{
    return (&*g_mission->agents.pages.begin())[id >> 4] + (id & 0xf);
}

#endif // AGENT3_TYPES_H

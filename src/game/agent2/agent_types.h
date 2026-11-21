// agent_types.h: the classes shared by agent_stats.cpp (0x437830..0x43a280) and agent4/agent_logic.cpp (0x43a280..0x460160): the
// 0x1c8-byte placed-object record (PlacedObj), the mission agent (Agent, 0x5928 bytes) with its combat-stat, aiming,
// target-picking, input-polling, animation and path-goal members, the mission object, the game data, the level
// (scene) and the navigation mesh, the animation controller (Skeleton, Channel) and the small std::vector /
// std::_Uninit_* instantiation helpers the linker kept. ONE definition of each class for this directory. Offsets are
// the ones read or written by a function of these two modules (the address of one such function is cited at the
// field); the rest of every object is padding. Names provisional.
//
// The classes shared with the other agent modules are one definition each in src/game: Agent / AgentBody / AgentTable /
// Threat (agent_obj.h), ItemRef (item_ref.h), Mission / TeamRec (mission.h), PlacedObj / IfaceB / Quad (placed_obj.h), PolyMesh
// (terrain_b/poly_mesh.h), SkillCap (skill_cap.h); this header includes them. Curve (src/game/agent/curve.h) is the 0xd8-byte Channel here.
#ifndef AGENT2_AGENT_TYPES_H
#define AGENT2_AGENT_TYPES_H
#include <stddef.h>
#include <math.h>
#include <vector>
#include "../../rgl/rgl_math.h"
#include "../tstring.h"
#include "../mat4.h"
#include "../agent/item_kind.h"       // ItemKind, g_items, item type enum
#include "../../include/height_field.h"   // HeightField (line_clear 0x436da0, src/game/agent/agent.cpp)

using rgl::vector4;
using rgl::matrix;

struct shape_base; struct face; struct capsule; struct contact;

// ---- globals ------------------------------------------------------------------------------------------------
#include "../../include/fx.h"            // g_step_tbl 0x5a9c20 (entry 0 is the frame step)
#include "../dialog_manager.h"            // g_script 0xa70440

// ---- polymorphic records derived from IfaceB (vtable 0x5908f8 = ??_7IfaceB@@6B@) --------------------------------
#include "../placed_obj.h"             // IfaceB, Quad, PlacedObj (shared with src/game/agent)
// the 0x1c8-byte placed scene object (vtable 0x590b84; PlacedObj of src/game/agent/agent.h, "PlacedObj" in
// src/game/agent3, "E1c8" in the std helper rows below). Constructor 0x437830, deleting destructor 0x437870
// (compiler-generated), copy constructor 0x437bb0, assignment 0x437d60; reset() is 0x4352f0 (src/game/agent).

// ---- item reference: src/game/item_ref.h (agent_obj.h includes it) ----

// ---- 2-D helpers --------------------------------------------------------------------------------------------
#include "../../include/vec.h"
float __cdecl signed_angle(const vec2* a, const vec2* b);       // 0x4381e0 (also the "blend_target" of the slices)
float __cdecl closer_angle(const float* v, const float* lo, const float* hi);   // 0x4325a0 (earlier object)
float __cdecl fmin_ref(const float* a, const float* b);         // 0x4324b0
float __cdecl fmax_ref(const float* a, const float* b);         // 0x4324d0
void __cdecl noise_gradient(float scale, const float* in, float* out);   // 0x414ae0

// ---- the navigation mesh: src/game/terrain_b/poly_mesh.h ----
#include "../terrain_b/poly_mesh.h"

// ---- the level (scene: first field of the mission object) -----------------------------------------------------
#include "../light.h"                   // Light (shared with strat / script)
#include "../../rgl/scene/world.h"      // rgl::world: the level / scene (first field of the mission object), rgl::cell

// ---- animation: the skeleton animation (skel_anim: one key_array per bone, the root's tail; FxDef::start_anim 0x439fc0)
#include "../../rgl/resource.h"                         // skel_anim, key_array (include/key_table.h)
// SubAnim, AnimDef, AnimSet: include/anim_set.h
#include "../skeleton.h"                // node_list, node_groups (shared with agent / agent3)

// ---- the mesh instance's node lists (0x438d80, 0x438f90, 0x439000) ---------------------------------------------
struct V32 { float x, y, z; float pad[5]; };            // 32-byte vertex record (x, y, z first)
struct vec_e32 {
    int al; V32* first; V32* last; V32* end;
    unsigned size() const { return first == 0 ? 0 : last - first; }
    V32& operator[](unsigned i) { return *(first + i); }
};
#include "../../rgl/mesh.h"                   // rgl::mesh (calc_bounds 0x438d80 is its member; the node lists point at it)

// ---- the agent: src/game/agent_obj.h (Agent, AgentBody, Threat, InputRec, InputState; one definition for src/game) ----
#include "../agent_obj.h"
#include "../anim_slot.h"          // AnimSlot (shared with agent3; agent_stats.cpp defines set() out of line)
#include "../../include/timer.h"
float __cdecl hit_chance(int* who, Agent* a);           // 0x438cb0

// ---- the mission object (g_mission 0x5e292c) and the game data (g_game 0x5e2930) ---------------------------------
// AgentTable: src/game/agent_obj.h
// HandRec / TeamRec / Mission (g_mission 0x5e292c): src/game/mission.h (ClassInfo is the per-weapon-class hand record)
#include "../mission.h"

#include "../skill_cap.h"                            // SkillCap, g_skill_tbl 0x8ee3100 (shared with agent)
#include "../../include/game_data.h"                // GameData / TroopRec / g_game: the shared classes (skill_level 0x437b20, emit_event 0x435790)


// ---- STL instantiation helpers (VC7.1 <vector>/<memory>/<algorithm> code the linker kept from this object) ------
#include "../../include/ptr_tag.h"
#include "../../include/pod_elems.h"   // E8 / E12 / E100 / rec36 / rec136 (shared)
#include "../hnode.h"                             // hnode / node_gt (shared with agent4: PolyMesh::open)
#include "../../include/clock_table.h"
typedef PlacedObj E1c8;                                  // the 0x1c8-byte polymorphic record
#include "../alloc_construct.h"   // allocator<tstring / E12 / rec36 / rec136>::construct (0x436c90 .. 0x436cf0, agent/agent.cpp)
// the construct calls of the vectors' _Uninit_copy / _Uninit_fill_n instances stay out of line, as in the original: the 0x1c8-byte
// record's (0x439040, agent_stats.cpp) and the 4-byte one (0x436d80, agent.cpp: the ragdoll bone pointers' copy 0x438aa0)
struct NewtonRagDollBone;
namespace std {
#ifndef ALLOC_PLACED_CONSTRUCT_DEFINED_HERE
template<> void allocator<E1c8>::construct(E1c8* p, const E1c8& v);
#endif
template<> void allocator<const NewtonRagDollBone*>::construct(const NewtonRagDollBone** p, const NewtonRagDollBone* const& v);
}
#include "../int_vec.h"                                 // IntVec (was vec_int here: fill_ptr 0x438c30)
// 0x4b2520 is defined (party2/party_2.cpp) as push_heap_elem over the 12-byte OpenItem record, which hnode mirrors (face, cost, priority)
struct OpenItem {
    int node; float cost; float dist;
#if !MB_LP64
    // the load-order copy cl needs here (0x438b10); not in the port: a user-written copy makes the record non-trivial, and the
    // Itanium ABI would then pass push_heap_elem's by-value record by address from this unit but in registers in party_2.cpp
    OpenItem(const OpenItem& o) : node(o.node), cost(o.cost), dist(o.dist) {}
#endif
};
struct HeapGt { bool operator()(const OpenItem& a, const OpenItem& b) const { return a.dist > b.dist; } };
void __cdecl push_heap_elem(OpenItem* first, int hole, int top, OpenItem val, HeapGt pred);   // 0x4b2520
// the free functions of this object
void __cdecl adjust_heap_node(hnode* first, int hole, int bottom, hnode val, node_gt pred);                      // 0x438b10

#endif // AGENT2_AGENT_TYPES_H

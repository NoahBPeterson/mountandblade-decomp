// agent_queries.h: the classes of agent_queries.cpp (0x434b20..0x437830): item references with their modifier-adjusted stats, the
// troop record's small accessors, the agent's camera / steering / equipment members, the mission object's
// accessors, the placed-object record and the small container / math helpers. Offsets are the ones read by a
// function of this module (address cited at each field); the rest of every object is padding. All names are
// provisional. The item definition record is src/game/data/item_kind_include.h, the item reference item_ref.h, the agent agent_obj.h, the
// mission object mission.h, the physics set phys_set.h.
#ifndef AGENT_AGENT_H
#define AGENT_AGENT_H
#include <math.h>
#include "engine/math/rgl_math.h"
#include "engine/core/tstring.h"
#include "engine/math/mat4.h"
#include "game/data/item_kind.h"
#include "engine/fx/env_curve.h"        // quaternion (w, x, y, z)

namespace rgl { struct mesh; }
using rgl::vector4;
using rgl::matrix;

#include "game/data/item_ref.h"             // ItemRef, imod_* (shared with agent2 / agent3 / ui / script)
int round_float(float* f);              // 0x432540 (cdecl): rounds a float to an int
int slot_class(int item, int flag);     // 0x4350c0 (register convention, defined static in agent_queries.cpp)

// named record tables (0x435260, 0x4352a0)
struct Rec20 { tstring name; char pad[0x10]; };      // 0x20-byte named record (table g_tbl20)
#include "game/mission/mission_obj.h"      // MissionObj: the scene prop kind (g_propkinds)
#include "game/data/ptype.h"
int find_in_table20(const tstring* name);   // 0x435260 (cdecl)
int find_in_table78(const tstring* name);   // 0x4352a0 (cdecl)
int ftoi_helper(int x);                     // 0x435440

// ---- troop record (0x508 bytes, array at world+0x13a00): packed 4-bit skill levels and per-slot values
// (the address comments below stay bare: the resolver reads the last 0x... of a declaration's comment)
#include "game/data/skill_cap.h"            // SkillCap, g_skill_tbl 0x8ee3100 (shared with agent2)

#include "game/data/troop.h"   // Troop / Troop: the 0x508-byte troop record (has_flag10 0x435130, stat_int 0x435150, scaled_b0 0x435170, nibble 0x4351a0, falloff 0x4375b0, adjust 0x437770)

// (0x4351f0 is Missile::ranged_ref: ../missile.h, included below)

// ---- placed scene object (0x1c8 bytes, vtable 0x590b84; ctor / copy ctor are in the next object). reset() is
// called by 0x435860 (init_placed), 0x437830 (ctor) and 0x477ea0 (vector<..>::resize).
#include "game/mission/placed_obj.h"             // PlacedObj, IfaceB, Quad (shared with agent2; reset 0x4352f0)
int __cdecl clamp_index(const int* v, const int* lo, const int* hi);   // 0x432510 (earlier object)
void __stdcall init_placed(PlacedObj* o, const int* kind, const int* id, const int* value, const matrix* m);   // 0x435860

// ---- RecZ sub-object initialiser (called by the RecZ ctor 0x477a30): the blow record's Blow::init 0x4353a0
#include "game/agent/blow.h"

// ---- scene and mission ----------------------------------------------------------------------------------
struct shape_base;
#include "engine/scene/world.h"            // rgl::world: the scene, first field of the mission object (0x4417d0 reads it at +0)
#include "game/mission/mission.h"                       // Mission, TeamRec, g_mission (one definition for src/game); ModeRec: mode_rec.h
#include "game/data/mode_rec.h"
extern ModeRec* g_mission_templates;                         // 0x5e2a00
// the placement block at +0x13fb4 (current_color, position_delta, trigger_frame) and its setter 0x435650 are GameData's (game/data/game_data.h)

// event emitter (0x4b09e0) used by the game-data object (s00514d40 names the same 0x435790 GameData::emit_event)
#include "game/data/rec34.h"                 // rec34_table, g_scripts (0xa75eb0)

// ---- the agent ---------------------------------------------------------------------------------------------
#include "game/agent/agent_obj.h"                // Agent, AgentBody, Threat (one definition for src/game; shared with agent2 / agent3 / script)
#include "engine/math/vec.h"

// ---- key-frame sampling (0x436ae0, 0x436b90)
void slerp(quaternion* out, const quaternion* a, const quaternion* b, const float* t);      // 0x4238c0
#include "game/input/key_table.h"   // key_array (one definition for src/game and src/engine; find 0x423a50, sample 0x436ae0)

// (0x435990 / 0x4359c0 / 0x4359e0 are std::vector<ArrSet / CapDef / PlacedObj>::size: agent_queries.cpp)
struct Triple { int a, b, c; };

// ---- STL instantiations of the object (std::allocator<T>::construct, std::_Uninit_copy), element types provisional
#include "engine/core/pod_elems.h"   // E8 / E12 / E100 / rec36 / rec136 (shared)
#include "engine/core/alloc_construct.h"   // alloc_tstring / alloc_rec36 / alloc_rec136 / alloc_E12 / alloc_int, uninit_copy_E12

// ---- height field + 2-D grid walkers (0x436da0, 0x437290)
#include "engine/physics/height_field.h"      // HCell, HeightField (line_clear 0x436da0)


// ---- time of day / misc free functions
float time_of_day();                    // 0x4356e0
bool is_daytime();                      // 0x435700
bool is_night();                        // 0x435740
float __cdecl accel_factor(const float* a, const float* b, const float* c);   // 0x435ae0
int __cdecl team_leader(const Agent* a);                                   // 0x435b60
int __cdecl team_leader(const Agent* a); // 0x435b60

// ---- Missile: the 0xb0-byte IfaceB-derived record (vtable 0x590b78, constructor 0x4377d0, slot1 0x437810): src/include/recj.h
#include "game/agent/missile.h"   // Missile, the record of Pool<Missile> (Mission::poolJ)

#endif // AGENT_AGENT_H

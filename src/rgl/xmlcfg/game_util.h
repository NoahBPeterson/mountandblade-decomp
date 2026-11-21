// game_util.h: the classes and declarations of game_util.cpp (0x4320b0..0x43383f): key-code group tables, the
// agent's team relation tests, scalar and 2-D / 3-D vector helpers, the Newton force callback, the tick timer,
// the actor's weighted item pick, std::copy instantiations, the argument constructors of the sphere and capsule
// collision shapes, collision face helpers, the message ring, the height-field sampler and the scene object's
// placement setters. Names provisional; every offset is read or written by the function cited.
//
// Reused (do not redefine): src/game/agent/agent.h (Agent, ItemRef, Mission/g_mission, vec2, HeightField, HCell,
// round_float, clamp_index), curve.h (g_ticks = the tick table at 0x5e2948), src/game/mat4.h (rgl::mat4::relative
// 0x4329d0, rgl::mat3), src/game/tstring.h. What those headers lack is added by classes derived from theirs
// (vec2x, HeightFieldX, mat3: no new fields, only the members this object defines) or by overlay views of
// the raw offsets (see MIGRATION.md, "Header conflicts").
#ifndef RGL_XMLCFG_GAME_UTIL_H
#define RGL_XMLCFG_GAME_UTIL_H
#include <math.h>
#include "../../game/agent/agent.h"

// ---- key-code groups (ids 0x97.., 0xc7..0xed come in triples; Module System header_triggers key_* numbering) ----
// not_group_base 0x4320b0 and group_base 0x432130: key_groups.h
int __stdcall in_id_set(int id);            // 0x432640
// Agent::id_matches_dir 0x4326d0 (dir 0..3 = the four arrow-key groups) is declared in game/agent_obj.h

// ---- the agent: team relations and the actor's item pick -------------------------------------------------------
// Agent members that agent.h does not declare. The "actor" of the s00432a50 slice is the same 0x1c8-byte record
// (+0x2c, +0x12c agree with agent.h: defs->data is the table of AnimDef (include/anim_set.h): subs +0x10, nsubs +0x14).
inline AnimDef* agent_recs(const Agent* a) { return a->defs->data; }        // the actor's table of 0x30-byte records (+0x12c; fill_weights 0x432f60, pick 0x433040)
#include "../../include/game_data.h"      // GameData / g_game 0x5e2930: the troop table at +0x13a00 (0x433140 reads the record's flags at +0x30; this module called the record ItemDef)

// is_enemy 0x432350, is_friend 0x4323a0, is_neutral 0x4323f0, speed_scale 0x432450, f100_is_0x48_and_charged 0x432470, fill_weights
// 0x432f60 and pick 0x433040 are members of Agent (agent_obj.h), defined in game_util.cpp (agent_class 0x433140 is Agent's too).

// ---- scalar helpers (cdecl, arguments by pointer) ----------------------------------------------------------------
float __cdecl fmin_ref(const float* a, const float* b);     // 0x4324b0
float __cdecl fmax_ref(const float* a, const float* b);     // 0x4324d0
int __cdecl imax_ref(const int* a, const int* b);           // 0x4324f0
// clamp_index (0x432510) and round_float (0x432540) are declared in agent.h
int __cdecl rand_below(const int* n);                       // 0x432580
float __cdecl closer_angle(const float* ref, const float* a, const float* b);   // 0x4325a0

// ---- vectors -----------------------------------------------------------------------------------------------------
struct vec2x : vec2 {                       // agent.h's vec2 only has normalize()
    void rotate_a(float ang);               // 0x432600
    void rotate_b(float ang);               // 0x4327f0
};
// 12-byte float triple with the engine's inline operators (the element of the std::copy_backward at 0x433260)
#include "../../include/vec.h"
void __cdecl vec3_sub(Vec3* out, const Vec3* a, const Vec3* b);      // 0x432830
// agent.h's slot_pair (0x432790) is defined here; rgl::mat3 (mat4.h, with mul_t 0x432860) is the 3x3 rotation, rows padded to 4 floats.
// rgl::mat4::relative (0x4329d0) is declared in src/game/mat4.h

// ---- Newton physics callback and tick timer ---------------------------------------------------------------------
struct NewtonBody;
void __cdecl apply_force_and_torque(const NewtonBody* body);        // 0x432a50
#include "../../include/phys_set.h"   // PendingForce (g_phys.pending)


#include "../../include/timer.h"
void __cdecl rand_cone_dir(vector4* out, vector4 dir, float spread);       // 0x432c40
void __cdecl bounce(Vec3* out, Vec3* v, const Vec3* n);                    // 0x432e30


// ---- collision shapes: shape_base, sphere (0x4332a0), capsule (0x433310), face (0x4333c0, 0x433440) and composite
// (0x433470) are src/include/shape.h's; the constructors and helpers named here are defined in game_util.cpp ----
#include "../../include/shape.h"
bool __cdecl in_box(const vector4* lo, const vector4* hi, const vector4* p);   // 0x4336d0

// ---- message ring, height field, scene object ---------------------------------------------------------------------
#include "../../game/strat/gui_manager.h"             // gui_manager (push 0x4334b0 is defined in game_util.cpp)

#endif

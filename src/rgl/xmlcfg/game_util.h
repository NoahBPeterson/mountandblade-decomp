// game_util.h: the classes and declarations of game_util.cpp (0x4320b0..0x43383f): key-code group tables, the
// agent's team relation tests, scalar and 2-D / 3-D vector helpers, the Newton force callback, the tick timer,
// the actor's weighted item pick, std::copy instantiations, the argument constructors of the sphere and capsule
// collision shapes, collision face helpers, the message ring, the height-field sampler and the scene object's
// placement setters. Names provisional; every offset is read or written by the function cited.
//
// Reused (do not redefine): src/game/agent/agent.h (Agent, ItemRef, Mission/g_mission, vec2, HeightField, HCell,
// round_float, clamp_index), curve.h (g_time = the tick table at 0x5e2948), src/game/mat4.h (rgl::mat4::relative
// 0x4329d0, rgl::rot3), src/game/tstring.h. What those headers lack is added by classes derived from theirs
// (AgentX, vec2x, HeightFieldX, rot3x: no new fields, only the members this object defines) or by overlay views of
// the raw offsets (see MIGRATION.md, "Header conflicts").
#ifndef RGL_XMLCFG_GAME_UTIL_H
#define RGL_XMLCFG_GAME_UTIL_H
#include <math.h>
#include "../../game/agent/agent.h"

// ---- key-code groups (ids 0x97.., 0xc7..0xed come in triples; Module System header_triggers key_* numbering) ----
int __cdecl not_group_base(int x);          // 0x4320b0 false for the first code of each triple group_base() returns
int __cdecl group_base(int id);             // 0x432130 first id of the triple, -1 otherwise
int __stdcall in_id_set(int id);            // 0x432640
int __stdcall id_matches_dir(int id, int dir);   // 0x4326d0 (dir 0..3 = the four arrow-key groups)

// ---- the agent: team relations and the actor's item pick -------------------------------------------------------
// Agent members that agent.h does not declare. The "actor" of the s00432a50 slice is the same 0x1c8-byte record
// (+0x2c, +0x12c agree with agent.h: f12c is the table of 0x30-byte Record entries, items at +0x10, count +0x14).
struct SkelX {                              // what the +0x304 pointer of the body really is (0x432470)
    char pad0[0x40];
    struct Part { char pad[0xd8]; float charge; } *f40;     // +0x40
};
struct ItemRec { int pad0, pad4; unsigned flags; char pad_c[0x40 - 0xc]; };     // 0x40 bytes, flags at +8 (0x432f60)
struct Record { char pad0[0x10]; ItemRec* items; int count; char pad18[0x30 - 0x18]; };   // 0x30, items +0x10, count +0x14
#include "../../include/game_data.h"      // GameData / g_game 0x5e2930: the troop table at +0x13a00 (0x433140 reads the record's flags at +0x30; this module called the record ItemDef)

struct AgentX : Agent {
    Record* recs() const { return *(Record**)f12c; }        // the actor's table of 0x30-byte records (+0x12c)
    unsigned char flags130() const { return *((const unsigned char*)this + 0x130); }   // +0x130 (0x432f60)
    bool is_enemy(const Agent* o) const;                    // 0x432350 (relation == -1)
    __declspec(noinline) bool is_friend(const Agent* o) const;   // 0x4323a0 (relation == 1)
    int is_neutral(const Agent* o) const;                   // 0x4323f0
    float speed_scale() const;                              // 0x432450
    bool f100_is_0x48_and_charged() const;                  // 0x432470 (state +0x100 == 0x48)
    void fill_weights(float* out, int idx, int rec);        // 0x432f60 (idx unused)
    void fill_weights_x(float* out, int idx, int rec);      // 0x432f60 (declaration only, see pick)
    int pick(int idx);                                      // 0x433040
    int hand_kind() const;                                  // 0x433140
};

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
// agent.h's slot_pair (0x432790) is defined here; rgl::rot3 (mat4.h) is the 3x3 rotation, rows padded to 4 floats.
struct rot3x : rgl::rot3 {
    __declspec(noinline) rot3x mul_t(const rot3x& b) const;          // 0x432860 row i, col j = dot(b.row i, this->row j)
};
// rgl::mat4::relative (0x4329d0) is declared in src/game/mat4.h

// ---- Newton physics callback and tick timer ---------------------------------------------------------------------
struct NewtonBody;
void __cdecl apply_force_and_torque(const NewtonBody* body);        // 0x432a50
// a force/torque the game wants applied to one body on the next physics step
struct PendingForce {
    const NewtonBody* body;                 // 0xa18210
    vector4 force;                          // 0xa18214
    vector4 torque;                         // 0xa18224
};
extern PendingForce g_pending;              // 0xa18210

#include "../../include/timer.h"
void __cdecl rand_cone_dir(vector4* out, vector4 dir, float spread);       // 0x432c40
void __cdecl bounce(Vec3* out, Vec3* v, const Vec3* n);                    // 0x432e30

// ---- mesh morph time clamp --------------------------------------------------------------------------------------
// A member of the rgl::mesh of src/rgl/particles/mesh_obj.h (0x433190: +0xa1 local flag, +0xa4 key frames, +0xc8
// c8), which cannot be included together with agent.h's rgl.h: a partial view, same offsets.
struct keyframes12 { void* frames; int* times; int count; };
struct mesh_node {
    char pad0[0x18];
    mesh_node* parent;                      // +0x18
    char pad1c[0xa1 - 0x1c];
    char local_a1;                          // +0xa1
    char pada2[2];
    keyframes12 kf;                         // +0xa4
    char padb0[0xc8 - 0xb0];
    float c8;                               // +0xc8
    keyframes12& get_kf();                  // 0x40ec00 (local_a1 ? kf : parent->get_kf())
    void clamp_c8(const float* t);          // 0x433190
};

// ---- collision shapes: shape_base, sphere (0x4332a0), capsule (0x433310), face (0x4333c0, 0x433440) and composite
// (0x433470) are src/include/shape.h's; the constructors and helpers named here are defined in game_util.cpp ----
#include "../../include/shape.h"
bool __cdecl in_box(const vector4* lo, const vector4* hi, const vector4* p);   // 0x4336d0

// ---- message ring, height field, scene object ---------------------------------------------------------------------
#include "../../include/msg_log.h"             // MsgLog (one definition for src/game and src/rgl; push 0x4334b0 is defined in game_util.cpp)
struct HeightFieldX : HeightField {         // agent.h's HeightField has cols, w, h, cell
    void sample(float x, float y, float* out_h, int* out_id);   // 0x433510
};
struct Obj3 {                               // scene object with a placement block
    char pad0[0xe8];
    unsigned flags;                         // +0xe8 (bit 0: pending, bit 1: timed)
    char pade8[0x134 - 0xec];
    matrix m134;                            // +0x134 (0x433770)
    vector4 v174;                           // +0x174
    vector4 v184;                           // +0x184
    int m194;                               // +0x194
    char pad198[4];
    int m19c, m1a0, m1a4;                   // +0x19c (0x433810)
    void set_v184(const vector4* v);        // 0x433740
    void set_placement(const matrix* m, const vector4* a, const vector4* b, int c);   // 0x433770
    void reset_pending(int v);              // 0x433810
};

#endif

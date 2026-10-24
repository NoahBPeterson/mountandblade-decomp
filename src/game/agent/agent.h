// agent.h: the classes of agent.cpp (0x434b20..0x437830): item references with their modifier-adjusted stats, the
// troop record's small accessors, the agent's camera / steering / equipment members, the mission object's
// accessors, the placed-object record and the small container / math helpers. Offsets are the ones read by a
// function of this module (address cited at each field); the rest of every object is padding. All names are
// provisional. The item definition record is src/include/item_kind.h, the item reference item_ref.h, the agent agent_obj.h, the
// mission object mission.h, the physics set phys_set.h.
#ifndef AGENT_AGENT_H
#define AGENT_AGENT_H
#include <math.h>
#include "rgl.h"
#include "../tstring.h"
#include "../mat4.h"
#include "./item_kind.h"
#include "./curve.h"        // quaternion (w, x, y, z)

namespace rgl { struct mesh; }
using rgl::vector4;
using rgl::matrix;

#include "../item_ref.h"             // ItemRef, imod_* (shared with agent2 / agent3 / ui / script)
int round_float(float* f);              // 0x432540 (cdecl): rounds a float to an int
int slot_class(int item, int flag);     // 0x4350c0 (register convention, defined static in agent.cpp)

// named record tables (0x435260, 0x4352a0)
struct Rec20 { tstring name; char pad[0x10]; };      // 0x20-byte named record (table g_tbl20)
#include "../../include/rec78.h"    // Rec78: the 0x78-byte named record (table g_tbl78), shared with ui/troop_inv and rgl/mesh2
extern Rec20* g_tbl20;                  // 0x5e28f8
extern int g_tbl20_count;               // 0x5e29e8
extern Rec78* g_tbl78;                  // 0x5e29dc
extern int g_tbl78_count;               // 0x5e29e0
int find_in_table20(const tstring* name);   // 0x435260 (cdecl)
int find_in_table78(const tstring* name);   // 0x4352a0 (cdecl)
int ftoi_helper(int x);                     // 0x435440

// ---- troop record (0x508 bytes, array at world+0x13a00): packed 4-bit skill levels and per-slot values
// (the address comments below stay bare: the resolver reads the last 0x... of a declaration's comment)
#include "../skill_cap.h"            // SkillCap, g_skill_tbl 0x8ee3100 (shared with agent2)
extern int g_clamp_level;               // 0x8ee3860
extern const int g_tier_dist[];         // 0x590a70
#include "../../include/troop.h"   // Troop / TroopRec: the 0x508-byte troop record (has_flag10 0x435130, stat_int 0x435150, scaled_b0 0x435170, nibble 0x4351a0, falloff 0x4375b0, adjust 0x437770)

// ---- two item references at +0x80 of the record 0x4351f0 reads (a different object than Troop: ints, not floats)
struct Wielder {
    char pad0[0x80];
    ItemRef a, b;                       // +0x80 / +0x88
    ItemRef ranged_ref();               // 0x4351f0
    rgl::mesh* make_ranged_mesh();      // 0x44f6a0 (agent4/agent_update.cpp: clones the display mesh of the carried ranged / thrown item)
};

// ---- placed scene object (0x1c8 bytes, vtable 0x590b84; ctor / copy ctor are in the next object). reset() is
// called by 0x435860 (init_placed), 0x437830 (ctor) and 0x477ea0 (vector<..>::resize).
#include "../placed_obj.h"             // PlacedObj, IfaceB, Quad (shared with agent2; reset 0x4352f0)
int __cdecl clamp_index(const int* v, const int* lo, const int* hi);   // 0x432510 (earlier object)
extern const int g_value_max;           // 0x590a9c upper bound of a placed object's value (0x435860)
void __stdcall init_placed(PlacedObj* o, const int* kind, const int* id, const int* value, const matrix* m);   // 0x435860

// ---- RecZ sub-object initialiser (called by the RecZ ctor 0x477a30): three rows plus ids
struct RecZSubB {
    int m0, m4;
    vector4 v8, v18, v28;
    int m38, m3c, m40, m44, m48, m4c, m50, m54;
    RecZSubB* init();                   // 0x4353a0 (returns this)
};

// ---- scene and mission ----------------------------------------------------------------------------------
struct shape_base;
struct Level {                          // the scene: first field of the mission object (0x4417d0 reads it at +0)
    void query_box(const float* bmin, const float* bmax, unsigned mask, shape_base** out, int* count);   // 0x42aa50
    int trace_ray(const float* a, const float* b, float maxlen, shape_base** hit, int unused);   // 0x439d60
};
#include "../mission.h"                       // Mission, TeamRec, g_mission (one definition for src/game); ModeRec: mode_rec.h
#include "../mode_rec.h"
extern ModeRec* g_modes;                         // 0x5e2a00
// object with a placement block at +0x13fb4 (set by 0x45a170 / 0x44c210: the scene's spawn data; provisional)
struct Placement {
    char pad0[0x13fb4];
    vector4 v13fb4;
    vector4 v13fc4;
    char pad13fd4[0x14014 - 0x13fd4];
    matrix m14014;
    void set_matrix(const matrix* m);            // 0x435650
};

// event emitter (0x4b09e0) used by the game-data object (s00514d40 names the same 0x435790 GameData::emit_event)
struct EmitSub {
    char emit(int flags, int zero, int* count, __int64* vals, unsigned tag);   // 0x4b09e0
};
struct Emitter {                                 // 0x34 bytes, array g_emitters
    char pad0[4];
    tstring name;                                // +4
    char pad14[0x18 - 0x14];
    EmitSub sub;                                 // +0x18
    char pad19[0x34 - 0x19];
};
extern Emitter* g_emitters;                      // 0xa75eb0
extern int g_emitter_idx[];                      // 0xa75ebc
struct SoundCtl {
    char pad0[0x1a068];
    int f1a068;
    tstring name;                                // +0x1a06c
    int f1a07c;
    char emit_event(int id, int a, int b);       // 0x435790 (ret 0xc)
};

// floats at +0xf8 with a 0x2c stride (0x4354b0): a view of the agent
struct Agent_f8 {
    char pad[0xf8];
    float v[1][0x2c / 4];
    void set_slot(int i, float x);               // 0x4354b0
};

// ---- the agent ---------------------------------------------------------------------------------------------
#include "../agent_obj.h"                // Agent, AgentBody, Obj_a0, Threat (one definition for src/game; shared with agent2 / agent3 / script)
struct CamParams {                               // camera parameter block written by Agent::camera_params
    char pad[0x40];
    vector4 v40;                                 // +0x40 camera position offset
    vector4 v50;                                 // +0x50 target offset
    float f60;                                   // +0x60
};
#include "../../include/vec.h"

// ---- key-frame sampling (0x436ae0, 0x436b90)
void slerp(quaternion* out, const quaternion* a, const quaternion* b, const float* t);      // 0x4238c0
#include "../../include/key_table.h"   // KeyTable (one definition for src/game and src/rgl; find 0x423a50, sample 0x436ae0)
struct KeyFrames {
    vector4* vals; int* keys; int count;
    void find(int* a, int* b, float* t, const float* time);     // 0x40f0d0
};
struct KeyHolder {
    char pad0[0x28];
    KeyFrames kf;                       // +0x28
    vector4 sample(int key);            // 0x436b90 (hidden return pointer)
};

// ---- small vector-like containers: [0] allocator slot, +4 first, +8 last (0x435990, 0x4359c0, 0x4359e0)
struct Vec38 { int pad; char* first; char* last; int size() const; };   // 0x38-byte elements
struct Vec2c { int pad; char* first; char* last; int size() const; };   // 0x2c-byte elements
struct Vec1c8 { int pad; char* first; char* last; int size() const; };  // 0x1c8-byte elements
struct Triple { int a, b, c; };

// ---- STL instantiations of the object (std::allocator<T>::construct, std::_Uninit_copy), element types provisional
#include "../../include/pod_elems.h"   // E8 / E12 / E100 / rec36 / rec136 (shared)
struct int3 { int a, b, c; };       // 12 bytes
struct int1 { int a; };
struct alloc_int3;
#define AGENT_ALLOC(N, T) struct N { __declspec(noinline) void construct(T* p, const T& v); };
AGENT_ALLOC(alloc_tstring, tstring)
AGENT_ALLOC(alloc_rec36, rec36)
AGENT_ALLOC(alloc_rec136, rec136)
AGENT_ALLOC(alloc_int3, int3)
AGENT_ALLOC(alloc_int1, int1)
int3* uninit_copy_int3(const int3* first, const int3* last, int3* dest, alloc_int3& al);   // 0x436d10

// ---- height field + 2-D grid walkers (0x436da0, 0x437290)
#include "../../include/height_field.h"      // HCell, HeightField (line_clear 0x436da0)
struct Grid2 {
    char pad0[8];
    int dim[2];                         // +8, +0xc cells per axis
    float mn[2];                        // +0x10
    float mx[2];                        // +0x18
    float cell[2];                      // +0x20
    void walk(const float* a, const float* b, int (*cells)[2], int* count);   // 0x437290
};

// ---- time of day / misc free functions
float time_of_day();                    // 0x4356e0
bool is_daytime();                      // 0x435700
bool is_night();                        // 0x435740
float __cdecl accel_factor(const float* a, const float* b, const float* c);   // 0x435ae0
int __cdecl team_field5280(const Agent* a);                                   // 0x435b60

// ---- RecJ: the 0xb0-byte IfaceB-derived record (vtable 0x590b78, constructor 0x4377d0, slot1 0x437810): src/include/recj.h
#include "../../include/recj.h"

#endif // AGENT_AGENT_H

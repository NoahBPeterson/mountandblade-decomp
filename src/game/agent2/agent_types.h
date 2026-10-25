// agent_types.h: the classes shared by agent_stats.cpp (0x437830..0x43b100) and agent_ai.cpp (0x43b100..0x43e190): the
// 0x1c8-byte placed-object record (PlacedObj), the mission agent (Agent, 0x5928 bytes) with its combat-stat, aiming,
// target-picking, input-polling, animation and path-goal members, the mission object, the game data, the level
// (scene) and the navigation mesh, the animation controller (Skeleton, Channel) and the small std::vector /
// std::_Uninit_* instantiation helpers the linker kept. ONE definition of each class for this directory. Offsets are
// the ones read or written by a function of these two modules (the address of one such function is cited at the
// field); the rest of every object is padding. Names provisional.
//
// The classes shared with the other agent modules are one definition each in src/game: Agent / AgentBody / AgentTable /
// Threat (agent_obj.h), ItemRef (item_ref.h), Mission / TeamRec (mission.h), PlacedObj / IfaceB / Quad (placed_obj.h), NavMesh
// (nav_mesh.h), SkillCap (skill_cap.h); this header includes them. Curve (src/game/agent/curve.h) is the 0xd8-byte Channel here.
#ifndef AGENT2_AGENT_TYPES_H
#define AGENT2_AGENT_TYPES_H
#define HNODE_LOAD_ORDER_COPY                    // hnode.h: agent_stats.cpp 0x438b10 needs the user-written copy (before mission.h pulls hnode.h in)
#include <stddef.h>
#include <math.h>
#include <vector>
#include "rgl.h"
#include "../tstring.h"
#include "../mat4.h"
#include "../agent/item_kind.h"       // ItemKind, g_items, item type enum
#include "../../include/height_field.h"   // HeightField (line_clear 0x436da0, src/game/agent/agent.cpp)

using rgl::vector4;
using rgl::matrix;

struct shape_base; struct face; struct capsule; struct contact;

// ---- globals ------------------------------------------------------------------------------------------------
extern __int64 g_ticks[];              // 0x5e2948 clock table, 8 bytes per clock (0x43c9c0: g_ticks[2] is "now")
#include "../../include/fx.h"            // g_step_tbl 0x5a9c20 (entry 0 is the frame step)
extern float g_step_tbl[]; // 0x5a9c20 per-clock step (seconds; fx.h; named again here for the resolution audit, which reads only the headers a source includes itself)
#include "../dialog_manager.h"            // g_script 0xa70440
extern int g_agent_count;              // 0x5aa050 number of agents (0x4385e0)

// ---- polymorphic records derived from IfaceB (vtable 0x5908f8 = ??_7IfaceB@@6B@) --------------------------------
#include "../placed_obj.h"             // IfaceB, Quad, PlacedObj (shared with src/game/agent)
// the 0x1c8-byte placed scene object (vtable 0x590b84; PlacedObj of src/game/agent/agent.h, "Rec1c8" in
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
extern char g_range_tbl[];             // 0x590aac per-mode {lo, hi} float pairs of Agent::smooth_turn (0x438860)

// ---- the navigation mesh: src/game/nav_mesh.h ----
#include "../nav_mesh.h"

// ---- the level (scene: first field of the mission object) -----------------------------------------------------
#include "../cell.h"                                  // cell: one grid cell (0x34 bytes; collect_masked 0x4396b0, collect_point 0x439780)
#include "../light.h"                   // Light (shared with strat / script)
struct level {
    char pad0[8];
    int dim[2];                                         // 0x08 cells per axis
    float mn[2];                                        // 0x10
    float mx[2];                                        // 0x18
    float cellsz;                                       // 0x20
    char pad24[4];
    cell** cells;                                       // 0x28 cells[grid] -> array of cells
    char pad2c[0x64 - 0x2c];
    HeightField* hf;                                    // 0x64
    char pad68[0xb4 - 0x68];
    NavMesh* nav;                                       // 0xb4
    int lights_alloc;                                   // 0xb8 (std::vector<Light*>: allocator, first, last, end)
    Light** lights_first;                               // 0xbc
    Light** lights_last;                                // 0xc0
    void walk(const float* a, const float* b, int (*cells)[2], int* count);        // 0x437290 (src/game/agent)
    void to_cell(const float* p, int* out) const;                                    // 0x41ed00
    void query_box(const float* bmin, const float* bmax, unsigned mask, shape_base** out, int* count);   // 0x42aa50
    void remove_entity(int id);                                                     // 0x41f0d0
    void query_segment(const float* a, const float* b, unsigned mask, shape_base** out, int* count);   // 0x439870
    void nav_search(const float* p, const int* face, const int* max_depth, int depth, int* best, float* best_d);   // 0x439900
    int nearest_face(const float* p);                                               // 0x439a50
    void floor_at(const float* p, float* out_z, face** out_face);             // 0x439be0
    float floor_z(const float* p);                                                  // 0x439d40
    bool trace_ray(const float* a, const float* b, float maxlen, shape_base** hit, int unused);   // 0x439d60
    int locate_near(const vector4* pos, int* hint, int* mode);                      // 0x43abf0
    // 0x43ac40: vector4 arguments by value
    void light_at(vector4 p, vector4 n, vector4* out_dir, vector4* out_col, unsigned flags, float scale);
};

// ---- animation: the animator at body+0x304 (Skeleton), its channels (Curve in src/game/agent/curve.h) ---------------
struct Elem12 {                                         // 12-byte element of an animation's track list
    int a[3];
    vector4 sample(int idx) const;                       // 0x436ae0
};
struct Anim {                                           // object with a list of 12-byte tracks at +0x1c (0x439fc0)
    char pad0[0x18];
    std::vector<Elem12> elems;                          // 0x18 (first at 0x1c)
    vector4 sample(int idx) const;                       // 0x436b90
};
// SubAnim, AnimDef, DefList: src/game/agent_obj.h
#include "../channel.h"             // Channel (shared with agent3)
#include "../skeleton.h"                // Region, Skeleton, node_list, node_groups (shared with agent / agent3)

// ---- the mesh instance's node lists (0x438d80, 0x438f90, 0x439000) ---------------------------------------------
struct V32 { float x, y, z; float pad[5]; };            // 32-byte vertex record (x, y, z first)
struct vec_e32 {
    int al; V32* first; V32* last; V32* end;
    unsigned size() const { return first == 0 ? 0 : last - first; }
    V32& operator[](unsigned i) { return *(first + i); }
};
struct mnode {
    char pad0[0x18];
    mnode* master;                                      // 0x18
    char pad1c[0x70 - 0x1c];
    vec_e32 verts;                                      // 0x70
    char pad80[0xa1 - 0x80];
    char is_owner;                                      // 0xa1
    char pada2[2];
    struct { int a0; int* data; int count; } ints;      // 0xa4
    char pad_b0[0xc8 - 0xb0];
    float f_c8;                                         // 0xc8
    vec_e32* get_verts();                               // 0x40eba0
    void* get_ints();                                   // 0x40ec00
    void calc_bounds(vector4* mn, vector4* mx);         // 0x438d80
};

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
#include "../hnode.h"                             // hnode / node_gt (shared with agent4: NavMesh::open)
typedef PlacedObj E1c8;                                  // the 0x1c8-byte polymorphic record
struct allocE12  { void construct(E12* p, const E12& v); };             // 0x436cf0
struct allocstr  { void construct(tstring* p, const tstring& v); };     // 0x436c90
struct allocint  { void construct(int* p, const int& v); };             // 0x436d80
struct allocE1c8_def { void construct(E1c8* p, const E1c8& v); };       // 0x439040
struct allocE1c8 { void construct(E1c8* p, const E1c8& v); };           // 0x439040
struct vec12 : allocE12 {
    E12* first; E12* last; E12* end;
    E12* ufill(E12* f, unsigned n, const E12& v);       // 0x43a4a0
    E12* ucopy(E12* a, E12* b, E12* c);                 // 0x438c90
};
struct vecstr : allocstr {
    tstring* first; tstring* last; tstring* end;
    tstring* ucopy(tstring* f, tstring* l, tstring* d); // 0x43a4d0
    tstring* ufill(tstring* f, unsigned n, const tstring& v);   // 0x43b2b0
};
struct vecint : allocint { int* first; int* last; int* end; int* ucopy(int* f, int* l, int* d); };   // 0x43a4f0
struct vecE1c8 : allocE1c8 {
    E1c8* first; E1c8* last; E1c8* end;
    E1c8* ucopy(E1c8* f, E1c8* l, E1c8* d);             // 0x43b2e0
    E1c8* ufill(E1c8* f, unsigned n, const E1c8& v);    // 0x43db60
};
#include "../../include/ptr_vec.h"
struct vec1c8 {
    int al; E1c8* first; E1c8* last; E1c8* end;
    E1c8* at(int i);                                    // 0x438c10
};
#include "../int_vec.h"                                 // IntVec (was vec_int here: fill_ptr 0x438c30)
// vector<E12>::_Ucopy -> std::_Uninit_copy over the 12-byte records of the next object (0x436d10, src/game/agent)
E12* __cdecl uninit_copy12(E12* f, E12* l, E12* d, void* al, ptr_tag tag);
void __cdecl push_heap_node(hnode* first, int hole, int top, hnode val, node_gt pred);   // 0x4b2520
// the free functions of this object
void __cdecl uninit_fill_n12(E12* f, unsigned n, const E12& v, allocE12& al, ptr_tag tag);                       // 0x438960
E1c8* __cdecl copy_backward1c8(E1c8* f, E1c8* l, E1c8* d);                                                       // 0x4389d0
tstring* __cdecl uninit_copy_str(tstring* f, tstring* l, tstring* d, allocstr& al, ptr_tag tag);                 // 0x438a10
int* __cdecl uninit_copy_int(int* f, int* l, int* d, allocint& al, ptr_tag tag);                                 // 0x438aa0
void __cdecl adjust_heap_node(hnode* first, int hole, int bottom, hnode val, node_gt pred);                      // 0x438b10
void __cdecl fill1c8(E1c8* f, E1c8* l, const E1c8& v);                                                           // 0x438c60
void __cdecl uninit_fill_n_str(tstring* f, unsigned n, const tstring& v, allocstr& al, ptr_tag tag);             // 0x438f00
E1c8* __cdecl uninit_copy1c8(E1c8* f, E1c8* l, E1c8* d, allocE1c8& al, ptr_tag tag);                             // 0x439060
void __cdecl uninit_fill_n1c8(E1c8* f, unsigned n, const E1c8& v, allocE1c8& al, ptr_tag tag);                   // 0x43ab50

#endif // AGENT2_AGENT_TYPES_H

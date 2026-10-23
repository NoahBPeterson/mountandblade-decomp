// agent_types.h: the classes shared by agent_stats.cpp (0x437830..0x43b100) and agent_ai.cpp (0x43b100..0x43e190): the
// 0x1c8-byte placed-object record (PlacedObj), the mission agent (Agent, 0x5928 bytes) with its combat-stat, aiming,
// target-picking, input-polling, animation and path-goal members, the mission object, the game data, the level
// (scene) and the navigation mesh, the animation controller (Skeleton, Channel) and the small std::vector /
// std::_Uninit_* instantiation helpers the linker kept. ONE definition of each class for this directory. Offsets are
// the ones read or written by a function of these two modules (the address of one such function is cited at the
// field); the rest of every object is padding. Names provisional.
//
// Overlap with other headers (they define smaller or different views of the same objects under the same names and
// cannot be included together with this one): src/game/agent/agent.h (Agent, Mission, ItemRef, PlacedObj, Troop,
// AgentBody, Skeleton, KeyHolder, IfaceB, Quad), src/game/agent/phys_set.h (PhysRec, PhysSet, shape views),
// src/game/agent/curve.h (Curve = the 0xd8-byte Channel here), src/game/agent3/agent_types.h (Agent, Mission, world,
// AnimSlot, Skeleton, ...), src/game/ui/*.h. The layouts agree where they overlap (see MIGRATION.md).
#ifndef AGENT2_AGENT_TYPES_H
#define AGENT2_AGENT_TYPES_H
#include <stddef.h>
#include <math.h>
#include <vector>
#include "rgl.h"
#include "../tstring.h"
#include "../mat4.h"
#include "../agent/item_kind.h"       // ItemKind, g_items, item type enum

using rgl::vector4;
using rgl::matrix;

struct shape_base; struct face_shape; struct capsule; struct contact;

// ---- globals ------------------------------------------------------------------------------------------------
extern int g_tick_lo;                  // 0x5e2958 (g_time[2], 1e-5 s ticks)
extern int g_tick_hi;                  // 0x5e295c
extern __int64 g_ticks[];              // 0x5e2948 clock table, 8 bytes per clock (0x43c9c0: g_ticks[2] is "now")
extern float g_step;                   // 0x5a9c20 frame step (0x43bdb0)
extern int g_ev_a;                     // 0xa70458 event result slots (0x437b20)
extern int g_ev_b;                     // 0xa7045c
extern int g_agent_count;              // 0x5aa050 number of agents (0x4385e0)

// ---- polymorphic records derived from IfaceB (vtable 0x5908f8 = ??_7IfaceB@@6B@) --------------------------------
struct IfaceB {
    int f4, f8, fc;                    // +4..+0xc, copied by the copy constructor 0x437bb0
    IfaceB() {}
    virtual ~IfaceB() {}
    virtual void slot1() {}
    virtual void slot2() {}
};
struct Quad {
    int a, b, c, d;
    Quad() : a(2), c(g_tick_lo), d(g_tick_hi) {}
};

// the 0x1c8-byte placed scene object (vtable 0x590b84; PlacedObj of src/game/agent/agent.h, "Rec1c8" in
// src/game/agent3, "E1c8" in the std helper rows below). Constructor 0x437830, deleting destructor 0x437870
// (compiler-generated), copy constructor 0x437bb0, assignment 0x437d60; reset() is 0x4352f0 (src/game/agent).
struct PlacedObj : IfaceB {
    int m10, m14;                      // +0x10
    matrix a18, a58, a98, ad8, a118;   // +0x18 .. +0x157 (five 0x40-byte frames; 0x437bb0 copies each)
    vector4 v158;                      // +0x158
    int m168;                          // +0x168
    float f16c;                        // +0x16c
    Quad q170;                         // +0x170 (a = 2 on construction, c/d = clock tick)
    int m180, m184, m188, m18c;        // +0x180
    tstring name;                      // +0x190
    int m1a0, m1a4, m1a8, m1ac, m1b0, m1b4, m1b8, m1bc, m1c0, m1c4;   // +0x1a0
    PlacedObj();                       // 0x437830
    PlacedObj(const PlacedObj& o);     // 0x437bb0
    PlacedObj& operator=(const PlacedObj& o);   // 0x437d60
    // implicit virtual destructor: deleting destructor 0x437870
    void reset();                      // 0x4352f0 (src/game/agent/agent.cpp)
};

// ---- item reference (src/game/agent/agent.h has the full class): index + packed word --------------------------
struct ItemRef {
    int item;                          // +0 index into g_items, -1 = none
    unsigned m4;                       // +4 amount in the low byte, modifier in byte 3
    int max_amount();                  // 0x434b70
    int stat_1ec();                    // 0x434c50
    int stat_21x(int which);           // 0x434cb0
    int stat_1f0a();                   // 0x434d70
    int stat_1e8();                    // 0x434df0
    int stat_204();                    // 0x435090
    void set_modifier(int k);          // 0x438bc0 (stores the modifier in the top byte, then reduce(0))
    void reduce(int n);                // 0x437510 (src/game/agent; 0x438520 inlines its body with n == 0)
};

// ---- 2-D helpers --------------------------------------------------------------------------------------------
struct vec2 { float x, y; };
float __cdecl signed_angle(const vec2* a, const vec2* b);       // 0x4381e0 (also the "blend_target" of the slices)
float __cdecl clamp_ref(const float* v, const float* lo, const float* hi);   // 0x4325a0 (earlier object)
float __cdecl fmin_ref(const float* a, const float* b);         // 0x4324b0
float __cdecl fmax_ref(const float* a, const float* b);         // 0x4324d0
void __cdecl noise_gradient(float scale, const float* in, float* out);   // 0x414ae0
extern char g_range_tbl[];             // 0x590aac per-mode {lo, hi} float pairs of Agent::smooth_turn (0x438860)

// ---- the navigation mesh ------------------------------------------------------------------------------------
struct NavVert { float x, y, z; float pad[3]; };        // 0x18 bytes
struct NavFace {                                        // 0x64 bytes: a convex polygon of up to 4 vertices
    int stamp;                                          // 0x00 visit stamp (compared with NavMesh::stamp) (0x439900)
    char pad04[0x18 - 4];
    float nx, ny, nz;                                   // 0x18 plane normal (nz is the divisor of the height query)
    char pad24[4];
    int vert_[4];                                       // 0x28 vertex indices
    int edge_[4];                                       // 0x38 edge ids
    int nverts;                                         // 0x48
    int nbr_[4];                                        // 0x4c neighbour face across edge i (< 0: none)
    char pad5c[8];
    // the originals index these through plain pointers (cl unrolls the loops when it sees the array bound)
    int* vert_p() { return (int*)((char*)this + 0x28); }
    int* edge_p() { return (int*)((char*)this + 0x38); }
    int* nbr_p() { return (int*)((char*)this + 0x4c); }
};
struct NavMesh {
    int stamp;                                          // 0x00 current visit stamp (0x439410)
    char pad04[0x14];
    std::vector<NavVert> verts;                         // 0x18
    std::vector<NavFace> faces;                         // 0x28
    bool inside(const int* face, const float* p);       // 0x4391d0 point inside the polygon (xy)
    float dist(const int* face, const float* p);        // 0x439280 xy distance outside the polygon + |dz| to its plane
    void next_stamp();                                  // 0x439410
    bool trace(int face, vector4 from, int end_face, const float* to, int* max_steps);   // 0x439460 (vector4 by value)
};

// ---- the level (scene: first field of the mission object) -----------------------------------------------------
struct cell {                                           // one grid cell (0x34 bytes)
    std::vector<shape_base*> shapes;                    // 0x00
    char pad10[0x24];
    void collect_masked(unsigned mask, shape_base** out, int* count);                                     // 0x4396b0
    void collect_point(const float* p, int* use3d, unsigned mask, shape_base** out, int* count);          // 0x439780
};
struct HeightField {
    bool line_clear(const vector4* a, const vector4* b);        // 0x436da0 (src/game/agent/agent.cpp)
};
struct Light {
    int type;                                           // +0 (2 = directional) (0x43ac40)
    float r, g, b;                                      // +4 / +8 / +0xc
    char pad10[0x24 - 0x10];
    vector4 pos;                                        // +0x24
    char pad34[0x44 - 0x34];
    vector4 dir;                                        // +0x44 (directional lights)
    float range;                                        // +0x54
    char pad58[0x6c - 0x58];
    unsigned flags;                                     // +0x6c (bit 0: half intensity)
};
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
    void locate(const float* p, int* out) const;                                    // 0x41ed00
    void query_box(const float* bmin, const float* bmax, unsigned mask, shape_base** out, int* count);   // 0x42aa50
    void remove_entity(int id);                                                     // 0x41f0d0
    void query_segment(const float* a, const float* b, unsigned mask, shape_base** out, int* count);   // 0x439870
    void nav_search(const float* p, const int* face, const int* max_depth, int depth, int* best, float* best_d);   // 0x439900
    int nearest_face(const float* p);                                               // 0x439a50
    void floor_at(const float* p, float* out_z, face_shape** out_face);             // 0x439be0
    float floor_z(const float* p);                                                  // 0x439d40
    bool trace_ray(const float* a, const float* b, float maxlen, shape_base** hit, int unused);   // 0x439d60
    int locate_near(const vector4* pos, int* hint, int* mode);                      // 0x43abf0
    // 0x43ac40: vector4 arguments by value
    void light_at(vector4 p, vector4 n, vector4* out_dir, vector4* out_col, unsigned flags, float scale);
};

// ---- animation: the animator at body+0x304 (Skeleton), its channels (Curve in src/game/agent/curve.h) ---------------
struct Elem12 {                                         // 12-byte element of an animation's track list
    int a[3];
    vector4 point(int idx) const;                       // 0x436ae0
};
struct Anim {                                           // object with a list of 12-byte tracks at +0x1c (0x439fc0)
    char pad0[0x18];
    std::vector<Elem12> elems;                          // 0x18 (first at 0x1c)
    vector4 point(int idx) const;                       // 0x436b90
};
struct SubAnim {                                        // 0x40 bytes: one sub-animation request (AnimCmd)
    const void* anim;                                   // +0 (0x43a980)
    char pad4[4];
    unsigned flags;                                     // +8 (0x10000000 = ...)
    float scale;                                        // +0xc
    int a, b;                                           // +0x10 / +0x14
    char pad18[0x2c - 0x18];
    float v2c, v30, v34, v38;                           // +0x2c (vector4, 0x43d040)
    float offset;                                       // +0x3c
};
struct AnimDef {                                        // 0x30 bytes
    char pad0[0x10];
    SubAnim* subs;                                      // +0x10
    char pad14[4];
    unsigned flags;                                     // +0x18
    char pad1c[0x2c - 0x1c];
    int f2c;                                            // +0x2c
};
struct DefList { AnimDef* defs; int count; };           // +0x12c of the agent
struct Channel {                                        // 0xd8 bytes (Curve of src/game/agent/curve.h)
    char pad0[8];
    unsigned flags;                                     // +8 (bone mask set)
    int fc;                                             // +0xc
    unsigned mask_lo, mask_hi;                          // +0x10 / +0x14 (bone mask bits)
    char pad18[0x38 - 0x18];
    float blend;                                        // +0x38 (0x43a980)
    int f3c;                                            // +0x3c
    char pad40[0x44 - 0x40];
    int k44, k48, k4c, k50;                             // +0x44 (0x439fc0: 0x33, 0x33, a, b)
    char pad54[8];
    Anim* anim;                                         // +0x5c
    vector4* pa;                                        // +0x60 (one point per track)
    vector4* pb;                                        // +0x64
    float* xform;                                       // +0x68 (quaternion blocks at +0, +0x70, +0x80, ...)
    float v6c[4];                                       // +0x6c
    vector4 va, vb;                                     // +0x7c, +0x8c
    char pad9c[0xa0 - 0x9c];
    float period;                                       // +0xa0
    char pada4[4];
    int mode;                                           // +0xa8
    char padac[0xc0 - 0xac];
    float w0, w1;                                       // +0xc0 / +0xc4
    char padc8[0xcc - 0xc8];
    int rate;                                           // +0xcc
    char padd0[0xd8 - 0xd0];
    bool busy();                                        // 0x433840 (Curve::step_reached)
    void set_phase(float period);                       // 0x433930
};
struct Region {                                         // 0x1dc bytes: one bone record
    char pad0[0x1b0];
    std::vector<shape_base*> shapes;                    // 0x1b0
    char pad1c0[0x1d0 - 0x1c0];
    int f1d0;                                           // 0x1d0 (cleared by 0x43d040)
    char pad1d4[0x1dc - 0x1d4];
};
struct Skeleton {                                       // the animation controller (Group of the slices)
    int nbones;                                         // +0
    Region* recs;                                       // +4
    int* sel;                                           // +8 -> ints at +0x28/+0x2c/+0x30 (bone-mask selectors)
    char pad0c[0x30 - 0xc];
    std::vector<shape_base*> shapes;                    // +0x30 (0x439ec0)
    Channel* chans;                                     // +0x40
    void remove_shape(shape_base* s);                   // 0x439ec0
    void start_anim(int idx, Anim* anim, int a, int b); // 0x439fc0 (thiscall, ret 0x10)
    void all_shapes(shape_base** out, int* count);      // 0x43a130
    void reset_rec(int i);                              // 0x4339d0
    float rec_phase(int i);                             // 0x433ac0
    void set_rec_phase(int i, float p);                 // 0x433b20
};

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
struct node_list {
    int al; mnode** first; mnode** last; mnode** end;
    void cap_all(const float* limit);                   // 0x438f90
};
struct node_groups {
    char pad0[0x34];
    node_list lists[5];                                 // 0x34 (16 bytes each)
    int nlists;                                         // 0x84
    void cap_all(const float* limit);                   // 0x439000
};

// ---- the agent --------------------------------------------------------------------------------------------------
// 0x18-byte entry of the sorted nearest-enemy list (0x43b300)
struct Threat { int id; float dist; vector4 dir; };
// animation / order slot: 2 x 0x2c bytes at agent+0xd4
struct AnimSlot {
    int cur;                                            // +0 current animation id (-1 none)
    int next;                                           // +4
    int sub;                                            // +8
    unsigned prev_mask;                                 // +0xc
    int f10;                                            // +0x10
    unsigned events;                                    // +0x14 priority word (low 17 bits)
    float scale;                                        // +0x18
    float f1c;                                          // +0x1c
    float rate;                                         // +0x20
    float phase;                                        // +0x24 (>= 0: pending phase for set_rec_phase)
    int f28;                                            // +0x28
    __declspec(noinline) bool set(int id, unsigned prio);   // 0x4378a0
};
struct Timer {
    int clock;
    int pad4;
    __int64 start;
    float elapsed();                                    // 0x432c10 (thiscall)
};
struct InputRec { unsigned bits; char pad[0x14]; unsigned lo; int hi; };   // queued input record (+0x18 time)
struct InputState { unsigned v[6]; };
struct Mesh { char pad[0xcc]; int f_cc; };              // quiver / holster display mesh
struct AgentBody {
    int f0, f4;
    int id;                                             // +8 scene entity id (0x43a6f0)
    char padc[0x58 - 0xc];
    rgl::matrix m;                                      // +0x58: s, f, u, o rows of the body frame (0x43b7b0, 0x43cc30)
    char pad98[0x304 - 0x98];
    Skeleton* skel;                                     // +0x304
};

struct Agent {                                          // 0x5928 bytes
    char pad0[4];
    int used;                                           // +4 (0x43b160)
    int id;                                             // +8 side / agent id (0x43b300)
    char padc[4];
    int f10;                                            // +0x10 (1 = non-human)
    int f14;                                            // +0x14 (1 = active)
    char pad18[0x2c - 0x18];
    int f2c;                                            // +0x2c mount id, -1 = on foot (0x437f00)
    int f30;                                            // +0x30 (0x43b8a0)
    char pad34[0x50 - 0x34];
    vector4 pos;                                        // +0x50
    vector4 vel;                                        // +0x60 move direction (0x4378f0, 0x438860, 0x43cc30)
    char pad70[0x88 - 0x70];
    int troop;                                          // +0x88 troop index
    ItemRef f8c;                                        // +0x8c (0x43b8a0)
    char pad94[0xa8 - 0x94];
    vec2 vel_xy;                                        // +0xa8 (0x43b7b0, 0x43bdb0)
    char padb0[0xc4 - 0xb0];
    vector4 nav_pos;                                    // +0xc4 (0x43cb30)
    AnimSlot anim[2];                                   // +0xd4 (stride 0x2c; anim[1].cur = +0x100)
    DefList* defs;                                      // +0x12c
    char pad130[0x14c - 0x130];
    Mesh* links[4];                                     // +0x14c (0x43a800, 0x4379c0)
    char pad15c[0x16c - 0x15c];
    node_groups* disp[4];                               // +0x16c quiver display (0x43a800)
    char pad17c[0x1c4 - 0x17c];
    int team;                                           // +0x1c4
    char pad1c8[4];
    unsigned char f1cc;                                 // +0x1cc (0x43a510)
    char pad1cd[3];
    ItemRef pairs[11];                                  // +0x1d0 equipped items
    int pad228;
    int slots[5];                                       // +0x22c (slots[0] = current, slots[4] at +0x23c)
    char pad240[0x24c - 0x240];
    float f24c;                                         // +0x24c (0x438180)
    AgentBody* body;                                    // +0x250
    char pad254[0x25c - 0x254];
    vector4 goal;                                       // +0x25c (0x43db90)
    vector4 goal_dir;                                   // +0x26c
    int goal_face;                                      // +0x27c
    char pad280[4];
    int goal_changed;                                   // +0x284
    char pad288[0x318 - 0x288];
    int f318;                                           // +0x318
    char pad31c[4];
    int path_face[0x405];                               // +0x320
    vector4 path_pos[0x3ff];                            // +0x1334
    int path_active;                                    // +0x5324
    int path_cur;                                       // +0x5328
    int path_count;                                     // +0x532c
    char path_flag;                                     // +0x5330
    char pad5331[0x5338 - 0x5331];
    int path_state;                                     // +0x5338
    char pad533c[0x5340 - 0x533c];
    float f5340, f5344, f5348, f534c;                   // +0x5340 (0x43a510)
    char pad5350[0x53a4 - 0x5350];
    InputState in_prev;                                 // +0x53a4
    char pad53bc[0x53c4 - 0x53bc];
    InputRec** q_buf;                                   // +0x53c4
    unsigned q_cap, q_head, q_count;                    // +0x53c8
    InputState in_last;                                 // +0x53d4
    Threat threats[16];                                 // +0x53ec (sorted by distance, nearest first)
    int nthreat;                                        // +0x556c
    char pad5570[0x55d8 - 0x5570];
    float fx, fy, fz, fw;                               // +0x55d8 facing direction (0x43b8a0, 0x43bb80, 0x43bdb0; 16 bytes copied by 0x43bdb0)
    int f55e8;                                          // +0x55e8
    char pad55ec[0x560c - 0x55ec];
    int active;                                         // +0x560c
    int f5610;                                          // +0x5610
    char pad5614[8];
    int f561c;                                          // +0x561c (default order id)
    char pad5620[4];
    vector4 prev_facing;                                // +0x5624 (0x43bdb0)
    float aim_error;                                    // +0x5634
    char pad5638[0x5640 - 0x5638];
    int target_id;                                      // +0x5640 (0x43cc30)
    char pad5644[0x5664 - 0x5644];
    float move_x, move_y, move_z;                       // +0x5664
    char pad5670[4];
    int weapon_kind;                                    // +0x5674
    char pad5678[0x56b4 - 0x5678];
    vector4 v56b4;                                      // +0x56b4 (0x43db90)
    int variant;                                        // +0x56c4 (range table index, 0x438860)
    float cur;                                          // +0x56c8 eased value (0x438860); the angle of 0x43d040
    float f56cc;                                        // +0x56cc (0x43d040)
    char pad56d0[0x56e4 - 0x56d0];
    float f56e4;                                        // +0x56e4
    char pad56e8[0x57f0 - 0x56e8];
    Timer timer;                                        // +0x57f0 (0x43bdb0)
    char pad5800[0x5830 - 0x5800];
    int f5830;                                          // +0x5830
    char pad5834[0x587c - 0x5834];
    int nav_face;                                       // +0x587c
    int disabled;                                       // +0x5880
    char pad5884[0x5928 - 0x5884];

    // 0x432790: the item reference of equipment slot idx, by hidden pointer. An inline function of the shared
    // header: the linker kept the first object's copy (this object calls it at 0x4385e0 and inlines the same
    // body at its other uses, slot_pair_inl / cur_pair).
    __declspec(noinline) ItemRef slot_pair(int idx) const;
    ItemRef slot_pair_inl(int idx) const
    {
        const ItemRef* p = 0;
        int i = slots[idx];
        if (i >= 0 && (idx == 0 || slots[4] == 0)) p = &pairs[i];
        ItemRef r;
        r.item = -1;
        if (p) {
            r.item = p->item;
            r.m4 = p->m4;
        }
        return r;
    }
    ItemRef cur_pair() const
    {
        const ItemRef* p = 0;
        if (slots[0] >= 0) p = &pairs[slots[0]];
        ItemRef r;
        r.item = -1;
        if (p) {
            r.item = p->item;
            r.m4 = p->m4;
        }
        return r;
    }
    ItemRef best_ammo(int idx, int* missing, int* total, int* slot_out);   // 0x436880 (src/game/agent)
    int pick_slot_anim(int slot);                       // 0x433040 (earlier object)

    void get_dir(vec2* out);                            // 0x4378f0
    inline void set_order(int slot, int id, unsigned prio, float rate);   // 0x4379c0
    bool has_weapon_for(int mode);                      // 0x437f00
    float stamina_scale();                              // 0x438180
    void set_default_order();                           // 0x4382e0
    float attack_speed();                               // 0x438360
    void clear_ammo(int unused);                        // 0x438520
    float damage_factor(int idx);                       // 0x4385e0
    void smooth_turn(float dt);                         // 0x438860
    void hand_pos(vector4* out);                        // 0x43a510
    void release_body();                                // 0x43a6f0
    void update_quiver();                               // 0x43a800
    void start_channel(int idx, SubAnim* cmd, unsigned* in_flags, float* base);   // 0x43a980 (ret 0x10)
    void collect_enemies(int unused);                   // 0x43b300 (ret 4)
    unsigned pick_target();                             // 0x43b4d0
    unsigned pick_random();                             // 0x43b5d0
    vector4* local_velocity(vector4* out);              // 0x43b7b0
    unsigned find_talk_target();                        // 0x43b8a0
    unsigned pick_front();                              // 0x43b9e0
    unsigned pick_duel();                               // 0x43bb80
    void update_aim_error();                            // 0x43bdb0
    void poll_input(int unused, InputState* out);       // 0x43c9c0 (ret 8)
    int update_nav_face();                              // 0x43cb30
    int move_anim(int mode);                            // 0x43cc30
    void update_anims(int unused);                      // 0x43d040 (ret 4)
    void set_goal_agent(unsigned id);                   // 0x43db90
    void set_goal_point(vector4 p, int face);           // 0x43dcb0
    void set_goal_near(const vector4* center, const float* radius);   // 0x43dd70
    void advance_path();                                // 0x43df60
};
float __cdecl hit_chance(int* who, Agent* a);           // 0x438cb0

// ---- the mission object (g_mission 0x5e292c) and the game data (g_game 0x5e2930) ---------------------------------
struct AgentTable {                                     // mission+4
    int count;                                          // +0
    char pad4[0x14 - 4];
    std::vector<Agent*> pages;                          // +0x14 (first at +0x18; id >> 4 -> page of 16 agents)
    void next_used(unsigned* i);                        // 0x43b160
    int first_used();                                   // 0x43b1b0
    __declspec(noinline) Agent* get(unsigned id);       // 0x43b1f0
};
struct HandRec {                                        // 0x6c bytes: per-weapon-class hand record, 3 per class
    int f0, f4;
    int a, b;                                           // +8 / +0xc (game options per class, 0x437f00)
    vector4 pos;                                        // +0x10
    char pad20[0x34 - 0x20];
    float dx, dy;                                       // +0x34 / +0x38
    char pad3c[0x58 - 0x3c];
    int mode;                                           // +0x58
    char pad5c[0x6c - 0x5c];
};
struct TeamRec { HandRec hand[3]; int rel[10]; };       // 0x16c bytes (rel[b]: relation to team b, 0x43b300)
struct Mission {
    level* scene;                                       // +0
    AgentTable agents;                                  // +4 (page table: +0x18)
    char pad28[0x5c - 0x28];
    int f5c;                                            // +0x5c
    char pad60[0x5284 - 0x60];
    TeamRec teams[8];                                   // +0x5284 (stride 0x16c; rel[] at +0x144 is +0x53c8: -1 hostile, 1 friend)
    char pad5de4[0x653c - (0x5284 + 8 * 0x16c)];
    float min_x, min_y, max_x, max_y;                   // +0x653c play-area bounds
    char pad654c[0x6604 - 0x654c];
    int f6604;                                          // +0x6604
    char pad6608[0x6698 - 0x6608];
    NavMesh nav;                                        // +0x6698
};
extern Mission* g_mission;                              // 0x5e292c

struct TroopRec {                                       // 0x508 bytes (array at game+0x13a00)
    tstring id;                                         // +0 (0x43a480)
    char pad10[0x20 - 0x10];
    tstring name;                                       // +0x20 (key into the translation table)
    unsigned __int64 flags;                             // +0x30
    char pad38[0x58 - 0x38];
    int lvl58;                                          // +0x58
    char pad5c[0x68 - 0x5c];
    unsigned nib[6];                                    // +0x68 packed 4-bit skill levels, 8 per word (0x437b20)
    float f80[8];                                       // +0x80 per-class proficiencies (0x4385e0, 0x43bdb0)
    char pada0[0x508 - 0xa0];
    tstring* display_name();                            // 0x43a480
};
struct SkillCap { char pad[0x34]; int cap; char pad2[0x44 - 0x38]; };   // 0x44 bytes, table at 0x8ee3100
extern SkillCap g_skill_tbl[];                          // 0x8ee3100
struct GameData {
    char pad0[0x13a00];
    TroopRec* chars;                                    // +0x13a00 (stride 0x508)
    int skill_level(int troop, int skill);              // 0x437b20 (thiscall, ret 8)
    char emit_event(int id, int a, int b);              // 0x435790 (ret 0xc; src/game/agent)
};
extern GameData* g_game;                                // 0x5e2930


// ---- STL instantiation helpers (VC7.1 <vector>/<memory>/<algorithm> code the linker kept from this object) ------
struct ptr_tag {};     // std::_Nonscalar_ptr_iterator_tag: empty class passed by value; an UNINITIALISED local pushes whatever EAX holds
struct E12 { int a[3]; };                                // 12-byte element
struct hnode {                                           // 12-byte heap node (key float at +8); user-written copy (load order)
    int a, b; float key;
    hnode() {}
    hnode(const hnode& o) : a(o.a), b(o.b), key(o.key) {}
    hnode& operator=(const hnode& o) { a = o.a; b = o.b; key = o.key; return *this; }
};
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
struct ptr_vec {
    int al; int* first; int* last; int* end;
    unsigned size() const { return first == 0 ? 0 : last - first; }
    bool empty() const { return size() == 0; }
    void pop_back_if();                                 // 0x438bf0
};
struct vec1c8 {
    int al; E1c8* first; E1c8* last; E1c8* end;
    E1c8* at(int i);                                    // 0x438c10
};
struct vec_int {
    int al; int* first; int* last; int* end;
    int* fill_ptr(int* f, unsigned n, const int& x);    // 0x438c30
};
struct node_gt { bool operator()(const hnode& a, const hnode& b) const { return a.key > b.key; } };
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

// agent.h: the classes of agent.cpp (0x434b20..0x437830): item references with their modifier-adjusted stats, the
// troop record's small accessors, the agent's camera / steering / equipment members, the mission object's
// accessors, the placed-object record and the small container / math helpers. Offsets are the ones read by a
// function of this module (address cited at each field); the rest of every object is padding. All names are
// provisional. The item definition record is in item_kind.h, the physics set in phys_set.h.
#ifndef AGENT_AGENT_H
#define AGENT_AGENT_H
#include <math.h>
#include "rgl.h"
#include "../tstring.h"
#include "../mat4.h"
#include "./item_kind.h"
#include "./curve.h"        // quaternion (w, x, y, z)

using rgl::vector4;
using rgl::matrix;

// item modifiers (Module System header_item_modifiers.py)
enum {
    imod_plain = 0, imod_cracked, imod_rusty, imod_bent, imod_chipped, imod_battered, imod_poor, imod_crude, imod_old,
    imod_cheap, imod_fine, imod_well_made, imod_sharp, imod_balanced, imod_tempered, imod_deadly, imod_exquisite,
    imod_masterwork, imod_heavy, imod_strong, imod_powerful, imod_tattered, imod_ragged, imod_rough, imod_sturdy,
    imod_thick, imod_hardened, imod_reinforced, imod_superb, imod_lordly, imod_lame, imod_swaybacked, imod_stubborn,
    imod_timid, imod_meek, imod_spirited, imod_champion, imod_fresh, imod_day_old, imod_two_day_old, imod_smelling,
    imod_rotten, imod_large_bag
};

// ---- item reference: item index + packed word (amount in the low byte, proficiency in the low 16 bits for
// weapons, modifier in byte 3). 8 bytes, returned by value through a hidden pointer (0x4351f0, 0x432790).
// The slices of this range called it `Pair` (s00435490, s00435c10, s004362e0) and `ItemRef` (s00434ad0): one class.
struct ItemRef {
    int item;                           // +0 index into g_items, -1 = none (0x434b70, 0x435fc0)
    union {
        unsigned m4;                    // +4 (0x434be0: amount in the low byte)
        struct { unsigned char amount; unsigned char pad[2]; signed char mod; } b;   // mod: byte 7 (0x434c50)
    };
    tstring mesh_name();                // 0x434b20
    int max_amount() const;             // 0x434b70
    void add_amount(int n);             // 0x434be0
    int room_left();                    // 0x434c30
    int stat_1ec();                     // 0x434c50
    unsigned stat_21x(int which);       // 0x434cb0
    int stat_1f0a();                    // 0x434d70
    int stat_1e8();                     // 0x434df0
    int stat_200();                     // 0x434eb0
    int stat_1f4();                     // 0x434f60
    int stat_1f0b();                    // 0x434fd0
    int stat_214();                     // 0x435040
    int stat_204();                     // 0x435090
    void reduce(int n);                 // 0x437510
};
int round_float(float* f);              // 0x432540 (cdecl): rounds a float to an int
int slot_class(int item, int flag);     // 0x4350c0 (register convention, defined static in agent.cpp)

// named record tables (0x435260, 0x4352a0)
struct Rec20 { tstring name; char pad[0x10]; };      // 0x20-byte named record (table g_tbl20)
struct Rec78 { tstring name; char pad[0x68]; };      // 0x78-byte named record (table g_tbl78)
extern Rec20* g_tbl20;                  // 0x5e28f8
extern int g_tbl20_count;               // 0x5e29e8
extern Rec78* g_tbl78;                  // 0x5e29dc
extern int g_tbl78_count;               // 0x5e29e0
int find_in_table20(const tstring* name);   // 0x435260 (cdecl)
int find_in_table78(const tstring* name);   // 0x4352a0 (cdecl)
int ftoi_helper(int x);                     // 0x435440

// ---- troop record (0x508 bytes, array at world+0x13a00): packed 4-bit skill levels and per-slot values
// (the address comments below stay bare: the resolver reads the last 0x... of a declaration's comment)
struct SkillCap { char pad[0x34]; int cap; char pad38[0x44 - 0x38]; };   // 0x44 bytes, cap at +0x34 (read by Troop::nibble)
extern SkillCap g_skill_tbl[];          // 0x8ee3100
extern int g_clamp_level;               // 0x8ee3860
extern const int g_tier_dist[];         // 0x590a70
struct Troop {
    char pad0[0x30];
    unsigned __int64 flags30;           // +0x30 (0x435130: bit 0x10)
    char pad38[0x68 - 0x38];
    unsigned nib[6];                    // +0x68 packed 4-bit levels, 8 per word (0x4351a0); nib[3] bits 12..15 = a skill tier (0x4375b0)
    float f80[8];                       // +0x80 per-slot values (0x435150 truncates one, 0x437770 adjusts one)
    char padA0[0xb0 - 0xa0];
    int b0;                             // +0xb0 (0x435170)
    bool has_flag10();                  // 0x435130
    int stat_int(int i);                // 0x435150
    int scaled_b0();                    // 0x435170
    int nibble(int i);                  // 0x4351a0
    float falloff(float dist, float k); // 0x4375b0
    void adjust(int i, float k);        // 0x437770
};

// ---- two item references at +0x80 of the record 0x4351f0 reads (a different object than Troop: ints, not floats)
struct Wielder {
    char pad0[0x80];
    ItemRef a, b;                       // +0x80 / +0x88
    ItemRef ranged_ref();               // 0x4351f0
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
struct TeamSlot { int a, b, c, d, e, f; };       // 0x18 (0x435b90)
struct ModeRec { char pad[0x20]; unsigned flags; char pad2[0x58 - 0x24]; };   // 0x58, array g_modes
extern ModeRec* g_modes;                         // 0x5e2a00
struct Mission {
    Level* scene;                                // +0 (0x4417d0)
    char pad4[0x5c - 4];
    int f5c;                                     // +0x5c (0x435f10)
    char pad60[0x5280 - 0x60];
    int field5280[1];                            // +0x5280, stride 0x16c per team (0x435b60)
    char pad5284[0x53c8 - 0x5284];
    int relation[1];                             // +0x53c8, stride 0x16c per team (0x435b90)
    char pad53cc[0x5df0 - 0x53cc];
    int f5df0;                                   // +0x5df0 index into g_modes (0x435840)
    char pad5df4[0x622c - 0x5df4];
    TeamSlot slots[8];                           // +0x622c (0x435b90)
    char mode_flag1() const;                     // 0x435840
};
extern Mission* g_mission;                       // 0x5e292c

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
struct Obj_a0 { char pad[0xa0]; vector4 d; };    // +0xa0 direction vector (0x436130)
#include "../skeleton.h"                // BoneRec, Skeleton (shared with agent2 / agent3)
struct AgentBody {
    char pad0[0x58];
    rgl::mat4 m;                                 // +0x58 (0x4417d0)
    char pad98[0x304 - 0x98];
    Skeleton* skel;                              // +0x304 (0x435c10)
};
struct CamParams {                               // camera parameter block written by Agent::camera_params
    char pad[0x40];
    vector4 v40;                                 // +0x40 camera position offset
    vector4 v50;                                 // +0x50 target offset
    float f60;                                   // +0x60
};
#include "../../include/vec.h"

struct Agent {
    char pad0[4];
    int f4;                                      // +4 (0x4355f0)
    int f8;                                      // +8 (0x435f10: compared with Mission::f5c)
    char padc[0x10 - 0xc];
    int f10;                                     // +0x10 (0x435c10, 0x4355f0 reads f4/f10)
    int f14;                                     // +0x14 (0x435c10)
    char pad18[0x2c - 0x18];
    int f2c;                                     // +0x2c (0x435c10, 0x4362e0)
    char pad30[0x60 - 0x30];
    vector4 vel;                                 // +0x60 velocity (0x435f10)
    char pad70[0x80 - 0x70];
    float f80;                                   // +0x80 (0x435c10)
    char pad84[0x98 - 0x84];
    vector4 v98;                                 // +0x98 (0x435c10)
    char pada8[0xbc - 0xa8];
    Obj_a0* fbc;                                 // +0xbc (0x436130)
    char padc0[0xd4 - 0xc0];
    int fd4;                                     // +0xd4 (0x4354d0)
    char padd8[0xe8 - 0xd8];
    unsigned fe8;                                // +0xe8 (0x4354d0, 0x435c10)
    char padec[0x114 - 0xec];
    unsigned f114;                               // +0x114 (0x4354d0)
    char pad118[0x12c - 0x118];
    struct TroopDef { char pad[0x18]; unsigned flags; char pad2[0x30 - 0x1c]; } **f12c;   // +0x12c (0x4354d0)
    char pad130[0x1c4 - 0x130];
    int team;                                    // +0x1c4 (0x435490, 0x435b60)
    char pad1c8[0x1d0 - 0x1c8];
    ItemRef pairs[11];                           // +0x1d0 equipped items (0x432790: 8-byte entries)
    int pad228;
    int slots[5];                                // +0x22c (slots[1] at +0x230, slots[4] at +0x23c; 0x432790)
    char pad240[0x250 - 0x240];
    AgentBody* body;                             // +0x250 (0x435c10, 0x4417d0)
    char pad254[0x2c8 - 0x254];
    int f2c8;                                    // +0x2c8 (0x435b90)
    char pad2cc[0x533c - 0x2cc];
    int f533c;                                   // +0x533c (0x435b90)
    char pad5340[0x55d8 - 0x5340];
    float dirx, diry, dirz;                      // +0x55d8 (0x435f10)
    char pad55e4[0x56c4 - 0x55e4];
    int f56c4;                                   // +0x56c4 (0x435520)
    char pad56c8[4];
    float f56cc;                                 // +0x56cc (0x436130)
    char pad56d0[0x57d8 - 0x56d0];
    char f57d8;                                  // +0x57d8 (0x435c10)
    char pad57d9[0x5830 - 0x57d9];
    int f5830;                                   // +0x5830 (0x435c10)
    char pad5834[0x58dc - 0x5834];
    int phys_idx;                                // +0x58dc (0x4417d0)
    vector4 anchor;                              // +0x58e0 (0x4417d0)

    ItemRef slot_pair(int idx) const;            // 0x432790 (earlier object)
    inline ItemRef slot_pair_inl(int idx) const; // same body, inlined by the compiler at some call sites
    int clamped_team() const;                    // 0x435490
    bool flag_0x400_or_0x114() const;            // 0x4354d0
    void classify(int key);                      // 0x435520
    float scale_f4() const;                      // 0x4355f0
    float scale_f10() const;                     // 0x435620
    void reset_if_enemy_charged();               // 0x435b90
    void camera_params(CamParams* p);            // 0x435c10
    void steer(float dt);                        // 0x435f10
    ItemRef best_weapon_slot() const;            // 0x435fc0
    float equipment_value() const;               // 0x436050
    void update_pitch(float dt);                 // 0x436130
    void pick_sounds(int key, int mode, int* snd_a, int* snd_b, int* snd_c, int* variant);   // 0x4362e0
    void weapon_info(int idx, unsigned* skill, int* stat);                                   // 0x436810
    ItemRef best_ammo(int idx, int* missing, int* total, int* slot_out);                     // 0x436880
    int total_missing_ammo();                                                                // 0x436a70
    void build_static_collision();               // 0x4417d0 (phys_set.cpp)
};

inline ItemRef Agent::slot_pair_inl(int idx) const
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

// ---- key-frame sampling (0x436ae0, 0x436b90)
void slerp(quaternion* out, const quaternion* a, const quaternion* b, const float* t);      // 0x4238c0
struct KeyTable {
    quaternion* vals; int* keys; int count;
    void find(int* cur, float* frac, float* key);               // 0x423a50
    quaternion sample(int key);                                 // 0x436ae0 (hidden return pointer)
};
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
struct rec36 { int a[9]; };         // 0x24 bytes
struct rec136 { int a[34]; };       // 0x88 bytes
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

// ---- polymorphic records derived from IfaceB (vtable 0x5908f8 = ??_7IfaceB@@6B@, s00431050). IfaceB carries three
// words at +4..+0xc (copied by the 0x1c8-byte record's copy constructor 0x437bb0, next object).
// RecJ: 0xb0-byte record, vtable 0x590b78: slot 0 = vector deleting dtor 0x431750 (s00431050), slot 1 = reset().
struct RecJ : IfaceB {
    int pad10[(0x50 - 0x10) / 4];
    int m50;                                    // +0x50 (0x437810)
    int pad54[(0x70 - 0x54) / 4];
    Quad q70;                                   // +0x70 (a = 2, c/d = clock tick copied from g_time[2]; ctor 0x4377d0)
    int m80;                                    // +0x80 (0x437810)
    int pad84;
    int m88;                                    // +0x88 (0x437810)
    int pad8c[(0x98 - 0x8c) / 4];
    int m98;                                    // +0x98 (0x437810)
    int pad9c[(0xb0 - 0x9c) / 4];
    RecJ();                                     // 0x4377d0
    virtual void slot1();                       // reset: 0x437810
};

#endif // AGENT_AGENT_H

// Shared declarations of slices s004543a0 / s00454e30 / s00457710 (Agent: ragdoll/body update, blow damage
// reduction by armor, corpse spawn, death-state machine). Copied into each slice dir under the same name; keep
// identical. Names are provisional; every offset comes from the functions at 0x4543a0..0x457af0.
#ifndef AGENT_LIFE_H
#define AGENT_LIFE_H
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "rgl.h"
using rgl::vector4;
using rgl::matrix;

#define MB_NOTHROW throw()

struct tstring {
    int pos; char* buf; int cap; int len;
    tstring() {}
    tstring(const char* s) MB_NOTHROW;              // 0x42bbe0
    tstring(const tstring& o) MB_NOTHROW;           // 0x42c110
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
    ~tstring() MB_NOTHROW;                          // 0x42bc40
};
struct StrTable { tstring* lookup(const tstring& key) MB_NOTHROW; };   // 0x43a1e0
extern StrTable g_strtable;                         // 0x5e2a18
struct MsgLog { void add(tstring msg, int color); };                   // 0x452c00 (thiscall, ret 0x14)
extern MsgLog g_msglog;                             // 0x5d7080

// 4x4 frame (rgl::matrix layout); operator* is 0x40e870 (this = left operand, hidden return pointer)
struct V4 { float x, y, z, w; };                    // constructor-free storage (the original builds frames without ctors)
struct mat4 {
    V4 s, f, u, o;
    mat4 operator*(const mat4& b) const;            // 0x40e870
};
struct pos : matrix {
    matrix relative(const matrix& o) const;         // 0x4329d0 (returns by value)
    void rotate_su(const float* angle);             // 0x40e670
};
pos* __cdecl identity_pos(pos* out);                // 0x40cd00 (returns out)
unsigned __cdecl pack_color(const float* c);        // 0x40e440
float frand();                                      // 0x40e400

// ---- shapes ---------------------------------------------------------------------------------------------
struct shape_base {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    shape_base() : m4(-1), m8(8), mc(-1), min_x(100000000), min_y(100000000), max_x(-100000000), max_y(-100000000) {}
    virtual ~shape_base() {}
};
struct sphere_shape : shape_base {                  // vtable 0x54a4f0, size 0x44
    vector4 v20, v30; float radius;
    sphere_shape(vector4 center, const float* r);   // 0x4332a0
};
struct capsule : shape_base {                       // vtable 0x54a540, size 0x64
    vector4 v20, v30, v40, v50; float radius;
    capsule(float r, const vector4* a, const vector4* b);   // 0x433310
};
float __cdecl capsule_sphere_gap(capsule* c, sphere_shape* s);   // 0x409d20
struct face_shape;
struct Ctx {
    void floor_at(const float* p, float* out_z, face_shape** out_face);   // 0x439be0
    float floor_z(const float* p);                                         // 0x439d40
};

// ---- items --------------------------------------------------------------------------------------------
struct ItemRef {                                    // 8 bytes: item index + amount/modifier word
    int item; unsigned m4;
    int stat_200();                                 // 0x434eb0
};
struct ItemKind {                                   // 0x238 bytes (array at 0x5e29d0)
    char pad0[0x20c];
    int f20c;                                       // +0x20c (shield size, cm?)
    char pad210[0x238 - 0x210];
    void get_carry_pos(mat4* out, int* bone_flags, int* bone);   // 0x433e40
};
extern ItemKind* g_items;                           // 0x5e29d0

// ---- animator ---------------------------------------------------------------------------------------------
struct NewtonRagDollBone;
struct SkelElem {                                   // 0x1dc bytes: runtime state of one bone
    char pad0[4];
    mat4 m4;                                        // +0x04
    mat4 m44;                                       // +0x44: local frame
    mat4 m84;                                       // +0x84: world frame
    char padc4[0x1d8 - 0xc4];
    NewtonRagDollBone* bone;                        // +0x1d8
};
struct ChanRec { char pad0[4]; int parent; char pad8[0xdc - 8]; };     // 0xdc bytes: one bone definition
struct SkelData { char pad0[0x1c]; ChanRec* items; };                   // items at +0x1c
struct Skeleton {
    void set_visible(char on);                      // 0x452f80
    int count;                                      // +0
    SkelElem* elems;                                // +4 (0x1dc-byte records)
    SkelData* data;                                 // +8
    char padc[0x64 - 0xc];
    int has_poses;                                  // +0x64
    char* rec(int i);                               // 0x43f260
};
struct CamParams;
struct AgentBody {
    virtual void v0();  virtual void v1();  virtual void v2();  virtual void v3();
    virtual void v4();  virtual void v5();  virtual void v6();  virtual void v7();
    virtual void v8();  virtual void v9();  virtual void v10(); virtual void v11();
    virtual void v12(); virtual void v13(); virtual void v14(); virtual void v15();
    virtual void v16(); virtual void v17();
    virtual void set_mode(int mode);                // slot 18 (+0x48)
    virtual void v19(); virtual void v20(); virtual void v21(); virtual void v22(); virtual void v23();
    virtual void set_color(unsigned c);             // slot 24 (+0x60)
    char pad4[4];
    int f8;                                         // +8
    char padc[0x58 - 0xc];
    mat4 m;                                         // +0x58
    CamParams* camp;                                // +0x98
    char pad9c[0x294 - 0x9c];
    float f294;                                     // +0x294
    int f298;                                       // +0x298
    char pad29c[0x304 - 0x29c];
    Skeleton* skel;                                 // +0x304
};

// ---- mission / game ----------------------------------------------------------------------------------------
struct Agent;
struct NewtonBody;
struct Mission {
    Ctx* scene;                                     // +0
    char pad4[0x1c - 4];
    Agent** pages;                                  // +0x1c (id >> 4; 16 agents per page)
    char pad20[0x5c - 0x20];
    int local_id;                                   // +0x5c
    char pad60[0x5df0 - 0x60];
    int cur_rec;                                    // +0x5df0
    void queue_dead(Agent* a);                      // 0x442dc0
};
extern Mission* g_mission;                          // 0x5e292c
struct TroopRec {                                   // 0x508 bytes
    char pad0[0x30]; unsigned f30; char pad34[0x508 - 0x34];
    tstring* display_name();                        // 0x43a480
};
struct GameData { char pad0[0x13a00]; TroopRec* chars; };
extern GameData* g_game;                            // 0x5e2930
struct Rec58 { char pad[0x20]; unsigned flags; char pad24[0x58 - 0x24]; };
extern Rec58* g_rec58;                              // 0x5e2a00
extern __int64 g_ticks[];                           // 0x5e2948 (100 kHz clocks)
extern float g_blend_rate;                          // 0x5a9c28
extern int g_fx_on;                                 // 0x5aa05c
extern int g_tick_a;                                // 0x5aa048
extern int g_tick_b;                                // 0xa18234

struct Timer {
    int clock; int pad4; __int64 start;
    float elapsed() const;                          // 0x432c10
};

struct SoundMgr { int play(int sound, int priority, const vector4* pos, float pitch, float* out); };   // 0x442650 (ret 0x14)
extern SoundMgr g_sounds;                           // 0x5aa128
extern int g_sound_blood;                           // 0x5aa2e4
struct PhysSet { bool none_sleeping(int idx); };    // 0x43a8a0
extern PhysSet g_phys;                              // 0xa181f8

// ---- blows -----------------------------------------------------------------------------------------------------
struct HitInfo {
    char pad0[0x20];
    float px, py, pz;                               // +0x20
    char pad2c[0x40 - 0x2c];
    float dx, dy, dz;                               // +0x40
};
struct Blow {
    char pad0[0x38];
    int type;                                       // +0x38 (hit location / damage class)
    float damage_in;                                // +0x3c
    float damage_out;                               // +0x40
    int absorbed;                                   // +0x44
    int dmg_kind;                                   // +0x48 (0 cut, 1 pierce, 2 blunt)
    char pad4c[8];
    HitInfo* hit;                                   // +0x54
};
extern float g_armor_a[3];                          // 0xa1817c
extern float g_armor_b[3];                          // 0xa18188

struct face_shape : shape_base {
    vector4 v[8]; vector4 normal; int count;
};

// ---- the agent (0x5928 bytes) -----------------------------------------------------------------------------
struct mesh;
struct vec2 { float x, y; };
struct Font {
    // 0x452bb0 (thiscall, ret 0x24): clears the mesh, lays the text out into it, returns the mesh
    mesh* layout_clear(vector4* out_extent, const tstring* text, const vector4* pos, const vec2* scale, mesh* m,
                       const unsigned* color, const float* a, const float* b, int flags);
};
extern Font* g_font;                                // 0x5e2934
struct AgentFx {                                    // *(agent+0x190): per-agent text/label state
    char pad0[0x38];
    mesh** pmesh;                                   // +0x38
    char pad3c[0xc4 - 0x3c];
    float scale;                                    // +0xc4
    char padc8[4];
    int visible;                                    // +0xcc
};
struct Agent {
    char pad0[8];
    int id;                                         // +8
    char padc[4];
    int f10;                                        // +0x10 (0 = human, 1 = horse)
    int f14;                                        // +0x14
    char pad18[0x2c - 0x18];
    int f2c;                                        // +0x2c
    int f30;                                        // +0x30
    int f34;                                        // +0x34
    int f38;                                        // +0x38
    char pad3c[0x50 - 0x3c];
    vector4 apos;                                   // +0x50
    vector4 dirv;                                   // +0x60
    float rot[3];                                   // +0x70
    char pad7c[0x84 - 0x7c];
    float f84;                                      // +0x84
    int troop;                                      // +0x88
    char pad8c[0x98 - 0x8c];
    vector4 eye;                                    // +0x98
    char pada8[0xbc - 0xa8];
    face_shape* floor;                              // +0xbc
    char padc0[0x190 - 0xc0];
    AgentFx* fx;                                    // +0x190
    char pad194[0x1c4 - 0x194];
    int team;                                       // +0x1c4
    char pad1c8[0x240 - 0x1c8];
    int armor[3];                                   // +0x240
    char pad24c[4];
    AgentBody* body;                                // +0x250
    char pad254[0x290 - 0x254];
    float f290;                                     // +0x290
    char pad294[0x5340 - 0x294];
    float spin[4];                                  // +0x5340
    char pad5350[0x5638 - 0x5350];
    int has_blow;                                   // +0x5638
    Blow last_blow;                                 // +0x563c (0x58 bytes)
    char pad5694[0x56a4 - 0x5694];
    vector4 vel;                                    // +0x56a4
    char pad56b4[0x56e8 - 0x56b4];
    int f56e8;                                      // +0x56e8
    char pad56ec[0x57e0 - 0x56ec];
    Timer t57e0;                                    // +0x57e0
    char pad57f0[0x5830 - 0x57f0];
    int state;                                      // +0x5830
    char queued;                                    // +0x5834
    char pad5835[3];
    int mount_id;                                   // +0x5838
    NewtonBody* mount_body;                         // +0x583c
    char pad5840[4];
    int f5844;                                      // +0x5844
    vector4 spawn;                                  // +0x5848
    char pad5858[0x5880 - 0x5858];
    int ragdoll;                                    // +0x5880
    mat4 hold;                                      // +0x5884
    char f58c4;
    char f58c5;
    char pad58c6[2];
    float vx, vy, vz, vw;                           // +0x58c8
    char f58d8;                                     // +0x58d8
    char pad58d9[3];
    int phys_idx;                                   // +0x58dc
    float anchor[3];                                // +0x58e0
    char pad58ec[0x5918 - 0x58ec];
    Timer t5918;                                    // +0x5918

    ItemRef best_weapon_slot() const;               // 0x435fc0 (hidden return pointer)
    void reduce_blow(Blow* b);                      // 0x454e30
    void drop_held();                               // 0x4529e0
    void on_removed();                              // 0x454320
    void start_ragdoll();                           // 0x44fc60
    void release_body();                            // 0x43a6f0
    void ragdoll_update();                          // 0x4543a0
    bool is_friend(const Agent* o) const;           // 0x4323a0
    void spawn_corpse();                            // 0x4554c0
    void draw_name();                               // 0x4552c0
    void note_blow(Blow* b);                        // 0x457af0
    void tick_death();                              // 0x457710
    void build_static_collision();                  // 0x4417d0
    void camera_params(CamParams* p);               // 0x435c10
};
#endif

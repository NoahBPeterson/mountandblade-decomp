// misc.h: the classes of terrain_a.cpp (0x464200..0x46e590) that are not the terrain object: a mission helper
// block, the element stubs of the std::vector / list / stable_sort instantiations, the prop tween, the missile,
// the texture / shader / material binders, the mesh vertex-index fillers, the effect-slot record, the troop
// auto-equip, the vertex / index buffer uploader and the shadow-view draw pass. Names provisional; each offset cites
// the function that reads or writes it. Where a shared header (src/rgl/*.h, src/game/ui/*.h) already has the class
// but not the member, the member lives on a derived "view" class of the same layout (name ending in _x).
#ifndef TERRAIN_A_MISC_H
#define TERRAIN_A_MISC_H
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <string.h>
#include <vector>
#include <list>
#include <algorithm>
#include <new>
#include "./common.h"
#include "./terrain.h"
#include "./game_state.h"
#include "../ui/troop_inv.h"
#include "../../rgl/resource_manager.h"
#include "../../rgl/particles/mesh_obj.h"

// ---- mission helper block (global 0x5e292c; the Mission object of src/game/ui/mission.h seen through 0x4676c0.. ) ----
struct m64 { int d[16]; };                      // 64-byte block (a matrix)
struct slot130 {                                // 0x130 bytes, array at mission+0x6b0
    m64 a;                                      // 0x00
    m64 b;                                      // 0x40
    m64 c;                                      // 0x80
    int f0c0, f0c4;                             // 0xc0 / 0xc4
    char padc8[0xe8 - 0xc8];
    int f0e8;                                   // 0xe8
    char padec[0x130 - 0xec];
};
struct ModeRec2 { char pad[0x34]; char* recs; char pad2[0x58 - 0x38]; };    // 0x58, array at 0x5e2a00; +0x34 -> 0x4c-byte records
extern ModeRec2* g_modes2;                      // 0x5e2a00 (0x4677a0)
struct World3 { char pad0[0x13a00]; Troop* troops; char pad1[0x13a50 - 0x13a04]; int* tbl; };   // the world object 0x5e2930
extern World3* g_world3;                        // 0x5e2930 (0x4677a0, 0x467940)
extern float g_speed_base;                      // 0x5aa07c (0x467740)

struct Mission3 {
    char pad0[0x6b0];
    slot130 slots[18];                          // 0x6b0 (0x4676e0)
    char pad1c70[0x5df0 - 0x6b0 - sizeof(slot130) * 18];
    int mode;                                   // 0x5df0 (0x4677a0)
    int f5df4;                                  // 0x5df4
    char pad5df8[0x6214 - 0x5df8];
    int f6214, f6218;                           // 0x6214 / 0x6218
    char pad621c[0x6520 - 0x621c];
    int f6520;                                  // 0x6520
    char pad6524[0x6824 - 0x6524];
    char done[64];                              // 0x6824 (0x4676c0)
    void clear_done();                          // 0x4676c0
    void copy_slot(int i);                      // 0x4676e0
    float speed_factor(float x);                // 0x467740 (thiscall, ignores this)
    void add_amount(int idx, unsigned flags);   // 0x4677a0
};

struct RuleRec { int f0; unsigned flags; unsigned f8; };    // flags: 0x100/0x200/0x400 and 0x10000/0x20000/0x40000 (0x467940)
struct CandRec { int f0; int troop; unsigned char f8; };
bool __stdcall rule_accepts(RuleRec* a, CandRec* b, int p3, char p4, char p5, char p6);    // 0x467940

// ---- std::vector<T> (VC7.1 layout: allocator at +0, _First +4, _Last +8, _End +0xc) and allocator::construct stubs ----
struct alloc36 { void construct(rec36* p, const rec36& v); };              // 0x436cb0
struct alloc136 { void construct(rec136* p, const rec136& v); };           // 0x436cd0
struct alloc24 { void construct(elem24* p, const elem24& v); void destroy(elem24* p) { p->~elem24(); } };            // 0x468130
struct alloc100 { void construct(particle100* p, const particle100& v); void destroy(particle100* p) { p->~particle100(); } };   // 0x468170
struct alloc52 { void construct(elem52* p, const elem52& v); void destroy(elem52* p) { p->~elem52(); } };            // 0x468190
struct alloc24_def { void construct(elem24* p, const elem24& v); };        // 0x468130 (the definition)
struct alloc100_def { void construct(particle100* p, const particle100& v); };   // 0x468170
struct alloc52_def { void construct(elem52* p, const elem52& v); };        // 0x468190

struct vec36 { alloc36 _Alval; rec36* first; rec36* last; rec36* end; int size() const; rec36* _Ucopy(rec36* f, rec36* l, rec36* d); };         // 0x467ac0 / 0x46c150
struct vec24 { alloc24 _Alval; elem24* first; elem24* last; elem24* end; int size() const; elem24* _Ucopy(elem24* f, elem24* l, elem24* d); };   // 0x467ae0 / 0x46c170
struct vec52 { alloc52 _Alval; elem52* first; elem52* last; elem52* end; int size() const; elem52* _Ucopy(elem52* f, elem52* l, elem52* d); };   // 0x467b00 / 0x46c1b0
struct vec516 { int alloc; s516* first; s516* last; s516* end; int size() const; };      // 0x467b20
struct vec100 { alloc100 _Alval; particle100* first; particle100* last; particle100* end; particle100* _Ucopy(particle100* f, particle100* l, particle100* d); };   // 0x46c190

struct elem56 { tstring name; int a[10]; };     // 0x38: fill 0x46b3a0, copy 0x468050, copy_backward 0x4680c0
struct E1c8 {                                   // 0x1c8-byte polymorphic record (operator= 0x437d60)
    virtual ~E1c8();
    char body[0x1c8 - 4];
    E1c8& operator=(const E1c8& o);             // 0x437d60
};

// ---- timed transform tween of a scene prop (0x467c50) -------------------------------------------------------
struct PropEnt {                                // the scene entity the prop drives (vtable slot 23 takes the new frame)
    virtual void v0(); virtual void v1(); virtual void v2(); virtual void v3(); virtual void v4();
    virtual void v5(); virtual void v6(); virtual void v7(); virtual void v8(); virtual void v9();
    virtual void v10(); virtual void v11(); virtual void v12(); virtual void v13(); virtual void v14();
    virtual void v15(); virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v20(); virtual void v21(); virtual void v22();
    virtual void set_frame(const rgl::matrix* m, const void* extra);     // slot 23 (+0x5c)
};
struct Prop {
    char pad0[0x18];
    rgl::matrix cur;                            // +0x18
    char pad58[0xd8 - 0x58];
    rgl::matrix to;                             // +0xd8  target frame
    rgl::matrix from;                           // +0x118 start frame
    char extra[0x10];                           // +0x158
    int state;                                  // +0x168  (-1 = idle)
    float duration;                             // +0x16c
    int clock;                                  // +0x170
    char pad174[4];
    __int64 start;                              // +0x178
    float last_t;                               // +0x180
    char pad184[0x1b4 - 0x184];
    PropEnt* ent;                               // +0x1b4
    void update(int unused);                    // 0x467c50
};

// ---- missile (pool record; 0x467f80, 0x467fb0) --------------------------------------------------------------------
struct MissionCam { char pad[0x6654]; float cam[3]; };      // the mission object's camera position (+0x6654)
extern MissionCam* g_mission;                   // 0x5e292c
struct Missile {
    char pad0[0x20];
    rgl::vector4 p20;                           // +0x20
    char pad30[0x40 - 0x30];
    rgl::vector4 p40;                           // +0x40
    char pad50[4];
    float f54;                                  // +0x54
    char pad58[0x68 - 0x58];
    int f68;                                    // +0x68
    char pad6c[4];
    int clock;                                  // +0x70
    char pad74[4];
    __int64 t0;                                 // +0x78
    char pad80[8];
    int ammo_item;                              // +0x88
    char pad8c[4];
    char f90;                                   // +0x90
    char pad91[3];
    float f94;                                  // +0x94
    char pad98[4];
    char f9c;                                   // +0x9c
    char pad9d[3];
    int fa0;                                    // +0xa0
    int fa4;                                    // +0xa4
    float ammo_scale();                         // 0x467f80
    void init();                                // 0x467fb0
};

// per-component max / min of two 3-vectors (0x468300, 0x468360)
void __cdecl vmax3(float* a, const float* b);   // 0x468300
void __cdecl vmin3(float* a, const float* b);   // 0x468360

// ---- render-state cache calls (global object 0xa17910; the class is src/game/renderer.h's, which needs rgl.h) ---------
#include "../../include/renderer.h"       // renderer (shared)
extern IDirect3DBaseTexture9* g_tex_cache[];    // 0xa17e5c texture bound per stage
extern unsigned char g_rs_normalize;            // 0xa17e58 cached D3DRS_NORMALIZENORMALS
extern unsigned char g_rs_zwrite_off;           // 0xa17e59 cached "depth write disabled" bit
extern unsigned char g_rs_specular;             // 0xa17e57 cached D3DRS_SPECULARENABLE
extern unsigned char g_rs_zfunc_le;             // 0xa17e5b cached "depth test on" bit

// ---- texture / shader / skin: the members the binders add (views of src/rgl/resource.h's classes) ---------------------
struct texture_x : texture {
    void bind(int stage, float t);              // 0x4683c0 (+0x28 is the frame count of an animated texture)
};
struct material_x : material {
    void bind(texture** textures, const float* t);      // 0x468440
    void bind_pixel_shader();                   // 0x461f20
};
struct skin_x : skin {
    void apply_state(const float* t);           // 0x4684d0 (flags +0x1c, shader +0x30, specular colour +0x34, power +0x44)
};

// ---- the mesh members of this object (a view of rgl::mesh) -----------------------------------------------------------
#include "../../rgl/mesh_x.h"             // mesh_x (shared with src/rgl/core/resmgr.h)
struct mesh_list_x : rgl::mesh_list { void apply(rgl::matrix m); };            // 0x46c990
struct lod_mesh_x : rgl::lod_mesh { void apply(const rgl::matrix* m); };       // 0x46c9f0

// ---- effect slot record (0xd8 bytes; reset 0x4686a0, inline ctor / dtor in 0x46b9d0) --------------------------------
#include "../../include/fx.h"           // Quad4, EnvRec (shared with src/rgl/scene)
struct FxDef2 {
    int count;                                  // +0
    char pad4[0x40 - 4];
    EnvRec* env;                                // +0x40
    int nenv;                                   // +0x44
    int* envidx;                                // +0x48
    char pad4c[0x60 - 0x4c];
    int clock;                                  // +0x60
    void set_env_count(int n);                  // 0x46b9d0
};

// ---- level index of a record (0x468770) ----------------------------------------------------------------------------
extern int g_lod_max;                           // 0x8ee3ac4
struct Rec468770 {
    char pad0[0x58];
    int f58;                                    // +0x58
    char pad5c[0x7a - 0x5c];
    unsigned char b7a;                          // +0x7a (low nibble = level)
    int level_index();                          // 0x468770
};

// ---- troop auto-equip (0x4687a0 / 0x4687e0; a view of Troop) ---------------------------------------------------------
enum { itp_two_handed = 0x00200000, itp_civilian = 0x20000000 };
enum { ek_item_0 = 0, ek_head = 4, ek_body, ek_foot, ek_gloves, ek_horse };
enum {
    tf_hero = 0x00000010, tf_inactive = 0x00000020,
    tf_guarantee_boots = 0x00100000, tf_guarantee_armor = 0x00200000, tf_guarantee_helmet = 0x00400000,
    tf_guarantee_gloves = 0x00800000, tf_guarantee_horse = 0x01000000, tf_guarantee_shield = 0x02000000,
    tf_guarantee_ranged = 0x04000000
};
struct TroopEq : Troop {
    void unequip_all();                         // 0x4687a0
    void auto_equip(int seed, int unused, char civilian, char no_helmet);    // 0x4687e0
};

// ---- camera-relative view parameters (0x46b940) --------------------------------------------------------------------
#include "../view_params.h"            // ViewParams (shared with terrain_b)

// ---- an agent's equipment slots, line of fire (0x46b190, 0x46b2d0, 0x46b410) ------------------------------------------------
struct ItemRefX : ItemRef {
    void reduce(int n);                         // 0x437510
};
__int64 __cdecl pick_slot_item(int troop, int slot, int mask, int n, int* cand, char remove, char* found);    // 0x46b190
extern unsigned __int64 g_bit[];                // 0x5a8e68 (g_bit[i] == 1 << i)
struct AgentEq {
    char pad0[0x10];
    int f10;                                    // +0x10 (nonzero: no equipment, every slot -1)
    char pad14[0x88 - 0x14];
    int troop;                                  // +0x88
    char pad8c[0x1d0 - 0x8c];
    ItemRef sel[10];                            // +0x1d0 chosen (item, modifier) per slot
    char found[10];                             // +0x220
    char pad22a[0x56f0 - 0x22a];
    int mask;                                   // +0x56f0 excluded-slot mask
    int cand[8];                                // +0x56f4 candidate items
    int ncand;                                  // +0x5714
};
void __stdcall AgentEq_pick_slots(AgentEq* a);  // 0x46b2d0
struct FireRay {
    char pad0[0x20];
    rgl::vector4 from;                          // +0x20
    rgl::vector4 to;                            // +0x30
    char pad40[0x60 - 0x40];
    float width;                                // +0x60
};
struct FireAgent {
    char pad0[0x20];
    rgl::vector4 org;                           // +0x20
    rgl::vector4 pos;                           // +0x30
    rgl::vector4 dir;                           // +0x40 (x, y, z)
    char pad50[0x88 - 0x50];
    int troop;                                  // +0x88
    void ray(FireRay* r);                       // 0x46b410
};

struct ProjView { void project(float* out, unsigned p); };    // 0x4621f0 (the frustum's clip projection of a point; the frustum is the one at cam+0x10c)

// ---- scene view setup (0x46ca50) -------------------------------------------------------------------------------------
struct light_src {                              // one scene light; types 0/1/2 map to D3DLIGHT_POINT/SPOT/DIRECTIONAL
    int type;                                   // 0x00
    float diffuse[3];                           // 0x04
    int pad10;
    float specular[3];                          // 0x14
    int pad20;
    float pos[3];                               // 0x24
    int pad30[5];
    float dir[3];                               // 0x44
    int pad50;
    float range;                                // 0x54
    float atten0, atten1, atten2;               // 0x58
    float theta, phi;                           // 0x64
};
struct fog_rec {                                // fog record at RenderScene+0x1b0: two parameter sets selected by camera height against `level`
    int f0;
    DWORD a_start;                              // 0x04
    DWORD a_end;                                // 0x08
    DWORD a_color;                              // 0x0c  (set A: camera at/above the level)
    char enabled;                               // 0x10
    char pad11[7];
    DWORD b_start;                              // 0x18
    DWORD b_end;                                // 0x1c
    DWORD b_color;                              // 0x20  (set B: camera below the level)
    float level;                                // 0x24
};
struct RenderScene {                            // the render scene (rgl::scene, 0x1d8 bytes) as 0x46ca50 sees it; named Scene before (collided with party/game.h, terrain_b/scene_file.h)
    D3DVIEWPORT9 vp_local;                      // 0x00
    D3DVIEWPORT9 vp_alt;                        // 0x18
    rgl::frustum cam;                           // 0x30
    char pad150[0x160 - 0x150];
    char lights_active;                         // 0x160
    char clip_on;                               // 0x161
    char pad162[6];
    char cull_flip;                             // 0x168
    char fog_invert;                            // 0x169
    char msaa;                                  // 0x16a
    char pad16b;
    float clip_plane[4];                        // 0x16c
    float ambient[3];                           // 0x17c
    int pad188;
    unsigned flags;                             // 0x18c  bit 8: clear colour, bit 9: clear depth
    DWORD clear_color;                          // 0x190
    std::vector<light_src*> lights;             // 0x194 (first at +0x198)
    char pad1a4;
    char use_alt_vp;                            // 0x1a5
    char pad1a6[0x1b0 - 0x1a6];
    fog_rec* fog;                               // 0x1b0
    void apply_state();                         // 0x46ca50
};
extern int g_lights_enabled;                    // 0x5aa024
extern int g_cull_mode;                         // 0xa17ecc (cached D3DCULL_* of the next draw)
extern unsigned char g_fog_on;                  // 0xa17e15 cached D3DRS_FOGENABLE
extern unsigned char g_lighting_on;             // 0xa17e16 cached D3DRS_LIGHTING

// ---- vertex / index buffer uploader (0x46d060) -------------------------------------------------------------------------
struct GeomBuffer {
    int kind;                                   // 0x00: 0 = vertex buffer, 1 = index buffer
    int stride;                                 // 0x04: bytes per element (2 for indices)
    IDirect3DVertexBuffer9* vb;                 // 0x08
    unsigned fmt;                               // 0x0c: vertex field flags (see mesh::fill_vertices)
    int cursor;                                 // 0x10: elements already used
    int gen;                                    // 0x14: rebuild generation
    tstring name;                               // 0x18
    int capacity;                               // 0x28: elements
    int pad2c;
    int is_dynamic;                             // 0x30
    std::vector<rgl::mesh*> meshes;             // 0x34
    IDirect3DIndexBuffer9* ib;                  // 0x44
    int upload(void* scratch, void* tag);       // 0x46d060
};
extern char g_buffers_dirty;                    // 0x5e2938 (set when a buffer must be recreated larger)

// ---- shadow-view draw pass (0x46d8c0) ------------------------------------------------------------------------------------
struct light_t {
    int type;                                   // 0: point, 2: always on
    char pad4[0x24 - 4];
    rgl::vector4 pos;                           // 0x24
    char pad34[0x54 - 0x34];
    float range;                                // 0x54
};
struct flag_obj { char pad[0x10]; char on; };
struct entry { rgl::mesh* m; int extra; };      // 8 bytes
#include "../../include/mat4.h"
using rgl::rot3;
struct view_t {
    char pad0[0xf8];
    rot3 rot;                                   // 0xf8 (rows s, f, u of the camera frame)
    rgl::vector4 org;                           // 0x128
    char pad138[0x160 - 0x138];
    char b160;                                  // lights enabled
    char pad161;
    char b162;                                  // pass kind
    char pad163;
    int f164;                                   // pass kind
    char pad168[0x194 - 0x168];
    std::vector<light_t*> lights;               // 0x194
    char pad1a4[0x1b0 - 0x1a4];
    flag_obj* p1b0;
    std::vector<entry> ents;                    // 0x1b4
};
struct vb_slot {                                // 0x4c bytes
    char pad0[4];
    UINT stride;                                // 4
    IDirect3DVertexBuffer9* vb;                 // 8
    char padc[0x48 - 0xc];
    IDirect3DVertexDeclaration9* decl;          // 0x48
};
struct drawer {
    char pad0[0x14];
    vb_slot vbs[10];                            // 0x14
    char pad30c[0x3b4 - 0x30c];
    int cur_a;                                  // 0x3b4
    int cur_ib;                                 // 0x3b8
    int cur_b;                                  // 0x3bc
    char pad3c0[0x4183c8 - 0x3c0];
    material* cur_mat;                          // 0x4183c8
    int f4183cc;
    skin* cur_part;                             // 0x4183d0
    IDirect3DIndexBuffer9* ib(int i) { return *(IDirect3DIndexBuffer9**)((char*)this + 0x350 + i * 0x4c); }
    void draw(view_t* v);                       // 0x46d8c0
};
extern char g_lighting;                         // 0xa17e16 cached D3DRS_LIGHTING
extern char g_b15;                              // 0xa17e15 cached D3DRS_FOGENABLE
extern char g_light_on[64];                     // 0xa17e17 per-light enable cache
extern char g_b57;                              // 0xa17e57 cached D3DRS_SPECULARENABLE
extern char g_b58;                              // 0xa17e58 cached D3DRS_NORMALIZENORMALS
extern char g_zwrite;                           // 0xa17e59 cached D3DRS_ZWRITEENABLE
extern char g_zfunc_always;                     // 0xa17e5b
extern int g_e68;                               // 0xa17e68 pixel shader bound
extern int g_tss_a;                             // 0xa17e6c
extern int g_tss_b;                             // 0xa17e70
extern int g_tss_c;                             // 0xa17e78
extern int g_tss_d;                             // 0xa17e7c
extern int g_alpha_test;                        // 0xa17ea0
extern int g_texfactor;                         // 0xa17ea4
extern int g_cull_base;                         // 0xa17ecc
extern int g_cull_cur;                          // 0xa17ed0
extern unsigned g_stage_arg[8];                 // 0x5a9c00 D3DTA_* by 3-bit code

#endif // TERRAIN_A_MISC_H

// shadow.h: strat.h of s0042a1f0 / s0042aaf0 / s00429890 extended for s0042b080 / s0042b790 (mesh.h of s0041f720,
// frustum, scene and resource-manager pieces added; the render-layer types are global here, as in strat.h).
// The strategic_entity (vtable 0x54b5d0, see s00421aa0 fx.h) with its shadow / ground-contact code at
// 0x42a090-0x42b000, and the scene "world" (Ctx) it lives in. ALL NAMES ARE PROVISIONAL.
#ifndef MB_STRAT_H
#define MB_STRAT_H
#include <stddef.h>
#include <vector>
#include "rgl.h"
#include "tstring.h"

using rgl::vector4;
using rgl::matrix;

#define OFFSET_CHECK3(T, m, off) typedef char T##_##m##_at3_##off[(offsetof(T, m) == (off)) ? 1 : -1]

// ---- scene lights (record 0x70+ bytes). type 2 = directional (direction at +0x44), else a point light at +0x24.
struct Light {
    int type;                   // 0x00
    vector4 color;              // 0x04
    char pad14[0x24 - 0x14];
    vector4 pos;                // 0x24
    char pad34[0x44 - 0x34];
    vector4 dir;                // 0x44
    char pad54[0x6c - 0x54];
    unsigned flags;             // 0x6c  (bit 0: disabled; bit 1: no shadows)
};
OFFSET_CHECK3(Light, dir, 0x44);
OFFSET_CHECK3(Light, flags, 0x6c);

struct StratEnt;
struct shape;
struct cell { char pad[0x34]; void collect(const float* bmin, const float* bmax, int* flat, unsigned mask, shape** out, int* count); };  // 0x426500

// face record of the static ground mesh (0x88 bytes): vertex count and up to 4 vertex indices, plane normal at +0x54
struct GFace { int n; int idx[4]; char pad14[0x54 - 0x14]; float normal[3]; char pad60[0x88 - 0x60]; };
struct GVert { vector4 p; vector4 n; float shade; };   // 0x24 bytes: position, normal (+0x10), shading weight (+0x20)
struct Ground {
    char pad0[4];
    GVert* verts;               // 0x04
    char pad8[0x14 - 8];
    GFace* faces;               // 0x14
};

// the scene/world object (strategic_entity::ctx): collision grid cells at +0x28, ground at +0xb0, lights at +0xb8
struct Ctx {
    char pad0[8];
    int n[2];                   // 0x08 grid size
    float lo[2];                // 0x10
    float pad18[2];
    float step[2];              // 0x20
    cell** cells;               // 0x28
    char pad2c[0x68 - 0x2c];
    float amb[3];               // 0x68 base ambient colour
    char pad74[0x7c - 0x74];
    std::vector<StratEnt*> ents_a;   // 0x7c (first at 0x80)
    std::vector<StratEnt*> ents_b;   // 0x8c (first at 0x90)
    char pad9c[0xb0 - 0x8c - sizeof(std::vector<StratEnt*>)];
    Ground* ground;             // 0xb0
    char padb4[4];
    std::vector<Light*> lights; // 0xb8 (first at 0xbc)
    void locate(const float* p, int* out) const;    // 0x41ed00
    Light* first_dir_light();                       // 0x42a1a0
    void sample_lights(const vector4* p, vector4* dir, vector4* amb);   // 0x42ad60
    vector4 ambient();                              // 0x42b6d0 (by-value return)
    StratEnt* find_entity(int id);                  // 0x42ba50
    void query_box(const float* bmin, const float* bmax, unsigned mask, shape** out, int* count);   // 0x42aa50
};
OFFSET_CHECK3(Ctx, cells, 0x28);
OFFSET_CHECK3(Ctx, lights, 0xb8);

// ---- pieces of the render layer (mesh.h of s0041f720 de-namespaced; scene, resmgr and skin redefined below)


// 16-byte rgl string (see rgl::string in rgl.h); this one also has the default ctor and dtor.
struct mstr {
    char* p; int size, cap, extra;
    mstr();                                     // 0x42bbd0
    mstr(const char* s);                        // 0x42bbe0
    ~mstr();                                    // 0x42bc40
};

// 64-byte transform: rows s, f, u and origin o (same layout as rgl::matrix).
struct mat4 {
    vector4 s, f, u, o;
    mat4 operator*(const mat4& b) const;       // 0x40e870 (this = left operand)
    vector4 transform_point(const vector4& p) const;   // 0x408460
};
mat4* identity(mat4* out);                      // 0x40cd00 (cdecl; returns out)
void unpack_color(vector4* out, const unsigned* argb);   // 0x40e4e0 (cdecl)
unsigned pack_color(const vector4* c);                   // 0x40e440 (cdecl)

// std::vector helpers: the original inlines push_back's fast path and indexes through &*begin()
template<class T> __forceinline void vpush(std::vector<T>& v, const T& x)
{
    if (v.size() < v.capacity()) {
        T* p = v._Mylast;
        *p = x;
        v._Mylast = p + 1;
    } else {
        v.insert(v.end(), x);
    }
}
template<class T> inline T& vat(std::vector<T>& v, size_t i) { return (&*v.begin())[i]; }

// element types of the mesh's std::vectors
struct vec2 { float x, y; vec2() {} vec2(float x_, float y_) { x = x_; y = y_; } };
struct vertex60 {                // 0x60 bytes, vector at mesh+0x50
    vector4 pos;                 // 0x00
    vector4 f10, f20;
    vector4 normal;              // 0x30
    vec2 uv;                     // 0x40
    int f48, f4c;
    int index;                   // 0x50: point index
    unsigned color;              // 0x54: packed ARGB
    int f58;
    int f5c;
};
struct elem30 { char b[0x30]; };            // vector at +0x60
struct point20 { vector4 a, b; };           // 0x20, vector at +0x70 (rest, current)
struct elem14 { int id; std::vector<int> items; };   // 0x14, vector at +0x80 (items' element type unknown)
struct face20 { vector4 n; int a, b, c, d; };   // 0x20, vector at +0x90

// inline vector helpers used by the mesh code (distinct from the out-of-line rgl::operator* in rgl.h)
inline vector4 vscale(float s, const vector4& v) { return vector4(s * v.x, s * v.y, s * v.z); }
inline vector4 vadd(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }

struct frame_t { std::vector<vector4> verts; std::vector<vector4> norms; };   // 0x20: morph key frame

// morph key frames: frames[count], times[count]
struct keyframes {               // 12 bytes at mesh+0xa4
    frame_t* frames; float* times; int count;
    keyframes() { count = 0; frames = 0; times = 0; }
    keyframes& operator=(const keyframes& o);                                   // 0x4125e0
    void find(int* a, int* b, float* t, const float* time);                     // 0x40f0d0
};

struct obj_c0 { char pad[0x24]; vector4 out; vector4 in; };   // object attached at mesh+0xc0 (0x34.. read, 0x24.. written)

struct mat_t { char pad[0x20]; int f20; int get_flags(); };     // int get_flags() is 0x40eab0
struct res_obj4 { char pad[0x4c]; int f4c; };
struct res_t {
    char pad0[0x30];
    mat_t* mat;                  // 0x30
    char pad1[0x48 - 0x34];
    res_obj4* p48;               // 0x48
    char pad2[0x5c - 0x4c];
    int f5c;                     // 0x5c
    char pad3[0x68 - 0x60];
    void (*fn68)(res_t*);        // 0x68
};

// Axis-aligned box with centre and radius (0x34 bytes): ctor 0x40eaf0, reset 0x416e40, extend 0x416ef0,
// update 0x416f70 (see s00416ef0), extend_box 0x4201b0 (s0041ffe0). It is also mesh::v22c.
struct aabb {
    vector4 min, max, center;
    float radius;
    __declspec(noinline) aabb();                        // 0x40eaf0
    void reset();                                       // 0x416e40
    void extend(const vector4* p);                      // 0x416ef0
    void update();                                      // 0x416f70
    void extend_box(const aabb* b, const mat4* m);      // 0x4201b0
};

struct slot_pair { int a, b; };
inline slot_pair zero_pair() { slot_pair p; p.a = 0; p.b = 0; return p; }
struct slot {                    // 0x2c bytes, array of 3 at mesh+0x110
    int f0, f4, f8, fc;
    int f10, f14;
    slot_pair p;                 // 0x18
    int f20, f24;
    char f28, f29;
};

struct mesh;
struct scene;

struct mesh_base {
    virtual ~mesh_base() = 0;                   // vtable 0x54a6c8: slot 0 = _purecall
    virtual int base_slot1() { return 0; }      // 0x408020
    mstr name;                                  // 0x04
    int f14;
    __forceinline mesh_base() { f14 = 0; }
};

struct mesh : mesh_base {
    mesh* parent;                // 0x18
    mstr name2;                  // 0x1c
    res_t* res;                  // 0x2c
    unsigned flags;              // 0x30
    int f34;
    char f38;
    unsigned color;              // 0x3c: packed ARGB multiplier
    int f40;
    float f44;
    int f48, f4c;
    std::vector<vertex60> v50;
    std::vector<elem30> v60;
    std::vector<point20> v70;
    std::vector<elem14> v80;
    std::vector<face20> v90;
    char local_a0, local_a1;     // 0xa0: faces are local, 0xa1: points/vertices local
    keyframes kf;                // 0xa4
    std::vector<mesh*> kids;     // 0xb0
    obj_c0* p_c0;                // 0xc0
    float c4, c8;
    mat4 m_cc;
    unsigned mask;               // 0x10c
    slot slots[3];               // 0x110 (slots[0].f28 at 0x138 and slots[1].f28 at 0x164 are dirty flags)
    mat4 m194, m1d4;
    int f214, f218, f21c, f220, f224, f228;
    aabb v22c;

    mesh();                                         // 0x412a90
    virtual ~mesh();                                // 0x417730 (scalar deleting dtor)
    virtual int slot1() { return 1; }               // 0x48b1f0
    virtual void slot2();                           // 0x412980

    // parent-chain accessors: walk up until a mesh that owns its data (recursive; the out-of-line copies at
    // 0x40eba0.. are the tail-call-optimised loops, callers inline one level)
    std::vector<point20>& get70() { return local_a1 ? v70 : parent->get70(); }    // 0x40eba0
    std::vector<point20>& get70_();                 // 0x40eba0 (out-of-line call)
    std::vector<vertex60>& get50() { return local_a1 ? v50 : parent->get50(); }   // 0x40ebd0
    keyframes& get_kf() { return local_a1 ? kf : parent->get_kf(); }              // 0x40ec00
    std::vector<face20>& get90() { return local_a0 ? v90 : parent->get90(); }     // 0x40ec30
    std::vector<elem30>& get60() { return local_a1 ? v60 : parent->get60(); }     // 0x40ec60
    std::vector<elem30>& get60_();                  // 0x40ec60 (out-of-line call)
    std::vector<vertex60>& get50_();                // 0x40ebd0 (out-of-line call)
    int has_morph();                                // 0x40f7f0
    void FUN_00412700();                            // 0x412700
    void render_child(scene* sc);                   // 0x414960
    void ensure_local();                            // 0x412ee0
    int add_point(const vector4* p);                // 0x412f50
    void append(mesh* other, const mat4* m);        // 0x412fe0
    void update_morph();                            // 0x413320
    int render(scene* sc, const mat4& frame);       // 0x413750
    int add_vertex(int index, const vec2* uv, const unsigned* color, const vector4* pos, const int* f5c);   // 0x413c50
    void add_face(int a, int b, int c);             // 0x411920
    void clear_all();                               // 0x428e80 (resizes every vector to 0)
};


struct scene;
struct mesh_list {               // 16 bytes: std::vector<mesh*>
    std::vector<mesh*> v;
    void render_all(scene* sc, const mat4& m);      // 0x413ac0
};
struct lod_mesh {
    char pad0[0x34];
    mesh_list lists[5];          // 0x34
    int count;                   // 0x84
    int cur;                     // 0x88
    mat4 m8c;
    unsigned mask;               // 0xcc
    void render(scene* sc, const mat4& m);          // 0x413b00
};

struct mesh_pool {
    char pad0[0xc8];
    std::vector<mesh*> meshes;   // 0xc8
    mesh* get_mesh(res_t* r);                       // 0x413d30
};


// ---- view frustum (s0041e470 / s0041ee20)
struct plane { vector4 pos, normal; };
struct frustum {
    char perspective;           // +0
    char flag1;                 // +1
    char pad2[2];
    int nplanes;                // +4
    plane planes[6];            // +8
    matrix m;                   // +0xc8
    float left, right, top, bottom, nearz, farz;   // +0x108
    frustum();                  // 0x41ef60
    void build_planes();        // 0x41e4f0
};

// ---- the render target / skin resources (s0041b5a0): only the fields used here
struct render_res { char pad0[0x18]; char loaded; char pad19[0x24 - 0x19]; int f24; };
struct skin {
    char pad0[0x1c];
    unsigned id;                // +0x1c
    tstring file;               // +0x20
    int f30;                    // +0x30
    char pad34[0x48 - 0x34];
    render_res* refs[5];        // +0x48
};
struct resmgr {
    res_t* lookup(const mstr& name);                        // 0x418a70
    skin* get_shadow_skin(int w, int h, int fmt, int f34);   // 0x41c420
    int lookup_file(tstring* name);                         // 0x418e90
};
extern resmgr g_resmgr;                                 // 0x5d6fa0

// ---- the render scene (0x1d8 bytes; ctor 0x429740): viewport/camera block with an embedded frustum at +0x30
struct scene {
    int f0, f4, f8, fc, f10;    // 0x00
    float f14;                  // 0x14
    int f18, f1c, f20, f24, f28;
    float f2c;
    frustum fr;                 // 0x30 (0x120 bytes)
    render_res* target;         // 0x150
    char pad154[0x164 - 0x154];
    int f164;
    char pad168[0x18c - 0x168];
    int f18c, f190;
    std::vector<obj_c0*> list;  // 0x194: attached objects collected this frame
    char pad1a4[0x1a5 - 0x1a4];
    char f1a5;
    char pad1a6[0x1a8 - 0x1a6];
    unsigned mask;              // 0x1a8
    char pad1ac[0x1c8 - 0x1ac];
    std::vector<mesh*> cache;   // 0x1c8: one mesh per resource
    scene();                    // 0x429740
};

extern __int64 g_time[];                                // 0x5e2948 clock table (64-bit ticks)
extern int g_shadows_on;                                // 0x5aa030

struct render_info { char valid; float depth; };
struct Psys { void update(scene* sc); };                // 0x414960
struct FxDef { void draw(scene* sc, const matrix* m, const float* p); };   // 0x428ff0 (provisional)

// ---- the collision capsule built on the stack by update_ground_cache (vtable 0x54a540, size 0x64)
struct shape_base {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    virtual ~shape_base() {}
    virtual int shape_type();
    virtual void s2(); virtual void s3(); virtual void s4(); virtual void s5(); virtual void s6();
    virtual void s7(); virtual void s8(); virtual void s9();
    virtual vector4* get_min(vector4* out);             // slot 10
    virtual vector4* get_max(vector4* out);             // slot 11
};
struct shape_ext : shape_base { shape_ext(); };         // 0x407ff0 (out of line)
struct capsule : shape_ext {
    vector4 p0, p1, p2, p3;                             // 0x20..0x60
    float radius;                                       // 0x60
    virtual vector4* get_min(vector4* out);             // 0x40ad80
    virtual vector4* get_max(vector4* out);             // 0x40ade0
};
int shape_contact_count(shape_base* a, shape* b, int max, void* out);   // 0x40bfe0 (cdecl)

// ---- the strategic_entity (vtable 0x54b5d0; layout partly from s00421aa0 fx.h)
struct MeshEnt {
    virtual ~MeshEnt();
    virtual int type();         // slot 1 (9 = strategic entity)
    virtual void s2(); virtual void s3(); virtual void s4(); virtual void s5();
    virtual void s6(); virtual void s7(); virtual void s8();
    virtual void render(scene* sc, render_info* out);   // 9: 0x420310 (base), 0x42a090 (StratEnt)
    Ctx* ctx;                   // 0x04
    int id;                     // 0x08
    char padc[0x58 - 0xc];
    matrix pos;                 // 0x58 (origin at 0x88)
    char pad98[0xc0 - 0x98];
    float radius;               // 0xc0
    vector4 off;                // 0xc4 (shadow-caster offset in the local frame)
    char padd4[0x138 - 0xd4];
    mesh* shadow_a;             // 0x138
    mesh* shadow_b;             // 0x13c
    char f140;                  // 0x140
    char pad141[3];
    float f144;                 // 0x144 (passed by address to the fx draw)
    unsigned mode;              // 0x148
    vector4 scale;              // 0x14c
};
struct StratEnt : MeshEnt {
    virtual void render(scene* sc, render_info* out);   // 9: 0x42a090
    float f15c;                 // 0x15c
    float f160, f164;           // 0x160 / 0x164
    int f168, f16c, f170;       // 0x168 / 0x16c / 0x170
    int ids[64];                // 0x174
    int nids;                   // 0x274
    vector4 last_pos;           // 0x278
    skin* p288;                 // 0x288
    scene* sc28c;               // 0x28c
    char pad290[0x294 - 0x290];
    float f294;                 // 0x294
    int f298;                   // 0x298 (id of a linked entity, -1 if none)
    char pad29c[0x2a0 - 0x29c];
    int f2a0;                   // 0x2a0
    char pad2a4[4];
    __int64 t2a8;               // 0x2a8
    int f2b0;                   // 0x2b0
    char pad2b4[4];
    __int64 t2b8;               // 0x2b8
    char pad2c0[0x304 - 0x2c0];
    FxDef* fx;                  // 0x304
    char pad308[4];
    Psys** emitters_first;      // 0x30c
    Psys** emitters_last;       // 0x310
    char pad314[0x31c - 0x314];
    int f31c;                   // 0x31c

    void build_shadow_frame(matrix* m, vector4* amb);   // 0x42aaf0
    void update_ground_cache();                         // 0x42a480
    void update_shadows(scene* sc, bool a, bool b);     // 0x42a1f0
    void create_shadow_mesh(mesh** slot, char flag);    // 0x42b080
    void shadow_tick();                                 // 0x42b790
};
OFFSET_CHECK3(MeshEnt, ctx, 4);
OFFSET_CHECK3(MeshEnt, pos, 0x58);
OFFSET_CHECK3(MeshEnt, radius, 0xc0);
OFFSET_CHECK3(MeshEnt, shadow_a, 0x138);
OFFSET_CHECK3(MeshEnt, mode, 0x148);
OFFSET_CHECK3(StratEnt, f15c, 0x15c);
OFFSET_CHECK3(StratEnt, ids, 0x174);
OFFSET_CHECK3(StratEnt, nids, 0x274);
OFFSET_CHECK3(StratEnt, last_pos, 0x278);
OFFSET_CHECK3(StratEnt, p288, 0x288);
OFFSET_CHECK3(StratEnt, f294, 0x294);
OFFSET_CHECK3(StratEnt, t2a8, 0x2a8);
OFFSET_CHECK3(StratEnt, t2b8, 0x2b8);
OFFSET_CHECK3(StratEnt, fx, 0x304);
OFFSET_CHECK3(StratEnt, emitters_first, 0x30c);
OFFSET_CHECK3(StratEnt, f31c, 0x31c);

#endif

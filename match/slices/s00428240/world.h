// world.h: types shared by slices s00428240 / s00428b10 / s004291c0 (copied into each slice dir; keep identical).
// The entity factory, the entity load() overrides, the skeleton (FxDef) instance and the skinning code around
// 0x428240-0x429886. ALL NAMES ARE PROVISIONAL unless a comment cites evidence.
#ifndef MB_WORLD_H
#define MB_WORLD_H
#include <stddef.h>
#include <vector>
#include "rgl.h"

using rgl::vector4;

#define OFFSET_CHECK(T, m, off) typedef char T##_##m##_at_##off[(offsetof(T, m) == (off)) ? 1 : -1]

// rgl string with a read cursor (16 bytes), as in s0041c760.h
struct tstring {
    int pos; char* buf; int cap; int len;
    tstring();                                 // 0x42bbd0
    ~tstring();                                // 0x42bc40
    bool equals(const char* s) const;          // 0x42bd10
    bool ne(const char* s) const;              // 0x42bdb0 (!equals)
};
void mb_error(const char* fmt, ...);           // 0x42cc80

// Serialization stream (vtable family 0x54a740); slot numbers as in s0041c760.h. Only the reader slots are used here.
struct stream_base {
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4();
    virtual void s5(); virtual void s6(); virtual void s7(); virtual void s8(); virtual void s9();
    virtual void s10(); virtual void s11(); virtual void s12(); virtual void s13(); virtual void s14();
    virtual void s15(); virtual void s16(); virtual void s17(); virtual void s18();
    virtual stream_base& r_int(int* v);                    // 19 (+0x4c)
    virtual void s20();
    virtual stream_base& r_tstr(tstring* v);               // 21 (+0x54)
};
stream_base& r_matrix(stream_base& s, rgl::matrix* m);     // 0x415f30 (cdecl)

// ---- collision shapes (layouts from s0040d7d0 / s00409e90 / s0040bcb0) ----------------------------------
struct shape_base {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    virtual ~shape_base() {}                            // slot 0
    virtual int shape_type();                           // slot 1 (0 sphere, 1 capsule, 2 face, 3 composite, 4 manifold)
    virtual void v2(); virtual void v3(); virtual void v4(); virtual void v5(); virtual void v6();
    virtual void v7(); virtual void v8(); virtual void v9(); virtual void v10(); virtual void v11();
    virtual void v12(); virtual void v13();
    virtual void copy_from(const shape_base* o);        // slot 14
};
struct shape_ext : shape_base { shape_ext(); };         // 0x407ff0
struct composite : shape_ext {                          // vtable 0x54a63c, size 0x28
    shape_base** bodies; int count;
    composite() : bodies(0), count(0) {}
    __forceinline composite(const composite& o) { ((shape_base*)this)->copy_from(&o); }
};
struct sphere_shape : shape_base {                      // vtable 0x54a4f0, size 0x44
    vector4 v20, v30; float radius;
    sphere_shape(const sphere_shape& o);                // 0x40c030
};
struct capsule_shape : shape_base {                     // vtable 0x54a540, size 0x64
    vector4 p0, p1, p2, p3; float radius;
    capsule_shape(const capsule_shape& o);              // 0x40a580
};
struct face_shape : shape_base {                        // vtable 0x54a588, size 0xb4
    char body[0xb4 - sizeof(shape_base)];
    face_shape(const face_shape& o);                    // 0x40ae70
};
struct manifold : shape_base {                          // vtable 0x54a5f4
    shape_base* build_composite();                      // 0x40cb60
};

// ---- frames (layout of rgl::matrix; operator* is the thiscall at 0x40e870 with a hidden return pointer) ----
struct mat3 {
    vector4 s, f, u;
    mat3() {}
    __forceinline mat3(const vector4& a, const vector4& b, const vector4& c) : s(a), f(b), u(c) {}
};
struct mat4 {
    vector4 s, f, u, o;
    mat4 operator*(const mat4& b) const;               // 0x40e870 (this = left operand)
    __declspec(noinline) mat3* rot_t(mat3* out) const;                      // 0x429670: transposed rotation part (returns out)
};
struct pos : rgl::matrix { mat3 rot3(const rgl::matrix& m) const; };   // 0x40a920

// ---- the mesh / particle-system object (0x260 bytes, vtable 0x54a6f4; mesh.h of s00413320, psys.h of s00411a90) --
struct elem30 { char b[0x30]; };                        // vector at mesh+0x60: skin vertices
struct SkinVert {                                       // the same 0x30 bytes seen as a skinning record
    float w[4];                                         // 0x00
    int bone[4];                                        // 0x10
    float bonef[4];                                     // 0x20
};
struct point20 { vector4 a, b; };                       // vector at +0x70: (rest, current) points
struct face20 { vector4 n; int a, b, c, d; };           // vector at +0x90
struct vertex60 {                                       // vector at +0x50
    vector4 nrm, f10, f20, nrm0;
    float u, v; int f48, f4c;
    int index;                                          // 0x50: point index
    unsigned color; int f58, f5c;
};
struct elem14 { int id; std::vector<int> items; };      // vector at +0x80
struct vert_vec : std::vector<vertex60> {
    const vertex60& operator[](size_t i) const;         // 0x40fb60 (out of line; also used for const lookups)
};
// std::vector<elem30>::iterator: operator+ stays out of line in the skinning code
struct skin_iter {
    SkinVert* cur;
    skin_iter() : cur(0) {}
    skin_iter(SkinVert* p) : cur(p) {}
    __declspec(noinline) skin_iter operator+(int n) const;   // 0x4296d0
};
struct scene;
struct res_t;
struct mat_t { char pad[0x20]; int f20; };
struct res_t {
    char pad0[0x1c];
    unsigned flags;                                     // 0x1c
    char pad20[0x30 - 0x20];
    mat_t* mat;                                         // 0x30
};
struct mesh {
    virtual ~mesh();                                    // 0: 0x417730
    virtual int slot1();                                // 1: 0x48b1f0
    virtual void slot2();                               // 2: 0x412980
    char pad04[0x18 - 4];
    mesh* parent;                                       // 0x18
    char pad1c[0x2c - 0x1c];
    res_t* res;                                         // 0x2c
    char pad30[0x50 - 0x30];
    vert_vec v50;                                       // 0x50
    std::vector<elem30> v60;                            // 0x60
    std::vector<point20> v70;                           // 0x70
    std::vector<elem14> v80;                            // 0x80
    std::vector<face20> v90;                            // 0x90
    char local_a0, local_a1;                            // 0xa0 / 0xa1
    char pada2[0x114 - 0xa2];
    int f114;                                           // 0x114 (slots[0].f4)
    char pad118[0x138 - 0x118];
    char f138;                                          // 0x138 (slots[0].f28: dirty flag)
    char pad139[0x220 - 0x139];
    mat4* bone_mats;                                    // 0x220: 32 matrices, allocated on first skinning
    const void* bone_src;                               // 0x224
    int nbones;                                         // 0x228: max bone index + 1
    char pad22c[0x260 - 0x22c];
    mesh* clone();                                      // 0x41c6d0
    int render(scene* sc, const mat4& frame);           // 0x413750
    void ensure_local();                                // 0x412ee0
    void ensure_skin();                                 // 0x412700
    std::vector<elem30>& get60_();                      // 0x40ec60 (out of line)
    std::vector<elem30>& get60() { return local_a1 ? v60 : parent->get60_(); }
    void clear_geometry();                              // 0x428e80
};
struct Psys : mesh {};                                  // particle system (s00411a90): same object, other element type

struct lod_mesh2 {                                      // 0xe0 bytes (entity.h of s0041f720)
    char pad0[0x34];
    struct list { std::vector<mesh*> v; } lists[5];     // 0x34
    int count;                                          // 0x84
    int cur;                                            // 0x88
    char pad8c[0xe0 - 0x8c];
    void select(const float* dist);                     // 0x4205b0
    void render(scene* sc, const mat4& m);              // 0x413b00
};

// ---- the render scene (0x1d8 bytes; ctor 0x429740): viewport/camera block with an embedded frustum at +0x30 ----
struct frustum {
    char b[0x120];
    frustum();                                          // 0x41ef60
};
struct render_target;
struct scene {
    int f0, f4, f8, fc, f10;                            // 0x00
    float f14;                                          // 0x14
    int f18, f1c, f20, f24, f28;                        // 0x18
    float f2c;                                          // 0x2c
    frustum fr;                                         // 0x30
    render_target* target;                              // 0x150
    int f154, f158, f15c;                               // 0x154
    char f160, f161, f162;                              // 0x160
    char pad163;
    int f164;                                           // 0x164
    char f168, f169, f16a;                              // 0x168
    char pad16b;
    int f16c, f170, f174, f178;                         // 0x16c
    vector4 ambient;                                    // 0x17c
    int f18c;                                           // 0x18c
    int f190;                                           // 0x190
    std::vector<void*> list;                            // 0x194
    char f1a4; char f1a5; char pad1a6[2];
    unsigned mask;                                      // 0x1a8
    char f1ac; char pad1ad[3];
    int f1b0;                                           // 0x1b0
    std::vector<void*> v1b4;                            // 0x1b4
    int f1c4;                                           // 0x1c4
    std::vector<mesh*> cache;                           // 0x1c8: one mesh per resource
    scene();                                            // 0x429740
    void reset();                                       // 0x428f30
};

// ---- resources and the resource manager ----------------------------------------------------------------
struct res_shape {                                      // object returned by resmgr::get_object60 (0x418c10)
    char pad[0x18];
    shape_base* shape;                                  // 0x18
    shape_base* clone_shape();                          // 0x4285e0
};
struct FxRes;
struct resmgr {                                         // global at 0x5d6fa0
    res_shape* get_object60(const tstring& name);       // 0x418c10
    FxRes* get_object70(const tstring& name);           // 0x418cf0
    mesh* get_object(const tstring* name);              // 0x419540
};
extern resmgr g_resmgr;                                 // 0x5d6fa0

// ---- std::vector helpers (as in mesh.h) ------------------------------------------------------------------
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

// ---- skeleton / effect definition ("FxDef", fx.h of s00421110) -------------------------------------------
struct quat4 {
    float v[4];
    quat4() {}
    quat4(float w, float x, float y, float z) { v[1] = x; v[2] = y; v[3] = z; v[0] = w; }
};
void quat_from_matrix(quat4* out, const float* m);     // 0x428b10 (cdecl)
// std::vector<Psys*> whose push_back stays out of line: 0x4119d0 (a pointer-vector instantiation shared by ICF)
struct PsysList : std::vector<Psys*> {
    void push_back(Psys* const& x);                     // 0x4119d0
};
struct FxRec {                                          // 0xdc-byte record of the loaded skeleton
    char pad0[4];
    int parent;                                         // 0x04
    char pad1[4];
    mat4 m;                                             // 0x0c
    char pad2[0xdc - 0x4c];
};
struct FxElem {                                         // 0x1dc bytes, vtable 0x54b454, dtor 0x41f1d0
    virtual ~FxElem();
    mat4 m4;                                            // 0x04: world frame (parent * local)
    mat4 m44;                                           // 0x44: local frame as loaded
    mat4 m84;                                           // 0x84: copy of the world frame
    quat4 q;                                            // 0xc4: rotation of the local frame
    char pad0[0xe4 - 0xd4];
    quat4 qe4;                                          // 0xe4
    vector4 vf4;                                        // 0xf4
    int t[2][2];                                        // 0x104: -1000
    int c[2];                                           // 0x114
    quat4 qa[2][2];                                     // 0x11c
    quat4 qb[2][2];                                     // 0x15c
    PsysList psys;                                      // 0x19c
    int f1ac;                                           // 0x1ac
    std::vector<lod_mesh2*> groups;                     // 0x1b0
    int parent;                                         // 0x1c0 (-1 = none)
    int nchild;                                         // 0x1c4
    int* children;                                      // 0x1c8
    const FxRec* src;                                   // 0x1cc: the loaded record
    float f1d0, f1d4;                                   // 0x1d0 / 0x1d4
    int f1d8;                                           // 0x1d8
    FxElem();                                           // 0x428d10
};
struct EnvRec {                                         // 0xd8 bytes
    char pad0[0x60];
    vector4* k0; vector4* k1; vector4* k2;              // 0x60 / 0x64 / 0x68
    char pad1[0xd8 - 0x6c];
    void alloc(int n);                                  // 0x428c60
};
struct FxRes {                                          // object returned by resmgr::get_object70 (0x418cf0)
    char pad[0x18];
    int count;                                          // 0x18
    FxRec* recs;                                        // 0x1c
};
struct FxDef {                                          // 0x7c bytes
    int count;                                          // 0x00
    FxElem* elems;                                      // 0x04
    FxRes* res;                                         // 0x08
    vector4 v0c;                                        // 0x0c
    int f1c;                                            // 0x1c
    std::vector<Psys*> psys;                            // 0x20
    std::vector<lod_mesh2*> groups;                     // 0x30
    EnvRec* env;                                        // 0x40
    int nenv;                                           // 0x44
    int f48, f4c;                                       // 0x48 / 0x4c
    char b50;                                           // 0x50
    float f54;                                          // 0x54
    char b58;                                           // 0x58
    int f5c;                                            // 0x5c
    int f60;                                            // 0x60
    int f64;                                            // 0x64
    char b68;                                           // 0x68
    vector4 v6c;                                        // 0x6c
    FxDef(int a);                                       // 0x428820
    void init(FxRes* r);                                // 0x4288c0
    void render(scene* sc, const mat4* frame, const float* dist);   // 0x428ff0
    void skin(Psys* p, char flag);                      // 0x4291c0
    void skin_group(lod_mesh2* g);                      // 0x4296f0
};

// ---- scene entities (vtable 0x54b460 and derived; entity.h of s0041f720, fx.h of s00421110) ---------------
struct World;
struct entity {                                         // vtable 0x54b460, size 0x15c, ctor 0x41fba0
    virtual ~entity();                                  // 0: body 0x4213f0, deleting 0x4214e0
    virtual int type_id();                              // 1: 0x408020
    virtual rgl::string type_name();                    // 2: 0x41fcb0
    virtual void v3();                                  // 0x41fcd0
    virtual void v4();                                  // 0x41ffe0
    virtual void v5();                                  // 0x41fd80
    virtual void v6();                                  // 0x41fd90
    virtual void v7();                                  // 0x41fda0
    virtual void v8();                                  // 0x41fdd0
    virtual void v9();                                  // 0x420310
    virtual void v10();                                 // 0x4280e0
    virtual void v11();                                 // 0x4280e0
    virtual void v12();                                 // 0x41fde0
    virtual void v13();                                 // 0x41fdf0
    virtual void v14();                                 // 0x41fe00
    virtual void v15();                                 // 0x41fe10
    virtual void load(stream_base* s);                  // 16: 0x4284e0
    virtual void v17();                                 // 0x408030
    virtual void v18();                                 // 0x41fe30
    virtual void v19();                                 // 0x420960
    virtual void v20();                                 // 0x41fe50
    virtual void v21();                                 // 0x41fe70
    virtual void v22();                                 // 0x41fe90
    virtual void v23();                                 // 0x41ff40
    World* owner;                                       // 0x04
    int id;                                             // 0x08
    char pad0[0x58 - 0x0c];
    rgl::matrix local;                                  // 0x58
    shape_base* shape;                                  // 0x98: collision shape
    char pad1[0x118 - 0x9c];
    std::vector<mesh*> meshes;                          // 0x118
    char pad2[0x140 - 0x128];
    char f140;                                          // 0x140: visible
    char pad3[3];
    float f144;                                         // 0x144
    char pad4[0x15c - 0x148];
    entity();                                           // 0x41fba0
};
struct mesh_entity : entity {                           // vtable 0x54b4d8
    virtual int type_id();                              // 0x48b1f0
    virtual rgl::string type_name();                    // 0x4214c0
};
struct static_entity : entity {                         // vtable 0x54b7b8
    virtual ~static_entity() {}                         // 0x4280f0
    virtual int type_id();                              // 0x493300
    virtual rgl::string type_name();                    // 0x4280c0
    virtual void v11();                                 // 0x48b170
};
struct dynamic_entity : entity {                        // vtable 0x54b668, size 0x1d8
    char pad5[0x1d8 - sizeof(entity)];
    dynamic_entity();                                   // 0x425370
    virtual ~dynamic_entity();                          // 0x427cd0 (body), deleting 0x427d20
};
struct skeleton_entity : entity {                       // vtable 0x54b730, size 0x178
    char pad5[0x170 - sizeof(entity)];
    FxDef* fx;                                          // 0x170
    int nfx;                                            // 0x174
    skeleton_entity();                                  // 0x427d80
    virtual ~skeleton_entity();                         // 0x427e10 (body), deleting 0x427e80
    virtual void load(stream_base* s);                  // 0x4286e0
    virtual void v24(); virtual void v25(); virtual void v26(); virtual void v27();
    virtual void render(scene* sc);                     // 28: 0x428f80
};
struct strategic_entity : entity {                      // vtable 0x54b5d0, size 0x320
    char pad5[0x320 - sizeof(entity)];
    strategic_entity();                                 // 0x421720
    virtual ~strategic_entity();                        // 0x428120 (body), deleting 0x428220
    virtual void load(stream_base* s);                  // 0x4286d0 (thunk to 0x4284e0)
};

struct World {
    char pad0[0x8c];
    std::vector<entity*> ents2;                         // 0x8c
    int next_id;                                        // 0x9c
    entity* instantiate(int type_id);                   // 0x428240
    entity* create_entity(int type_id);                 // 0x428440
};
OFFSET_CHECK(entity, shape, 0x98);
OFFSET_CHECK(entity, meshes, 0x118);
OFFSET_CHECK(entity, f144, 0x144);
OFFSET_CHECK(World, ents2, 0x8c);
OFFSET_CHECK(World, next_id, 0x9c);
OFFSET_CHECK(mesh, v50, 0x50);
OFFSET_CHECK(mesh, v90, 0x90);
OFFSET_CHECK(mesh, local_a0, 0xa0);
OFFSET_CHECK(mesh, bone_mats, 0x220);
OFFSET_CHECK(mesh, nbones, 0x228);
OFFSET_CHECK(scene, fr, 0x30);
OFFSET_CHECK(scene, target, 0x150);
OFFSET_CHECK(scene, ambient, 0x17c);
OFFSET_CHECK(scene, list, 0x194);
OFFSET_CHECK(scene, mask, 0x1a8);
OFFSET_CHECK(scene, v1b4, 0x1b4);
OFFSET_CHECK(scene, cache, 0x1c8);
OFFSET_CHECK(FxElem, psys, 0x19c);
OFFSET_CHECK(FxElem, groups, 0x1b0);
OFFSET_CHECK(FxElem, parent, 0x1c0);
OFFSET_CHECK(FxElem, src, 0x1cc);
OFFSET_CHECK(FxDef, groups, 0x30);
OFFSET_CHECK(FxDef, v6c, 0x6c);

#endif

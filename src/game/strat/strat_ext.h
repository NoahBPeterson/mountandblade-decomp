// strat_ext.h: classes that this directory USES but does not own (render layer, resources, collision shapes).
// Only the fields and methods the functions of strat_entity.cpp / gui_manager.cpp touch are declared; the
// owning modules (other TUs: mesh 0x412a90.., scene 0x428f30/0x429740, frustum 0x41e4f0/0x41ef60, resource
// manager 0x418a70.., shapes 0x407ff0..) declare the full classes. Layouts are the ones the byte-exact slice
// functions verified; each offset is cited by the function that reads/writes it in strat_entity.h.
// Global-namespace names (scene, mesh, ...) are kept: the symbols map vtables/mangled names by them.
#ifndef STRAT_EXT_H
#define STRAT_EXT_H
#include <stddef.h>
#include <vector>
#include "rgl.h"
#include "../mat4.h"
#include "../tstring.h"
#include "../rgl_mesh.h"                    // rgl::res_t / mat_t / res_obj4 / elem30 / elem14 / mesh_base (one definition for the rgl.h universe)
#include "../../include/mesh_prims.h"
#include "../../include/frustum.h"

using rgl::vector4;
using rgl::matrix;
using rgl::mat4;
using rgl::aabb;
using rgl::point20;
using rgl::plane;
using rgl::frustum;

#define OFFSET_CHECK3(T, m, off) typedef char T##_##m##_at3_##off[(offsetof(T, m) == (off)) ? 1 : -1]

struct StratEnt;
struct MeshEnt;
struct Ctx;
struct scene;
struct mesh;

// 64-byte-per-element helpers of the mesh's std::vectors (rgl::mesh in src/rgl/mesh.h is the namespaced twin)
// element types of the mesh's std::vectors
#include "../../include/vec.h"
// vec2 is an aggregate; strat_entity.cpp builds its uv offsets through a constructor call (a temporary, then a copy)
struct vec2c : vec2 { vec2c(float x_, float y_) { x = x_; y = y_; } };
#include "../../include/mesh_parts.h"        // rgl::vertex60, frame_t, obj_c0, slot, render_info, keyframes
using rgl::vertex60;
using rgl::frame_t;
using rgl::obj_c0;
using rgl::slot;
using rgl::render_info;
using rgl::elem30;                           // 0x30, vector at +0x60 (src/game/rgl_mesh.h)
using rgl::elem14;                           // 0x14, vector at +0x80 (items' element type unknown)
using rgl::face20;                           // 0x20, vector at +0x90 (src/include/mesh_prims.h)

// inline vector helpers used by the mesh code (src/game/rgl_mesh.h; distinct from the out-of-line rgl::operator* in rgl.h)
using rgl::vscale;
using rgl::vadd;

using rgl::keyframes;

using rgl::mat_t;
using rgl::res_obj4;
using rgl::res_t;

struct mesh;
struct scene;

using rgl::mesh_base;                 // vtable 0x54a6c8 (src/game/rgl_mesh.h)

struct mesh : mesh_base {
    mesh* parent;                // 0x18
    tstring name2;                  // 0x1c
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
    virtual ~mesh();                                // 0x4127f0 (body; scalar deleting dtor 0x417730)
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


// ---- render target / skin resources: only the fields used here
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
    res_t* lookup(const tstring& name);                      // 0x418a70
    skin* get_shadow_skin(int w, int h, int fmt, int f34);   // 0x41c420
    int lookup_file(tstring* name);                          // 0x418e90
};
extern resmgr g_resmgr;                                      // 0x5d6fa0

// ---- the render scene (0x1d8 bytes; ctor 0x429740): viewport/camera block with an embedded frustum at +0x30.
// shadow_tick (0x42b790) fills the block field by field.
struct scene {
    int f0, f4, f8, fc, f10;    // 0x00
    float f14;                  // 0x14
    int f18, f1c, f20, f24, f28;
    float f2c;
    frustum fr;                 // 0x30 (0x120 bytes)
    render_res* target;         // 0x150
    char pad154[0x164 - 0x154];
    int f164;                   // 0x164
    char pad168[0x18c - 0x168];
    int f18c, f190;             // 0x18c
    char pad194[0x1a5 - 0x194];
    char f1a5;                  // 0x1a5
    char pad1a6[2];
    unsigned mask;              // 0x1a8
    char pad1ac[0x1d8 - 0x1ac];
    scene();                    // 0x429740
};
OFFSET_CHECK3(scene, target, 0x150);
OFFSET_CHECK3(scene, f164, 0x164);
OFFSET_CHECK3(scene, f1a5, 0x1a5);
OFFSET_CHECK3(scene, mask, 0x1a8);

extern __int64 g_time[];                                // 0x5e2948 clock table (64-bit ticks, 1e-5 s)
extern int g_shadows_on;                                // 0x5aa030

struct Psys { void update(scene* sc); };                // 0x414960
struct FxDef { void draw(scene* sc, const matrix* m, const float* p); };   // 0x428ff0 (provisional)

// ---- the collision capsule built on the stack by update_ground_cache (vtable 0x54a540, size 0x64): src/include/shape.h.
// Its base constructor is the out-of-line 0x407ff0 here (capsule(capsule::outline_init())).
#include "../../include/shape.h"

#endif // STRAT_EXT_H

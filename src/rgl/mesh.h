// mesh.h: the 0x260-byte mesh object (rgl::mesh, vtable 0x54a6f4, ctor 0x412a90, dtor 0x4127f0), its element types, the
// particle emitter that hangs off it (mesh+0xb0), the render scene it is drawn into (0x1d8 bytes, ctor 0x429740), the
// global scene registry (0x5fe898) and the lod mesh. One definition for src/rgl (mesh_obj.cpp 0x40e940..0x413b5f, psys,
// the scene objects), src/game/terrain_b / ui and the resource loaders. Merged from the former mesh.h (the slices'
// copies s00412a90, s00413320, s00413c50, s00413e50) and mesh_obj.h (mesh_obj.cpp's complete layout: the destructor
// 0x4127f0, copy-assignments and particle code need `skin_vert`, `bone_influence`, `frame_t` with its out-of-line
// ctor, `keyframes` with its destructor, the owned attachment pointer, the scene's registration fields), and
// src/rgl/scene/scene.h's full scene (constructor 0x429740, reset 0x428f30, Ctx::render_scene 0x429890, ~scene 0x41f4e0).
//
// All names provisional except where an address is cited; every offset below is read or written by a
// function of one of those modules (the citing function address is given).
#ifndef RGL_MESH_H
#define RGL_MESH_H
#include <vector>
#include <stddef.h>
#include <string.h>
#include "./rgl_string.h"
#include "./rgl_math.h"
#include "../include/vec.h"
#include "../include/mesh_prims.h"
#include "../include/mesh_parts.h"       // vertex60, frame_t, obj_c0, slot, render_info, keyframes
#include "./resource.h"
#include "../include/frustum.h"

namespace rgl {

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

// ---- element types of the mesh's std::vectors ---------------------------------------------------------------
// 0x30 bytes, vector at +0x60: per-vertex skinning record (build_skin 0x411d20 writes w[], bone[], bonef[])
struct skin_vert { float w[4]; int bone[4]; float bonef[4]; };
// 8 bytes: (vertex index, weight). Also the scene's queue entries (key = the mesh pointer, weight 0).
struct idx_weight { int index; float weight; };
// 0x14 bytes, vector at +0x80: the vertices one bone influences (build_skin 0x411d20)
struct bone_influence { int bone; std::vector<idx_weight> verts; };
// face20 (0x20 bytes, vector at +0x90; a, b, c at +0x10: add_face 0x411920) is in src/include/mesh_prims.h

struct scene;
struct psys;                                   // the particle emitter: psys.h's / scene/emitter.h's views; mesh_obj.h's `emitter` is the simulation view

// ---- the mesh ------------------------------------------------------------------------------------------------
struct mesh;
struct scene;
struct mesh_pool;

struct mesh : named_object {
    mesh* parent;                // 0x18
    string name2;                // 0x1c
    skin* res;                   // 0x2c: the material it is drawn with (def->refcount at +0x64 is released by ~mesh)
    unsigned flags;              // 0x30
    int f34;
    char f38;                    // 0x38: forces alpha blending (setup_material 0x40f3f0)
    unsigned color;              // 0x3c: packed ARGB multiplier
    int f40;
    float f44;
    int f48, f4c;
    std::vector<vertex60> v50;
    std::vector<skin_vert> v60;
    std::vector<point20> v70;
    std::vector<bone_influence> v80;
    std::vector<face20> v90;
    char local_a0, local_a1;     // 0xa0: faces are local, 0xa1: points/vertices local
    keyframes kf;                // 0xa4
    std::vector<psys*> kids;     // 0xb0: particle systems (render calls psys::update 0x414960 on them); mesh_obj.h's `emitter` is the same object
    obj_c0* p_c0;                // 0xc0 (deleted by ~mesh)
    float c4, c8;
    matrix m_cc;                 // 0xcc
    unsigned mask;               // 0x10c
    slot slots[3];               // 0x110
    matrix m194, m1d4;
    int f214;                    // 0x214 sort-key term (compute_sortkey 0x412980)
    __int64 sortkey;             // 0x218
    int* p220;                   // 0x220 (delete[] in ~mesh)
    int f224, f228;
    aabb bounds;                 // 0x22c

    mesh();                                         // 0x412a90
    virtual ~mesh();                                // 0x4127f0
    virtual int type();                             // 0x48b1f0 (returns 1)
    virtual void compute_sortkey();                 // 0x412980

    // parent-chain accessors: walk up until a mesh that owns its data. In-class (so inline, COMDAT) loops: the
    // out-of-line copies are 0x40eba0 .. 0x40ec60. They must stay inline-linkage functions (a non-inline
    // definition in the .cpp gives its callers interprocedural register information the original's callers
    // lack: should_batch 0x40f7f0 differs) and __declspec(noinline) (cl expands the loop into update_morph
    // otherwise). Several callers test the flag themselves first (`local_a1 ? v70 : parent->get70()`).
    __declspec(noinline) std::vector<point20>& get70() { mesh* n = this; while (!n->local_a1) n = n->parent; return n->v70; }     // 0x40eba0
    __declspec(noinline) std::vector<vertex60>& get50() { mesh* n = this; while (!n->local_a1) n = n->parent; return n->v50; }    // 0x40ebd0
    __declspec(noinline) keyframes& get_kf() { mesh* n = this; while (!n->local_a1) n = n->parent; return n->kf; }               // 0x40ec00
    __declspec(noinline) std::vector<face20>& get90() { mesh* n = this; while (!n->local_a0) n = n->parent; return n->v90; }     // 0x40ec30
    __declspec(noinline) std::vector<skin_vert>& get60() { mesh* n = this; while (!n->local_a1) n = n->parent; return n->v60; }  // 0x40ec60
    // the call sites that expand one level of the parent-chain walk themselves (entity::render 0x420310)
    std::vector<point20>& get70_rec() { return local_a1 ? v70 : parent->get70(); }
    void ensure_local();                            // 0x412ee0
    int add_point(const vector4* p);                // 0x412f50
    int add_face(int a, int b, int c);              // 0x411920
    void append(mesh* other, const matrix* m);      // 0x412fe0
    void update_morph();                            // 0x413320
    int render(scene* sc, const matrix& frame);     // 0x413750
    int add_vertex(int index, const vec2* uv, const unsigned* color, const vector4* pos, const int* f5c);   // 0x413c50
    int should_batch();                             // 0x40f7f0
    void setup_material();                          // 0x40f3f0
    void ensure_skin();                             // 0x412700
    void build_skin();                              // 0x411d20
    void update_emitters(const float* dt, const matrix& frame);   // 0x4126a0
    // ---- src/game/party2 (the map party's model meshes) ----
    bool in_use() const;                            // 0x4b17d0 (a keyframe track is present: own, or the parent's)
    void set_time(const float* t);                  // 0x433190 (ret 4)
    void translate(const vector4* d);               // 0x44f240 (ret 4)
    mesh* clone();                                  // 0x41c6d0 (src/game/party2: s0049aca0 / s0049b630)
    void clear_geometry();                          // 0x428e80
    void compute_normals();                         // 0x451c50
    // ---- src/game/agent5 (agent_hud.cpp: the HUD's item and status meshes) ----
    void append_skinned(mesh* other, const matrix* m, int bone, unsigned color, char flip);   // 0x44eb00 (ret 0x14)
    void append_skinned(mesh* other, const mat4* m, int bone, unsigned color, char flip);     // 0x44eb00 (the definition's own signature: rgl::mesh::append_skinned(mesh*, const mat4*, int, unsigned, char))
    void add_quad(const vector4* p0, const vector4* p1, const vec2* uv0, const vec2* uv1, const unsigned* color);   // 0x44e970 (ret 0x14: a two-point quad; world_data/obj_tbl.cpp)
    void scale(float s);                            // 0x44f0c0 (ret 4)
    void set_color(vector4 c);                      // 0x47af30 (ret 0x10: the colour multiplier of every vertex)
    void set_bone_index(int i);                     // 0x47b360 (ret 4)
    void set_part(const matrix* frame);            // 0x495ba0 (ret 4)
    void calc_bounds(vector4* lo, vector4* hi);     // 0x438d80 (ret 8)
    void set_offset(const vector4& d);              // 0x4fb8e0 (agent_hud.cpp: shifts every point along its vertex normal)
    void darken_vertices();                         // 0x4fbef0 (agent_hud.cpp)
};

// The game's own growable array (VC7.1 vector layout: allocator slot, first, last, end) whose clear() frees the
// block, unlike std::vector::clear. Also used by ../../game/entity.h for the collision-shape arrays.
template<class T> struct vec {
    int alloc;
    T* first; T* last; T* end;
    void clear() { if (first != 0) operator delete(first); first = 0; last = 0; end = 0; }
};

// std::vector<void*> with its copy-assignment out of line (0x43f0a0)
struct ptr_list : std::vector<void*> {
    ptr_list& operator=(const ptr_list& o);                      // 0x43f0a0
};

struct world;

// The render scene (0x1d8 bytes, ctor 0x429740, reset 0x428f30, dtor 0x41f4e0 in src/game/entity.cpp): viewport / camera block
// with the view frustum at +0x30. Offsets from the constructor 0x429740 (the field stores), reset 0x428f30, Ctx::render_scene
// 0x429890 (reflection pass), ~scene 0x41f4e0 and the mesh code (mesh::render 0x413750, scene_registry 0x412040/0x412100).
// Queue entries are (mesh pointer, 0.0f) idx_weights in scene+0x1b4; the scene itself is queued in the global
// registry 0x5fe898 (vector of scene* at +0x3a4) the first time something is added.
struct scene {
    int f0, f4, f8, fc, f10;    // 0x00
    float f14;                  // 0x14
    int f18, f1c, f20, f24, f28;    // 0x18
    float f2c;                  // 0x2c
    frustum fr;                 // 0x30 (planes from 0x38, count at 0x34, camera frame at 0xf8)
    texture* target;            // 0x150 render target of this pass
    texture* rt;                // 0x154 render target of the reflection pass
    scene* refl;                // 0x158 lazily created reflection scene
    int f15c;                   // 0x15c
    char f160, f161, f162, pad163;
    int f164;                   // 0x164
    char f168, f169, f16a, pad16b;
    int f16c, f170, f174;       // 0x16c
    float f178;                 // 0x178 clip height
    vector4 ambient;            // 0x17c
    unsigned flags;             // 0x18c (bit 12: queue at the end instead of the front, register 0x412040)
    int f190;                   // 0x190
    ptr_list list;              // 0x194 attached objects (obj_c0*) collected this frame (the world's light list is copied here)
    bool registered;            // 0x1a4
    char f1a5, pad1a6[2];       // 0x1a5
    unsigned mask;              // 0x1a8
    bool active;                // 0x1ac
    char pad1ad[3];
    void* f1b0;                 // 0x1b0
    std::vector<idx_weight> queue;   // 0x1b4
    union {
        world* owner;           // 0x1c4 (scene modules)
        mesh_pool* view;        // 0x1c4: the particle mesh pool / camera (psys::update)
    };
    std::vector<mesh*> cache;   // 0x1c8: one mesh per resource
    scene();                    // 0x429740
    ~scene();                   // 0x41f4e0 (src/game/entity.cpp)
    void reset();               // 0x428f30
    mesh* get_mesh(skin* r);                        // 0x412c30
};

struct scene_registry {          // global object 0x5fe898
    char pad0[0x3a4];
    std::vector<scene*> scenes;  // 0x3a4
    void register_scene(scene* s);                  // 0x412040
    void add_mesh(scene* s, mesh* m);               // 0x412100
    bool render(scene* sc);                         // 0x47e520 (src/game/party2: render the scene into its target; false = try again)
};
extern scene_registry g_scene_registry;             // 0x5fe898  // old name of g_batch, removed after the names merge

struct mesh_list {               // 16 bytes: std::vector<mesh*>
    std::vector<mesh*> v;
    void render_all(scene* sc, const matrix& m);    // 0x413ac0
};
struct lod_mesh {
    char pad0[0x34];
    mesh_list lists[5];          // 0x34
    int count;                   // 0x84
    int cur;                     // 0x88
    matrix m8c;                  // 0x8c
    unsigned mask;               // 0xcc
    void render(scene* sc, const matrix& m);        // 0x413b00
};

extern int g_vert_budget;        // 0x5a9c34 base vertex budget of should_batch (provisional name)

} // namespace rgl
#endif

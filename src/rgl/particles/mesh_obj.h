// mesh_obj.h: the classes of mesh_obj.cpp (0x40e940..0x413b5f): the 0x260-byte mesh object (rgl::mesh, vtable
// 0x54a6f4, ctor 0x412a90, dtor 0x4127f0), the scene it is rendered into, the particle emitter that hangs off
// it (mesh+0xb0) and the element types of the std::vector instantiations the linker placed in this object.
//
// DROP-IN REPLACEMENT for src/rgl/mesh.h (see MIGRATION.md, "Header conflicts"): same class names, field
// names and offsets, plus what the destructor 0x4127f0, the copy-assignments and the particle code of this
// object need and mesh.h could not hold: `skin_vert` (was elem30), `bone_influence` (was elem14: its items
// are 8-byte (index, weight) records, not int), `frame_t` with its out-of-line ctor, `keyframes` with its
// destructor, the owned attachment pointer, `aabb`'s ctor, the scene's registration fields, `mesh_list`,
// `lod_mesh`, and the members mesh.h did not declare. It is a separate file because this directory may not
// edit src/rgl/mesh.h; do not include both in one translation unit (the class names are the same).
//
// All names provisional except where an address is cited; every offset below is read or written by a
// function of this module (the citing function address is given).
#ifndef RGL_MESH_OBJ_H
#define RGL_MESH_OBJ_H
#include <vector>
#include <stddef.h>
#include <string.h>
#include "../rgl_string.h"
#include "../rgl_math.h"
#include "../../include/vec.h"
#include "../../include/mesh_prims.h"
#include "../resource.h"

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

// inline vector helpers of the mesh and particle code (rgl_math.h declares the out-of-line operators
// 0x407cd0 / 0x407d00; these are what the callers inline). v * s and s * v take s by reference.
inline vector4 vscale(float s, const vector4& v) { return vector4(s * v.x, s * v.y, s * v.z); }
inline vector4 vmul(const vector4& a, const float& s) { return vector4(a.x * s, a.y * s, a.z * s); }
inline vector4 vmul(const float& s, const vector4& a) { return vector4(s * a.x, s * a.y, s * a.z); }
inline vector4 vadd(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
__forceinline vector4 operator+(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }

// 0x40e4e0 as setup_material calls it: the hidden return pointer form of rgl_math.h's unpack_color(out, argb)
// (same cdecl call, but the result is copied out of a temporary)
vector4 unpack_color_ret(const unsigned* argb);                         // 0x40e4e0

matrix* identity(matrix* out);                                          // 0x40cd00 (cdecl, returns out)

// the matrix operations that rgl_math.h does not declare (callers: emitter::spawn 0x40ec90)
struct frame_ops : matrix {
    vector4 transform_point(const vector4& p) const;                     // 0x408460
    vector4 rot(const vector4& p) const;                                 // 0x40a9c0 (rotates by the s, f, u rows)
};

// ---- element types of the mesh's std::vectors ---------------------------------------------------------------
struct vertex60 {                // 0x60 bytes, vector at mesh+0x50 (index read by build_skin 0x411d20)
    vector4 pos;                 // 0x00
    vector4 f10, f20;
    vector4 normal;              // 0x30
    vec2 uv;                     // 0x40
    int f48, f4c;
    int index;                   // 0x50: point index
    unsigned color;              // 0x54: packed ARGB
    int f58;                     // 0x58: linear colour (0x419360)
    int f5c;
};
// 0x30 bytes, vector at +0x60: per-vertex skinning record (build_skin 0x411d20 writes w[], bone[], bonef[])
struct skin_vert { float w[4]; int bone[4]; float bonef[4]; };
// 8 bytes: (vertex index, weight). Also the scene's queue entries (key = the mesh pointer, weight 0).
struct idx_weight { int index; float weight; };
// 0x14 bytes, vector at +0x80: the vertices one bone influences (build_skin 0x411d20)
struct bone_influence { int bone; std::vector<idx_weight> verts; };
// face20 (0x20 bytes, vector at +0x90; a, b, c at +0x10: add_face 0x411920) is in src/include/mesh_prims.h

// 0x20 bytes: one morph key frame (two vector4 arrays). Out-of-line ctor 0x411780, copy 0x411c10.
struct frame_t {
    std::vector<vector4> verts, norms;
    frame_t();                                      // 0x411780
    void copy_from(const frame_t& o);               // 0x411c10
};

} // namespace rgl
#include "../../include/keyframes.h"      // rgl::keyframes (frame_t above)
namespace rgl {

// object attached at mesh+0xc0, deleted with the mesh (~mesh 0x4127f0)
struct obj_c0 { char pad[0x24]; vector4 out; vector4 in; };


struct slot {                    // 0x2c bytes, array of 3 at mesh+0x110 (ctor loop in mesh::mesh 0x412a90)
    int f0, f4, f8, fc;
    int f10, f14;
    int pa, pb;                  // 0x18
    int f20, f24;
    char f28, f29;               // slots[0].f28 (0x138) and slots[1].f28 (0x164) are dirty flags
    slot()
    {
        f0 = 0; f4 = 0; f8 = 0; fc = 0;
        f28 = 1; f29 = 0;
        memset(&pa, 0, 8);
        f20 = 0; f24 = -1;
    }
};

struct scene;
// ---- the particle emitter (hangs off mesh+0xb0) ----------------------------------------------------------------
// The same object as src/rgl/psys.h's `psys` (flags +0xe8; psys::update 0x414960 and psys::emit 0x413e50 are
// its render-time methods): this class has the simulation fields (update 0x412150, spawn 0x40ec90) and the
// particle vector psys.h holds as raw pointers.
// Module System particle-system flags (header_particle_systems.py)
enum {
    psf_always_emit         = 0x0000000002,
    psf_global_emit_dir     = 0x0000000010,
    psf_emit_at_water_level = 0x0000000020,
    psf_billboard_2d        = 0x0000000100,
    psf_billboard_3d        = 0x0000000200,
    psf_turn_to_velocity    = 0x0000000400,
    psf_randomize_rotation  = 0x0000001000,
    psf_randomize_size      = 0x0000002000
};
struct emitter_particle {        // 0x64 bytes (same record as psys.h's `particle`)
    vector4 s, f, u;             // 0x00 orientation rows (spawn 0x40ec90 copies the emitter frame here)
    vector4 pos;                 // 0x30
    vector4 vel;                 // 0x40 (psys.h: dir)
    float age;                   // 0x50 0..1, dies at >= 1 (update 0x412150)
    float f54;
    float angle;                 // 0x58
    float angvel;                // 0x5c
    float scale;                 // 0x60
};
struct emitter {
    char pad0[0xc];
    std::vector<emitter_particle> particles;   // +0x0c (first +0x10)
    float noise_scale;           // +0x1c
    float noise_amount;          // +0x20
    float drag;                  // +0x24
    char pad28[0xa0 - 0x28];
    float lifetime;              // +0xa0
    char pada4[0xc4 - 0xa4];
    vector4 accel;               // +0xc4
    float accel_scale;           // +0xd4
    char padd8[0xe8 - 0xd8];
    unsigned flags;              // +0xe8 psf_*
    float water_z;               // +0xec
    int disabled;                // +0xf0
    matrix world;                // +0xf4
    matrix local;                // +0x134
    float box[3];                // +0x174 emission box half-extents
    char pad180[4];
    vector4 vel_base;            // +0x184
    float vel_scale;             // +0x194
    float f198;                  // +0x198 spawn rate
    int spawn_left;              // +0x19c
    int spawn_total;             // +0x1a0
    float spawn_accum;           // +0x1a4
    char pad1a8[4];
    float radius;                // +0x1ac
    float spin_scale;            // +0x1b0
    float spin_drag;             // +0x1b4
    int noise_index;             // +0x1b8 (indexes g_noise_table)
    void update(const float* dt, const matrix& frame);      // 0x412150
    void spawn(emitter_particle* out);                      // 0x40ec90
    void render_update(scene* sc);                          // 0x414960 (src/rgl/psys.cpp: psys::update)
};
extern __int64 g_noise_table[];                             // 0x5e2948

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
    std::vector<emitter*> kids;  // 0xb0: particle emitters (render calls psys::update 0x414960 on them)
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
};

// Queue entries are (mesh pointer, 0.0f) idx_weights in scene+0x1b4; the scene itself is queued in the global
// registry 0x5fe898 (vector of scene* at +0x3a4) the first time something is added.
struct scene {
    char pad0[0x18c];
    unsigned flags;              // 0x18c (bit 12: queue at the end instead of the front, register 0x412040)
    char pad190[4];
    std::vector<obj_c0*> list;   // 0x194: attached objects collected this frame
    bool registered;             // 0x1a4
    char pad1a5[3];
    unsigned mask;               // 0x1a8
    bool active;                 // 0x1ac
    char pad1ad[7];
    std::vector<idx_weight> queue;   // 0x1b4
    mesh_pool* view;             // 0x1c4: the particle mesh pool / camera (psys::update)
    std::vector<mesh*> cache;    // 0x1c8: one mesh per resource
    mesh* get_mesh(skin* r);                        // 0x412c30
};

struct scene_registry {          // global object 0x5fe898
    char pad0[0x3a4];
    std::vector<scene*> scenes;  // 0x3a4
    void register_scene(scene* s);                  // 0x412040
    void add_mesh(scene* s, mesh* m);               // 0x412100
};
extern scene_registry g_scene_registry;             // 0x5fe898

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

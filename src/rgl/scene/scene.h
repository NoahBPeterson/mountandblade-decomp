// scene.h: the render scene (0x1d8 bytes, ctor 0x429740, reset 0x428f30), the render_info out parameter of the
// entity render slot, the LOD record, and the mesh-side helpers the scene modules call.
//
// ../mesh.h (the mesh object) defines a partial rgl::scene that cannot be extended from outside; it is renamed
// while that header is included, and this header defines the full class. Calls into mesh::render therefore cast
// the scene pointer (mesh_render below). Report: ../mesh.h's scene should be replaced by this one.
#ifndef SCENE_SCENE_H
#define SCENE_SCENE_H
#include <vector>
#include "./scene_math.h"
#include "./frustum.h"
#include "./emitter.h"
#define scene mesh_h_scene
#include "../mesh.h"
#undef scene

namespace rgl {

struct world;

// The game's own growable array (VC7.1 vector layout: allocator slot, first, last, end) whose clear() frees the
// block, unlike std::vector::clear. Also used by ../../game/entity.h for the collision-shape arrays.
template<class T> struct vec {
    int alloc;
    T* first; T* last; T* end;
    void clear() { if (first != 0) operator delete(first); first = 0; last = 0; end = 0; }
};

// out parameter of the entity render slot: a hit flag and the view depth of the first point (0x420310)
struct render_info { char valid; float depth; };

// std::vector<void*> with its copy-assignment out of line (0x43f0a0)
struct ptr_list : std::vector<void*> {
    ptr_list& operator=(const ptr_list& o);                      // 0x43f0a0
};

// the render scene: viewport / camera block with the view frustum at +0x30. Offsets from the constructor 0x429740
// (the field stores), reset 0x428f30, Ctx::render_scene 0x429890 (reflection pass) and ~scene 0x41f4e0.
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
    int f18c, f190;             // 0x18c, 0x190
    ptr_list list;              // 0x194 attached objects collected this frame (the world's light list is copied here)
    char f1a4, f1a5, pad1a6[2]; // 0x1a4
    unsigned mask;              // 0x1a8
    char f1ac, pad1ad[3];       // 0x1ac
    void* f1b0;                 // 0x1b0
    std::vector<void*> v1b4;    // 0x1b4
    world* owner;               // 0x1c4
    std::vector<mesh*> cache;   // 0x1c8 one mesh per resource
    scene();                    // 0x429740
    ~scene();                   // 0x41f4e0 (src/game/entity.cpp)
    void reset();               // 0x428f30
};

// mesh::render takes ../mesh.h's partial scene
inline int mesh_render(mesh* m, scene* sc, const matrix& frame) { return m->render((mesh_h_scene*)sc, frame); }

// ---- mesh members the scene modules add (no new fields: the derived class only carries methods) ------------
struct mesh_ext : mesh {
    std::vector<point20>& get70_();                             // 0x40eba0 (out-of-line copy of get70)
    std::vector<elem30>& get60_();                              // 0x40ec60 (out-of-line copy of get60)
    void ensure_skin();                                         // 0x412700
    void update_emitters(const float* dt, const matrix& frame); // 0x4126a0
    void clear_geometry();                                      // 0x428e80
    mesh* clone();                                              // 0x41c6d0 (a copy of a resource mesh)
    mat4*& bone_mats() { return *(mat4**)&f220; }               // 0x220: 32 matrices, allocated on first skinning (0x4291c0)
    const void*& bone_src() { return *(const void**)&f224; }    // 0x224
    int& nbones() { return *(int*)&f228; }                      // 0x228: max bone index + 1
};

// std::vector<point20>::iterator and operator[] const stay out of line in entity::render
struct iter32 {
    point20* _Ptr;
    iter32() {}
    iter32(point20* p) : _Ptr(p) {}
    iter32 operator+(int off) const;                            // 0x40f880
};
struct v70_view : std::vector<point20> {
    const point20& at(unsigned i) const;                        // 0x40fb80
};

// five lists of child meshes owned by an LOD record; the destructor deletes every child
struct mesh_list2 {
    std::vector<mesh*> v;
    void set_color(const int& c);                               // 0x421990 (tints every mesh)
    ~mesh_list2() { for (std::vector<mesh*>::iterator it = v.begin(); it < v.end(); ++it) delete *it; }
};
struct lod_mesh2 {              // 0xe0 bytes; the record of mesh.h's lod_mesh with the name at 0xd0
    char pad0[0x34];
    mesh_list2 lists[5];        // 0x34
    int count;                  // 0x84
    int cur;                    // 0x88
    matrix m8c;                 // 0x8c
    unsigned mask;              // 0xcc
    string name;                // 0xd0
    void select(const float* dist);                             // 0x4205b0
    void render(scene* sc, const matrix& m);                    // 0x413b00
};

// resource-manager members the scene code calls that ../resource.h does not declare (no new fields)
struct resmgr_ext : resource_manager {
    void release(void* res);                                    // 0x4175f0 (clears field +0x14 of the resource)
    mesh_ext* get_object(const string* name);                      // 0x419540 (a mesh resource by name; callers clone it)
    texture* get_render_target(int w, int h, int fmt, int ext, int f34);   // 0x41b7d0
};
inline resmgr_ext& resmgr() { return *(resmgr_ext*)&g_resmgr; }

// the global that collects the scenes being rendered (object 0x5fe898; mesh.h's scene owner calls it)
struct scene_registry {
    void register_obj(scene* sc);                               // 0x412040
};
extern scene_registry g_scene_reg;                              // 0x5fe898

} // namespace rgl
#endif

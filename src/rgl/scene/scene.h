// scene.h: the render scene (0x1d8 bytes, ctor 0x429740, reset 0x428f30), the render_info out parameter of the
// entity render slot, the LOD record, and the mesh-side helpers the scene modules call.
//
// The scene itself (and render_info, ptr_list, vec, scene_registry) is defined once in ../mesh.h.
#ifndef SCENE_SCENE_H
#define SCENE_SCENE_H
#include <vector>
#include "./scene_math.h"
#include "./frustum.h"
#include "./emitter.h"
#include "../mesh.h"           // mesh, scene (full), scene_registry, ptr_list, render_info, vec
#include "../mesh_x.h"         // mesh_x (clone 0x41c6d0)

namespace rgl {

struct world;

inline int mesh_render(mesh* m, scene* sc, const matrix& frame) { return m->render(sc, frame); }

// ---- mesh members the scene modules add (no new fields: the derived class only carries methods) ------------
struct mesh_ext : mesh {
    // (the out-of-line copy 0x40eba0 of get70 is mesh::get70 itself, __declspec(noinline) in ../mesh.h)
    std::vector<skin_vert>& get60_();                              // 0x40ec60 (out-of-line copy of get60)
    void clear_geometry();                                      // 0x428e80
    // (clone, 0x41c6d0: mesh_x::clone of ../mesh_x.h, called as ((mesh_x*)m)->clone())
    mat4*& bone_mats() { return *(mat4**)&p220; }               // 0x220: 32 matrices, allocated on first skinning (0x4291c0)
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
// (the out-of-line operator[] const of the point vector, 0x40fb80, is std::vector<point20>::operator[]: declared below, outside rgl)

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
    // (render 0x413b00 is rgl::lod_mesh::render, mesh.h: call it through ((lod_mesh*)group)->render)
};

} // namespace rgl

// resource-manager members the scene code calls that ../resource.h does not declare (no new fields): the definitions are
// rgl/core/resmgr.cpp's `resmgr` (global class), which derives from resource.h's resource_manager the same way
struct lod_group;
struct resmgr : resource_manager {
    // (release 0x4175f0, which clears field +0x14 of the resource, is the free stdcall clear_f14: CALL_STDCALL_ON below)
    rgl::mesh* get_object(const rgl::string* name);             // 0x419540 (a mesh resource by name; callers clone it)
    texture* get_render_target(int w, int h, int fmt, int ext, int f34);   // 0x41b7d0
    std::vector<rgl::mesh*> find_meshes(const rgl::string* prefix);      // 0x41b960 (hidden return pointer, ret 4)
    lod_group* make_lod_mesh(const rgl::string& pat, bool copy);         // 0x41ca60 (ret 8)
    lod_group* get_lod_mesh(const rgl::string& name);                    // 0x41cdc0
    texture* get_texture(int w, int h, int fmt);                           // 0x41c400
    skin* get_shadow_skin(int w, int h, int fmt, int f34);                 // 0x41c420
};
#include "../../game/world_data/thiscall_free.h"                    // CALL_STDCALL_ON
struct named_object;
void __stdcall clear_f14(named_object* o);                      // 0x4175f0 (the resource manager's release; mem_stream.cpp)
namespace rgl {
typedef ::resmgr resmgr_ext;
inline ::resmgr& resmgr() { return *(::resmgr*)&g_resmgr; }

} // namespace rgl

// std::vector<point20>::operator[] const stays out of line in entity::render (0x40fb80), like the original
namespace std {
template<> const rgl::point20& vector<rgl::point20, allocator<rgl::point20> >::operator[](size_t) const;
}
#endif

// scene.h: the render scene (0x1d8 bytes, ctor 0x429740, reset 0x428f30), the render_info out parameter of the
// entity render slot and the mesh-side helpers the scene modules call.
//
// The scene itself (and render_info, ptr_list, vec) is defined once in ../mesh.h.
#ifndef SCENE_SCENE_H
#define SCENE_SCENE_H
#include <vector>
#include "./scene_math.h"
#include "./frustum.h"
#include "./emitter.h"
#include "../mesh.h"           // mesh, scene (full), ptr_list, render_info, vec
#include "../mesh.h"         // rgl::mesh (clone 0x41c6d0)

namespace rgl {

struct world;

inline int mesh_render(mesh* m, scene* sc, const matrix& frame) { return m->render(sc, frame); }

// std::vector<point20>::iterator and operator[] const stay out of line in entity::render
// (the out-of-line operator[] const of the point vector, 0x40fb80, is std::vector<point20>::operator[]: declared below, outside rgl)

} // namespace rgl

struct named_object;

// std::vector<point20>::operator[] const stays out of line in entity::render (0x40fb80), like the original
namespace std {
template<> const rgl::point20& vector<rgl::point20, allocator<rgl::point20> >::operator[](size_t) const;
}
#endif

// mesh_prims.h: geometry records that the mesh (rgl::mesh, 0x260 bytes) and the code around it embed or index.
//   aabb    : axis-aligned box with centre and radius (0x34 bytes; mesh+0x22c is mesh::bounds, also ResourceInfo::bounds).
//             ctor 0x40eaf0 (src/rgl/particles/mesh_obj.cpp), reset 0x416e40 / extend 0x416ef0 / update 0x416f70
//             (src/rgl/core/resource_io.cpp), extend_box 0x4201b0 (src/game/strat).
//   point20 : 0x20 bytes, the element of mesh::v70 (vector at mesh+0x70: rest, current).
//   face20  : 0x20 bytes, the element of the face vector at mesh+0x90 (normal and four indices; add_face 0x411920).
// Namespace rgl. Needs rgl::vector4 (rgl_base.h picks the module's own declaration).
#ifndef MB_MESH_PRIMS_H
#define MB_MESH_PRIMS_H
#include "./rgl_base.h"
#include "./mat4.h"

namespace rgl {

struct aabb {
    vector4 min;                 // +0
    vector4 max;                 // +0x10
    vector4 center;              // +0x20
    float radius;                // +0x30
    __declspec(noinline) aabb();                        // 0x40eaf0
    void reset();                                       // 0x416e40
    void extend(const vector4* p);                      // 0x416ef0
    void update();                                      // 0x416f70
    void extend_box(const aabb* b, const mat4* m);      // 0x4201b0
};

struct point20 { vector4 a, b; };           // 0x20, vector at +0x70 (rest, current)

// 0x20 bytes, vector at mesh+0x90 (mesh_obj.h; strat_ext.h's twin of the mesh): a, b, c at +0x10 (add_face 0x411920)
struct face20 { vector4 n; int a, b, c, d; };

} // namespace rgl

#endif // MB_MESH_PRIMS_H

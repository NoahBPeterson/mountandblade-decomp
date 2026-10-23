// frustum.h: the view frustum (0x120 bytes; rgl::scene embeds it at +0x30, CellGrid's ViewParams at +0x34).
//   Constructor 0x41ef60 (src/rgl/scene/world_items.cpp: flags, plane count, frame at +0xc8, six clip floats),
//   plane builder 0x41e4f0 and sphere test 0x41e470 (src/rgl/core/frustum_grid.cpp), Ctx::render_scene 0x429890
//   (planes[i].in_front, nplanes, build_planes, the frame at +0xc8 mirrored for the reflection pass),
//   projection 0x4620d0 / view_matrix 0x462170 / to_clip 0x4621f0 (src/game/renderer.cpp, from the layout below).
// One record seen by four modules (rgl scene, game renderer, terrain_b, strat); this is the union of their views.
#ifndef MB_FRUSTUM_H
#define MB_FRUSTUM_H
#include "./rgl_base.h"
#include "./mat4.h"

struct D3DXMATRIX;

namespace rgl {

struct plane {                  // 0x20 bytes
    vector4 pos, normal;
    int in_front(const vector4* p, float eps) const;            // 0x41e470 (sphere centre p, radius eps)
};

struct frustum {
    char perspective;           // +0   (0x4620d0, 0x4621f0; shadow_tick 0x42b790 clears it for the orthographic shadow camera)
    char left_handed;           // +1   (0x4620d0; the ctor stores 0; the slices called it flag1)
    char pad2[2];
    int nplanes;                // +4
    plane planes[6];            // +8
    matrix m;                   // +0xc8 camera frame (s = right, f = up, u = backwards)   0x462170, 0x4621f0 (a mat4 there)
    float left, right, top, bottom, nearz, farz;    // +0x108   0x4620d0, 0x4621f0
    frustum();                  // 0x41ef60
    void build_planes();        // 0x41e4f0
    D3DXMATRIX projection();    // 0x4620d0 (hidden return pointer)
    D3DXMATRIX view_matrix();   // 0x462170 (hidden return pointer; src/game/renderer.cpp defines it as frustum_view::view_matrix, a mat4 result)
    vector4 to_clip(const vector4& p);              // 0x4621f0 (hidden return pointer)
};

} // namespace rgl

#endif // MB_FRUSTUM_H

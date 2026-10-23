// frustum.h: the view frustum (0x120 bytes) embedded in the render scene (+0x30) and in CellGrid's ViewParams (+0x34).
// Layout from frustum::projection 0x4620d0 / view_matrix 0x462170 (src/game/renderer.cpp) and the scene renderer
// 0x46e590; the constructor 0x41ef60 is in an earlier object. src/game/renderer.h and src/rgl/scene/frustum.h declare
// their own views of the same record (see MIGRATION.md, "Header conflicts").
#ifndef TERRAIN_B_FRUSTUM_H
#define TERRAIN_B_FRUSTUM_H
#include <d3dx9.h>
#include "../../rgl/rgl_math.h"

struct pos : rgl::matrix {
    rgl::matrix relative(const rgl::matrix& o) const;     // 0x4329d0
};
struct plane4 { rgl::vector4 pos, normal; };
struct frustum {
    char perspective;               // +0
    char left_handed;               // +1
    char pad2[2];
    int nplanes;                    // +4
    plane4 planes[6];               // +8
    pos m;                          // +0xc8 camera frame (s = right, f = up, u = backwards)
    float left, right, top, bottom, nearz, farz;   // +0x108
    frustum();                              // 0x41ef60 (CellGrid's ViewParams)
    D3DXMATRIX projection();                // 0x4620d0 (hidden return pointer)
    D3DXMATRIX view_matrix();               // 0x462170 (hidden return pointer)
};

#endif // TERRAIN_B_FRUSTUM_H

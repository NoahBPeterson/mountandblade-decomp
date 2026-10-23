// frustum.h: the view frustum (0x120 bytes) embedded in the render scene (+0x30) and in CellGrid's ViewParams (+0x34).
// Layout from frustum::projection 0x4620d0 / view_matrix 0x462170 (src/game/renderer.cpp) and the scene renderer
// 0x46e590; the constructor 0x41ef60 is in an earlier object. The class is shared (src/include/frustum.h); terrain_b
// reads its frame with the 4x4 view/projection helpers as D3DXMATRIX.
#ifndef TERRAIN_B_FRUSTUM_H
#define TERRAIN_B_FRUSTUM_H
#include <d3dx9.h>
#include "../../rgl/rgl_math.h"
#include "../../include/mat4.h"
#include "../../include/frustum.h"

using rgl::frustum;

#endif // TERRAIN_B_FRUSTUM_H

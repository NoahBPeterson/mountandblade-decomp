// cs_mat.h: the identity writers, which are global-scope functions (rgl/shapes/shapes.cpp): identity_pos on the global `pos`,
// identity3 on rgl::mat3 (both cdecl and returning their argument). Separate from cs_common.h because include/mat4.h cannot be
// included together with the rgl_mesh.h universe.
#ifndef CHAR_SHEET_CS_MAT_H
#define CHAR_SHEET_CS_MAT_H
#include "../../include/mat4.h"             // rgl::mat3, the global `pos`, rgl::matrix
pos* __cdecl identity_pos(pos* out);                                // 0x40cd00
rgl::mat3* __cdecl identity3(rgl::mat3* out);                       // 0x40cd60
#endif

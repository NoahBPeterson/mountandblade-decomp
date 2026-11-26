// renderer_xform.h: a frame as a D3D matrix. Only renderer.cpp (set_view) and config.cpp (renderer::renderer) use it, so it is
// not part of renderer.h (a static inline there shifts the compiler's symbol numbering in every TU that includes the header).
#ifndef MB_RENDERER_XFORM_H
#define MB_RENDERER_XFORM_H
#include "engine/render/renderer_include.h"
#include "engine/render/d3d_xform.h"

// a frame as a D3D matrix (rows s, f, u, o) with the w column forced to (0, 0, 0, 1)
static inline void to_d3d(D3DMATRIX* out, const rgl::matrix& m)
{
    *out = *as_d3d(m);      // the 16 floats in place (a struct copy), then the w column
    out->_14 = 0.0f; out->_24 = 0.0f; out->_34 = 0.0f; out->_44 = 1.0f;
}

#endif // MB_RENDERER_XFORM_H

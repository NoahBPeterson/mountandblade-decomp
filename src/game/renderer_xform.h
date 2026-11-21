// renderer_xform.h: a frame as a D3D matrix. Only renderer.cpp (set_view) and config.cpp (renderer::renderer) use it, so it is
// not part of renderer.h (a static inline there shifts the compiler's symbol numbering in every TU that includes the header).
#ifndef MB_RENDERER_XFORM_H
#define MB_RENDERER_XFORM_H
#include "../include/renderer.h"

// a frame as a D3D matrix (rows s, f, u, o) with the w column forced to (0, 0, 0, 1)
static inline void to_d3d(D3DMATRIX* out, const rgl::matrix& m)
{
    out->_11 = m.s.x; out->_12 = m.s.y; out->_13 = m.s.z; out->_14 = 0.0f;
    out->_21 = m.f.x; out->_22 = m.f.y; out->_23 = m.f.z; out->_24 = 0.0f;
    out->_31 = m.u.x; out->_32 = m.u.y; out->_33 = m.u.z; out->_34 = 0.0f;
    out->_41 = m.o.x; out->_42 = m.o.y; out->_43 = m.o.z; out->_44 = 1.0f;
}

#endif // MB_RENDERER_XFORM_H

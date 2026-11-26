// d3d_xform.h: a frame (rgl::matrix) as the D3DMATRIX the device takes. Its own header, not part of renderer.h: an inline there
// moves cl's numbering in every TU that includes it (launcher_options_dialog.cpp's http_fetch changed bytes).
#ifndef MB_D3D_XFORM_H
#define MB_D3D_XFORM_H
#include "engine/render/renderer_include.h"

// a frame as the D3D matrix it is laid out as (rows s, f, u, o, every lane carried; D3DMATRIX is the SDK's, so this is the one conversion)
HRESULT set_transform(IDirect3DDevice9* dev, D3DTRANSFORMSTATETYPE state, const rgl::matrix& m);   // SetTransform of the frame (engine/physics/collision_shapes.cpp)

// the frame in place as the SDK's D3DMATRIX (the same 16 floats: rows s, f, u, o); SetTransform takes it by pointer, no copy
static inline const D3DMATRIX* as_d3d(const rgl::matrix& m) { return reinterpret_cast<const D3DMATRIX*>(&m); }

#endif // MB_D3D_XFORM_H

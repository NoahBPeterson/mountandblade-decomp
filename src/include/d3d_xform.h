// d3d_xform.h: a frame (rgl::matrix) as the D3DMATRIX the device takes. Its own header, not part of renderer.h: an inline there
// moves cl's numbering in every TU that includes it (options_dlg.cpp's http_fetch changed bytes).
#ifndef MB_D3D_XFORM_H
#define MB_D3D_XFORM_H
#include "renderer.h"

// a frame as the D3D matrix it is laid out as (rows s, f, u, o, every lane carried; D3DMATRIX is the SDK's, so this is the one conversion)
HRESULT set_transform(IDirect3DDevice9* dev, D3DTRANSFORMSTATETYPE state, const rgl::matrix& m);   // SetTransform of the frame (shapes/shapes.cpp)

#endif // MB_D3D_XFORM_H

//-----------------------------------------------------------------------------
// File: mesh_vert_ray.cpp  (0x52dae0..0x52dcc0)
//
// Desc: one object of mesh_3's range, split where the original's caller / callee objects meet: Mesh::vert_ray (the ray-vs-vertex distance) and pick_edge, which calls edge_ray (mesh_edge_ray.cpp).
//       pick_vert / pick_edge / pick_poly call the ray functions as external functions (the original compiled them in
//       separate objects). Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "./mesh_3.h"
#include "../strat/gui_manager.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../tstring.h"
#include "../../rgl/rgl_math.h"
#include "../../rgl/stream.h"
#include "../../rgl/mesh.h"
#include "../float_consts.h"   // shared float constants kept in memory (0x549cdc ...)

#include "../key_names.h"

static __forceinline float vdot(const rgl::vector4& a, const rgl::vector4& b)
{ return a.x * b.x + (a.y * b.y + a.z * b.z); }

// @ 0x52dae0
void Mesh_s2da20::vert_ray(const rgl::vector4& p, const rgl::vector4& d, unsigned& index,
                               float* a, float* b)
{
    const mface_s2da20& v = verts.first[index];
    float t = vdot(v.pos - p, d);
    rgl::vector4 q;
    if (t < 0.0f)
        q = p;
    else
        q = p + d * t;
    *a = (q - v.pos).length();
    *b = (q - p).length();
}

// @ 0x52dc00
unsigned Mesh_s2da20::pick_edge(const rgl::vector4& p, const rgl::vector4& d)
{
    float best = 1e8f;
    unsigned besti = (unsigned)-1;
    for (unsigned i = 0; i < (unsigned)edges.size(); i++) {
        float a, b;
        edge_ray(p, d, i, &a, &b);
        if (b < 2.0f)
            b = 2.0f;
        float ratio = a / b;
        if (ratio < 0.1f && ratio < best) {
            besti = i;
            best = ratio;
        }
    }
    return besti;
}

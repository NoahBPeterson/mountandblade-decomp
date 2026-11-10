//-----------------------------------------------------------------------------
// File: mesh_edge_ray.cpp  (0x52dcc0..0x52dfd0)
//
// Desc: one object of mesh_3's range, split where the original's caller / callee objects meet: Mesh::edge_ray (the ray-vs-edge distance) and pick_poly, which calls poly_ray (mesh_poly_ray.cpp).
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

// @ 0x52dcc0
void Mesh_s2da20::edge_ray(const rgl::vector4& p, const rgl::vector4& d, unsigned& index,
                               float* a, float* b)
{
    const medge_s2da20& e = edges.first[index];
    const rgl::vector4& v1 = verts.first[e.v0].pos;
    const rgl::vector4& v2 = verts.first[e.v1].pos;

    rgl::vector4 end = p + d * 1000.0f;
    rgl::vector4 u = end - p;
    rgl::vector4 w = v2 - v1;
    rgl::vector4 rv = p - v1;

    float e1 = w.x * rv.x + w.y * rv.y + w.z * rv.z;       // v . r
    float b1 = u.x * w.x + u.y * w.y + u.z * w.z;          // u . v
    float c1 = w.x * w.x + w.y * w.y + w.z * w.z;          // v . v
    float a1 = u.x * u.x + u.y * u.y + u.z * u.z;          // u . u

    float den = a1 * c1 - b1 * b1;
    float s = (b1 * e1 - c1 * (u.x * rv.x + u.y * rv.y + u.z * rv.z)) / den;
    float t = (e1 + s * b1) / c1;                       // edge parameter from the UNCLAMPED ray parameter

    if (s < 0.0f)
        s = 0.0f;
    else if (s > 1.0f)
        s = 1.0f;
    if (t < 0.0f)
        t = 0.0f;
    else if (t > 1.0f)
        t = 1.0f;

    rgl::vector4 r = p + u * s;
    rgl::vector4 q = v1 + w * t;

    *a = (q - r).length();
    *b = (q - p).length();          // distance from the ray origin to the edge point
}

// @ 0x52df40
unsigned Mesh_s2da20::pick_poly(const rgl::vector4& p, const rgl::vector4& d)
{
    float best = 1e8f;
    unsigned besti = (unsigned)-1;
    for (unsigned i = 0; i < (unsigned)polys.size(); i++) {
        float dist;
        poly_ray(p, d, i, &dist);
        if (dist < best) {
            best = dist;
            besti = i;
        }
    }
    return besti;
}

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

    float endx = d.x * 1000.0f + p.x;
    float endy = d.y * 1000.0f + p.y;
    float endz = d.z * 1000.0f + p.z;
    float ux = endx - p.x;
    float uy = endy - p.y;
    float uz = endz - p.z;

    float wx = v2.x - v1.x;
    float wy = v2.y - v1.y;
    float wz = v2.z - v1.z;

    float aw = p.x - v1.x;
    float bw = p.y - v1.y;
    float cw = p.z - v1.z;

    float e1 = wx * aw + wy * bw + wz * cw;             // v . w
    float b1 = ux * wx + uy * wy + uz * wz;             // u . v
    float d1 = ux * aw + uy * bw + uz * cw;             // u . w   (negated below)
    float c1 = wx * wx + wy * wy + wz * wz;             // v . v
    float a1 = ux * ux + uy * uy + uz * uz;             // u . u

    float den = a1 * c1 - b1 * b1;
    float s = (b1 * e1 - c1 * d1) / den;
    float t = (e1 + s * b1) / c1;                       // edge parameter from the UNCLAMPED ray parameter

    if (s < 0.0f)
        s = 0.0f;
    else if (s > 1.0f)
        s = 1.0f;
    if (t < 0.0f)
        t = 0.0f;
    else if (t > 1.0f)
        t = 1.0f;

    float rx = p.x + ux * s;
    float ry = p.y + uy * s;
    float rz = p.z + uz * s;
    float qx = v1.x + wx * t;
    float qy = v1.y + wy * t;
    float qz = v1.z + wz * t;

    *a = (float)sqrt((rx - qx) * (rx - qx) + (ry - qy) * (ry - qy) + (rz - qz) * (rz - qz));
    *b = (float)sqrt((qx - p.x) * (qx - p.x) + (qy - p.y) * (qy - p.y) +
                     (qz - p.z) * (qz - p.z));          // distance from the ray origin to the edge point
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

// Mesh editing helpers: picking the vertex/edge/polygon nearest to a mouse ray.
// Class layout recovered from this run of functions (see symbols/slices/s0052da20.txt).
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <dinput.h>
#include <math.h>
#include "rgl.h"

// A tiny dynamic array.  Layout verified here: the data pointers live at +4/+8 of the
// object, i.e. one dword precedes the first element pointer.
template<class T> struct vec {
    int  unk0;      // +0x0
    T*   first;     // +0x4
    T*   last;      // +0x8   (end of used range)
    T*   end;       // +0xc
    int size() const { return first ? (int)(last - first) : 0; }
    void clear();                       // 0x41f4b0
    void push_back(const T& v);         // 0x477c10 / 0x477d20 / 0x477e10
};

struct mface {                          // 0x18, vec<mface>::clear is 0x41f4b0
    rgl::vector4 pos;                   // 0x00
    unsigned char flag;                 // 0x10
    char pad[3];
    int index;                          // 0x14
};

struct medge {                          // 0x34
    int v0;                             // 0x00
    int v1;                             // 0x04
    char pad08[0x0c];
    unsigned char flag;                 // 0x14
    char pad15[0x34 - 0x15];
};

struct mpoly {                          // 0x64
    char pad00[0x28];
    int idx[4];                         // 0x28
    int idx2[4];                        // 0x38
    int count;                          // 0x48
    char pad4c[0x14];
    unsigned char flag;                 // 0x60
    char pad61[3];
};

struct MeshData {
    char pad0[0x18];
    vec<mface> verts;                   // 0x18
    vec<mpoly> polys;                   // 0x28
    vec<medge> edges;                   // 0x38
    vec<mface> verts2;                  // 0x48
    vec<mpoly> polys2;                  // 0x58
    vec<medge> edges2;                  // 0x68
    int mode;                           // 0x78
    int selected;                       // 0x7c
};

// The ray casters are declared only: at /O2 cl 13.10 summarises the body of a callee defined
// earlier or later in the same TU and then hoists the loop-invariant vector reads out of the
// picking loops below, which the original build (separate TU) could not do.  Calling through
// this opaque view keeps the loops byte-identical.
struct MeshImpl : MeshData {
    void vert_ray(const rgl::vector4& p, const rgl::vector4& d, unsigned& index,
                  float* a, float* b);                          // 0x52dae0
    void edge_ray(const rgl::vector4& p, const rgl::vector4& d, unsigned& index,
                  float* a, float* b);                          // 0x52dcc0
    void poly_ray(const rgl::vector4& p, const rgl::vector4& d, unsigned& index,
                  float* dist);                                 // 0x52dfd0
};

namespace rgl {
void sub(vector4& out, const vector4& a, const vector4& b);   // 0x432830: out = a - b (xyz)
}

struct Mesh : MeshData {    unsigned pick_vert(const rgl::vector4& p, const rgl::vector4& d);
    unsigned pick_edge(const rgl::vector4& p, const rgl::vector4& d);
    unsigned pick_poly(const rgl::vector4& p, const rgl::vector4& d);
    void vert_ray(const rgl::vector4& p, const rgl::vector4& d, unsigned& index,
                  float* a, float* b);
    void edge_ray(const rgl::vector4& p, const rgl::vector4& d, unsigned& index,
                  float* a, float* b);
    void poly_ray(const rgl::vector4& p, const rgl::vector4& d, unsigned& index,
                  float* dist);
};

// @ 0x52da20
unsigned Mesh::pick_vert(const rgl::vector4& p, const rgl::vector4& d)
{
    float best = 1e8f;
    unsigned besti = (unsigned)-1;
    for (unsigned i = 0; i < (unsigned)verts.size(); i++) {
        float a, b;
        ((MeshImpl*)this)->vert_ray(p, d, i, &a, &b);
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

// @ 0x52dae0
void Mesh::vert_ray(const rgl::vector4& p, const rgl::vector4& d, unsigned& index,
                               float* a, float* b)
{
    const mface& v = verts.first[index];
    float t = (v.pos.x - p.x) * d.x + (v.pos.y - p.y) * d.y + (v.pos.z - p.z) * d.z;
    rgl::vector4 q;
    if (t < 0.0f)
        q = p;
    else
        q = rgl::vector4(p.x + d.x * t, p.y + d.y * t, p.z + d.z * t);
    *a = (float)sqrt((q.x - v.pos.x) * (q.x - v.pos.x) +
                     (q.y - v.pos.y) * (q.y - v.pos.y) +
                     (q.z - v.pos.z) * (q.z - v.pos.z));
    *b = (float)sqrt((q.x - p.x) * (q.x - p.x) +
                     (q.y - p.y) * (q.y - p.y) +
                     (q.z - p.z) * (q.z - p.z));
}

// @ 0x52dc00
unsigned Mesh::pick_edge(const rgl::vector4& p, const rgl::vector4& d)
{
    float best = 1e8f;
    unsigned besti = (unsigned)-1;
    for (unsigned i = 0; i < (unsigned)edges.size(); i++) {
        float a, b;
        ((MeshImpl*)this)->edge_ray(p, d, i, &a, &b);
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

// @ 0x52dcc0
void Mesh::edge_ray(const rgl::vector4& p, const rgl::vector4& d, unsigned& index,
                               float* a, float* b)
{
    const medge& e = edges.first[index];
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
    float t = (b1 * s - d1) / a1;

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
    *b = (float)sqrt((rx - p.x) * (rx - p.x) + (ry - p.y) * (ry - p.y) +
                     (rz - p.z) * (rz - p.z));
}

// @ 0x52df40
unsigned Mesh::pick_poly(const rgl::vector4& p, const rgl::vector4& d)
{
    float best = 1e8f;
    unsigned besti = (unsigned)-1;
    for (unsigned i = 0; i < (unsigned)polys.size(); i++) {
        float dist;
        ((MeshImpl*)this)->poly_ray(p, d, i, &dist);
        if (dist < best) {
            best = dist;
            besti = i;
        }
    }
    return besti;
}

// Ray / convex-polygon intersection.  The polygon's vertices (at most four) are gathered once,
// their mean gives the plane point, the first three give the normal, and the hit point is then
// tested against every edge (a hit outside the polygon reports 1e6, i.e. "no hit").
// @ 0x52dfd0
void Mesh::poly_ray(const rgl::vector4& p, const rgl::vector4& d, unsigned& index,
                    float* dist)
{
    const mpoly& poly = polys.first[index];
    int n = poly.count;
    rgl::vector4 pts[4];
    rgl::vector4 mean(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < n; i++) {
        pts[i] = verts.first[poly.idx[i]].pos;
        mean.x += pts[i].x;
        mean.y += pts[i].y;
        mean.z += pts[i].z;
    }
    float inv = 1.0f / (float)n;
    mean.x *= inv;
    mean.y *= inv;
    mean.z *= inv;

    rgl::vector4 e1, e2, normal;
    rgl::sub(e1, pts[1], pts[0]);
    rgl::sub(e2, pts[2], pts[0]);
    cross(normal, e1, e2);
    normal.normalize();

    if (dot(mean - p, d) < 0.0f) {          // face points away from the ray
        *dist = 1e6f;
        return;
    }
    float t = dot(normal, mean - p) / dot(normal, d);
    rgl::vector4 hit(p.x + d.x * t, p.y + d.y * t, p.z + d.z * t);

    bool outside = false;
    for (int i = 0; i < n && !outside; i++) {
        const rgl::vector4& a = pts[i];
        const rgl::vector4& b = pts[(i + 1) % n];
        rgl::vector4 edge = b - a;
        float len = edge.length();
        if (len > 1e-6f)
            edge = edge * (1.0f / len);
        else
            edge = rgl::vector4(0.0f, 1.0f, 0.0f);
        rgl::vector4 c;
        cross(c, edge, normal);
        if (dot(hit - a, c) > 0.0f)
            outside = true;
    }
    if (outside) {
        *dist = 1e6f;
        return;
    }
    *dist = (pts[0] - p).length();
}

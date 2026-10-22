// Mesh editing operations: ray/polygon picking and the selection transforms.
// Class layout recovered from this run of functions (see symbols/slices/s0052dfd0.txt).
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

struct mface {                          // 0x18
    rgl::vector4 pos;                   // 0x00
    unsigned char flag;                 // 0x10   (selection / visibility flag)
    char pad[3];
    int index;                          // 0x14   (selection stamp used by the transforms)
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
    int idx[4];                         // 0x28   vertex indices (count <= 4)
    int idx2[4];                        // 0x38
    int count;                          // 0x48
    char pad4c[0x14];
    unsigned char flag;                 // 0x60
    char pad61[3];
};

namespace rgl {
void sub(vector4& out, const vector4& a, const vector4& b);   // 0x432830: out = a - b (xyz)
}

struct Mesh {
    char pad0[0x18];
    vec<mface> verts;                   // 0x18
    vec<mpoly> polys;                   // 0x28
    vec<medge> edges;                   // 0x38
    vec<mface> verts2;                  // 0x48
    vec<mpoly> polys2;                  // 0x58
    vec<medge> edges2;                  // 0x68
    int mode;                           // 0x78   0 = vertex, 1 = edge, 2 = polygon
    int selected;                       // 0x7c

    void poly_ray(const rgl::vector4& p, const rgl::vector4& d, unsigned& index,
                  float* dist);
    void toggle_flag(int type, int index);
    void scale_selected(int dummy, const rgl::vector4* scale);
    void transform_selected(int dummy, const float* m);
    void remove_selected();

    void gather_selection(int mode);    // 0x4c54f0   re-stamps the +0x14 selection field
    void delete_vert(int index);        // 0x4c8af0
    void delete_edge(int index);        // 0x4ca9b0
    void delete_poly(int index);        // 0x4cb940
    void rebuild();                     // 0x470920
};

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

// Select/deselect one vertex, edge or polygon.  Only the vertex case updates `selected`.
// @ 0x52e510
void Mesh::toggle_flag(int type, int index)
{
    if (type == 0) {
        verts.first[index].flag = !verts.first[index].flag;
        if (verts.first[index].flag)
            selected = index;
        else
            selected = -1;
    } else if (type == 1) {
        edges.first[index].flag = !edges.first[index].flag;
    } else {
        polys.first[index].flag = !polys.first[index].flag;
    }
}

// Scale the selected vertices about their centroid.  The +0x14 selection stamp is recomputed
// first (0x4c54f0); the scale factors are per axis.
// @ 0x52e590
void Mesh::scale_selected(int dummy, const rgl::vector4* scale)
{
    gather_selection(dummy);
    float sx = 0.0f, sy = 0.0f, sz = 0.0f;
    int n = 0;
    for (int i = 0; i < verts.size(); i++) {
        mface& v = verts.first[i];
        if (v.index) {
            sx += v.pos.x;
            sy += v.pos.y;
            sz += v.pos.z;
            n++;
        }
    }
    float inv = 1.0f / (float)n;
    float mx = sx * inv;
    float my = sy * inv;
    float mz = sz * inv;
    for (int i = 0; i < verts.size(); i++) {
        mface& v = verts.first[i];
        if (!v.index)
            continue;
        v.pos.x = (v.pos.x - mx) * scale->x + mx;
        v.pos.y = (v.pos.y - my) * scale->y + my;
        v.pos.z = (v.pos.z - mz) * scale->z + mz;
    }
}

// Rotate/scale the selected vertices by a 3x3 matrix about their centroid.
// @ 0x52e6d0
void Mesh::transform_selected(int dummy, const float* m)
{
    gather_selection(dummy);
    float sx = 0.0f, sy = 0.0f, sz = 0.0f;
    int n = 0;
    for (int i = 0; i < verts.size(); i++) {
        mface& v = verts.first[i];
        if (v.index) {
            sx += v.pos.x;
            sy += v.pos.y;
            sz += v.pos.z;
            n++;
        }
    }
    float inv = 1.0f / (float)n;
    float mx = sx * inv;
    float my = sy * inv;
    float mz = sz * inv;
    for (int i = 0; i < verts.size(); i++) {
        mface& v = verts.first[i];
        if (!v.index)
            continue;
        float dx = v.pos.x - mx;
        float dy = v.pos.y - my;
        float dz = v.pos.z - mz;
        v.pos = rgl::vector4(dz * m[8] + dy * m[4] + dx * m[0] + mx,
                             dz * m[9] + dy * m[5] + dx * m[1] + my,
                             dz * m[10] + dy * m[6] + dx * m[2] + mz);
    }
}

// Delete every flagged element of the current mode, then rebuild the derived data.  The
// index is held still after a deletion, since the array shrinks under it.
// @ 0x52e870
void Mesh::remove_selected()
{
    if (mode == 0) {
        for (int i = 0; i < verts.size(); i++) {
            if (verts.first[i].flag) {
                delete_vert(i);
                i--;
            }
        }
    }
    if (mode == 1) {
        for (int i = 0; i < edges.size(); i++) {
            if (edges.first[i].flag) {
                delete_edge(i);
                i--;
            }
        }
    } else {
        for (int i = 0; i < polys.size(); i++) {
            if (polys.first[i].flag) {
                delete_poly(i);
                i--;
            }
        }
    }
    rebuild();
}

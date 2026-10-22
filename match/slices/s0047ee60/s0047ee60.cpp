// Slice s0047ee60: Geom::build - rebuilds the whole polygon topology of a terrain geometry: a per-vertex
// edge list, the face/edge adjacency of every corner, the per-face centroid, the per-edge midpoint, the
// per-face normal/area and the per-corner weight, then the bounding box.
// Layouts are recovered from the ctor 0x478150 (four vector triples + bmin/bmax) and from this function;
// the coding style follows the sibling mesh pass 0x470a00 (same author): vector4 temps built through the
// three-argument constructor, accumulation into the member in source order, inline length()/normalize.
#include <vector>
#include <math.h>
#include <stddef.h>
#include "rgl.h"

using rgl::vector4;

#define MB_NOTHROW throw()
#define MB_INLINE __forceinline

struct tstring {
    int pos; char* buf; int cap; int len;
    tstring() {}
    tstring(const char* s) MB_NOTHROW;              // 0x42bbe0
    tstring(const tstring& o) MB_NOTHROW;           // 0x42c110
    ~tstring() MB_NOTHROW;                          // 0x42bc40
};
struct MsgLog { void add(tstring msg, int color); };    // 0x452c00 (thiscall, ret 0x14)
extern MsgLog g_msglog;                                 // 0x5d7080

// 0x24-byte point record (mangled type E36x): position at +0, the rest carried by the terrain pass.
struct E36x {
    float px, py, pz;               // +0x00
    int f0c;                        // +0x0c
    char pad10[0x10];               // +0x10
    float f20;                      // +0x20
};

// 0x88-byte face record (mangled type E136).
struct E136 {
    int n;                          // +0x00 corner count
    int v[4];                       // +0x04 point ids
    int e[4];                       // +0x14 edge id, then neighbour face id
    int f24;                        // +0x24
    int adj[4];                     // +0x28 per-corner id (-1: none)
    float w[4];                     // +0x38 per-corner weight
    int f48, f4c, f50;              // +0x48
    vector4 normal;                 // +0x54
    float area;                     // +0x64
    unsigned f68;                   // +0x68
    int f6c, f70;                   // +0x6c
    vector4 center;                 // +0x74 centroid
    int f84;                        // +0x84
};

// 0x24-byte directed edge record (mangled type E36).
struct E36 {
    int count;                      // +0x00 number of adjacent faces
    int f4;                         // +0x04 face that created the edge
    int f8;                         // +0x08 the other adjacent face (-1: none)
    int f0c;                        // +0x0c start point
    int f10;                        // +0x10 end point
    vector4 mid;                    // +0x14 midpoint
};

struct Geom {
    std::vector<E36x> pts;          // +0x00
    std::vector<E136> elems;        // +0x10
    std::vector<E36> edges;         // +0x20
    char pad30[0x14];               // +0x30 a fourth vector triple (unused here)
    vector4 bmin;                   // +0x44
    vector4 bmax;                   // +0x54
    void build();                   // 0x47ee60
    void compute_bounds();          // 0x4704b0
};

#define OFFSET_CHECK(T, m, off) typedef char T##_##m##_at_##off[(offsetof(T, m) == (off)) ? 1 : -1]
OFFSET_CHECK(E36x, px, 0x00);
OFFSET_CHECK(E36x, f0c, 0x0c);
OFFSET_CHECK(E36x, f20, 0x20);
OFFSET_CHECK(E136, n, 0x00);
OFFSET_CHECK(E136, v, 0x04);
OFFSET_CHECK(E136, e, 0x14);
OFFSET_CHECK(E136, adj, 0x28);
OFFSET_CHECK(E136, w, 0x38);
OFFSET_CHECK(E136, normal, 0x54);
OFFSET_CHECK(E136, area, 0x64);
OFFSET_CHECK(E136, center, 0x74);
OFFSET_CHECK(E136, f84, 0x84);
OFFSET_CHECK(E36, count, 0x00);
OFFSET_CHECK(E36, f4, 0x04);
OFFSET_CHECK(E36, f8, 0x08);
OFFSET_CHECK(E36, f0c, 0x0c);
OFFSET_CHECK(E36, f10, 0x10);
OFFSET_CHECK(E36, mid, 0x14);
OFFSET_CHECK(Geom, pts, 0x00);
OFFSET_CHECK(Geom, elems, 0x10);
OFFSET_CHECK(Geom, edges, 0x20);
OFFSET_CHECK(Geom, bmin, 0x44);
OFFSET_CHECK(Geom, bmax, 0x54);

// vector4::normalize(), inlined exactly as 0x407d60 does it (it returns the old length).
static float MB_INLINE normalize(vector4& v)
{
    float len = v.length();
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        v.x = inv * v.x;
        v.y = inv * v.y;
        v.z = inv * v.z;
    } else {
        v.x = 0.0f;
        v.y = 1.0f;
        v.z = 0.0f;
    }
    return len;
}

// vector4::operator*(const float&), inlined as 0x407cd0 does it (a value-returning vector4 temp).
static vector4 MB_INLINE scale(const vector4& v, const float& s)
{
    vector4 r;
    r.x = v.x * s;
    r.y = v.y * s;
    r.z = v.z * s;
    return r;
}

// @ 0x47ee60
void Geom::build()
{
    int* counts = new int[pts.size()];
    int** lists = new int*[pts.size()];
    for (unsigned i = 0; i < pts.size(); i++) {
        lists[i] = new int[0x20];
        counts[i] = 0;
    }

    int warned = 0;
    for (unsigned fi = 0; fi < elems.size(); fi++) {
        for (int k = 0; k < elems[fi].n; k++) {
            int v0 = elems[fi].v[k];
            int v1 = elems[fi].v[(k + 1) % elems[fi].n];
            bool found = false;
            for (int j = 0; j < counts[v0]; j++) {
                if (edges[lists[v0][j]].f10 == v1)
                    continue;
                if (edges[lists[v0][j]].f0c != v1)
                    continue;
                elems[fi].e[k] = lists[v0][j];
                edges[lists[v0][j]].f8 = fi;
                edges[lists[v0][j]].count = edges[lists[v0][j]].count + 1;
                if (edges[lists[v0][j]].count > 2)
                    edges[lists[v0][j]].count = 2;
                found = true;
            }
            if (!found) {
                E36 e;
                e.f0c = v0;
                e.f10 = v1;
                edges.push_back(e);
                unsigned idx = edges.size() - 1;
                if (counts[v1] < 0x20) {
                    lists[v1][counts[v1]] = idx;
                    counts[v1]++;
                    edges[idx].f4 = fi;
                    edges[idx].f8 = -1;
                    edges[idx].count = 1;
                    elems[fi].e[k] = idx;
                } else if (!warned) {
                    g_msglog.add(tstring("WARNING: neighbour faces exceeds the maximum in manifold"), 0xffff8888);
                    warned = 1;
                }
            }
        }
    }

    // edge id -> the face on the other side of it
    for (unsigned fi = 0; fi < elems.size(); fi++) {
        for (int k = 0; k < elems[fi].n; k++) {
            int f4 = edges[elems[fi].e[k]].f4;
            if (f4 == fi)
                elems[fi].e[k] = edges[elems[fi].e[k]].f8;
            else
                elems[fi].e[k] = f4;
        }
    }

    // face centroid
    for (unsigned fi = 0; fi < elems.size(); fi++) {
        elems[fi].center = vector4(0.0f, 0.0f, 0.0f);
        for (int k = 0; k < elems[fi].n; k++) {
            E136& g = elems[fi];
            E36x& v = pts[g.v[k]];
            g.center.x = g.center.x + v.px;
            g.center.y = v.py + g.center.y;
            g.center.z = v.pz + g.center.z;
        }
        E136& h = elems[fi];
        float s = 1.0f / (float)h.n;
        h.center.x = s * h.center.x;
        h.center.y = s * h.center.y;
        h.center.z = s * h.center.z;
    }

    // edge midpoint
    for (unsigned i = 0; i < edges.size(); i++) {
        E36& e = edges[i];
        E36x& a = pts[e.f0c];
        E36x& b = pts[e.f10];
        e.mid = scale(vector4(a.px + b.px, a.py + b.py, a.pz + b.pz), 0.5f);
    }

    // face normal and area
    for (unsigned fi = 0; fi < elems.size(); fi++) {
        E36x& p1 = pts[elems[fi].v[1]];
        E36x& p0 = pts[elems[fi].v[0]];
        float ax = p1.px - p0.px;
        float ay = p1.py - p0.py;
        float az = p1.pz - p0.pz;
        E36x& p2 = pts[elems[fi].v[2]];
        E36x& q0 = pts[elems[fi].v[0]];
        vector4 b;
        b.x = p2.px - q0.px;
        b.y = p2.py - q0.py;
        b.z = p2.pz - q0.pz;
        vector4& n = elems[fi].normal;
        n.x = ay * b.z - az * b.y;
        n.y = b.x * az - b.z * ax;
        n.z = b.y * ax - b.x * ay;
        float len = normalize(elems[fi].normal);
        elems[fi].area = fabsf(len) * 0.5f;
        if (elems[fi].n > 3) {
            E36x& r2 = pts[elems[fi].v[2]];
            E36x& r0 = pts[elems[fi].v[0]];
            float cx = r2.px - r0.px;
            float cy = r2.py - r0.py;
            float cz = r2.pz - r0.pz;
            E36x& r3 = pts[elems[fi].v[3]];
            E36x& s0 = pts[elems[fi].v[0]];
            b.x = r3.px - s0.px;
            b.y = r3.py - s0.py;
            b.z = r3.pz - s0.pz;
            vector4 d;
            d.x = cy * b.z - cz * b.y;
            d.y = cz * b.x - cx * b.z;
            d.z = cx * b.y - cy * b.x;
            elems[fi].area += fabsf(d.length()) * 0.5f;
        }
    }

    compute_bounds();

    for (unsigned fi = 0; fi < elems.size(); fi++) {
        for (int k = 0; k < elems[fi].n; k++) {
            if (elems[fi].adj[k] != -1)
                elems[fi].w[k] = 1.0f;
        }
    }

    delete[] counts;
    for (unsigned i = 0; i < pts.size(); i++)
        delete[] lists[i];
    delete[] lists;
}

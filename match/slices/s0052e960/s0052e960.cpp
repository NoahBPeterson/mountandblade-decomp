// Mesh editing: duplicate the selected polygons into the secondary buffers, projecting every
// copied vertex through a basis built from the caller's matrix.
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
    unsigned char flag;                 // 0x10
    char pad[3];
    int index;                          // 0x14   (source index this copy came from)
};

struct medge {                          // 0x34
    int v0;                             // 0x00
    int v1;                             // 0x04
    int unk08;                          // 0x08   (-10000, empty marker)
    int unk0c;                          // 0x0c   (-10000)
    int unk10;                          // 0x10
    unsigned char flag;                 // 0x14
    char pad15[3];
    float t0;                           // 0x18   (1.0f on a fresh edge)
    float t1;                           // 0x1c
    int index;                          // 0x20   (source edge index)
    char pad24[0x34 - 0x24];
};

struct mpoly {                          // 0x64
    int unk00;                          // 0x00
    char pad04[0x24];
    int idx[4];                         // 0x28   vertex indices (count <= 4)
    int idx2[4];                        // 0x38   edge indices
    int count;                          // 0x48
    char pad4c[0x10];
    int unk5c;                          // 0x5c
    unsigned char flag;                 // 0x60
    char pad61[3];
};

// Inline copy of the vector cross product: the original inlines this in the frame builder
// (rgl::cross at 0x407eb0 is the out-of-line version and shows up as a call instead).
namespace rgl {
inline vector4 vcross(const vector4& a, const vector4& b)
{
    return vector4(a.y * b.z - a.z * b.y,
                   a.z * b.x - a.x * b.z,
                   a.x * b.y - a.y * b.x);
}
}

// Basis -> world: the copy buffers hold coordinates in the (s, f, u) frame, so applying the
// copy is just a column combination plus the origin.
namespace rgl {
inline vector4 to_world(const matrix& m, const vector4& p)
{
    return vector4(m.s.x * p.x + m.f.x * p.y + m.u.x * p.z + m.o.x,
                   m.s.y * p.x + m.f.y * p.y + m.u.y * p.z + m.o.y,
                   m.s.z * p.x + m.f.z * p.y + m.u.z * p.z + m.o.z);
}
}

struct Mesh {
    char pad0[0x18];    vec<mface> verts;                   // 0x18
    vec<mpoly> polys;                   // 0x28
    vec<medge> edges;                   // 0x38
    vec<mface> verts2;                  // 0x48   copy buffers
    vec<mpoly> polys2;                  // 0x58
    vec<medge> edges2;                  // 0x68
    int mode;                           // 0x78   0 = vertex, 1 = edge, 2 = polygon
    int selected;                       // 0x7c

    void make_copy(const rgl::matrix& m);
    int  vertex_index(int index, const rgl::matrix& m);
    int  edge_index(int index, const rgl::matrix& m);
    void apply_copy(const rgl::matrix& m);

    void rebuild();                     // 0x470920
};

// Duplicate every flagged polygon into polys2, mapping its vertex/edge references through
// find-or-create helpers so shared vertices stay shared in the copy.  The basis rows are
// (s, f, up) with up = world Z: s = f x up normalised, f = up x s.
// @ 0x52e960
void Mesh::make_copy(const rgl::matrix& m)
{
    rgl::matrix basis = m;
    basis.u = rgl::vector4(0.0f, 0.0f, 1.0f);
    basis.s = rgl::vcross(basis.f, basis.u);
    basis.s.normalize();
    basis.f = rgl::vcross(basis.u, basis.s);

    if (mode == 2) {
        edges2.clear();
        polys2.clear();
        verts2.clear();
        for (int i = 0; i < polys.size(); i++) {
            mpoly& p = polys.first[i];
            if (!p.flag)
                continue;
            mpoly t;
            t.idx[0] = -100000;
            t.idx[1] = -100000;
            t.idx[2] = -100000;
            t.idx[3] = -100000;
            t.idx2[0] = -100000;
            t.idx2[1] = -100000;
            t.idx2[2] = -100000;
            t.idx2[3] = -100000;
            t.unk00 = 0;
            t.count = p.count;
            t.unk5c = 0;
            t.flag = 0;
            for (int j = 0; j < p.count; j++) {
                t.idx[j] = vertex_index(p.idx[j], basis);
                t.idx2[j] = edge_index(p.idx2[j], basis);
            }
            polys2.push_back(t);
        }
    }
}

// Return the copy index of source vertex `index`, creating it the first time it is asked for.
// @ 0x52ebc0
int Mesh::vertex_index(int index, const rgl::matrix& m)
{
    for (unsigned i = 0; i < (unsigned)verts2.size(); i++) {
        if (verts2.first[i].index == index)
            return (int)i;
    }
    int n = verts2.size();
    mface f;
    f.pos = m.transform_point_to_local(verts.first[index].pos);
    f.flag = 0;
    f.index = index;
    verts2.push_back(f);
    return n;
}

// Same for edges: the copy keeps the two endpoint indices remapped and remembers where it came
// from in +0x20.
// @ 0x52eca0
int Mesh::edge_index(int index, const rgl::matrix& m)
{
    for (unsigned i = 0; i < (unsigned)edges2.size(); i++) {
        if (edges2.first[i].index == index)
            return (int)i;
    }
    int n = edges2.size();
    medge e;
    e.v0 = vertex_index(edges.first[index].v0, m);
    e.v1 = vertex_index(edges.first[index].v1, m);
    e.unk08 = -10000;
    e.unk0c = -10000;
    e.unk10 = 0;
    e.flag = 0;
    e.t0 = 1.0f;
    e.t1 = 1.0f;
    e.index = index;
    edges2.push_back(e);
    return n;
}

// Flatten the copy buffers into the real ones: every copied vertex goes back to world space via
// the same basis, its edges/polygons are appended with the index ranges shifted, and the derived
// data is rebuilt.  All selection flags are cleared first.
// @ 0x52edb0
void Mesh::apply_copy(const rgl::matrix& m)
{
    rgl::matrix basis = m;
    basis.u = rgl::vector4(0.0f, 0.0f, 1.0f);
    basis.s = rgl::vcross(basis.f, basis.u);
    basis.s.normalize();
    basis.f = rgl::vcross(basis.u, basis.s);

    selected = -1;
    for (unsigned i = 0; i < (unsigned)verts.size(); i++)
        verts.first[i].flag = 0;
    selected = -1;
    for (unsigned i = 0; i < (unsigned)edges.size(); i++)
        edges.first[i].flag = 0;
    selected = -1;
    for (unsigned i = 0; i < (unsigned)polys.size(); i++)
        polys.first[i].flag = 0;

    if (polys2.size() == 0)
        return;

    int voff = verts.size();
    int eoff = edges.size();

    for (unsigned i = 0; i < (unsigned)verts2.size(); i++) {
        mface v;
        v.pos = rgl::to_world(basis, verts2.first[i].pos);
        v.flag = 1;
        verts.push_back(v);
    }

    for (unsigned i = 0; i < (unsigned)edges2.size(); i++) {
        medge e;
        e.v0 = edges2.first[i].v0 + voff;
        e.v1 = edges2.first[i].v1 + voff;
        e.unk08 = -10000;
        e.unk0c = -10000;
        e.unk10 = 0;
        e.flag = 1;
        e.t0 = 1.0f;
        e.t1 = 1.0f;
        edges.push_back(e);
    }

    for (unsigned i = 0; i < (unsigned)polys2.size(); i++) {
        mpoly& p = polys2.first[i];
        mpoly t;
        t.idx[0] = -100000;
        t.idx[1] = -100000;
        t.idx[2] = -100000;
        t.idx[3] = -100000;
        t.idx2[0] = -100000;
        t.idx2[1] = -100000;
        t.idx2[2] = -100000;
        t.idx2[3] = -100000;
        t.unk00 = 0;
        t.count = p.count;
        t.unk5c = 0;
        t.flag = 1;
        for (int j = 0; j < p.count; j++) {
            t.idx[j] = p.idx[j] + voff;
            t.idx2[j] = p.idx2[j] + eoff;
        }
        polys.push_back(t);
    }

    rebuild();
}

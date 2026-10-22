// Slice s004d27d0: Mesh::split_marked_edge(), the "subdivide the selected edge" step of the mesh editor.
// Layouts as in the neighbour slice s004704b0 (verified there). The Edge counter `uses` at +0x20 sits between
// crease[] and mid. ALL NAMES PROVISIONAL.
//   0x4d27d0  split_marked_edge(): find the single edge whose `flag` is set, push a copy of it whose second
//             vertex is a new midpoint vertex, split every face using the edge into triangles fanning out to
//             that midpoint (reusing the face record itself for the last triangle), then append the midpoint
//             vertex and rebuild adjacency.
#include <math.h>
#include <stdlib.h>
#include <vector>
#include "rgl.h"

using rgl::vector4;

struct Vtx {                                              // 0x18
    vector4 pos;                                          // 0x00
    char f10;                                             // 0x10 (zeroed on a new midpoint vertex)
    char pad11[3];
    int id;                                               // 0x14
};
struct Face {                                             // 0x64
    int f0;                                               // 0x00
    int f4;
    vector4 centroid;                                     // 0x08
    vector4 normal;                                       // 0x18
    int vid[4];                                           // 0x28
    int eid[4];                                           // 0x38
    int n;                                                // 0x48
    int nbr[4];                                           // 0x4c
    int f5c;                                              // 0x5c
    char f60;                                             // 0x60 marked face
    char pad61[3];
    Face()
    {
        for (int i = 0; i < 4; i++) {
            vid[i] = -100000;
            eid[i] = -100000;
        }
        f0 = 0;
        f5c = 0;
        f60 = 0;
        n = 0;
    }
};
struct Edge {                                             // 0x34
    int v0, v1;                                           // 0x00
    int face[2];                                          // 0x08
    int nfaces;                                           // 0x10
    char flag;                                            // 0x14 "split this edge"
    char pad15[3];
    float crease[2];                                      // 0x18
    int uses;                                             // 0x20 marked faces using this edge
    vector4 mid;                                          // 0x24
    Edge(int a, int b) : v0(a)
    {
        face[0] = -10000;
        face[1] = -10000;
        nfaces = 0;
        flag = 0;
        crease[0] = 1.0f;
        crease[1] = 1.0f;
        v1 = b;
    }
};

struct Mesh {
    char pad0[0x18];
    std::vector<Vtx> verts;               // 0x18 (first at 0x1c)
    std::vector<Face> faces;              // 0x28
    std::vector<Edge> edges;              // 0x38
    bool split_marked_edge();                     // 0x4d27d0
    void build_adjacency();                       // 0x470920 (another TU)
};
struct MeshExt {
    void build_adjacency();                       // 0x470920
};

template<class T> inline T& vat(std::vector<T>& v, size_t i) { return (&*v.begin())[i]; }

// @ 0x4d27d0
bool Mesh::split_marked_edge()
{
    int cnt = 0;
    int mark = -1;
    for (unsigned i = 0; i < edges.size(); i++) {
        if (vat(edges, i).flag) {
            cnt++;
            mark = (int)i;
        }
    }
    if (cnt != 1)
        return false;
    int split = (int)edges.size();        // index the copy of the marked edge will get
    Vtx v;
    v.f10 = 0;
    volatile float zsum = vat(verts, vat(edges, mark).v0).pos.z + vat(verts, vat(edges, mark).v1).pos.z;
    v.pos = vector4((vat(verts, vat(edges, mark).v0).pos.x + vat(verts, vat(edges, mark).v1).pos.x) * 0.5f,
                    (vat(verts, vat(edges, mark).v0).pos.y + vat(verts, vat(edges, mark).v1).pos.y) * 0.5f,
                    zsum * 0.5f);
    Edge ne(vat(edges, mark).v0, (int)verts.size());
    ne.flag = 1;
    edges.push_back(ne);
    for (int slot = 0; slot < vat(edges, mark).nfaces; slot++) {
        int fi = vat(edges, mark).face[slot];
        int n = vat(faces, fi).n;
        int corners[4];
        int nc = 0;
        for (int j = 0; j < n; j++) {
            int vi = vat(faces, fi).vid[j];
            if (vi != vat(edges, mark).v0 && vi != vat(edges, mark).v1)
                corners[nc++] = j;
        }
        if (nc == 2 && abs(corners[0] - corners[1]) != 1) {
            int t = corners[0];
            corners[0] = corners[1];
            corners[1] = t;
        }
        for (int k = 0; k < nc; k++) {
            Face nf;
            int c = corners[k];
            nf.n = 3;
            nf.vid[0] = vat(faces, fi).vid[(c + n - 1) % n];
            nf.vid[1] = vat(faces, fi).vid[c];
            nf.vid[2] = (int)verts.size();
            nf.eid[0] = vat(faces, fi).eid[(c + n - 1) % n];
            int last = (int)edges.size() - 1;         // the split edge, or the previous corner's edge
            if ((nf.vid[2] == vat(edges, last).v0 && nf.vid[0] == vat(edges, last).v1) ||
                (nf.vid[2] == vat(edges, last).v1 && nf.vid[0] == vat(edges, last).v0))
                nf.eid[2] = last;
            else
                nf.eid[2] = mark;
            nf.eid[1] = (int)edges.size();
            Edge e2(nf.vid[1], nf.vid[2]);
            edges.push_back(e2);
            faces.push_back(nf);
        }
        int c = corners[nc - 1];
        int v0 = vat(faces, fi).vid[c];
        int v1 = vat(faces, fi).vid[(c + 1) % n];
        int v2 = (int)verts.size();
        int e0 = vat(faces, fi).eid[c];
        int e1;
        if ((v1 == vat(edges, split).v0 && v2 == vat(edges, split).v1) ||
            (v1 == vat(edges, split).v1 && v2 == vat(edges, split).v0))
            e1 = split;
        else
            e1 = mark;
        vat(faces, fi).n = 3;
        vat(faces, fi).vid[0] = v0;
        vat(faces, fi).vid[1] = v1;
        vat(faces, fi).vid[2] = v2;
        vat(faces, fi).eid[0] = e0;
        vat(faces, fi).eid[1] = e1;
        vat(faces, fi).eid[2] = (int)edges.size() - 1;
    }
    vat(edges, mark).v0 = (int)verts.size();
    verts.push_back(v);
    ((MeshExt*)this)->build_adjacency();
    return true;
}

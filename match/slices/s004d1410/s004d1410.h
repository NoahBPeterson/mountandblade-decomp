// Slice s004d1410 (provisional names). Two passes of the mesh editor's "triangulate / extrude" step.
// Layout as in the neighbour slices s004d2df0 / s004d27d0 (the editor mesh, graph_obj in gui_edit.txt): three
// engine growable arrays (an empty allocator slot, then begin/last/end):
//   this+0x18  vec<Vtx>   stride 0x18   (position, flag byte at +0x10, id at +0x14)
//   this+0x28  vec<Face>  stride 0x64   (vertex ids +0x28, edge ids +0x38, count +0x48)
//   this+0x38  vec<Edge>  stride 0x34   (two vertex ids, two face slots, face count, flag)
// Helpers owned by other slices: 0x470920 build_adjacency, 0x477c10/0x477d20/0x477e10 the three
// out-of-line push_back instantiations, 0x4c5670 calc_edge_mode, 0x4c5f60 find_triangle, 0x4c8000 normal.
#ifndef S004D1410_H
#define S004D1410_H

#include <math.h>
#include <vector>
#include "rgl.h"

using rgl::vector4;

struct Vtx {                                        // 0x18
    vector4 pos;                                    // +0
    char sel;                                       // +0x10 vertex selected
    char pad11[3];
    int id;                                         // +0x14
    Vtx() {}
    Vtx(float x_, float y_, float z_) : pos(x_, y_, z_) { sel = 0; }
};

struct Face {                                       // 0x64
    int f0;                                         // +0
    int f4;                                         // +4
    vector4 centroid;                               // +8
    vector4 normal;                                 // +0x18
    int vid[4];                                     // +0x28
    int eid[4];                                     // +0x38
    int n;                                          // +0x48
    int nbr[4];                                     // +0x4c
    int f5c;                                        // +0x5c
    char f60;                                       // +0x60
    char pad61[3];
    Face()
    {
        f0 = 0;
        f5c = 0;
        f60 = 0;
    }
};

struct Edge {                                       // 0x34
    int v[2];                                       // +0 endpoint vertex ids
    int face[2];                                    // +8
    int nfaces;                                     // +0x10
    char sel;                                       // +0x14 edge selected (user selection)
    char pad15[3];
    float crease[2];                                // +0x18
    int sel2;                                       // +0x20 snapshot of the selection (calc_edge_mode)
    vector4 mid;                                    // +0x24
    Edge()
    {
        sel = 0;
        nfaces = 0;
        crease[0] = 1.0f;
        crease[1] = 1.0f;
    }
    Edge(int a, int b)
    {
        v[0] = a;
        face[0] = -10000;
        face[1] = -10000;
        nfaces = 0;
        sel = 0;
        crease[0] = 1.0f;
        crease[1] = 1.0f;
        v[1] = b;
    }
};

struct Mesh {
    char pad0[0x18];
    std::vector<Vtx> verts;                         // 0x18
    std::vector<Face> faces;                        // 0x28
    std::vector<Edge> edges;                        // 0x38
    char pad48[0x7c - 0x48];
    int f7c;                                        // 0x7c
    bool build_first_pass();                        // 0x4d1410
    void build_second_pass();                       // 0x4d1880
    int calc_edge_mode(int mode);                   // 0x4c5670
    void find_triangle(const int* edge, int* a, int* b);   // 0x4c5f60
    vector4 normal(const int* edge);                // 0x4c8000
    void build_adjacency();                         // 0x470920
};

#endif

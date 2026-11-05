// poly_mesh.h: the polygon-mesh topology object (vertices, edges, polygons) with its derived data, the 0x68-byte
// container class (NavCore) it is built on, and the small point-set class with a bounding box. The same object is
// the nav map of the scene (load 0x4781d0 .. build 0x4785e0) and the "PolyMesh" of the mesh editor
// (symbols/slices/s0052dfd0.txt: rebuild 0x470920). Offsets are the ones read or written by a function of
// terrain_b.cpp (cited); records are cited at their uses. Names provisional (the slices called the record types
// Vtx18/NavVert (vertex), Edge/NavEdge/E52 (edge), Face/NavFace/E100 (polygon), the object PolyMesh/NavMap).
#ifndef TERRAIN_B_POLY_MESH_H
#define TERRAIN_B_POLY_MESH_H
#include <stddef.h>
#include <vector>
#include <queue>
#include <algorithm>
#include <functional>
#include "../../rgl/rgl_math.h"
#include "../hnode.h"

using rgl::vector4;
using rgl::matrix;

// std::vector<T>::iterator-free element access: the original indexes through &*begin()
template<class T> inline T& vat(std::vector<T>& v, size_t i) { return (&*v.begin())[i]; }

struct raw_t {};                               // selects the constructors that leave the record untouched

// 0x18-byte vertex: position (4th lane unused) and a flag byte. 0x4704b0..0x470a00, 0x4781d0, 0x4785e0; the vector
// instantiations 0x476620 (_Insert_n), 0x477480, 0x477530, 0x477c10 are of this record (E24 in the slices).
struct MVert {
    vector4 pos;                // +0x00
    char flag;                  // +0x10 (0x4781d0, 0x4785e0 write 0)
    int pad14;
};

// 0x34-byte edge. The default constructor is the E52() of vector<>::resize(n) (0x4776b0, 0x477db0); the sequence
// of stores is that of 0x477db0.
struct MEdge {
    int v0, v1;                 // +0x00 endpoints (0x470750)
    int face[2];                // +0x08 adjacent polygons (0x470920, 0x470a00)
    int nfaces;                 // +0x10 (0x470920)
    char flag;                  // +0x14
    char pad15[3];
    float crease[3];            // +0x18 per adjacent polygon (0x470750 writes crease[j]); the path search reads crease[0] / crease[1]
                                //       as the portal's clearance seen from face[0] / from the other side (0x444c50)
    vector4 mid;                // +0x24 midpoint (0x470a00)
    MEdge()
    {
        v0 = -10000; v1 = -10000; face[0] = -10000; face[1] = -10000;
        flag = 0;
        nfaces = 0;
        crease[0] = 1.0f;
        crease[1] = 1.0f;
    }
    MEdge(raw_t) {}
};

// 0x64-byte polygon (up to four corners). The default constructor is the E100() of vector<>::resize(n) (0x4775a0,
// 0x477ca0): eight ints -100000 over vid/eid.
struct MPoly {
    int stamp;                  // +0x00 visit stamp of the path search, compared with PolyMesh::stamp (0x439900; 0x477ca0 zeroes)
    int parent;                 // +0x04 face the search came from (0x444c50, 0x4452f0)
    vector4 centroid;           // +0x08 (0x470a00; the search's face centre)
    vector4 normal;             // +0x18 (0x4705e0; nz is the divisor of the height query)
    union {
        struct {
            int vid[4];         // +0x28 corner vertices (0x4705e0, 0x4784d0)
            int eid[4];         // +0x38 corner edges (0x470920, 0x4784d0)
        };
        int lim[8];
    };
    int n;                      // +0x48 corner count
    int nbr[4];                 // +0x4c neighbour polygon per edge, -1: none (0x470a00)
    int f5c;                    // +0x5c (0x4784d0)
    char f60;                   // +0x60 (0x4784d0)
    char pad61[3];
    MPoly()
    {
        stamp = 0; f5c = 0; f60 = 0; n = 0;
        for (int i = 0; i < 8; i++)
            lim[i] = -100000;
    }
    MPoly(raw_t) {}
    // the path search indexes these through plain pointers (cl unrolls the loops when it sees the array bound)
    int* vert_p() { return (int*)((char*)this + 0x28); }
    int* edge_p() { return (int*)((char*)this + 0x38); }
    int* nbr_p() { return (int*)((char*)this + 0x4c); }
};


// The four-vector container (constructor 0x478150, ICF-folded with the same constructor of other element types).
struct NavCore {
    std::vector<int> v0;        // +0x00
    std::vector<MVert> verts;   // +0x10
    std::vector<MPoly> faces;              // +0x20
    std::vector<MEdge> edges;              // +0x30
    int m40;
    vector4 e44, f54;
    int m64;
    NavCore();                  // 0x478150
};

struct stream_base;

// ---- the open list of the path search (agent4/agent_logic.cpp 0x444c50, 0x4452f0): face id, cost so far (g), priority (f);
// min-heap on f. pop() is expanded inline by the original (cl would not do that for the library member: pop_inl).
struct PQ : std::priority_queue<hnode, std::vector<hnode>, node_gt> {
    __forceinline void pop_inl()
    {
        std::pop_heap(c.begin(), c.end(), comp);
        c.pop_back();
    }
};

// The mesh object: the nav map of the mission scene (Mission +0x6698) and the mesh editor's polygon mesh. The visit stamp and
// the search's open list, then the vertex / polygon / edge tables (+0x18 / +0x28 / +0x38) and the grid step.
struct PolyMesh {
    int stamp;                                  // +0x00 current visit stamp (0x439410)
    PQ open;                                    // +0x04 std::priority_queue (vector +4..0x13, comparator +0x14) (0x444c50)
    std::vector<MVert> verts;                   // +0x18
    std::vector<MPoly> faces;                   // +0x28
    std::vector<MEdge> edges;                   // +0x38
    char pad48[0x90 - 0x48];                    // (mesh editor / map-screen state; the nav map never reads it)
    int step;                                   // +0x90 (0x4785e0)
    void compute_face_normals();                // 0x4705e0
    void compute_edge_creases();                // 0x470750
    void build_adjacency();                     // 0x470920 (symbols: PolyMesh::rebuild)
    void build_geometry();                      // 0x470a00
    void load(stream_base* s);                  // 0x4781d0
    int add_edge(int a, int b);                 // 0x478450
    int add_face(int n, int v0, int v1, int v2, int v3, int e0, int e1, int e2, int e3);   // 0x4784d0
    void clear();                               // 0x478580
    void build(struct HeightField* hf, int step_);     // 0x4785e0
    // ---- the navigation queries (agent2 / agent4) ----
    bool inside(const int* face, const float* p);       // 0x4391d0 point inside the polygon (xy)
    float dist(const int* face, const float* p);        // 0x439280 xy distance outside the polygon + |dz| to its plane
    void next_stamp();                                  // 0x439410
    bool trace(int face, vector4 from, int end_face, const float* to, int* max_steps);   // 0x439460 (vector4 by value)
    bool find_path(int from, int to, int* out_faces, vector4* out_pos, int* out_count, int* out_steps,
                   const float* w0, const float* w1, const float* w2);      // 0x444c50 (agent4/agent_logic.cpp)
    bool reachable(const int* from, const int* to, const int* max_steps);   // 0x4452f0 (agent4/agent_logic.cpp)
};

// The same two functions seen from outside their object: their callers were compiled without the bodies, so cl must
// not assume their register use (it would keep `this` in ecx across the calls).
struct MeshExt {
    void build_adjacency();                     // 0x470920
    void compute_face_normals();                // 0x4705e0
};


#endif // TERRAIN_B_POLY_MESH_H

// Slice s004d1d80: one method of a polygon-mesh topology object (an rgl mesh: vertices at +0x18 with a
// 0x18-byte record, faces +0x28 at 0x64 bytes, edges +0x38 at 0x34 bytes). Layouts verified in the
// neighbour slice s004704b0 (0x470920 etc.); the Face/Edge/vertex field names are the ones that slice uses.
//   0x4d1d80  add_triangle(tri): reject a triangle that already exists (a face using all three vertices) or
//             that would be non-manifold (an edge already used by two faces), create its missing edges and
//             append the face, orienting it towards the average normal of the faces it touches.
// ALL NAMES PROVISIONAL.
#include <math.h>
#include <vector>
#include "rgl.h"

using rgl::vector4;

struct Vtx {                                              // 0x18: position, two flags, the vertex id
    float x, y, z;                                        // 0x00
    float f0c;
    int f10;                                              // 0x10 (zeroed when a vertex is appended)
    int id;                                               // 0x14
};
struct Face {                                             // 0x64
    int f0;                                               // 0x00
    int f4;                                               // 0x04
    vector4 centroid;                                     // 0x08
    vector4 normal;                                       // 0x18
    int vid[4];                                           // 0x28
    int eid[4];                                           // 0x38
    int n;                                                // 0x48
    int nbr[4];                                           // 0x4c
    int f5c;                                              // 0x5c
    char f60;                                             // 0x60
    char pad61[3];
    Face()
    {
        for (int i = 0; i < 4; i++) vid[i] = -100000;
        for (int i = 0; i < 4; i++) eid[i] = -100000;
        f0 = 0;
        f5c = 0;
        f60 = 0;
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
    bool add_triangle(int* tri);                  // 0x4d1d80
    void finish();                                // 0x4d22d0 (another slice)
    bool split_marked_edge();                     // 0x4d27d0 (another slice)
    void build_adjacency();                       // 0x470920 (another TU)
};
// the callee is compiled without its body in this TU: cl must not assume its register use
struct MeshExt {
    void build_adjacency();                       // 0x470920
};

template<class T> inline T& vat(std::vector<T>& v, size_t i) { return (&*v.begin())[i]; }

// vector4 with the (1e-10) normalize that callers inline (rgl::vector4::normalize at 0x407d60).
struct Vec4 : vector4 {
    Vec4() {}
    Vec4(float a, float b, float c) : vector4(a, b, c) {}
    float normalize_inl()
    {
        float len = length();
        if (len > 1e-10f) {
            float inv = 1.0f / len;
            x *= inv; y *= inv; z *= inv;
        } else {
            x = 0.0f; y = 1.0f; z = 0.0f;
        }
        return len;
    }
};

// @ 0x4d1d80
bool Mesh::add_triangle(int* tri)
{
    std::vector<int> found;
    for (unsigned i = 0; i < faces.size(); i++) {
        int cnt = 0;
        for (int j = 0; j < vat(faces, i).n; j++) {
            int v = vat(faces, i).vid[j];
            if (v == tri[0]) cnt++;
            if (v == tri[1]) cnt++;
            if (v == tri[2]) cnt++;
        }
        if (cnt > 0)
            found.push_back((int)i);
        if (cnt > 2)
            return false;
    }
    float nx = 0.0f, ny = 0.0f, nz = 0.0f;
    for (unsigned i = 0; i < found.size(); i++) {
        nx = nx + vat(faces, found[i]).normal.x;
        ny = ny + vat(faces, found[i]).normal.y;
        nz = nz + vat(faces, found[i]).normal.z;
    }
    Vec4 nrm(nx / (float)found.size(), ny / (float)found.size(), nz / (float)found.size());
    nrm.normalize_inl();
    float d1x = vat(verts, tri[2]).x - vat(verts, tri[0]).x;
    float d1y = vat(verts, tri[2]).y - vat(verts, tri[0]).y;
    float d1z = vat(verts, tri[2]).z - vat(verts, tri[0]).z;
    float d2x = vat(verts, tri[1]).x - vat(verts, tri[0]).x;
    float d2y = vat(verts, tri[1]).y - vat(verts, tri[0]).y;
    float d2z = vat(verts, tri[1]).z - vat(verts, tri[0]).z;
    float cx = d2y * d1z - d2z * d1y;
    float cy = d2z * d1x - d2x * d1z;
    float cz = d2x * d1y - d2y * d1x;
    if (cx * nrm.x + cy * nrm.y + cz * nrm.z < 0.0f) {     // faces away from the neighbours' normal
        int t = tri[0];
        tri[0] = tri[2];
        tri[2] = t;
    }
    int eid[3] = {-1, -1, -1};
    for (unsigned i = 0; i < edges.size(); i++) {
        for (int j = 0; j < 3; j++) {
            Edge& e = vat(edges, i);
            if ((e.v0 == tri[j] && e.v1 == tri[(j + 1) % 3]) ||
                (e.v1 == tri[j] && e.v0 == tri[(j + 1) % 3])) {
                eid[j] = (int)i;
                if (e.nfaces == 2)
                    return false;
            }
        }
    }
    Face face;
    face.n = 3;
    for (int j = 0; j < 3; j++) {
        face.vid[j] = tri[j];
        if (eid[j] == -1) {
            eid[j] = (int)edges.size();
            Edge e(tri[j], tri[(j + 1) % 3]);
            edges.push_back(e);
        }
        face.eid[j] = eid[j];
    }
    faces.push_back(face);
    ((MeshExt*)this)->build_adjacency();
    return true;
}

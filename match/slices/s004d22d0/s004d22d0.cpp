// Slice s004d22d0: Mesh::finish(), the last step of building a polygon mesh. Layouts as in the neighbour
// slice s004704b0 (verified there); the Edge counter at +0x20 (between crease[] and mid) counts the marked
// faces that use an edge. ALL NAMES PROVISIONAL.
//   0x4d22d0  finish(): zero the per-edge use counters, average the normals of the marked faces, then for
//             every edge used by exactly one marked face: duplicate its two vertices (once per edge, reusing
//             an existing duplicate), renumber the face references, rebuild adjacency and set the normal.
#include <math.h>
#include <vector>
#include "rgl.h"

using rgl::vector4;

struct Vtx {                                              // 0x18
    float x, y, z;                                        // 0x00
    float f0c;
    char f10;                                             // 0x10 (zeroed on the duplicate)
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
};
struct Edge {                                             // 0x34
    int v0, v1;                                           // 0x00
    int face[2];                                          // 0x08
    int nfaces;                                           // 0x10
    char flag;                                            // 0x14
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
    void finish();                                // 0x4d22d0
    void build_adjacency();                       // 0x470920 (another TU)
    void set_slot(int kind, vector4* dir);        // 0x4c7f90
    void rebuild();                               // 0x4cb840
};
struct MeshExt {
    void build_adjacency();                       // 0x470920
    void set_slot(int kind, vector4* dir);        // 0x4c7f90
    void rebuild();                               // 0x4cb840
};

template<class T> inline T& vat(std::vector<T>& v, size_t i) { return (&*v.begin())[i]; }

// vector4 with the inlined (1e-10) normalize; it returns the old length, like rgl::vector4::normalize.
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

// @ 0x4d22d0
void Mesh::finish()
{
    for (unsigned i = 0; i < edges.size(); i++)
        vat(edges, i).uses = 0;
    Vec4 n(0.0f, 0.0f, 0.0f);
    int cnt = 0;
    for (unsigned i = 0; i < faces.size(); i++) {
        if (vat(faces, i).f60) {
            n.x = n.x + vat(faces, i).normal.x;
            cnt++;
            n.y = n.y + vat(faces, i).normal.y;
            n.z = n.z + vat(faces, i).normal.z;
            for (int j = 0; j < vat(faces, i).n; j++)
                vat(edges, vat(faces, i).eid[j]).uses++;
        }
    }
    if (cnt == 0)
        return;
    int nv = (int)verts.size();
    for (unsigned i = 0; i < edges.size(); i++) {
        if (vat(edges, i).uses == 1) {
            Edge e(vat(edges, i).v0, vat(edges, i).v1);
            for (int j = 0; j < 2; j++) {
                int k = nv;
                while (k < (int)verts.size() && vat(verts, k).id != (&e.v0)[j])
                    k++;
                if (k == (int)verts.size()) {         // no copy yet: append one
                    Vtx v = vat(verts, (&e.v0)[j]);
                    v.f10 = 0;
                    v.id = (&e.v0)[j];
                    verts.push_back(v);
                    (&e.v0)[j] = (int)verts.size();
                } else {
                    (&e.v0)[j] = k;
                }
            }
            for (unsigned f = 0; f < faces.size(); f++) {
                for (int j = 0; j < vat(faces, f).n; j++) {
                    if (vat(faces, f).f60) {
                        if (vat(faces, f).eid[j] == (int)i)
                            vat(faces, f).eid[j] = (int)edges.size();
                        if (vat(faces, f).vid[j] == vat(verts, e.v0).id)
                            vat(faces, f).vid[j] = e.v0;
                        else if (vat(faces, f).vid[j] == vat(verts, e.v1).id)
                            vat(faces, f).vid[j] = e.v1;
                    }
                }
            }
            edges.push_back(e);
        }
    }
    for (unsigned i = 0; i < edges.size(); i++) {
        if (vat(edges, i).uses > 1) {
            for (int k = nv; k < (int)verts.size(); k++) {
                if (vat(edges, i).v0 == vat(verts, k).id)
                    vat(edges, i).v0 = k;
                if (vat(edges, i).v1 == vat(verts, k).id)
                    vat(edges, i).v1 = k;
            }
        }
    }
    ((MeshExt*)this)->build_adjacency();
    float inv = 1.0f / (float)cnt;
    n.x = n.x * inv;
    n.y = n.y * inv;
    n.z = n.z * inv;
    n.normalize_inl();
    ((MeshExt*)this)->set_slot(2, &n);
    ((MeshExt*)this)->rebuild();
}

// Shared declarations of slices s00444c50 / s004452f0 (navigation-mesh path search). Copied into each slice dir under
// the same name; keep identical. Names provisional; layouts from s00439100 (NavMesh) and s00438860 (heap node).
#ifndef MB_NAV_SEARCH_H
#define MB_NAV_SEARCH_H
#include <vector>
#include <queue>
#include <algorithm>
#include <functional>
#include <memory>
#include <math.h>
#include "rgl.h"
using rgl::vector4;

inline vector4 vscale(float s, const vector4& v) { return vector4(s * v.x, s * v.y, s * v.z); }
inline vector4 vadd(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
inline vector4 vsub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }

// ---- open-list node: face id, cost so far (g), priority (f); min-heap on f (see s00438860 0x438b10) --------------
struct hnode {
    int id;
    float g;
    float f;
    hnode() {}
    hnode(int id_, float g_, float f_) : id(id_), g(g_), f(f_) {}
};
struct node_gt { bool operator()(const hnode& a, const hnode& b) const { return a.f > b.f; } };

// ---- navigation mesh (s00439100): visit stamps, open list at +4, 0x64-byte faces, 0x34-byte portals ----------------
// std::priority_queue<hnode, vector<hnode>, node_gt>; pop() is expanded inline by the original (cl would not do that
// for the library member, hence the forced copy of its body)
struct PQ : std::priority_queue<hnode, std::vector<hnode>, node_gt> {
    __forceinline void pop_inl()
    {
        std::pop_heap(c.begin(), c.end(), comp);
        c.pop_back();
    }
};

struct NavFace {                                        // 0x64 bytes
    int stamp;                                          // 0x00
    int parent;                                         // 0x04 face we came from
    vector4 c;                                          // 0x08 centre
    char pad18[0x38 - 0x18];
    int edge_[4];                                       // 0x38 portal ids
    int nverts;                                         // 0x48
    int nbr_[4];                                        // 0x4c neighbour across edge i (-1: none)
    char pad5c[8];
};
struct NavEdge {                                        // 0x34 bytes
    char pad0[8];
    int face;                                           // 0x08
    char pad0c[0x18 - 0x0c];
    float w_a;                                          // 0x18 clearance seen from `face`
    float w_b;                                          // 0x1c clearance from the other side
    char pad20[0x24 - 0x20];
    vector4 mid;                                        // 0x24 portal midpoint
};
struct NavVert { float x, y, z; float pad[3]; };
struct NavMesh {
    int stamp;                                          // 0x00
    PQ open;                                            // 0x04 std::priority_queue (vector +4..0x13, comparator +0x14)
    std::vector<NavVert> verts;                         // 0x18
    std::vector<NavFace> faces;                         // 0x28
    std::vector<NavEdge> edges;                         // 0x38
    void next_stamp();                                  // 0x439410
    bool reachable(const int* from, const int* to, const int* max_steps);   // 0x4452f0
    bool find_path(int from, int to, int* out_faces, vector4* out_pos, int* out_count, int* out_steps,
                   const float* w0, const float* w1, const float* w2);      // 0x444c50
};
#endif

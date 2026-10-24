// nav_mesh.h: the mission scene's navigation mesh (NavMesh: 0x18-byte vertices, 0x64-byte convex faces) embedded in the
// mission object at +0x6698 (src/game/agent2: nav queries 0x4391d0..0x439a50, Agent::update_nav_face 0x43cb30). One
// definition for src/game; Mission (mission.h) embeds it. Needs rgl::vector4 (rgl_base.h).
#ifndef GAME_NAV_MESH_H
#define GAME_NAV_MESH_H
#include <vector>
#include <queue>
#include <algorithm>
#include <functional>
#include "../include/rgl_base.h"
#include "./hnode.h"

using rgl::vector4;

// 0x18 bytes: the mission scene navigation mesh vertex (terrain_b/cell_grid.h NavVert is the 0x24-byte ground vertex of another object)
struct NavPoint18 { float x, y, z; float pad[3]; };
// ---- the open list of the path search (agent4/agent_think.cpp 0x444c50, 0x4452f0): face id, cost so far (g), priority (f);
// min-heap on f. pop() is expanded inline by the original (cl would not do that for the library member: pop_inl).
struct PQ : std::priority_queue<hnode, std::vector<hnode>, node_gt> {
    __forceinline void pop_inl()
    {
        std::pop_heap(c.begin(), c.end(), comp);
        c.pop_back();
    }
};
struct NavFace {                                        // 0x64 bytes: a convex polygon of up to 4 vertices
    int stamp;                                          // 0x00 visit stamp (compared with NavMesh::stamp) (0x439900)
    int parent;                                         // 0x04 face the search came from (0x444c50, 0x4452f0)
    vector4 c;                                          // 0x08 centre (0x444c50, 0x4452f0)
    float nx, ny, nz;                                   // 0x18 plane normal (nz is the divisor of the height query)
    char pad24[4];
    int vert_[4];                                       // 0x28 vertex indices
    int edge_[4];                                       // 0x38 edge ids
    int nverts;                                         // 0x48
    int nbr_[4];                                        // 0x4c neighbour face across edge i (< 0: none)
    char pad5c[8];
    // the originals index these through plain pointers (cl unrolls the loops when it sees the array bound)
    int* vert_p() { return (int*)((char*)this + 0x28); }
    int* edge_p() { return (int*)((char*)this + 0x38); }
    int* nbr_p() { return (int*)((char*)this + 0x4c); }
};
struct NavEdge {                                        // 0x34 bytes: a portal between two faces (0x444c50)
    char pad0[8];
    int face;                                           // 0x08 the face this portal is seen from first
    char pad0c[0x18 - 0x0c];
    float w_a;                                          // 0x18 clearance seen from `face`
    float w_b;                                          // 0x1c clearance from the other side
    char pad20[0x24 - 0x20];
    vector4 mid;                                        // 0x24 portal midpoint
};
struct NavMesh {
    int stamp;                                          // 0x00 current visit stamp (0x439410)
    PQ open;                                            // 0x04 std::priority_queue (vector +4..0x13, comparator +0x14) (0x444c50)
    std::vector<NavPoint18> verts;                         // 0x18
    std::vector<NavFace> faces;                         // 0x28
    std::vector<NavEdge> edges;                         // 0x38 (0x444c50)
    bool inside(const int* face, const float* p);       // 0x4391d0 point inside the polygon (xy)
    float dist(const int* face, const float* p);        // 0x439280 xy distance outside the polygon + |dz| to its plane
    void next_stamp();                                  // 0x439410
    bool trace(int face, vector4 from, int end_face, const float* to, int* max_steps);   // 0x439460 (vector4 by value)
    bool find_path(int from, int to, int* out_faces, vector4* out_pos, int* out_count, int* out_steps,
                   const float* w0, const float* w1, const float* w2);      // 0x444c50 (agent4/agent_think.cpp)
    bool reachable(const int* from, const int* to, const int* max_steps);   // 0x4452f0 (agent4/agent_think.cpp)
};


#endif // GAME_NAV_MESH_H

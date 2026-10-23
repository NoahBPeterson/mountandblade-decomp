// nav_mesh.h: the mission scene's navigation mesh (NavMesh: 0x18-byte vertices, 0x64-byte convex faces) embedded in the
// mission object at +0x6698 (src/game/agent2: nav queries 0x4391d0..0x439a50, Agent::update_nav_face 0x43cb30). One
// definition for src/game; Mission (mission.h) embeds it. Needs rgl::vector4 (rgl_base.h).
#ifndef GAME_NAV_MESH_H
#define GAME_NAV_MESH_H
#include <vector>
#include "../include/rgl_base.h"

using rgl::vector4;

// 0x18 bytes: the mission scene navigation mesh vertex (terrain_b/cell_grid.h NavVert is the 0x24-byte ground vertex of another object)
struct NavPoint18 { float x, y, z; float pad[3]; };
struct NavFace {                                        // 0x64 bytes: a convex polygon of up to 4 vertices
    int stamp;                                          // 0x00 visit stamp (compared with NavMesh::stamp) (0x439900)
    char pad04[0x18 - 4];
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
struct NavMesh {
    int stamp;                                          // 0x00 current visit stamp (0x439410)
    char pad04[0x14];
    std::vector<NavPoint18> verts;                         // 0x18
    std::vector<NavFace> faces;                         // 0x28
    bool inside(const int* face, const float* p);       // 0x4391d0 point inside the polygon (xy)
    float dist(const int* face, const float* p);        // 0x439280 xy distance outside the polygon + |dz| to its plane
    void next_stamp();                                  // 0x439410
    bool trace(int face, vector4 from, int end_face, const float* to, int* max_steps);   // 0x439460 (vector4 by value)
};


#endif // GAME_NAV_MESH_H

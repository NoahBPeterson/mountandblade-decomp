// mesh_x.h: the member functions that mesh-adjacent objects add to rgl::mesh without changing its layout (a view of the
// same 0x260-byte object, global namespace). One definition for src/rgl/core/resmgr.cpp (bounds, colour linearisation, tangents,
// clone: 0x419300..0x41ce80) and src/game/terrain_a (vertex-buffer fill: 0x468620, 0x46c4f0, 0x46c920).
#ifndef RGL_MESH_X_H
#define RGL_MESH_X_H
#include "./mesh.h"

struct mesh_x : rgl::mesh {
    void compute_bounds();                     // 0x419300 (bounds of the points)
    void linearise_vertex_colors();            // 0x419360 (gamma 2.2 to linear, copy to +0x58)
    void compute_tangents();                   // 0x41ce80
    mesh_x* clone();                           // 0x41c6d0
    int count(int kind);                        // 0x468620 (1: faces * 3, else vertices)
    void fill_vertices(int* p, const unsigned* fmt);    // 0x46c4f0
    void fill_indices(unsigned short* out);     // 0x46c920
    std::vector<rgl::face20>& get90_();         // 0x40ec30 (out-of-line call)
    std::vector<rgl::vertex60>& get50_();       // 0x40ebd0
};

#endif // RGL_MESH_X_H

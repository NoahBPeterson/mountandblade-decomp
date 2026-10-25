// mesh_x.h: the member functions that mesh-adjacent objects add to rgl::mesh without changing its layout (a view of the
// same 0x260-byte object, global namespace). One definition for src/rgl/core/resmgr.cpp (bounds, colour linearisation, tangents,
// clone: 0x419300..0x41ce80), src/game/terrain_a (vertex-buffer fill: 0x468620, 0x46c4f0, 0x46c920) and src/game/mesh_slot
// (mesh_ops.cpp / mesh_scene.cpp: colour, bone index, transform, skinned append: 0x47aa40..0x47b3b0, 0x44eb00..0x44f3c0).
// The base is rgl::mesh (scene/scene.h's mesh_ext adds only methods; its identity3/scene_math declarations clash with terrain_a.cpp).
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
    // the call sites that expand one level of the parent-chain walk themselves (the out-of-line copies are 0x40ebd0, 0x40ec30)
    std::vector<rgl::vertex60>& get50_rec() { return local_a1 ? v50 : parent->get50(); }
    std::vector<rgl::face20>& get90_rec() { return local_a0 ? v90 : parent->get90(); }
    void transform(rgl::mesh* src, const rgl::matrix* m, unsigned color);   // 0x47aa40 (ret 0xc)
    void set_color(rgl::vector4 color);                                // 0x47af30 (colour by value, ret 0x10)
    void set_bone_index(int bone);                                     // 0x47b360
    void multiply_color(const rgl::vector4* tint);                     // 0x47b3b0
    void flip_faces();                                                 // 0x4779b0 (terrain_b/mission.cpp)
    void append_skinned(rgl::mesh* other, const rgl::matrix* m, int bone, unsigned color, char flip);   // 0x44eb00
    void translate(const rgl::vector4* d);                             // 0x44f240
    void transform(rgl::matrix m);                                     // 0x44f3c0 (matrix by value, ret 0x40)
};

#endif // RGL_MESH_X_H

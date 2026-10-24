// lod_mesh_x.h: the member functions that terrain_a (apply, 0x46c990 / 0x46c9f0) and mesh_slot (append / set_color /
// transform, 0x47b480..0x47b690) define on rgl::mesh_list / rgl::lod_mesh. A class cannot be extended from outside, so each
// is a derived "view" of the same layout; ONE definition of the two views for src/game. No includes: the includer must have
// rgl::mesh_list, rgl::lod_mesh and rgl::matrix declared (src/rgl/mesh.h or rgl/particles/mesh_obj.h, rgl_math.h universe).
#ifndef GAME_LOD_MESH_X_H
#define GAME_LOD_MESH_X_H

struct mesh_list_x : rgl::mesh_list {
    void apply(rgl::matrix m);                                                              // 0x46c990 (terrain_a.cpp)
    void append(rgl::mesh_list* src, int bone, const rgl::matrix* m, const unsigned* color);   // 0x47b480 (mesh_slot/mesh_ops.cpp)
};
struct lod_mesh_x : rgl::lod_mesh {
    void apply(const rgl::matrix* m);                                                       // 0x46c9f0 (terrain_a.cpp)
    void set_color(unsigned color);                                                         // 0x47b5b0 (sets the bone index of every mesh)
    void transform(const rgl::matrix& m);                                                   // 0x47b610
    void append(rgl::lod_mesh* src, int bone, const rgl::matrix* m, const unsigned* color); // 0x47b690
};

#endif // GAME_LOD_MESH_X_H

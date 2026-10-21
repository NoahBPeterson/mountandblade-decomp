// Shared declarations of slices s00439100 / s00439a50 (level grid, navigation mesh). Copied into each slice dir
// under its own name; keep identical. Names are provisional; offsets come from the member functions at
// 0x439100..0x43a1e0.
#ifndef MB_NAV_H
#define MB_NAV_H
#include <stddef.h>
#include <vector>
#include "rgl.h"

using rgl::vector4;

// ---- collision shapes (layouts verified in s0040c670 / s0041bf40 / s004333c0) ----------------------------------
struct shape_base {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    shape_base() : m4(-1), m8(8), mc(-1), min_x(100000000), min_y(100000000), max_x(-100000000), max_y(-100000000) {}
    virtual ~shape_base() {}                            // slot 0
    virtual int shape_type();                           // slot 1 (0 sphere, 1 capsule, 2 face, 3 composite, 4 manifold)
    virtual void v2(); virtual void v3(); virtual void v4(); virtual void v5(); virtual void v6();
    virtual void v7(); virtual void v8(); virtual void v9();
    virtual vector4* get_min(vector4* out);             // slot 10 (+0x28)
    virtual vector4* get_max(vector4* out);             // slot 11 (+0x2c)
};
struct sphere_shape : shape_base {                      // vtable 0x54a4f0, size 0x44
    vector4 v20, v30; int m40;
    sphere_shape(const sphere_shape& o);                // 0x40c030
};
struct capsule : shape_base {                           // vtable 0x54a540, size 0x64
    vector4 p0, p1, p2, p3; float radius;
    capsule(const capsule& o);                          // 0x40a580
    capsule(float r, const vector4* a, const vector4* b);   // 0x433310
};
struct face_shape : shape_base {                        // vtable 0x54a588, size 0xb4
    vector4 v[8];                                       // 0x20 polygon vertices
    vector4 normal;                                     // 0xa0
    int count;                                          // 0xb0
    face_shape(const face_shape& o);                    // 0x40ae70
    bool contains_xy(const float* p) const;             // 0x4333c0
};
struct composite : shape_base {                         // vtable 0x54a63c, size 0x28
    shape_base** bodies; int count;
    composite(const composite& o);                      // 0x433470
};
struct manifold : shape_base {                          // vtable 0x54a5f4, size 0x50
    char pad[0x30];
    void* build_composite();                            // 0x40cb60
};
bool in_box(const vector4* lo, const vector4* hi, const vector4* p);   // 0x4336d0 (cdecl)
int shape_contact_count(shape_base* a, shape_base* b, int max, void* out);   // 0x40bfe0 (cdecl)

// ---- the navigation mesh ------------------------------------------------------------------------------
struct NavVert { float x, y, z; float pad[3]; };        // 0x18 bytes
struct NavFace {                                        // 0x64 bytes: a convex polygon of up to 4 vertices
    int stamp;                                          // 0x00 visit stamp (compared with NavMesh::stamp)
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
    int stamp;                                          // 0x00 current visit stamp
    char pad04[0x14];
    std::vector<NavVert> verts;                         // 0x18
    std::vector<NavFace> faces;                         // 0x28
    bool inside(const int* face, const float* p);       // 0x4391d0 point inside the polygon (xy)
    float dist(const int* face, const float* p);        // 0x439280 xy distance outside the polygon + |dz| to its plane
    void next_stamp();                                  // 0x439410
    bool trace(int face, float x, float y, float z, int unused, int end_face, const float* to, int* max_steps);   // 0x439460
};

// ---- the level grid -----------------------------------------------------------------------------------
// one grid cell (0x34 bytes): the shapes in it (and a list of entities at +0x30, unused here)
struct cell {
    std::vector<shape_base*> shapes;                    // 0x00
    char pad10[0x24];
    void collect_masked(unsigned mask, shape_base** out, int* count);                                     // 0x4396b0
    void collect_point(const float* p, int* use3d, unsigned mask, shape_base** out, int* count);          // 0x439780
};
struct HeightField {
    bool line_clear(const vector4* a, const vector4* b);        // 0x436da0
};
struct level {
    char pad0[8];
    int dim[2];                                         // 0x08 cells per axis
    float mn[2];                                        // 0x10
    float mx[2];                                        // 0x18
    float cellsz;                                       // 0x20
    char pad24[4];
    cell** cells;                                       // 0x28 cells[grid] -> array of cells
    char pad2c[0x64 - 0x2c];
    HeightField* hf;                                    // 0x64
    char pad68[0xb4 - 0x68];
    NavMesh* nav;                                       // 0xb4
    void walk(const float* a, const float* b, int (*cells)[2], int* count);        // 0x437290
    void locate(const float* p, int* out) const;                                    // 0x41ed00
    void query_segment(const float* a, const float* b, unsigned mask, shape_base** out, int* count);   // 0x439870
    void nav_search(const float* p, const int* face, const int* max_depth, int depth, int* best, float* best_d);   // 0x439900
    int nearest_face(const float* p);                                               // 0x439a50
    void floor_at(const float* p, float* out_z, face_shape** out_face);             // 0x439be0
    float floor_z(const float* p);                                                  // 0x439d40
    bool trace_ray(const float* a, const float* b, float maxlen, shape_base** hit, int unused);   // 0x439d60
};
#endif

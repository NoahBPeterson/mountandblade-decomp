// poly_mesh.h: the polygon-mesh topology object (vertices, edges, polygons) with its derived data, the 0x68-byte
// container class (NavData, nav_data.h) it is built on, and the small point-set class with a bounding box. The same object is
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
    char flag;                  // +0x10 selected (0x4781d0, 0x4785e0 write 0; the editor's selection: 0x4c53f0, 0x4d1410)
    int mark;                   // +0x14 the editor's scratch word: calc_vertex_mode's mark (0x4c54f0), the source vertex of a
                                //       duplicate (0x4d22d0) or of a copy (0x52e960)
};

// 0x34-byte edge. The default constructor is the E52() of vector<>::resize(n) (0x4776b0, 0x477db0); the sequence
// of stores is that of 0x477db0.
struct MEdge {
    int v0, v1;                 // +0x00 endpoints (0x470750)
    int face[2];                // +0x08 adjacent polygons (0x470920, 0x470a00)
    int nfaces;                 // +0x10 (0x470920)
    char flag;                  // +0x14 selected (0x4c53f0; 0x4d27d0 "split this edge")
    float crease[2];            // +0x18 per adjacent polygon (0x470750 writes crease[j], j < nfaces); the path search reads crease[0] /
                                //       crease[1] as the portal's clearance seen from face[0] / from the other side (0x444c50)
    int mark;                   // +0x20 the editor's scratch word: calc_edge_mode's mark (0x4c5670), clean's "used by a face"
                                //       (0x4cb840), finish's face count (0x4d22d0), the source edge of a copy (0x52e960)
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
    char flag;                  // +0x60 the editor's selection flag (0x4784d0 clears it; mesh_edit.cpp counts the selected polygons by it)
    MPoly()
    {
        stamp = 0; f5c = 0; flag = 0; n = 0;
        for (int i = 0; i < 8; i++)
            lim[i] = -100000;
    }
    MPoly(raw_t) {}
    // the path search indexes these through plain pointers (cl unrolls the loops when it sees the array bound)
    int* vert_p() { return (int*)((char*)this + 0x28); }
    int* edge_p() { return (int*)((char*)this + 0x38); }
    int* nbr_p() { return (int*)((char*)this + 0x4c); }
};



struct stream_base;
namespace rgl { struct mesh; }

// ---- the open list of the path search (agent4/agent_logic.cpp 0x444c50, 0x4452f0): face id, cost so far (g), priority (f);
// min-heap on f. pop() is expanded inline by the original (cl would not do that for the library member: pop_inl).
struct PQ : std::priority_queue<hnode, std::vector<hnode>, node_gt> {
    // the comparator copied from a temporary, as PolyMesh's constructor 0x4d1360 leaves it (the library's default constructor
    // value-initialises it: a byte store the original does not have)
    PQ() : std::priority_queue<hnode, std::vector<hnode>, node_gt>(node_gt()) {}
    __forceinline void pop_inl()
    {
        std::pop_heap(c.begin(), c.end(), comp);
        c.pop_back();
    }
};

// The mesh object: the nav map of the mission scene (Mission +0x6698) and the map editor's polygon mesh. The visit stamp and
// the search's open list, then the vertex / polygon / edge tables (+0x18 / +0x28 / +0x38), the editor's copy buffers (the
// selection duplicated by make_copy 0x52e960, placed by apply_copy 0x52edb0), its edit mode and current element, the preview
// light and the grid step. Constructor 0x4d1360 (options_panel/module_load.cpp), destructor 0x4cb780 (rgl/mesh2/mesh_2.cpp).
// The editor's members live in rgl/mesh2/gui_edit.cpp and mesh_2.cpp (0x4c53f0..0x4cb940), options_panel/mesh_edit.cpp and
// hud.cpp (0x4d1410..0x4d4b80), cls_map.cpp (0x4d9e80), cls_map/progress_bar.cpp (0x4e65e0..0x4e75a0) and mission5/mesh_3.cpp
// and its siblings (0x52da20..0x52edb0).
struct PolyMesh {
    int stamp;                                  // +0x00 current visit stamp (0x439410)
    PQ open;                                    // +0x04 std::priority_queue (vector +4..0x13, comparator +0x14) (0x444c50)
    std::vector<MVert> verts;                   // +0x18
    std::vector<MPoly> faces;                   // +0x28
    std::vector<MEdge> edges;                   // +0x38
    std::vector<MVert> copy_verts;              // +0x48 the editor's copy buffers (0x52e960 fills them, 0x52edb0 places them;
    std::vector<MPoly> copy_faces;              // +0x58  the nav map never reads them, the destructor frees them)
    std::vector<MEdge> copy_edges;              // +0x68
    int mode;                                   // +0x78 the map editor's edit mode (0: vertices; 1: edges; 2: polygons; 4e65e0..4e75a0)
    int cur;                                    // +0x7c the editor's current element (-1: none; 0x4c53f0)
    vector4 light;                              // +0x80 light direction of the preview builders (0x4e65e0): (1, 1, 1) normalised
    int step;                                   // +0x90 (0x4785e0; 1 from the constructor)
    PolyMesh();                                 // 0x4d1360
    ~PolyMesh();                                // 0x4cb780
    // ---- the map editor's selection and topology edits (rgl/mesh2/gui_edit.cpp, mesh_2.cpp) ----
    void select_mode(int mode, char on);        // 0x4c53f0
    void calc_vertex_mode(int mode);            // 0x4c54f0
    void calc_edge_mode(int mode);              // 0x4c5670
    void swap_face_vertices(int a, int b);      // 0x4c57d0 (swaps vertex indices in the edges)
    void swap_poly_normals(int a, int b);       // 0x4c5840 (swaps EDGE indices in the face rings: eid[])
    void swap_poly_colors(int a, int b);        // 0x4c58c0 (swaps VERTEX indices in the face rings: vid[])
    void remove_vertex_from_polys(int v);       // 0x4c5940 (removes faces whose eid[] contain edge v)
    void remove_vertex_from_edges(int v);       // 0x4c59c0
    void update_vertex_mode();                  // 0x4c5a20
    void update_edge_mode();                    // 0x4c5b60
    void update_poly_mode();                    // 0x4c5c80
    void refresh_selection();                   // 0x4c5dc0
    void find_triangle(const int* e, int* a, int* b);   // 0x4c5f60
    void update_terrain(int idx, const vector4& d);     // 0x4c7f90 (every vertex marked for the mode moves by d)
    vector4 normal(const int* e);               // 0x4c8000 (hidden return pointer)
    void remove_node(int i);                    // 0x4c8af0
    void rebuild();                             // 0x4ca8c0
    void remove_edge(int i);                    // 0x4ca9b0
    bool combine();                             // 0x4caa40
    bool split_edge(int e);                     // 0x4cb3a0
    void clean();                               // 0x4cb840
    void remove_face(int i);                    // 0x4cb940
    // ---- the mesh editor's builders (options_panel/mesh_edit.cpp, hud.cpp, cls_map.cpp) ----
    bool build_first_pass();                    // 0x4d1410
    void build_second_pass();                   // 0x4d1880
    bool add_triangle(int* tri);                // 0x4d1d80
    void finish();                              // 0x4d22d0
    bool split_marked_edge();                   // 0x4d27d0
    void add_quad(const vector4* p);            // 0x4d2df0
    bool on_selection_changed();                // 0x4d4b80 (hud.cpp)
    bool teardown();                            // 0x4d9e80 (cls_map.cpp)
    // ---- the map editor's picking and editing (mission5/mesh_3.cpp, mesh_vert_ray.cpp, mesh_edge_ray.cpp, mesh_poly_ray.cpp) ----
    unsigned pick_vert(const vector4& p, const vector4& d);     // 0x52da20 nearest vertex to the ray (p, d), -1: none
    void vert_ray(const vector4& p, const vector4& d, unsigned& index, float* a, float* b);   // 0x52dae0
    unsigned pick_edge(const vector4& p, const vector4& d);     // 0x52dc00
    void edge_ray(const vector4& p, const vector4& d, unsigned& index, float* a, float* b);   // 0x52dcc0
    unsigned pick_poly(const vector4& p, const vector4& d);     // 0x52df40
    void poly_ray(const vector4& p, const vector4& d, unsigned& index, float* dist);          // 0x52dfd0
    void toggle_flag(int type, int index);      // 0x52e510
    void scale_selected(int mode, const vector4* scale);        // 0x52e590
    void transform_selected(int mode, const rgl::mat3* m);      // 0x52e6d0 (m: the three basis rows)
    void remove_selected();                     // 0x52e870
    void make_copy(const matrix& m);            // 0x52e960
    int vertex_index(int index, const matrix& m);               // 0x52ebc0
    int edge_index(int index, const matrix& m);                 // 0x52eca0
    void apply_copy(const matrix& m);           // 0x52edb0
    // ---- the mesh editor's preview builders (cls_map/progress_bar.cpp) and the nav map's build ----
    void build_spikes(rgl::mesh* owner);        // 0x4e65e0
    void build_ribbons(rgl::mesh* owner);       // 0x4e6a70
    void polygon_to_mesh(rgl::mesh* out);       // 0x4e75a0
    void compute_face_normals();                // 0x4705e0
    void compute_edge_creases();                // 0x470750
    void build_adjacency();                     // 0x470920 (symbols: PolyMesh::rebuild)
    void build_geometry();                      // 0x470a00
    void load(stream_base* s);                  // 0x4781d0
    void save(stream_base& s);                  // 0x524f00 (mission5/game_2.cpp)
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

#endif // TERRAIN_B_POLY_MESH_H

// graph_obj.h: the map editor's mesh graph (nodes, faces and edges of an edited mesh) and its methods, which the
// original spreads over 0x4c53f0..0x4cb940 (gui_edit.cpp and mesh_2.cpp). One header for the three tables and the
// class; element layouts and fields cite the function that proves them.
// Merged from the slices s004c5080 / s004c5a20 ("gui_manager": cvector<vertex/poly/edge>), s004ca550 / s004caa40 /
// s004cb3a0 ("graph_obj": node/face/edge), s004c84f0 ("conv_rec" +0x1c vector of 0x18-byte nodes).
//
// Note for src/game/strat/gui_manager.h (not edited): the slices s004c5080/s004c5a20 attributed these methods to
// `gui_manager`, but vectors at +0x1c/+0x2c/+0x3c collide with that class' ctor-initialised pairs (+0x14, +0x24, +0x2c)
// and its vector at +0, so they are a different object. wheel_step/clear_selection/set_view (0x4c52f0..0x4c5190) do
// operate on the gui_manager at 0x5d7080 (see gui_edit.cpp).
#ifndef MESH2_GRAPH_OBJ_H
#define MESH2_GRAPH_OBJ_H
#include <new>
#include <vector>
#include <stdexcept>
#include "../../game/tstring.h"
#include "../rgl_math.h"
#include "../../game/hnode.h"   // hnode: the element of PolyMesh::open, the same object's +0x04

// 0x18 bytes: position + "used" selection byte (0x4c5a20: byte store at +0x10) + a flag word (0x4c5a20: int at +0x14)
struct node {
    float x, y, z, w;            // +0x00 (0x4caa40: averaged position)
    unsigned char used;          // +0x10
    char pad11[3];
    int flag;                    // +0x14
};

// 0x34 bytes
struct edge {
    int a;                       // +0x00 first node        (0x4c57d0)
    int b;                       // +0x04 second node
    int f0;                      // +0x08 first face        (0x4cb3a0: edges[e].f0)
    int f1;                      // +0x0c second face
    int n;                       // +0x10 number of faces using the edge (0x4cb3a0, 0x4caa40)
    unsigned char sel;           // +0x14 selected (0x4c5a20 "used"; 0x4cb3a0 "flag")
    char pad15[3];
    int f18;                     // +0x18 (0x4c5670: flag word of the s004c5080 view)
    char pad1c[4];
    int flag;                    // +0x20 temporary mark (0x4cb840 clean marks edges used by a face)
    char pad24[0x10];
};

// 0x64 bytes (the slices call it poly / face; named graph_face: `face` is the collision polygon of src/include/shape.h)
struct graph_face {
    char pad00[0x18];
    float pn[3];                 // +0x18 plane normal (0x4c8000 graph_obj::normal reads +0x18..+0x20)
    float pw;                    // +0x24 (0x4c8000: the 4th lane of the result)
    int  verts[4];               // +0x28 node indices of the ring (0x4c58c0)
    int  ends[4];                // +0x38 edge indices of the ring  (0x4c5840)
    int  count;                  // +0x48 number of ring entries
    char pad4c[0xc];
    int  count2;                 // +0x58 (0x4caa40 reads ends[8])
    char pad5c[4];
    unsigned char used;          // +0x60 selected
    char pad61[3];
};

struct gadget { virtual void release(int flag) = 0; };   // virtual delete-in-place slot (0x4cb9e0)

// The mesh graph is PolyMesh (game/terrain_b/poly_mesh.h) seen with the editor's element types: the visit stamp and the
// search's open list, the node / face / edge tables (VC7.1 std::vector: allocator word, then first/last/end), three
// more vectors of the editor (only freed here, by clear_vecs 0x4cb780: element types not known) and the selection state.
struct graph_obj {
    int stamp;                           // +0x00 (PolyMesh::stamp)
    std::vector<hnode> open;             // +0x04 PolyMesh::open's container (first at +0x08: cb780 frees it last)
    node_gt open_cmp;                    // +0x14 PolyMesh::open's comparator
    std::vector<node> nodes;             // +0x18 (first at +0x1c)
    std::vector<graph_face> faces;       // +0x28 (first at +0x2c)
    std::vector<edge> edges;             // +0x38 (first at +0x3c)
    std::vector<void*> vec48;            // +0x48 (first at +0x4c)
    std::vector<void*> vec58;            // +0x58 (first at +0x5c)
    std::vector<void*> vec68;            // +0x68 (first at +0x6c)
    int  mode;                           // +0x78 (0x4c5a20: 0 vertex / 1 edge / 2 face)
    int  cur;                            // +0x7c current element (0x4c53f0)

    // the lib's vector::_Tidy (inlined): free the buffer if any, then zero the triple
    template<class T> static void tidy(std::vector<T>& v)
    {
        if (v._Myfirst) operator delete(v._Myfirst);
        v._Myfirst = 0; v._Mylast = 0; v._Myend = 0;
    }

    void select_mode(int mode, char on);         // 0x4c53f0
    void calc_vertex_mode(int mode);             // 0x4c54f0
    void calc_edge_mode(int mode);               // 0x4c5670
    void swap_face_vertices(int a, int b);       // 0x4c57d0
    void swap_poly_normals(int a, int b);        // 0x4c5840 (swaps EDGE indices in the face rings: ends[], +0x38)
    void swap_poly_colors(int a, int b);         // 0x4c58c0 (swaps NODE indices in the face rings: verts[], +0x28)
    void remove_vertex_from_polys(int v);        // 0x4c5940 (removes faces whose ends[] contain edge v)
    void remove_vertex_from_edges(int v);        // 0x4c59c0
    void update_vertex_mode();                   // 0x4c5a20
    void update_edge_mode();                     // 0x4c5b60
    void update_poly_mode();                     // 0x4c5c80
    void refresh_selection();                    // 0x4c5dc0
    void find_triangle(const int* e, int* a, int* b);   // 0x4c5f60 (ret 0xc)
    void remove_node(int i);                     // 0x4c8af0
    void rebuild();                              // 0x4ca8c0
    void remove_edge(int i);                     // 0x4ca9b0
    bool combine();                              // 0x4caa40
    bool split_edge(int e);                      // 0x4cb3a0
    // 0x4cb780 is PolyMesh's destructor (game/terrain_b/poly_mesh.h)
    void clean();                                // 0x4cb840
    void remove_face(int i);                     // 0x4cb940
    // build_adjacency (0x470920) is PolyMesh::build_adjacency (game/terrain_b/poly_mesh.h): this graph is the same object
    rgl::vector4 normal(const int* e);           // 0x4c8000 (hidden return pointer)
    void update_terrain(int idx, const float* d);   // 0x4c7f90
};

#endif

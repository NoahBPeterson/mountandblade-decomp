// mesh_edit.h: the mesh editor's view of the polygon-mesh topology object (PolyMesh, ../terrain_b/poly_mesh.h:
// vertices MVert at +0x18 (0x18 bytes), polygons MPoly at +0x28 (0x64), edges MEdge at +0x38 (0x34)).
// The slices s004d1410 / s004d1d80 / s004d22d0 / s004d27d0 / s004d4b80 each declared their own Vtx / Face / Edge /
// EditMesh; they are the canonical MVert / MPoly / MEdge / PolyMesh (same offsets, cited per field below). `EditMesh` adds
// the editor's member functions (this module and hud.cpp) and no data.
//   MVert: +0x00 pos, +0x10 flag (selected: 0x4d1410, 0x4d4b80 test it, 0x4d27d0/0x4d22d0 zero it on a new
//          vertex), +0x14 the vertex id (pad14 in poly_mesh.h; 0x4d22d0 tags a duplicate with the source vertex,
//          0x4d4b80 tests it as the "selected" word)
//   MEdge: +0x14 flag (0x4d27d0 "split this edge", 0x4d1880 selection), +0x20 a counter (0x4d22d0 "marked faces
//          using this edge", 0x4d1880 the selection snapshot of calc_edge_mode): the float crease[2] of poly_mesh.h
//          is the same word; edge_word() reads it as the int the editor uses.
#ifndef OPTIONS_PANEL_MESH_EDIT_H
#define OPTIONS_PANEL_MESH_EDIT_H
#include <math.h>
#include <vector>
#include "../terrain_b/poly_mesh.h"
#include "../int_vec.h"
#include "../../rgl/mesh2/graph_obj.h"   // graph_obj: the editor-object methods 0x4c5670.. (see GraphCalls below)

inline int& edge_word(MEdge& e) { return *(int*)&e.crease[2]; }          // +0x20 (0x4d22d0, 0x4d1880)

// An edge built from its two endpoints (0x4d1d80, 0x4d22d0, 0x4d27d0, 0x4d1880): the constructor stores of the
// slices' Edge(int, int).
struct NewEdge : MEdge {
    NewEdge(int a, int b) : MEdge(raw_t())
    {
        v0 = a;
        face[0] = -10000;
        face[1] = -10000;
        nfaces = 0;
        flag = 0;
        crease[0] = 1.0f;
        crease[1] = 1.0f;
        v1 = b;
    }
};
// The polygon / edge as the first two passes (0x4d1410, 0x4d1880) default-construct them: only the fields the
// pass does not set itself are stored (the slice found this the shape of the original's stores).
struct PartFace : MPoly {
    PartFace() : MPoly(raw_t()) { f0 = 0; f5c = 0; f60 = 0; }
};
struct PartEdge : MEdge {
    PartEdge() : MEdge(raw_t()) { flag = 0; nfaces = 0; crease[0] = 1.0f; crease[1] = 1.0f; }
};
// 0x4d2df0 add_quad: the edge records are default-constructed with the two face slots only.
struct QuadEdge : MEdge {
    QuadEdge() : MEdge(raw_t()) { face[0] = -10000; face[1] = -10000; }
};
// A vertex at a position (0x4d1880).
struct NewVert : MVert {
    NewVert(float x_, float y_, float z_)
    {
        pos = vector4(x_, y_, z_);
        flag = 0;
    }
};

// vector4 with the (1e-10) normalize callers inline (rgl::vector4::normalize 0x407d60).
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

// std::vector<int> (../int_vec.h) that frees its buffer when it goes out of scope: the local `found` list of 0x4d1d80 and
// 0x4d4b80. The inline push_back falls back to the out-of-line _Insert_n at 0x43e7b0 (a std::vector<int> would bring its
// own copy of that instantiation, which allocates differently under the equivalence harness).
struct OwnedIntVec : IntVec {
    ~OwnedIntVec() { if (_Myfirst) operator delete(_Myfirst); }
    unsigned size() const { return _Myfirst == 0 ? 0 : (unsigned)(_Mylast - _Myfirst); }
    int& operator[](unsigned i) { return *(_Myfirst + i); }
};

struct EditMesh : PolyMesh {
    bool build_first_pass();                        // 0x4d1410
    void build_second_pass();                       // 0x4d1880
    bool add_triangle(int* tri);                    // 0x4d1d80
    void finish();                                  // 0x4d22d0
    bool split_marked_edge();                       // 0x4d27d0
    void add_quad(const vector4* p);                // 0x4d2df0
    bool on_selection_changed();                    // 0x4d4b80 (hud.cpp)
};
// The editor's calls into the map editor's mesh graph (rgl/mesh2/graph_obj.h graph_obj: calc_edge_mode 0x4c5670, find_triangle
// 0x4c5f60, normal 0x4c8000, update_terrain 0x4c7f90, clean 0x4cb840, remove_face 0x4cb940). graph_obj and PolyMesh are the same
// object: graph_obj's node / face / edge tables at +0x18 / +0x28 / +0x38 are PolyMesh's MVert / MPoly / MEdge vectors, and the
// editor passes its own `this` to these functions. They are only declared (no bodies), so cl cannot see their register use.
inline graph_obj* graph(EditMesh* m) { return (graph_obj*)(void*)m; }
#endif

// mesh.h: the 0x260-byte mesh object (vtable 0x54a6f4, ctor 0x412a90, dtor 0x4127f0) and the scene that
// renders it. Merged from the slices' mesh.h copies (s00412a90, s00413320, s00413c50, s00413e50) and
// mesh_obj (s00418a70); names are provisional, unknown fields keep their offsets in the field names.
// The base class is named_object (resource.h): vptr, name +4, f14 +0x14 (the slices' mesh_base).
#ifndef RGL_MESH_H
#define RGL_MESH_H
#include <vector>
#include "./rgl_string.h"
#include "./rgl_math.h"
#include "./resource.h"
#include "../include/vec.h"
#include "../include/mesh_prims.h"

namespace rgl {

// std::vector helpers: the original inlines push_back's fast path and indexes through &*begin()
template<class T> __forceinline void vpush(std::vector<T>& v, const T& x)
{
    if (v.size() < v.capacity()) {
        T* p = v._Mylast;
        *p = x;
        v._Mylast = p + 1;
    } else {
        v.insert(v.end(), x);
    }
}
template<class T> inline T& vat(std::vector<T>& v, size_t i) { return (&*v.begin())[i]; }

// element types of the mesh's std::vectors
struct vertex60 {                // 0x60 bytes, vector at mesh+0x50
    vector4 pos;                 // 0x00
    vector4 f10, f20;
    vector4 normal;              // 0x30
    vec2 uv;                     // 0x40
    int f48, f4c;
    int index;                   // 0x50: point index
    unsigned color;              // 0x54: packed ARGB
    int f58;                     // 0x58: linear colour (0x419360)
    int f5c;
};
struct elem30 { char b[0x30]; };            // vector at +0x60
struct elem14 { int id; std::vector<int> items; };   // 0x14, vector at +0x80 (items' element type unknown)
// face20 (0x20, vector at +0x90) is in src/include/mesh_prims.h

struct frame_t { std::vector<vector4> verts; std::vector<vector4> norms; };   // 0x20: morph key frame

} // namespace rgl
#include "../include/keyframes.h"      // rgl::keyframes (frame_t above)
namespace rgl {

struct obj_c0 { char pad[0x24]; vector4 out; vector4 in; };   // object attached at mesh+0xc0


struct slot {                    // 0x2c bytes, array of 3 at mesh+0x110
    int f0, f4, f8, fc;
    int f10, f14;
    int pa, pb;                  // 0x18
    int f20, f24;
    char f28, f29;               // slots[0].f28 (0x138) and slots[1].f28 (0x164) are dirty flags
};

struct mesh;
struct scene;
struct psys;
struct mesh_pool;

struct mesh : named_object {
    mesh* parent;                // 0x18
    string name2;                // 0x1c
    skin* res;                   // 0x2c: the material it is drawn with
    unsigned flags;              // 0x30
    int f34;
    char f38;
    unsigned color;              // 0x3c: packed ARGB multiplier
    int f40;
    float f44;
    int f48, f4c;
    std::vector<vertex60> v50;
    std::vector<elem30> v60;
    std::vector<point20> v70;
    std::vector<elem14> v80;
    std::vector<face20> v90;
    char local_a0, local_a1;     // 0xa0: faces are local, 0xa1: points/vertices local
    keyframes kf;                // 0xa4
    std::vector<psys*> kids;     // 0xb0: particle systems (mesh::render calls psys::update 0x414960 on them)
    obj_c0* p_c0;                // 0xc0
    float c4, c8;
    matrix m_cc;                 // 0xcc
    unsigned mask;               // 0x10c
    slot slots[3];               // 0x110
    matrix m194, m1d4;
    int f214, f218, f21c, f220, f224, f228;
    aabb bounds;                 // 0x22c

    mesh();                                         // 0x412a90
    virtual ~mesh();                                // 0x4127f0
    virtual int type();                             // 0x48b1f0 (returns 1)
    virtual void slot2();                           // 0x412980

    // parent-chain accessors: walk up until a mesh that owns its data (recursive; the out-of-line copies at
    // 0x40eba0.. are the tail-call-optimised loops, callers inline one level)
    std::vector<point20>& get70() { return local_a1 ? v70 : parent->get70(); }    // 0x40eba0
    std::vector<vertex60>& get50() { return local_a1 ? v50 : parent->get50(); }   // 0x40ebd0
    keyframes& get_kf() { return local_a1 ? kf : parent->get_kf(); }              // 0x40ec00
    std::vector<face20>& get90() { return local_a0 ? v90 : parent->get90(); }     // 0x40ec30
    std::vector<elem30>& get60() { return local_a1 ? v60 : parent->get60(); }     // 0x40ec60
    void ensure_local();                            // 0x412ee0
    int add_point(const vector4* p);                // 0x412f50
    void add_face(int a, int b, int c);             // 0x411920
    int render(scene* sc, const matrix& frame);     // 0x413750
    int add_vertex(int index, const vec2* uv, const unsigned* color, const vector4* pos, const int* f5c);   // 0x413c50
};

struct scene {
    char pad0[0x194];
    std::vector<obj_c0*> list;   // 0x194: attached objects collected this frame
    char pad1[0x1a8 - 0x1a4];
    unsigned mask;               // 0x1a8
    char pad2[0x1c4 - 0x1ac];
    mesh_pool* view;             // 0x1c4: the particle mesh pool / camera (psys::update)
    std::vector<mesh*> cache;    // 0x1c8: one mesh per resource
    mesh* get_mesh(skin* r);                        // 0x412c30
};

} // namespace rgl
#endif

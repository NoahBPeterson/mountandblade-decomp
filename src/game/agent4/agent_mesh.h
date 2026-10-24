// agent_mesh.h: the 0x260-byte mesh object (rgl::mesh, vtable 0x54a6f4, ctor 0x412a90) as the game-side code of
// agent4/agent_update.cpp sees it: the member functions that sit in this object (add_quad 0x44e970, append_skinned
// 0x44eb00, scale 0x44f0c0, translate 0x44f240, transform 0x44f3c0, compute_normals 0x451c50, append_plain 0x4520b0)
// need the class in the match/include/rgl.h universe. src/rgl/mesh.h is the same class in the rgl_math.h universe
// (the two vector4 declarations cannot be included together), so this is a second declaration of the same layout, trimmed
// to the members this object touches; the shared records (vertex60, frame_t, keyframes, slot, aabb, point20, face20) come
// from src/include. Offsets: src/rgl/mesh.h. Names provisional.
#ifndef AGENT4_MESH_H
#define AGENT4_MESH_H
#include <vector>
#include "rgl.h"
#include "../../include/vec.h"
#include "../../include/mesh_parts.h"
#include "../../include/mesh_prims.h"
#include "../../include/mat4.h"                  // rgl::mat4 (the shared frame; rotate_su / rotate_sf are pos2 members, agent/item_kind.h)

namespace rgl {

struct vec2 { float x, y; };                     // rgl::vec2 (the mangled names of add_quad / add_vertex carry it; the shared vertex60 stores the global ::vec2, same layout)

// 16-byte rgl string (see rgl::string in rgl.h); this one also has the default ctor and dtor.
struct mstr {
    char* p; int size, cap, extra;
    mstr();                                     // 0x42bbd0
    mstr(const char* s);                        // 0x42bbe0
    ~mstr();                                    // 0x42bc40
};

vector4* unpack_color(vector4* out, const unsigned* argb);   // 0x40e4e0 (cdecl; returns out)
unsigned pack_color(const vector4* c);                       // 0x40e440 (cdecl)

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

// element types of the mesh's std::vectors: vertex60, frame_t, keyframes, obj_c0, slot, aabb, point20, face20 are the shared records
struct elem30 { float w[4]; int bone[4]; float bonef[4]; };   // skinning record (4 weights, 4 bone ids, ids as floats)            // vector at +0x60
struct elem14 { int id; std::vector<int> items; };   // 0x14, vector at +0x80 (items' element type unknown)

// inline vector helpers used by the mesh code (distinct from the out-of-line rgl::operator* in rgl.h)
inline vector4 vscale(float s, const vector4& v) { return vector4(s * v.x, s * v.y, s * v.z); }
inline vector4 vadd(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }

struct mat_t { char pad[0x20]; int f20; int get_flags(); };     // int get_flags() is 0x40eab0
struct res_obj4 { char pad[0x4c]; int f4c; };
struct res_t {
    char pad0[0x30];
    mat_t* mat;                  // 0x30
    char pad1[0x48 - 0x34];
    res_obj4* p48;               // 0x48
    char pad2[0x5c - 0x4c];
    int f5c;                     // 0x5c
    char pad3[0x68 - 0x60];
    void (*fn68)(res_t*);        // 0x68
};

struct mesh;
struct scene;

struct mesh_base {
    virtual ~mesh_base() = 0;                   // vtable 0x54a6c8: slot 0 = _purecall
    virtual int base_slot1() { return 0; }      // 0x408020
    mstr name;                                  // 0x04
    int f14;
    __forceinline mesh_base() { f14 = 0; }
};

struct mesh : mesh_base {
    mesh* parent;                // 0x18
    mstr name2;                  // 0x1c
    res_t* res;                  // 0x2c
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
    std::vector<mesh*> kids;     // 0xb0
    obj_c0* p_c0;                // 0xc0
    float c4, c8;
    mat4 m_cc;
    unsigned mask;               // 0x10c
    slot slots[3];               // 0x110 (slots[0].f28 at 0x138 and slots[1].f28 at 0x164 are dirty flags)
    mat4 m194, m1d4;
    int f214, f218, f21c, f220, f224, f228;
    aabb v22c;

    mesh();                                         // 0x412a90
    virtual ~mesh();                                // 0x417730 (scalar deleting dtor)
    virtual int slot1() { return 1; }               // 0x48b1f0
    virtual void slot2();                           // 0x412980

    // parent-chain accessors: walk up until a mesh that owns its data (recursive; the out-of-line copies at
    // 0x40eba0.. are the tail-call-optimised loops, callers inline one level)
    std::vector<point20>& get70() { return local_a1 ? v70 : parent->get70(); }    // 0x40eba0
    std::vector<point20>& get70_();                 // 0x40eba0 (out-of-line call)
    std::vector<vertex60>& get50() { return local_a1 ? v50 : parent->get50(); }   // 0x40ebd0
    keyframes& get_kf() { return local_a1 ? kf : parent->get_kf(); }              // 0x40ec00
    std::vector<face20>& get90() { return local_a0 ? v90 : parent->get90(); }     // 0x40ec30
    std::vector<elem30>& get60() { return local_a1 ? v60 : parent->get60(); }     // 0x40ec60
    std::vector<elem30>& get60_();                  // 0x40ec60 (out-of-line call)
    std::vector<vertex60>& get50_();                // 0x40ebd0 (out-of-line call)
    int has_morph();                                // 0x40f7f0
    void FUN_00412700();                            // 0x412700
    void render_child(scene* sc);                   // 0x414960
    void ensure_local();                            // 0x412ee0
    int add_point(const vector4* p);                // 0x412f50
    void append(mesh* other, const mat4* m);        // 0x412fe0
    void update_morph();                            // 0x413320
    int render(scene* sc, const mat4& frame);       // 0x413750
    int add_vertex(int index, const vec2* uv, const unsigned* color, const vector4* pos, const int* f5c);   // 0x413c50
    void add_face(int a, int b, int c);             // 0x411920
    std::vector<face20>& get90_();                  // 0x40ec30 (out-of-line call)
    void build_skin();                              // 0x411d20
    void append_skinned(mesh* other, const mat4* m, int bone, unsigned color, char flip);   // 0x44eb00
    void scale(float s);                            // 0x44f0c0
    void translate(const vector4* d);               // 0x44f240
    void transform(mat4 m);                         // 0x44f3c0 (matrix by value, ret 0x40)
    void add_quad(const vector4* p0, const vector4* p1, const vec2* uv0, const vec2* uv1, const unsigned* color);   // 0x44e970 (ret 0x14)
    mesh* clone();                                  // 0x41c6d0
    void clear_geometry();                          // 0x428e80
    void calc_bounds(vector4* mn, vector4* mx);     // 0x438d80
    void finish_load();                             // 0x419300
    void compute_normals();                         // 0x451c50
    void append_plain(mesh* other, const mat4* m);  // 0x4520b0 (ret 8, tail of append_skinned)
};

struct scene {
    char pad0[0x194];
    std::vector<obj_c0*> list;   // 0x194: attached objects collected this frame
    char pad1[0x1a8 - 0x1a4];
    unsigned mask;               // 0x1a8
    char pad2[0x1c8 - 0x1ac];
    std::vector<mesh*> cache;    // 0x1c8: one mesh per resource
    mesh* get_mesh(res_t* r);                       // 0x412c30
};

struct mesh_list {               // 16 bytes: std::vector<mesh*>
    std::vector<mesh*> v;
    void render_all(scene* sc, const mat4& m);      // 0x413ac0
};
struct lod_mesh {
    char pad0[0x34];
    mesh_list lists[5];          // 0x34
    int count;                   // 0x84
    int cur;                     // 0x88
    mat4 m8c;
    unsigned mask;               // 0xcc
    void render(scene* sc, const mat4& m);          // 0x413b00
    __forceinline bool ready() { return cur < count; }
    char padd0[0x10];                            // size 0xe0
    lod_mesh();                                     // 0x41c2a0
    void add(int level, mesh* m);                   // 0x441ff0 (ret 8)
};

struct mesh_pool {
    char pad0[0xc8];
    std::vector<mesh*> meshes;   // 0xc8
    mesh* get_mesh(res_t* r);                       // 0x413d30
};

} // namespace rgl
#endif // AGENT4_MESH_H

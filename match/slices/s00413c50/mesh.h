// mesh.h: the 0x260-byte "mesh instance" object (vtable 0x54a6f4, ctor 0x412a90) and the scene that renders it.
// Shared by slices s00412a90, s00413320, s00413c50 (copied into each slice directory). Names are provisional.
// Layout is recovered from the ctor 0x412a90 and the member functions at 0x412c30..0x413d30; the unknown
// fields keep their offsets in the field names.
#ifndef MB_MESH_H
#define MB_MESH_H
#include <vector>
#include "rgl.h"

namespace rgl {

// 16-byte rgl string (see rgl::string in rgl.h); this one also has the default ctor and dtor.
struct mstr {
    char* p; int size, cap, extra;
    mstr();                                     // 0x42bbd0
    mstr(const char* s);                        // 0x42bbe0
    ~mstr();                                    // 0x42bc40
};

// 64-byte transform: rows s, f, u and origin o (same layout as rgl::matrix).
struct mat4 {
    vector4 s, f, u, o;
    mat4 operator*(const mat4& b) const;       // 0x40e870 (this = left operand)
};
mat4* identity(mat4* out);                      // 0x40cd00 (cdecl; returns out)
void unpack_color(vector4* out, const unsigned* argb);   // 0x40e4e0 (cdecl)
unsigned pack_color(const vector4* c);                   // 0x40e440 (cdecl)

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
struct vec2 { float x, y; };
struct vertex60 {                // 0x60 bytes, vector at mesh+0x50
    vector4 pos;                 // 0x00
    vector4 f10, f20;
    vector4 normal;              // 0x30
    vec2 uv;                     // 0x40
    int f48, f4c;
    int index;                   // 0x50: point index
    unsigned color;              // 0x54: packed ARGB
    int f58;
    int f5c;
};
struct elem30 { char b[0x30]; };            // vector at +0x60
struct point20 { vector4 a, b; };           // 0x20, vector at +0x70 (rest, current)
struct elem14 { int id; std::vector<int> items; };   // 0x14, vector at +0x80 (items' element type unknown)
struct face20 { vector4 n; int a, b, c, d; };   // 0x20, vector at +0x90

// inline vector helpers used by the mesh code (distinct from the out-of-line rgl::operator* in rgl.h)
inline vector4 vscale(float s, const vector4& v) { return vector4(s * v.x, s * v.y, s * v.z); }
inline vector4 vadd(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }

struct frame_t { std::vector<vector4> verts; std::vector<vector4> norms; };   // 0x20: morph key frame

// morph key frames: frames[count], times[count]
struct keyframes {               // 12 bytes at mesh+0xa4
    frame_t* frames; float* times; int count;
    keyframes() { count = 0; frames = 0; times = 0; }
    keyframes& operator=(const keyframes& o);                                   // 0x4125e0
    void find(int* a, int* b, float* t, const float* time);                     // 0x40f0d0
};

struct obj_c0 { char pad[0x24]; vector4 out; vector4 in; };   // object attached at mesh+0xc0 (0x34.. read, 0x24.. written)

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

struct vec3x { char b[0x34]; void init(); };    // 0x40eaf0 (the original's ctor, called from the mesh ctor body)

struct slot_pair { int a, b; };
inline slot_pair zero_pair() { slot_pair p; p.a = 0; p.b = 0; return p; }
struct slot {                    // 0x2c bytes, array of 3 at mesh+0x110
    int f0, f4, f8, fc;
    int f10, f14;
    slot_pair p;                 // 0x18
    int f20, f24;
    char f28, f29;
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
    vec3x v22c;

    mesh();                                         // 0x412a90
    virtual ~mesh();                                // 0x417730 (scalar deleting dtor)
    virtual int slot1() { return 1; }               // 0x48b1f0
    virtual void slot2();                           // 0x412980

    // parent-chain accessors: walk up until a mesh that owns its data (recursive; the out-of-line copies at
    // 0x40eba0.. are the tail-call-optimised loops, callers inline one level)
    std::vector<point20>& get70() { return local_a1 ? v70 : parent->get70(); }    // 0x40eba0
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
};

struct renderer { void submit(scene* sc, mesh* m); };   // 0x412100, global object at 0x5fe898
extern renderer g_renderer;                             // 0x5fe898
struct resmgr { res_t* lookup(const mstr& name); };     // 0x418a70, global object at 0x5d6fa0
extern resmgr g_resmgr;                                 // 0x5d6fa0

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
};

struct mesh_pool {
    char pad0[0xc8];
    std::vector<mesh*> meshes;   // 0xc8
    mesh* get_mesh(res_t* r);                       // 0x413d30
};

} // namespace rgl
#endif

// mesh.h (copy of s0046c4f0/mesh.h, trimmed to what slice s00499050 needs): the 0x260-byte "mesh instance"
// object (vtable 0x54a6f4, ctor 0x412a90). Names are provisional.
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
    void rotate_fu(const float* angle);         // 0x40e570 (thiscall)
    void rotate_su(const float* angle);         // 0x40e670
    void rotate_sf(const float* angle);         // 0x40e770
};
mat4* identity(mat4* out);                      // 0x40cd00 (cdecl; returns out)
mat4* identity3(mat4* out);                     // 0x40cd60 (cdecl; returns out)
vector4* unpack_color(vector4* out, const unsigned* argb);   // 0x40e4e0 (cdecl)
unsigned pack_color(const vector4* c);                   // 0x40e440 (cdecl)

// std::vector helpers: the original inlines push_back's fast path and indexes through the raw members
template<class T> __forceinline void vpush(std::vector<T>& v, const T& x)
{
    if (v.size() < v.capacity()) {
        T* p = v._Mylast;
        *p = x;
        v._Mylast = p + 1;
    } else {
        v.insert(v.end(), 1, x);
    }
}

// element types of the mesh's std::vectors
struct vec2 { float x, y; };

struct mat_t { char pad[0x20]; int f20; char pad24[0x6c - 0x24]; unsigned f6c; int get_flags(); };
struct res_obj4 { char pad[0x4c]; int f4c; };
struct res_t {
    char pad0[0x30];
    mat_t* mat;                  // 0x30
    char pad1[0x48 - 0x34];
    res_obj4* p48;               // 0x48
    char pad2[0x64 - 0x4c];
    int refs;                    // 0x64: reference count
    void (*fn68)(res_t*);        // 0x68
};

struct obj_c0 { char pad[0x24]; vector4 out; vector4 in; };   // object attached at mesh+0xc0

struct keyframes {               // 12 bytes at mesh+0xa4
    void* frames; float* times; int count;
    keyframes() { count = 0; frames = 0; times = 0; }
    keyframes& operator=(const keyframes& o);
    void find(int* a, int* b, float* t, const float* time);
};

struct aabb {
    vector4 min, max, center;
    float radius;
    aabb(); void reset(); void extend(const vector4* p); void update();
    void extend_box(const aabb* b, const mat4* m);
};

struct slot_pair { int a, b; };
struct slot {                    // 0x2c bytes, array of 3 at mesh+0x110
    int f0, f4, f8, fc;
    int f10, f14;
    slot_pair p;                 // 0x18
    int f20, f24;
    char f28, f29;
};

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
    std::vector<char> v50;       // only used as opaque storage here
    char v50b[0xa0 - 0x60];
    char local_a0, local_a1;
    keyframes kf;                // 0xa4
    std::vector<mesh*> kids;     // 0xb0
    obj_c0* p_c0;                // 0xc0
    float c4, c8;
    mat4 m_cc;
    unsigned mask;               // 0x10c
    slot slots[3];               // 0x110
    mat4 m194, m1d4;
    int f214, f218, f21c, f220, f224, f228;
    char pad22c[0x260 - 0x22c];

    mesh();                                         // 0x412a90
    virtual ~mesh();                                // 0x417730 (scalar deleting dtor)
    virtual int slot1() { return 1; }               // 0x48b1f0
    virtual void slot2();                           // 0x412980

    mesh* clone();                                  // 0x41c6d0
    mesh* copy_of();                                // 0x4958f0
    void set_part(const mat4* frame);               // 0x495ba0
    void translate(const vector4* d);               // 0x44f240
    void scale(float s);                            // 0x44f0c0
    void set_scale(const vector4 v);                // 0x47af30 (vector4 by value, thiscall)
    void set_alpha(float a);                        // 0x495f00
    void set_color(unsigned c);                     // 0x47b360
    void add_quad(const vector4* a, const vector4* b, const vec2* uv0, const vec2* uv1, const unsigned* color);  // 0x44e970
};

struct resmgr { mesh* get_object(const mstr& name); };   // 0x419540, global object at 0x5d6fa0
extern resmgr g_resmgr;                                 // 0x5d6fa0

} // namespace rgl
#endif

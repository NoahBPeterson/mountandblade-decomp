// Shared declarations for slices s0041c760 / s0041ce80 / s0041d6f0 (mesh LOD groups, the mesh tangent-frame
// builder and the binary mesh reader). Copied into each slice dir; keep identical.
// Names are provisional. Layouts: neighbours s00412a90 / s00413c50 / s00413e50 (mesh.h) and s0041ac10 (resmgr).
#ifndef MB_MESHRD_H
#define MB_MESHRD_H
#include <stddef.h>
#include <vector>
#include "rgl.h"

using rgl::vector4;

// rgl string with a read cursor (16 bytes), as in s00415c80/mbstream.h
struct tstring {
    int pos; char* buf; int cap; int len;
    tstring();                                 // 0x42bbd0
    ~tstring();                                // 0x42bc40
    tstring& operator=(const tstring& o);      // 0x42bc50
    bool equals(const tstring& o) const;       // 0x42bdd0
    bool match(int* a, int* b, const tstring* pat);   // 0x42c160 (provisional: wildcard match against a pattern)
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};
void mb_error(const char* fmt, ...);           // 0x42cc80

// Abstract stream (vtable 0x54a740), slot numbers as in s0041ac10.h
struct stream_base {
    virtual void reset();                      // 0
    virtual void rewind();                     // 1
    virtual stream_base& w_stream(stream_base* o) = 0;     // 2
    virtual stream_base& w_float(float* v) = 0;            // 3
    virtual stream_base& w_bool(char* v) = 0;              // 4
    virtual stream_base& w_u64(unsigned __int64* v) = 0;   // 5
    virtual stream_base& w_i64(__int64* v) = 0;            // 6
    virtual stream_base& w_uint(unsigned* v) = 0;          // 7
    virtual stream_base& w_int(int* v) = 0;                // 8
    virtual stream_base& w_byte(unsigned char* v) = 0;     // 9
    virtual stream_base& w_cstr(const char* s) = 0;        // 10
    virtual stream_base& w_tstr(const tstring* s) = 0;     // 11
    virtual stream_base& slot12(void* v) = 0;              // 12
    virtual stream_base& r_stream(stream_base* o) = 0;     // 13
    virtual stream_base& r_float(float* v) = 0;            // 14
    virtual stream_base& r_bool(char* v) = 0;              // 15
    virtual stream_base& r_uint(unsigned* v) = 0;          // 16
    virtual stream_base& r_u64(unsigned __int64* v) = 0;   // 17
    virtual stream_base& r_i64(__int64* v) = 0;            // 18
    virtual stream_base& r_int(int* v) = 0;                // 19
    virtual stream_base& r_byte(char* v) = 0;              // 20
    virtual stream_base& r_tstr(tstring* v) = 0;           // 21
};

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

namespace rgl {

struct mat4 {                    // 64-byte frame
    vector4 s, f, u, o;
};

struct IdxWeight { int index; float weight; };

// element types of the mesh's std::vectors
struct vertex60 {                // 0x60 bytes, vector at mesh+0x50
    vector4 nrm;                 // 0x00 vertex normal (normalised by the tangent-frame builder)
    vector4 tan;                 // 0x10 tangent (accumulated per face)
    vector4 bit;                 // 0x20 bitangent
    vector4 nrm0;                // 0x30 copy of the normal as read
    union {
        float uv[4];             // 0x40 u, v, then two more floats
        struct { float u, v; float f48, f4c; };
    };
    int index;                   // 0x50: point index
    unsigned color;              // 0x54: packed ARGB
    int f58;
    int f5c;
};
struct point20 { vector4 a, b; };           // 0x20, vector at +0x70 (rest, current)
struct elem14 { int id; std::vector<IdxWeight> items; };   // 0x14, vector at +0x80: bone influences
struct face20 { vector4 n; int a, b, c, d; };   // 0x20, vector at +0x90

struct frame_t { std::vector<vector4> verts; std::vector<vector4> norms; };   // 0x20: morph key frame
struct keyframes {               // 12 bytes at mesh+0xa4
    frame_t* frames; float* times; int count;
};

struct mat_t { char pad[0x20]; int f20; };
struct res_t {
    char pad0[0x30];
    mat_t* mat;                  // 0x30
    char pad1[0x64 - 0x34];
    int refs;                    // 0x64
};
struct vec3x { char b[0x34]; };

struct mesh;
struct mesh_base {
    virtual ~mesh_base() = 0;
    virtual int base_slot1() { return 0; }
    tstring name;                               // 0x04
    int f14;
};

struct mesh : mesh_base {
    mesh* parent;                // 0x18
    tstring name2;               // 0x1c
    res_t* res;                  // 0x2c
    unsigned flags;              // 0x30
    int f34;
    char f38;
    unsigned color;              // 0x3c
    int f40;
    float f44;
    int f48, f4c;
    std::vector<vertex60> v50;
    char pad60[0x10];            // 0x60 (std::vector<elem30>)
    std::vector<point20> v70;
    std::vector<elem14> v80;
    std::vector<face20> v90;
    char local_a0, local_a1;     // 0xa0: faces are local, 0xa1: points/vertices local
    keyframes kf;                // 0xa4
    char pad_b0[0x110 - 0xb0];
    struct slot { int f0, f4; char rest[0x2c - 8]; } slots[3];   // 0x110
    char pad_194[0x228 - 0x194];
    int f228;                    // max bone index + 1
    vec3x v22c;

    mesh* clone();                                  // 0x41c6d0
    void ensure_local();                            // 0x412ee0
    void build_skin();                              // 0x411d20
    void finish_load();                             // 0x419300 (provisional)
    void compute_tangents();                        // 0x41ce80
};

struct mesh_list { std::vector<mesh*> v; };         // 16 bytes
struct lod_mesh {
    char pad0[0x34];
    mesh_list lists[5];          // 0x34
    int count;                   // 0x84
    int cur;                     // 0x88
    mat4 m8c;
    unsigned mask;               // 0xcc
    tstring name;                // 0xd0
    lod_mesh();                  // 0x41c2a0
};

struct resmgr {
    char f0;
    int c4, c8, cc;
    std::vector<lod_mesh*> lods;   // +0x10
    std::vector<mesh*> meshes;     // +0x20
    char pad30[0xe0 - 0x30];
    res_t* lookup(const tstring& name);              // 0x418a70
    lod_mesh* make_lod_mesh(const tstring& pat, bool copy);   // 0x41ca60
    lod_mesh* get_lod_mesh(const tstring& name);     // 0x41cdc0
};
extern resmgr g_resmgr;                              // 0x5d6fa0

} // namespace rgl
#endif

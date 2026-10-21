// Shared declarations for slices s0041ac10 / s0041b5a0 / s0041bf40 (the resource manager, skeleton records and
// STL instantiations at 0x41ac10-0x41c760). Copied into each slice dir under its own name; keep identical.
// Names are provisional. Layouts come from the constructors/loaders below and from neighbour slices
// (s00416540 mbres.h, s00417850, s00411150 psys types, s0040c670 shapes).
#ifndef MB_RSRC_H
#define MB_RSRC_H
#include <stddef.h>
#include <vector>
#include <list>
#include "rgl.h"

// rgl string with a read cursor (16 bytes), as in s00415c80/mbstream.h
struct tstring {
    int pos; char* buf; int cap; int len;
    tstring();                                 // 0x42bbd0
    tstring(const char* s);                    // 0x42bbe0
    tstring(const tstring& o);                 // 0x42c110
    ~tstring();                                // 0x42bc40
    tstring& operator=(const tstring& o);      // 0x42bc50
    void assign(const char* s);                // 0x42bcb0
    bool equals(const char* s) const;          // 0x42bd10
    bool equals(const tstring& o) const;       // 0x42bdd0
    bool range_equals(int* from, int* to, const tstring* o) const;   // 0x42c160: this[*from .. min(*to,len)) == o (provisional)
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};
void mb_error(const char* fmt, ...);           // 0x42cc80

// Abstract stream (vtable 0x54a740), slot numbers as in s00415c80/mbstream.h
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

// Root of the resource classes, vtable 0x54a6c8 (slot 0 _purecall, slot 1 0x408020), as in mbres.h
struct res_base {
    virtual ~res_base() = 0;
    virtual int type();
    tstring name;                  // +4
    int f14;                       // +0x14
    res_base() : f14(0) {}
};
inline res_base::~res_base() {}
inline int res_base::type() { return 0; }
typedef res_base res_obj;          // element type of the manager's lists and vectors

// 12-byte record (s00417850): parallel arrays of n vec16 keys and n ints. Owns both arrays.
struct vec_array {
    rgl::vector4* v;               // +0
    int* idx;                      // +4
    int n;                         // +8
    vec_array() { n = 0; v = 0; idx = 0; }
    ~vec_array() { if (v) delete[] v; if (idx) delete[] idx; }
};

// (index, weight) pair (s00411150)
struct IdxWeight { int index; float weight; };
// 0x14 bytes: {bone, vector<IdxWeight>} (s00411150)
struct BoneInfluence { int bone; std::vector<IdxWeight> verts; };

// 64-byte frame; operator* is the thiscall at 0x40e870 (hidden return pointer, ret 8)
struct mat4 {
    rgl::vector4 s, f, u, o;
    mat4 operator*(const mat4& b) const;       // 0x40e870
};

stream_base* read_matrix(stream_base* s, mat4* m);        // 0x415e70 (cdecl; reads three rows, returns s)
stream_base* read_vec_array(stream_base* s, vec_array* a);          // 0x4173b0
stream_base* read_continuous_quats(stream_base* s, vec_array* a);   // 0x417e50
stream_base* read_pair(stream_base* s, void* p);                    // 0x417520 (provisional)

// bit table: g_bit[i] == 1 << i (64-bit entries)
extern __int64 g_bit[];                        // 0x5a8e68

// ---- skeleton records -------------------------------------------------------------------------------
// 0xdc-byte bone (ctor 0x41b0c0, dtor 0x41a510, vector deleting dtor 0x41a550)
struct Bone {
    int f0;                        // +0
    int parent;                    // +4  (< 0: root)
    unsigned flags;                // +8
    mat4 local;                    // +0xc
    mat4 world;                    // +0x4c
    tstring name;                  // +0x8c
    float f9c;                     // +0x9c
    int fa0, fa4, fa8;             // +0xa0
    rgl::vector4 va_c;             // +0xac
    rgl::vector4 vb_c;             // +0xbc
    std::vector<int> link;         // +0xcc (first at +0xd0)
    Bone();                        // 0x41b0c0
};

// 32-entry vec16 table, zero-initialised (s00417850, ctor 0x417cb0); count is overwritten by the loader
struct vec_table {
    rgl::vector4* data;            // +0
    int count;                     // +4
    vec_table();                   // 0x417cb0
    ~vec_table() { delete[] data; }
};

// vtable 0x54aeb4, 0x34 bytes, type() 7: skeleton model ("frame_set" in s00417850)
struct frame_set : res_base {
    int nbones;                    // +0x18
    Bone* bones;                   // +0x1c
    vec_table table;               // +0x20 (data, count)
    unsigned f28, f2c, f30;        // +0x28 body-part masks
    frame_set();                   // 0x417e10
    virtual ~frame_set();          // 0x41aee0 (deleting)
    virtual int type();            // 0x49df30 (7)
};

// vtable 0x54b24c, 0x34 bytes, type() 8: skeleton animation
struct skel_anim : res_base {
    std::vector<vec_array> keys;   // +0x18 (first at +0x1c)
    vec_array tail;                // +0x28 (v, idx, n)
    skel_anim();                   // 0x41b4e0
    // implicit virtual dtor: 0x41b520 (deleting 0x41bed0)
    virtual int type();            // 0x48b4a0 (8)
};

// ---- textures, shapes, bodies -----------------------------------------------------------------
// vtable 0x54aaac, 0x54 bytes, type() 2: texture record (also used for render targets)
struct render_res : res_base {
    char loaded;                   // +0x18
    unsigned id;                   // +0x1c
    int external;                  // +0x20 (render targets: arg4 of get_render_target)
    unsigned flags;                // +0x24 (not set by the ctor)
    int count;                     // +0x28
    int width, height;             // +0x2c, +0x30
    int f34;                       // +0x34
    int fmt, fmt2;                 // +0x38, +0x3c
    void *tex, *p44, *p48;         // +0x40 ..
    int f4c, f50;
    render_res() : width(0x80), height(0x80)
    {
        loaded = 0;
        id = -1;
        external = 0;
        tex = 0;
        p44 = 0;
        p48 = 0;
        count = 0;
        fmt2 = 0x1c;
        fmt = 0x1c;
        f4c = 0;
        f50 = -1;
        f34 = 0;
    }
    virtual ~render_res();         // 0x417f20
    virtual int type();            // 0x48b590
    void init();                   // 0x418520 (provisional)
    int load(int check_exists, tstring* root);   // 0x416540
};

// collision shape base (vptr + 7 ints, inline ctor), see s0040c670
struct shape_base {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    shape_base() : m4(-1), m8(8), mc(-1), min_x(100000000), min_y(100000000), max_x(-100000000), max_y(-100000000) {}
    virtual ~shape_base() {}                            // slot 0
    virtual int shape_type();                           // slot 1
    virtual void s2();
    virtual void load(stream_base* s);                  // slot 3
};
struct composite : shape_base {                         // vtable 0x54a63c, size 0x28
    shape_base** bodies; int count;
    composite() : bodies(0), count(0) {}
};
struct sphere_shape : shape_base {                      // vtable 0x54a4f0, size 0x44
    rgl::vector4 v20, v30; int m40;
};
struct capsule : shape_base {                           // vtable 0x54a540, size 0x64
    rgl::vector4 p0, p1, p2, p3; float radius;
    capsule();                                          // 0x408600
};
struct face : shape_base {                              // vtable 0x54a588, size 0xb4
    rgl::vector4 v20, v30, v40, v50, v[4], normal; int count;
    face();                                             // 0x408980
};
struct manifold : shape_base {                          // vtable 0x54a5f4, size 0x50
    char pad[0x30];
    manifold();                                         // 0x40c670
};
// body resource (type 6)
struct body : res_base {
    shape_base* shape;                                  // +0x18
};

// ---- the resource manager (global object at 0x5d6fa0) ----------------------------------------------

// small 0x2c-byte resource (vtable 0x54ac08, type() via 0x417040), ctor out of line
struct small_res : res_base {
    int a, b, c, d;
    char loaded;
    small_res();                   // 0x417010
};
// material record (s00416540): flags at +0x20, pixel-shader file at +0x58, vertex shader slot at +0xc0
struct material : res_base {
    char pad18[8];
    unsigned flags20;
    char pad24[0x58 - 0x24];
    tstring ps_file;
    char pad68[0xc0 - 0x68];
    small_res* vs;
    void create_shaders();                     // 0x416c00
};
// skin record (s00416540): five referenced resources
struct skin : res_base {
    char pad18[4];
    unsigned id;                               // +0x1c
    tstring file;                              // +0x20
    int f30;                                   // +0x30
    rgl::vector4 color;                        // +0x34
    float f44;                                 // +0x44
    render_res* refs[5];                       // +0x48
    int f5c, f60, f64, f68;                    // +0x5c
    __declspec(noinline) skin();               // 0x416cd0
};
struct mesh_res : res_base {};   // type 1 (mesh record; fields not needed here)
struct resmgr {
    char f0;                       // +0x00
    unsigned c4, c8, cc;           // +0x04 id counters (1..1000)
    std::vector<res_obj*> v10;     // +0x10
    std::vector<mesh_res*> meshes; // +0x20 (type 1)
    std::vector<render_res*> textures;// +0x30 (type 2)
    std::vector<material*> materials; // +0x40 (type 4)
    std::vector<skin*> skins;      // +0x50 (type 3)
    std::vector<body*> bodies;     // +0x60 (type 6)
    std::vector<frame_set*> skels; // +0x70 (type 7)
    std::vector<skel_anim*> anims; // +0x80 (type 8)
    std::vector<small_res*> v90;   // +0x90
    std::list<res_obj*> all;       // +0xa0
    std::vector<res_obj*> loaded;  // +0xac
    tstring cur;                   // +0xbc
    char pad_cc[2];
    char fce;                      // +0xce
    resmgr();                      // 0x41b000
    void add(res_obj* r);          // 0x41ba90
    render_res* get_render_target(int w, int h, int fmt, int ext, int f34);   // 0x41b7d0
    std::vector<mesh_res*> find_meshes(const tstring* prefix);                 // 0x41b960
    small_res* find_small(const tstring* name);                               // 0x418b90 (provisional)
    small_res* get_small(tstring name);                                       // 0x41bf40
    void load_shaders();                                                      // 0x41c000
    render_res* get_texture(int w, int h, int fmt);                           // 0x41c400
    skin* get_shadow_skin(int w, int h, int fmt, int f34);                    // 0x41c420
    int load_all(int check_exists, tstring* root);                            // 0x41c540
    int lookup_file(tstring* name);                                           // 0x418e90 (provisional)
};
extern resmgr g_resmgr;                                                       // 0x5d6fa0

stream_base* read_body(stream_base* s, body* b);        // 0x41b5a0
stream_base* read_frame_set(stream_base* s, frame_set* m);   // 0x41b180

#endif

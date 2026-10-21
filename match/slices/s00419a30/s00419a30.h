// s00419a30.h: declarations for the resource-set code (mesh serialiser, resmgr helpers, STL instantiations).
// Layouts come from the functions of s00419420 itself plus the verified neighbours (mbbin.h / mesh.h /
// mbstream.h of the s004128xx..s004178xx slices). Names are provisional.
#ifndef S00419A30_H
#define S00419A30_H
#include <vector>
#include <list>

// rgl string with a read cursor (16 bytes; from mbstream.h).
struct tstring {
    int pos;            // +0
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring();                                 // 0x42bbd0
    ~tstring();                                // 0x42bc40
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

// Abstract binary/text stream (vtable 0x54a740, see mbstream.h / mbbin.h). Only the slots used here are named.
struct stream_base {
    virtual void reset();                                  // 0
    virtual void rewind();                                 // 1
    virtual stream_base& w_stream(stream_base* o);         // 2
    virtual stream_base& w_float(float* v);                // 3  0xc
    virtual stream_base& w_bool(char* v);                  // 4
    virtual stream_base& w_u64(unsigned __int64* v);       // 5
    virtual stream_base& w_i64(__int64* v);                // 6
    virtual stream_base& w_uint(unsigned* v);              // 7  0x1c
    virtual stream_base& w_int(int* v);                    // 8  0x20
    virtual stream_base& w_byte(unsigned char* v);         // 9
    virtual stream_base& w_cstr(const char* s);            // 10 0x28
    virtual stream_base& w_tstr(const tstring* s);         // 11 0x2c
};

// ---- element types of the mesh (see mesh.h of s00413c50) ---------------------------------------
struct vec4 { float x, y, z, w; };
struct vec2 { float u, v; };
struct vertex60 {                // 0x60 bytes
    vec4 pos;                    // 0x00
    vec4 f10, f20;
    vec4 normal;                 // 0x30
    vec2 uv[2];                  // 0x40
    int index;                   // 0x50
    unsigned color;              // 0x54
    int f58, f5c;
};
struct elem30 { char b[0x30]; };
struct point20 { vec4 a, b; };
struct elem14 { int id; std::vector<int> items; };    // 0x14
struct face20 { vec4 n; int a, b, c, d; };            // 0x20

struct frame_t { std::vector<vec4> verts; std::vector<vec4> norms; };   // 0x20 (morph key frame)
// Morph key frames: frames[count], times[count] (written through the int slot).
struct keyframes { frame_t* frames; int* times; int count; };

struct res_t { char pad[0x4]; tstring name; };         // resource record: name at +4 (res->name)

struct mesh {                    // layout up to the vectors used by the serialiser
    virtual ~mesh();
    void m419360();                                  // 0x419360
    tstring name;                // +4
    int f14;
    mesh* parent;                // +0x18
    tstring name2;               // +0x1c
    res_t* res;                  // +0x2c
    unsigned flags;              // +0x30
    int f34; char f38; unsigned color; int f40; float f44; int f48, f4c;
    std::vector<vertex60> v50;   // +0x50
    std::vector<elem30> v60;     // +0x60
    std::vector<point20> v70;    // +0x70
    std::vector<elem14> v80;     // +0x80
    std::vector<face20> v90;     // +0x90
    char local_a0, local_a1;
    keyframes kf;                // +0xa4
};

// 16-byte vector of owned mesh pointers (lod_mesh::lists[i], see mesh.h).
struct ptr_vec : std::vector<mesh*> {
    void tidy() { _Tidy(); }
};
struct mesh_list : ptr_vec {
    ~mesh_list();                                        // 0x419420
    void forget();                                       // 0x419470
};

// ---- resource manager (the object of 0x418a70's resmgr; layout from the member functions) --------
void mb_error(const char* fmt, ...);                 // 0x42cc80 (cdecl)
extern int g_resource_flag;                          // 0x5e28fc

struct res34 { void m418520(); void m4167c0(); };    // element of the vector at +0x30
struct res94 {                                       // element of the vector at +0x90
    char pad[0x18];
    char b18; int f1c, f20, f24, f28;
    void set(int a, int b, int c, int d) { f1c = a; f20 = b; f24 = c; f28 = d; }
    void m417050(int arg);                           // 0x417050
};
// Root of the engine's named resources (vtable 0x54a6c8): slot 1 = type id, slot 3 = save(stream).
struct res_base {
    virtual ~res_base();
    virtual int type();                              // 1 mesh, 2 texture, 3 material, 4 shader, 6 body, 7 skeleton, 8 skeleton_anim
    virtual void slot2();
    virtual void save(stream_base* s);               // slot 3, used by shader / material
    tstring name;                                    // +4
};
struct texture : res_base { int f14; char pad[0x24 - 0x18]; int f24; };    // provisional
struct body_res;      // type 6: saved by 0x417200
struct skeleton_res;  // type 7: saved by 0x416490
struct anim_res;      // type 8: saved by 0x419030
stream_base* save_body(stream_base* s, body_res* b);          // 0x417200
stream_base* save_skeleton(stream_base* s, skeleton_res* k);  // 0x416490
stream_base* save_anim(stream_base* s, anim_res* a);          // 0x419030

// lod_mesh (0xe0 bytes, see mesh.h of s00413c50): five owned mesh lists, a string at +0xd0
struct lod_mesh {
    char pad0[0x34];
    mesh_list lists[5];                              // 0x34
    int count;                                       // 0x84
    int cur;                                         // 0x88
    char pad1[0xd0 - 0x8c];
    tstring name;                                    // 0xd0
};

struct resmgr {
    char pad0[0x10];
    std::vector<lod_mesh*> lods;                     // 0x10
    std::vector<mesh*> meshes;                       // 0x20: name-searched by find()
    std::vector<res34*> v30;                         // 0x30
    std::vector<int*> v40;                           // 0x40
    std::vector<int*> v50;                           // 0x50
    std::vector<int*> v60;                           // 0x60
    std::vector<int*> v70;                           // 0x70
    std::vector<int*> v80;                           // 0x80
    std::vector<res94*> v90;                         // 0x90
    std::list<res_base*> objs;                       // 0xa0
    std::vector<res_base*> all;                      // 0xac: every resource, tested by type()
    tstring name;                                    // 0xbc
    char c_cc, c_cd, c_ce;                           // 0xce: "meshes prepared" flag

    ~resmgr();                                       // 0x419e90
    void save(stream_base* s);                       // 0x419a30
    mesh* find(const tstring* name);                 // 0x418bd0
    mesh* get_object(const tstring* name);           // 0x419540
    int m419580(int arg);                            // 0x419580
    int m4195e0();                                   // 0x4195e0
    void m419640();                                  // 0x419640
    void m418e40();                                  // 0x418e40
};

// serialisers living in neighbouring slices (all cdecl)
stream_base* save_elem14(stream_base* s, elem14* e);       // 0x419190
stream_base* save_frame(stream_base* s, frame_t* f);       // 0x419200
stream_base* save_keyframes(stream_base* s, keyframes* k); // 0x4194c0
stream_base* save_mesh(stream_base* s, mesh* m);           // 0x419760

#endif

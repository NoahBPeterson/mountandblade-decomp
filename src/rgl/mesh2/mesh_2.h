// mesh_2.h: classes of mesh_2.cpp (0x4ca600..0x4ce720, TU map 104 "mesh_2"): the screen-mode queue, the graph
// editor's edge/face operations (graph_obj.h), the big editor records, the std::vector instantiations the linker placed
// in this object, the skeleton fix-up, the language tables' constructor and loader, the csv row and the scene
// object readers. Merged from the slices s004ca550 (0x4ca600..0x4ca9b0), s004caa40, s004cb3a0, s004cbcd0, s004cc680,
// s004cd010, s004cd850 and s004cdfd0 (up to 0x4ce680). Names are provisional unless a vtable maps them.
#ifndef MESH2_MESH_2_H
#define MESH2_MESH_2_H
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <io.h>
#include <new>
#include <vector>
#include <memory>
#include <stdexcept>
#include "../rgl_math.h"
#include "../stream.h"
#include "../../game/tstring.h"
#include "../../game/strat/gui_manager.h"
#include "./mesh2_common.h"
#include "./graph_obj.h"
#include "./screen_cfg.h"

using rgl::vector4;
using rgl::matrix;

// ============================================================================================================
// 0x4ca720..0x4ca830: language table release and the 0xb0-byte record range
// ============================================================================================================
// a 0x4c-byte record: a tstring at +0x18 and a (alloc, first, last) vector triple at +0x38. Its out-of-line
// destructor is 0x4c5110 (editor::clear_vec38 in gui_edit.cpp).
struct item {
    char d0[0x18];
    tstring name;                                // +0x18
    char d28[0x10];
    void* v0;                                    // +0x38
    void* v1;                                    // +0x3c
    void* v2;                                    // +0x40
    char d44[8];                                 // +0x44 .. +0x4c
    ~item();                                     // 0x4c5110
};

// two items: destroying a row destroys its `items[2]` array through __eh_vector_destructor_iterator
// (0x401830) with the out-of-line ~item.
struct row {
    item items[2];                               // +0x00, +0x4c
};

// the last pair is destroyed with an out-of-line call instead (0x4c5150).
struct rec {
    item a;                                      // +0x00
    item b;                                      // +0x4c
    void clear();                                // 0x4c5150
};

void holder_release(void* p, int n);             // 0x4c5150
void holder_release1(void* p);                   // 0x4c5150
// the CRT __eh_vector_destructor_iterator (0x401830, ret 0x10): destroys `n` elements of `size` bytes
// from p onwards, backwards, invoking the out-of-line destructor `d` (thiscall, element in ecx).
void __stdcall vec_dtor_iter(void* p, unsigned size, int n, void (*d)(void*));  // 0x401830
void item_dtor(void* p);                         // 0x4c5110 (same as ~item)
void operator_delete_array(void* p, unsigned n); // 0x53570d (operator delete[], ret 8)

struct mesh_sub {                                // rgl::mesh (0x260 bytes), dtor 0x4127f0
    ~mesh_sub();                                 // 0x4127f0
    char d[0x260];
};
struct scene_sub {                               // rgl::scene, dtor 0x41f4e0
    ~scene_sub();                                // 0x41f4e0
    char d[0x100];
};

struct big_holder {
    void release();                              // 0x4ca830
    char pad0[0x14];
    row rows[5];                                 // +0x14 .. +0x30c (5 * 0x98)
    rec tail;                                    // +0x30c .. +0x3a4
    char pad3a4[4];
    void* v3a8;                                  // +0x3a8 (freed if non-null, then cleared)
    void* v3ac;                                  // +0x3ac
    void* v3b0;                                  // +0x3b0
    char pad3b4[0x4183d4 - 0x3b4];
    void* px_d4;                                 // +0x4183d4 (delete[])
    void* px_d8;                                 // +0x4183d8 (delete[])
    int   pad_dc;                                // +0x4183dc
    mesh_sub m;                                  // +0x4183e0
    scene_sub s;                                 // +0x418640
};

// ============================================================================================================
// 0x4cb610..0x4cbc80: the big editor record's releasers and vector helpers
// ============================================================================================================
// the engine's virtual "destroy the contents then free" record (0x475d50)
struct res_rec {
    virtual void free_contents() = 0;            // +0x00
    virtual void v04() = 0; virtual void v08() = 0; virtual void v0c() = 0;
    virtual void v10() = 0; virtual void v14() = 0; virtual void v18() = 0; virtual void v1c() = 0;
    virtual void v20() = 0; virtual void v24() = 0; virtual void v28() = 0; virtual void v2c() = 0;
    virtual void v30() = 0; virtual void v34() = 0; virtual void v38() = 0; virtual void v3c() = 0;
    virtual void v40() = 0; virtual void v44() = 0; virtual void v48() = 0; virtual void release() = 0;  // +0x4c
    // the direct (non-virtual) teardown entry at 0x475d50: thiscall, `this` in ecx
    void remove();                               // 0x475d50
};

// the lib's 4-arg _Destroy_range (0x4727a0) takes (first, last, allocator&, tag): the tag is an empty
// class passed by value, so the call materialises a local `ptr_tag` before the branch.
struct ptr_tag { };
void obj_free(void* first, void* last, void* al, ptr_tag tag); // 0x4727a0
void delete_obj(void* p);                        // 0x535712

struct rec_2e4 {
    void release();                              // 0x4cb610
    char pad0[0x2e4];
    res_rec* p;                                  // +0x2e4
};
struct rec_1ac {
    void release();                              // 0x4cb650
    char pad0[0x1ac];
    res_rec* p;                                  // +0x1ac
};
struct rec_178 {
    void release();                              // 0x4cb680
    char pad0[0x178];
    res_rec* p;                                  // +0x178
};

struct vec3_holder {
    void clear();                                // 0x4cb6b0
    char pad0[4];
    void* begin;                                 // +0x04
    void* end;                                   // +0x08
    void* cap;                                   // +0x0c
};

// a list node that only counts elements
struct list_node {
    int* next;                                   // +0x00
    int* prev;                                   // +0x04
    int  count;                                  // +0x08
    void grow(unsigned n);                       // 0x4cb6f0
};

// the big record that owns a graph plus a few gadget slots
struct big_obj {
    void release();                              // 0x4cb9e0
    char pad0[0x8];
    void** vec08;                                // +0x08
    void** vec08end;
    void** vec08cap;
    char pad14[0x10];
    tstring ts24;                                // +0x24
    char pad34[0x10];
    void** vec44;                                // +0x44
    void** vec44end;
    void** vec44cap;
    char pad50[0x2400];
    tstring names[0x80];                         // +0x2450 .. +0x2c50
    char pad2c50[0x104];
    gadget* slots[64];                           // +0x2d54 .. +0x2e54 (8 rows of 8)
    char pad2e54[0x20];
    res_rec* p2e74;                              // +0x2e74
    char pad2e78[0x8];
    void** vec2e80;                              // +0x2e80
    void** vec2e80end;
    void** vec2e80cap;
    char pad2e8c[0x4];
    void** vec2e90;                              // +0x2e90
    void** vec2e90end;
    void** vec2e90cap;
    char pad2e9c[0x4];
    void** vec2ea0;                              // +0x2ea0
    void** vec2ea0end;
    void** vec2ea0cap;
    char pad2eac[0x4];
    void** vec2eb0;                              // +0x2eb0
    void** vec2eb0end;
    void** vec2eb0cap;
    // the lib's vector::_Tidy (inlined): free the buffer if any, then zero the triple
    void tidy(void* base) { void** q = (void**)base; if (q[0]) operator delete(q[0]); q[0] = 0; q[1] = 0; q[2] = 0; }
};

struct tvec {
    char pad0[4];
    tstring* begin;                              // +0x04
    tstring* end;                                // +0x08
    tstring* cap;                                // +0x0c
    unsigned size() const { return begin ? (unsigned)(end - begin) : 0; }
    bool reserve_50(unsigned n);                 // 0x4cbb90
    bool reserve_38(unsigned n);                 // 0x4cbbe0
    bool reserve_204(unsigned n);                // 0x4cbc30
    bool reserve_b0(unsigned n);                 // 0x4cbc80
    // the lib's _Xlen (0x43e2a0): thiscall, no arguments, never returns
    __declspec(noreturn) void _Xlen() const;                                 // 0x43e2a0
};

// ============================================================================================================
// 0x4cbcd0..0x4cd850: std::vector instantiations and the element types of this object
// ============================================================================================================
struct E44  { int v[11]; ~E44() {} }; // 0x2c-byte record (empty, user-declared dtor: the original's
                                      // _Destroy loop survives, the call inlines away)
struct E516 { int v[0x81]; };        // 0x204-byte record
struct scalar4 { int v; };           // 4-byte scalar element
struct R44 { int v[11]; };           // 0x2c trivial element
struct b16 { int v[4]; };            // 0x10 POD block

// 0x38-byte record: a tstring followed by five raw pointers that a user destructor frees.
struct E56 {
    tstring s;
    int mid[5];
    void* q[5];
    ~E56() { for (int i = 0; i < 5; ++i) free(q[i]); }
};

// empty base of E68 with the out-of-line 0x4c8330 (Block::free_materials in gui_edit.h is the same address)
struct block_base { void free_materials(); };    // 0x4c8330

struct M28 {                                // 0x1c member of E68
    char* p;                                // 0x00
    int n;                                  // 0x04
    char c;                                 // 0x08
    tstring s;                              // 0x0c
    M28() {}                                // needed by vector<E68>(size_type)
    M28(const M28&);                        // 0x4bd7a0
    ~M28() { if (p) delete[] p; }
    M28& operator=(const M28& o)
        { p = o.p; n = o.n; c = o.c; s = o.s; return *this; }
};

struct E50 {                                // 0x50
    tstring s;                              // 0x00
    int x0, x1, x2, x3;                     // 0x10
    b16 y;                                  // 0x20
    b16 z;                                  // 0x30
    std::vector<R44> v;                     // 0x40
    // memberwise, spelled out: the implicit one would call tstring's copy constructor, because the shared
    // tstring::operator= returns void (src/game/tstring.h)
    E50& operator=(const E50& o)
    {
        s = o.s;
        x0 = o.x0; x1 = o.x1; x2 = o.x2; x3 = o.x3;
        y = o.y;
        z = o.z;
        v = o.v;
        return *this;
    }
};

struct E68 : block_base {                   // 0x68
    tstring s;                              // 0x00
    int b0, b1, b2, b3, b4;                 // 0x10
    int b5, b6, b7, b8, b9;                 // 0x24
    M28 m;                                  // 0x38
    std::vector<scalar4> v;                 // 0x54
    int t;                                  // 0x64
    ~E68() { free_materials(); }
};

struct E176 {                               // 0xb0
    tstring s;                              // 0x00
    int a;                                  // 0x10
    int b;                                  // 0x14
    int c;                                  // 0x18
    std::vector<int> v;                     // 0x1c
    b16 big[8];                             // 0x2c
    int t;                                  // 0xac
};

// _Nonscalar_ptr_iterator_tag: an empty tag that cl 13.10 still passes as one 4-byte stack slot
// (which is where the last `mov eax, [esp+8]` in 0x4cc260 comes from).
struct nonscalar_tag {};
// 0x4ca7e0: std::_Destroy_range for the 0xb0-byte record E176 (frees +0x20, then destroys the string at +0)
void __cdecl destroy_110(void* first, void* last, void* v, nonscalar_tag tag);   // 0x4ca7e0

// two game constructors the linker placed between the vector members
struct U4c {                                // 0x4c repeated sub-object
    int a;                                  // 0x00
    int p04;                                // 0x04 (uninitialised)
    int p08;                                // 0x08 (uninitialised)
    int p0c;                                // 0x0c (uninitialised)
    int d;                                  // 0x10
    int e;                                  // 0x14
    tstring s;                              // 0x18
    int f0;                                 // 0x28
    int f1;                                 // 0x2c
    int g;                                  // 0x30
    int p34;                                // 0x34 (uninitialised)
    int h0;                                 // 0x38
    int h1;                                 // 0x3c
    int h2;                                 // 0x40
    int p44;                                // 0x44 (uninitialised)
    int i1;                                 // 0x48
    U4c() {
        h0 = 0; h1 = 0; h2 = 0;
        a = -1;
        g = 1;
        f0 = 0; f1 = 0;
        d = 0; e = 1;
        i1 = 0;
        s.assign("unknown buffer pair");    // 0x59607c
    }
};

struct Obj720 {
    U4c u[2];
    Obj720();                               // 0x4cd720
};

struct vec4f {
    float x, y, z, w;
    vec4f() {}
    vec4f(float a, float b, float c) : x(a), y(b), z(c) {}
};

struct Obj7a0 {
    tstring s;                              // 0x00
    float f10;                              // 0x10
    float f14;                              // 0x14
    float f18;                              // 0x18
    float f1c;                              // 0x1c
    vec4f v20;                              // 0x20
    vec4f v30;                              // 0x30
    int i40;                                // 0x40 (uninitialised)
    void* p44;                              // 0x44
    void* p48;                              // 0x48
    void* p4c;                              // 0x4c
    Obj7a0();                               // 0x4cd7a0
};

// ============================================================================================================
// 0x4cd850..0x4cdf20: skeleton fix-up and the csv row (the translation tables are LangTables, mesh2_common.h)
// ============================================================================================================
struct Bone {                                                   // 0x50 bytes: bone of a registered skeleton
    tstring name;                                               // +0x00
    int idx;                                                    // +0x10
    int parent;                                                 // +0x14
    int f18;                                                    // +0x18
    int f1c;                                                    // +0x1c
    vector4 v20;                                                // +0x20
    vector4 v30;                                                // +0x30
    std::vector<R44> m40;                                       // +0x40 (assigned with 0x4ccd80)
};
struct Skeleton {                                               // 0x20 bytes: one registered skeleton
    tstring model;                                              // +0x00
    std::vector<Bone> bones;                                    // +0x10 (first +0x14, last +0x18)
};
struct SkelRes {                                                // the object 0x418cf0 returns
    char pad[0x18];
    int count;                                                  // +0x18
    char* bones;                                                // +0x1c: 0xdc-byte bones
};
struct SkelBone {                                               // the resource bone, seen from the destination
    char pad[0x8c];
    tstring name;                                               // +0x8c
    int f9c;                                                    // +0x9c (source +0x10)
    int fa0;                                                    // +0xa0 (source +0x14)
    int fa4;                                                    // +0xa4 (source +0x18)
    int fa8;                                                    // +0xa8 (source +0x1c)
    vector4 vac;                                                // +0xac
    vector4 vbc;                                                // +0xbc
    std::vector<R44> vcc;                                       // +0xcc (source +0x40)
};
// The 0x10-byte "vector" these records carry: a leading word followed by the usual begin/end/cap trio
// (0x43f0a0 is the assignment, reading _Myfirst at +4 and _Mylast at +8).
struct mesh_vector {
    void* f0;                                                   // +0x00
    mesh** begin;                                               // +0x04
    mesh** end;                                                 // +0x08
    mesh** cap;                                                 // +0x0c
    mesh_vector() : begin(0), end(0), cap(0) {}                 // zeroes the trio, leaves f0
    unsigned size() const { return begin ? (unsigned)(end - begin) : 0; }
    void assign(const mesh_vector& o);                          // 0x43f0a0 (vector<scalar4>::operator=, thiscall ret 4)
};
// the by-value result of find_meshes: the temporary's inline destructor frees the buffer and zeroes the trio
struct mesh_vector_tmp : mesh_vector {
    ~mesh_vector_tmp() { if (begin) operator delete(begin); begin = 0; end = 0; cap = 0; }
};

struct resmgr {                                                 // global at 0x5d6fa0
    SkelRes* get_object70(const tstring& name);                 // 0x418cf0 (ret 4)
    mesh* get_mesh(const tstring* name);                        // 0x418c10 (provisional)
    // hidden-return form of the std::vector<mesh*> result: at the call site the buffer is a raw 0x10-byte block
    mesh_vector* find_meshes(mesh_vector* ret, tstring* prefix);   // 0x41b960 (ret 8)
    mesh_vector_tmp find_meshes_tmp(tstring* prefix);              // 0x41b960 (hidden return pointer first, ret 8)
};
extern resmgr g_resmgr;                                         // 0x5d6fa0

// the 0x18-byte alternative-text record: its implicit ctor only builds the leading tstring
struct alt_rec {
    tstring s;                                                  // +0x00
    int a;                                                      // +0x10
    int b;                                                      // +0x14
};
// the tstring vector a csv row carries: 0xc bytes (three raw pointers, no leading pad -- the original's
// ctor writes exactly these three dwords and its clear() frees the buffer and re-zeroes them)
struct str_vec {
    tstring* first;                                             // +0x00
    tstring* last;                                              // +0x04
    tstring* end;                                               // +0x08
    str_vec() : first(0), last(0), end(0) { }
    void clear() { if (first) operator delete(first); first = 0; last = 0; end = 0; }
};
struct csv_row {                                                // 0x238 bytes (0x4cdf20)
    tstring key;                                                // +0x00
    tstring value;                                              // +0x10
    tstring f20;                                                // +0x20
    char pad30[0x4];
    str_vec v34;                                                // +0x34
    float f40;                                                  // +0x40 (1.0f)
    int f44;                                                    // +0x44 (0)
    alt_rec alts[16];                                           // +0x48
    int f1c8;                                                   // +0x1c8 (0)
    char pad1cc[0x1d8 - 0x1cc];
    int f1d8;                                                   // +0x1d8 (10)
    char pad1dc[0x4];
    int f1e0;                                                   // +0x1e0 (0)
    int f1e4;                                                   // +0x1e4 (0)
    char pad1e8[0x21c - 0x1e8];
    float f21c;                                                 // +0x21c (1.0f)
    int f220;                                                   // +0x220 (100)
    float f224;                                                 // +0x224 (1.0f)
    str_vec v228;                                               // +0x228
    char pad234[0x4];
    csv_row();                                                  // 0x4cdf20
};

extern std::vector<Skeleton> g_skeletons;                      // 0x5d6fa0 + 0x70 (provisional)
void vec_insert_n(lang_bucket* vec, tstring* pos, unsigned n, const tstring& v);   // 0x438f00
void vec_insert_n2(lang_bucket* vec, tstring* pos, unsigned n, const tstring& v);  // 0x43e310
void __fastcall fix_skeletons(std::vector<Skeleton>* list);               // 0x4cd850


// ============================================================================================================
// 0x4cdfd0..0x4ce680: the scene object tables
// ============================================================================================================
// (`mesh` is the opaque loaded-mesh class; gui_manager.h declares a sized stand-in)
struct shape_res {                                              // the body shape 0x439100 returns
    virtual void v0(); virtual void v1(); virtual void v2(); virtual void v3();
    virtual void v4(); virtual void v5(); virtual void v6(); virtual void v7();
    virtual void v8(); virtual void v9();
    virtual vector4* get_bounds_min(vector4* out);              // 10 (+0x28)
    virtual vector4* get_bounds_max(vector4* out);              // 11 (+0x2c)
};
struct mesh_body_t { shape_res* copy_shape(); };                // 0x439100 (thiscall: mesh -> body shape)
inline shape_res* body_copy_shape(mesh* m) { return ((mesh_body_t*)m)->copy_shape(); }
struct mesh_bounds_t { void bounds(vector4* mn, vector4* mx); };   // 0x438d80 (thiscall, ret 8)
inline void mesh_node_bounds(mesh* node, vector4* mn, vector4* mx) { ((mesh_bounds_t*)node)->bounds(mn, mx); }
void vmin_ext(vector4* r, const vector4* a, const vector4* b);  // 0x40af80
void vmax_ext(vector4* r, const vector4* a, const vector4* b);  // 0x40b080

struct game_vec {                                               // the 12-byte record at MissionObj +0x6c
    void resize(void* owner, const char* name, int f);          // 0x4c1e10
    void stamp(int f);                                          // 0x4c1ee0
    void clear();
    int v[3];
    game_vec() { v[0] = 0; v[1] = 0; v[2] = 0; }
};

// 0x4cdfd0 fills a heap block: a header dword (count) followed by `count` records of 0x48 bytes.
struct shape_entry {                                            // 0x48 bytes
    tstring name;                                               // +0x00
    mesh_vector nodes;                                          // +0x10
    mesh* shape;                                                // +0x20 (0 when the name is "0")
    shape_res* body;                                            // +0x24 (body_copy_shape(shape))
    vector4 mn;                                                 // +0x28 (1e7f, merged with 0x438d80 bounds)
    vector4 mx;                                                 // +0x38 (-1e7f, merged with 0x438d80 bounds)
    shape_entry() : shape(0), body(0) {}
    std::vector<mesh*>& nodev() { return *(std::vector<mesh*>*)&nodes; }   // the vector view (allocator lane at +0x10)
};

struct ObjectList {                                             // "0x4cdfd0's class"
    tstring name;                                               // +0x00
    int f10;                                                    // +0x10
    int f14;                                                    // +0x14
    int count;                                                  // +0x18
    shape_entry* entries;                                       // +0x1c (points past the block header)
    void read(stream_base* s);                                  // 0x4cdfd0 (ret 4)
};

// One "mission object" record (0x4ce2a0).
struct MissionObj {
    tstring name;                                               // +0x00
    int f10;                                                    // +0x10
    int f14;                                                    // +0x14
    vector4 v18;                                                // +0x18 (0,0,0)
    vector4 frame;                                              // +0x28 (1,1,1; .w = 1.0f)
    int f38;                                                    // +0x38
    int f3c;                                                    // +0x3c
    mesh_vector nodes_;                                         // +0x40 (mesh list; begin at +0x44)
    mesh* shape;                                                // +0x50
    shape_res* body;                                            // +0x54
    vector4 v58;                                                // +0x58 (0,0,0; centre in read())
    float radius;                                               // +0x68 (0.1f)
    game_vec v70;                                               // +0x6c (12 bytes)
    int f78;                                                    // +0x78
    int f7c;                                                    // +0x7c
    int f80; int f84; int f88; int f8c; int f90; int f94;
    int f98; int f9c; int fa0; int fa4; int fa8; int fac;
    int padb0;
    MissionObj();                                               // 0x4ce2a0
    void read(stream_base* s);                                  // 0x4ce360 (ret 4)
};

// Another 0xb0-byte record (0x4ce680).
struct MissionRec2 {
    tstring name;                                               // +0x00
    char pad10[0x4];
    int f14;                                                    // +0x14 = 0
    int f18;
    char pad1c[0x4];
    int f20;                                                    // +0x20 = 0
    int f24;                                                    // +0x24 = 0
    int f28;                                                    // +0x28 = 0
    vector4 frame;                                              // +0x2c (1.0f scale)
    vector4 v3c;                                                // +0x3c
    int f4c;
    int f50;
    char pad54[0x58];
    int fac;                                                    // +0xac = 2
    MissionRec2();                                              // 0x4ce680
};

inline void vec_assign_43f0a0(mesh_vector* self, const mesh_vector* other) { self->assign(*other); }



#endif

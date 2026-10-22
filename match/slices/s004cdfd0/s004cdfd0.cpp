// Slice s004cdfd0: readers for the two scene-object tables (the continuation of s004cd850 / s004ce960).
// 0x4cdfd0 reads a list of named shapes into a heap block of per-entry records, 0x4ce2a0 / 0x4ce680 are
// plain record constructors, 0x4ce360 reads one "Mission object" link, 0x4ce720 constructs the 3 MB scene
// generator, 0x4ce870 / 0x4ce8d0 construct two 0x60-byte record types and 0x4ce930 destroys the first.
// ALL NAMES PROVISIONAL. Layouts and field offsets reproduced from the disassembly of this slice only.
//
// Reader convention (all three readers): the stream object arrives in arg1 and is called through its own
// vtable -- +0x54 (this, tstring*) reads a quoted string, +0x44 (this, tstring*) reads a name, +0x4c
// (this, int*) reads a bare integer.
#include <stddef.h>
#include <stdio.h>
#include <new>
#include <vector>
#include "rgl.h"
using rgl::vector4;

// ---- tstring (16 bytes; s004608d0) ----------------------------------------------------------------------
// The two explicit calls are written through the same method-address-pointer idiom as s004c2b20 (the bodies
// live in another slice, so they are declared-only here).
struct tstring {
    int pos; char* buf; int cap; int len;
    void ctor();                                                // 0x42bbd0
    void dtor();                                                // 0x42bc40
    tstring();                                                  // 0x42bbd0
    tstring(const char* s);                                     // 0x42bbe0
    ~tstring();                                                 // 0x42bc40
    void operator=(const tstring& o);                           // 0x42bc50
    void assign(const char* s);                                 // 0x42bcb0
    char equals(const char* s) const;                           // 0x42bd10
    char differs(const char* s) const;                          // 0x42bdb0
    tstring substr(int start, int end) const;                   // 0x42c4a0
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};
typedef char tstring_size_check[(sizeof(tstring) - 0x10) * 0 + 1];
#define TCTOR(x) (x).ctor()                                     // explicit tstring construction at a raw address
#define TDTOR(x) (x).dtor()

// ---- resources (s0041ac10) ------------------------------------------------------------------------------
struct mesh;                                                    // a loaded mesh (opaque here)
struct shape_res;                                              // the body shape 0x439100 returns
struct resmgr {
    std::vector<mesh*> find_meshes(const tstring* prefix);      // 0x41b960 (vector returned via hidden ret)
    mesh* get_mesh(const tstring* name);                        // 0x418c10 (provisional)
};
extern resmgr g_resmgr;                                         // 0x5d6fa0
shape_res* body_copy_shape(mesh* m);                            // 0x439100 (provisional: mesh -> body shape)
void mesh_node_bounds(mesh* node, vector4* mn, vector4* mx);    // 0x438d80
void vmin_ext(vector4* r, const vector4* a, const vector4* b);  // 0x40af80
void vmax_ext(vector4* r, const vector4* a, const vector4* b);  // 0x40b080
struct mesh {
    virtual void v0(); virtual void v1(); virtual void v2(); virtual void v3();
    virtual void v4(); virtual void v5(); virtual void v6(); virtual void v7();
    virtual void v8(); virtual void v9();
    virtual void get_bounds_min(vector4* out, int f);           // 10 (+0x28)
    virtual void get_bounds_max(vector4* out, int f);           // 11 (+0x2c)
};
// The per-shape "body" object 0x439100 returns: its +0x28/+0x2c slots fill a bounds vector4 and hand it back.
struct shape_res {
    virtual void v0(); virtual void v1(); virtual void v2(); virtual void v3();
    virtual void v4(); virtual void v5(); virtual void v6(); virtual void v7();
    virtual void v8(); virtual void v9();
    virtual vector4* get_bounds_min(vector4* out);              // 10 (+0x28)
    virtual vector4* get_bounds_max(vector4* out);              // 11 (+0x2c)
};
struct game_vec {                                               // the 12-byte record at MissionObj +0x6c
    void resize(void* owner, const char* name, int f);          // 0x4c1e10
    void stamp(int f);                                          // 0x4c1ee0
    void clear();
    int v[3];
    game_vec() { v[0] = 0; v[1] = 0; v[2] = 0; }
};

// ---- stream (s004b0940) ---------------------------------------------------------------------------------
struct stream_base {
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4();
    virtual void s5(); virtual void s6(); virtual void s7(); virtual void s8(); virtual void s9();
    virtual void s10(); virtual void s11(); virtual void s12(); virtual void s13(); virtual void s14();
    virtual void s15(); virtual void s16();
    virtual void v17(void* out);                                // +0x44 (reads a name)
    virtual void v18();                                         // +0x48
    virtual void v19(void* out);                                // +0x4c (reads a bare integer)
    virtual void v20();                                         // +0x50
    virtual void v21(void* out);                                // +0x54 (reads a quoted string)
};
const float HALF = 0.5f;                                        // 0x549d70

// ---- scene object tables --------------------------------------------------------------------------------
// The 0x10-byte "vector" these records carry: a leading word followed by the usual begin/end/cap trio
// (0x43f0a0 is the assignment, reading _Myfirst at +4 and _Mylast at +8).
struct mesh_vector {
    void* f0;                                                   // +0x00
    mesh** begin;                                               // +0x04
    mesh** end;                                                 // +0x08
    mesh** cap;                                                 // +0x0c
    mesh_vector() : begin(0), end(0), cap(0) {}                 // zeroes the trio, leaves f0
};
typedef char mesh_vector_size_check[(sizeof(mesh_vector) - 0x10) * 0 + 1];

// 0x4cdfd0 fills a heap block: a header dword (count) followed by `count` records of 0x48 bytes.
struct shape_entry {                                            // 0x48 bytes
    tstring name;                                               // +0x00 (stream +0x54)
    mesh_vector nodes;                                          // +0x10 (stream +0x54, then assigned from 0x41b960)
    mesh* shape;                                                // +0x20 (0 when the name is "none")
    shape_res* body;                                            // +0x24 (body_copy_shape(shape))
    vector4 mn;                                                 // +0x28 (10000.0f, merged with 0x438d80 bounds)
    vector4 mx;                                                 // +0x38 (-10000.0f, merged with 0x438d80 bounds)
};
typedef char shape_entry_size_check[(sizeof(shape_entry) - 0x48) * 0 + 1];

struct ObjectList {                                             // "0x4cdfd0's class"
    tstring name;                                               // +0x00
    int f10;                                                    // +0x10
    int f14;                                                    // +0x14
    int count;                                                  // +0x18
    shape_entry* entries;                                       // +0x1c (points past the block header)
    void read(stream_base* s);                                  // 0x4cdfd0 (ret 4)
};
// One "mission object" record (0x4ce2a0).
struct ptr3 { void* p0; void* p1; void* p2; ptr3() : p0(0), p1(0), p2(0) {} }; // a 12-byte pointer triple
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

// The scene generator (0x4ce720).
struct rot_state {                                              // 0x34 bytes
    rot_state();                                                // 0x40eaf0
};
struct SceneGen {
    char pad0[0x18];
    tstring names[15];                                          // +0x18, stride 0x28
    char pad270[0x10];
    int f280;                                                   // +0x280 = 0
    int f284;                                                   // +0x284 = 2
    char pad288[0xc3b0e0 - 0x288];
    float f_c3b0e0;                                             // +0xc3b0e0 = 2.0f
    char pad_c3b0e4[0x343b4ec - 0xc3b0e4];
    int f_343b4ec;                                              // +0x343b4ec = 0
    char pad2[0x5c3b8f4 - 0x343b4f0];
    int f_5c3b8f4;                                              // +0x5c3b8f4 = 0
    char pad3[0x843bd2c - 0x5c3b8f8];
    vector4 v_843bd2c;                                          // +0x843bd2c (0.2f, 0.2f, 0.2f)
    vector4 v_843bd3c;                                          // +0x843bd3c (0.6f, 0.55f, 0.5f)
    char pad4[0x843bd70 - 0x843bd4c];
    int f_843bd70;                                              // +0x843bd70 = 0x28
    char b_843bd74;                                             // +0x843bd74 = 0
    char b_843bd75;                                             // +0x843bd75 = 0
    char pad5[0x8440578 - 0x843bd76];
    rot_state states[0x900];                                    // +0x8440578, stride 0x34
    tstring tail;                                               // +0x845e784
    int f_845e7bc;                                              // +0x845e7bc = 0
    int f_845e7c0;                                              // +0x845e7c0 = 0
    int f_845e7c4;                                              // +0x845e7c4 = 0
    SceneGen(int a);                                            // 0x4ce720
    void gen_reset();                                           // 0x4647e0
};
// ---- 0x60-byte records (0x4ce870 / 0x4ce8d0) ------------------------------------------------------------
// Both start with the shared "head" base (vtable 0x5947d8): the zero word at +4, the 99 status word at +8
// and the name tstring at +0x1c. The base ctor is inlined into each derived ctor (vptr store, tstring ctor,
// f8 = 99), then the derived ctor stores its own vtable and the remaining fields.
struct head_rec {
    virtual ~head_rec() { }                                     // releasing: vptr reset + tstring
    int f4;                                                     // +0x04
    int f8;                                                     // +0x08 = 99
    char pad0c[0x1c - 0xc];
    tstring name;                                               // +0x1c
    head_rec() : name(), f8(99) { }
};
// The 0x10-byte owned "block" at +0x30: the pointer at +4 is released, +8/+0xc cleared (0x4c8bf0).
struct owned_block {
    int f0;                                                     // +0x30 (never initialised)
    void* begin;                                                // +0x34 = 0
    void* end;                                                  // +0x38 = 0
    void* cap;                                                  // +0x3c = 0
    ~owned_block();                                             // 0x4c8bf0
};
struct ItemRec : head_rec {                                     // vtable 0x596124
    char pad2c[4];                                              // +0x2c
    owned_block owned;                                          // +0x30
    int f40;                                                    // +0x40 = -1
    int pad44;
    int f48;                                                    // +0x48 = 0
    int pad4c;
    int clock;                                                  // +0x50 = g_ticks[0]
    int clock_hi;                                               // +0x54 = g_ticks[1]
    float f58;                                                  // +0x58 = 1.0f
    int f5c;                                                    // +0x5c = 0
    ItemRec();                                                  // 0x4ce870
};
struct ItemRec2 : head_rec {                                    // vtable 0x596148
    char pad2c[4];
    owned_block owned;
    int f40;
    int pad44;
    int f48;
    int pad4c;
    int clock;
    int clock_hi;
    float f58;
    int f5c;
    ItemRec2();                                                 // 0x4ce8d0
};
void vec_assign_43f0a0(mesh_vector* self, const mesh_vector* other); // 0x43f0a0
std::vector<mesh*> resmgr_find_meshes_41b960(resmgr* self, tstring* prefix); // 0x41b960
mesh_vector* resmgr_find_meshes_v41b960(resmgr* self, tstring* prefix, mesh_vector* ret);
                                    // hidden-return form: declared separately because at the call site the
                                    // result buffer is a raw 0x10-byte block, not a standard vector.

extern int g_ticks[];                                           // 0x5e2948 / 0x5e294c
void __fastcall free_rec(void* p);                              // 0x4c8bf0: releases the owned block
const char g_none[] = "none";                                   // 0x54ac00

// @ 0x4cdfd0
void ObjectList::read(stream_base* s)
{
    tstring buf;
    s->v21(&name);
    s->v17(&f10);
    s->v19((void*)&count);
    int cnt = count;
    char* block = (char*)::operator new(cnt * 0x48 + 4);
    if (block) {
        *(int*)block = cnt;
        shape_entry* e = (shape_entry*)(block + 4);
        entries = e;
        for (int k = cnt - 1; k >= 0; k--) {
            TCTOR(e->name);
            e->nodes.begin = 0;
            e->nodes.end = 0;
            e->nodes.cap = 0;
            e->shape = 0;
            e->body = 0;
            e++;
        }
    } else {
        entries = 0;
    }
    if (count > 0) {
        shape_entry* c = entries;
        for (int i = 0; i < count; i++) {
            s->v21(&c->name);
            mesh_vector nodes;
            resmgr_find_meshes_v41b960(&g_resmgr, &c->name, &nodes);
            vec_assign_43f0a0(&c->nodes, &nodes);
            if (nodes.begin)
                ::operator delete(nodes.begin);
            c->mn = vector4(10000.0f, 10000.0f, 10000.0f);
            c->mx = vector4(-10000.0f, -10000.0f, -10000.0f);
            nodes.begin = 0;
            nodes.end = 0;
            nodes.cap = 0;
            for (unsigned j = 0; j < (unsigned)((c->nodes.end - c->nodes.begin) >> 2); j++) {
                mesh* node = c->nodes.begin[j];
                vector4 a, b;
                mesh_node_bounds(node, &a, &b);
                if (c->mn.x < a.x) ; else c->mn.x = a.x;
                if (c->mn.y < a.y) ; else c->mn.y = a.y;
                if (c->mn.z < a.z) ; else c->mn.z = a.z;
                if (c->mx.x > a.x) ; else c->mx.x = a.x;
                if (c->mx.y > a.y) ; else c->mx.y = a.y;
                if (c->mx.z > a.z) ; else c->mx.z = a.z;
            }
            s->v21(&buf);
            if (buf.equals(g_none)) {
                c->shape = 0;
            } else {
                c->shape = g_resmgr.get_mesh(&buf);
                c->body = body_copy_shape(c->shape);
            }
            c = c + 1;
        }
    }
    s->v21(&buf);
}

// @ 0x4ce2a0
MissionObj::MissionObj()
{
    v18 = vector4(0.0f, 0.0f, 0.0f);
    frame = vector4(1.0f, 1.0f, 1.0f);
    shape = 0; body = 0;
    v58 = vector4(0.0f, 0.0f, 0.0f);
    frame.w = 1.0f;
    radius = 0.1f;
}

// @ 0x4ce360
void MissionObj::read(stream_base* s)
{
    char buf[0x200];
    vector4 mn(10000.0f, 10000.0f, 10000.0f);
    vector4 mx(-10000.0f, -10000.0f, -10000.0f);
    s->v21(&name);
    s->v19(&f10);
    s->v19(&f14);
    tstring t0;
    tstring t1;
    s->v21(&t0);
    if (t0.len > 1 && t0.differs(g_none)) {
        mesh_vector nodes;
        resmgr_find_meshes_v41b960(&g_resmgr, &t0, &nodes);
        vec_assign_43f0a0(&nodes_, &nodes);
        if (nodes.begin)
            ::operator delete(nodes.begin);
        unsigned j = 0;
        while (1) {
            if (nodes_.begin == 0 || (nodes_.end - nodes_.begin) <= (int)j)
                break;
            vector4 a, b;
            mesh_node_bounds(nodes_.begin[j], &a, &b);
            vmax_ext(&mx, &mx, &b);
            vmin_ext(&mn, &mn, &a);
            j++;
        }
        v58.x = (mn.x + mx.x) * HALF;
        v58.y = (mn.y + mx.y) * HALF;
        v58.z = (mn.z + mx.z) * HALF;
        v58.w = 0;
        radius = rgl::length(mx - v58);
    }
    s->v21(&t1);
    if (t1.len > 1) {
        shape = g_resmgr.get_mesh(&t1);
        body = body_copy_shape(shape);
        vector4 b;
        vmax_ext(&mx, &mx, body->get_bounds_max(&b));
        vmin_ext(&mn, &mn, body->get_bounds_min(&b));
        v58.x = (mn.x + mx.x) * HALF;
        v58.y = (mn.y + mx.y) * HALF;
        v58.z = (mn.z + mx.z) * HALF;
        v58.w = 0;
        radius = rgl::length(mx - v58);
    }
    _snprintf(buf, 0x200, "Mission object %s", name.c_str());
    v70.resize(s, buf, 0);
    v70.stamp(2);
}

// @ 0x4ce680
MissionRec2::MissionRec2()
{
    f20 = 0; f24 = 0; f28 = 0; f14 = 0;
    frame = vector4(1.0f, 1.0f, 1.0f);
    v3c = vector4(1.0f, 1.0f, 1.0f);
    fac = 2;
}

// @ 0x4ce720
SceneGen::SceneGen(int a)
{
    for (int i = 0; i < 15; i++)
        new (&names[i]) tstring;
    f_343b4ec = 0;
    int* px = (int*)((char*)this + 0x5c3b8f4);
    *px = 0;
    for (int i = 0; i < 0x900; i++)
        new (&states[i]) rot_state;
    new (&tail) tstring;
    f_845e7bc = 0;
    f_845e7c0 = 0;
    f_845e7c4 = 0;
    v_843bd2c = vector4(0.2f, 0.2f, 0.2f);
    v_843bd2c.w = a;
    v_843bd3c = vector4(0.6f, 0.55f, 0.5f);
    v_843bd3c.w = a;
    f_c3b0e0 = 2.0f;
    f_843bd70 = 0x28;
    b_843bd74 = 0;
    b_843bd75 = 0;
    f280 = 0;
    f284 = 2;
    gen_reset();
}

// @ 0x4ce870
ItemRec::ItemRec()
{
    f4 = 0;
    owned.begin = 0;
    owned.end = 0;
    owned.cap = 0;
    f48 = 0;
    clock = g_ticks[0];
    clock_hi = g_ticks[1];
    f5c = 0;
    f58 = 1.0f;
    f40 = -1;
}

// @ 0x4ce8d0
ItemRec2::ItemRec2()
{
    f4 = 0;
    owned.begin = 0;
    owned.end = 0;
    owned.cap = 0;
    f48 = 0;
    clock = g_ticks[0];
    clock_hi = g_ticks[1];
    f5c = 0;
    f58 = 1.0f;
    f40 = -1;
}

// @ 0x4ce930
owned_block::~owned_block()
{
    free_rec(this);
}

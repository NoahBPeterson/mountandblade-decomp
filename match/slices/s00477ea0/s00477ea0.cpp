// Slice s00477ea0: vector<Agent>::resize(n), id-pool (pages of 16 records) reset/alloc for the 0x5928- and
// 0xb0-byte records, and a height-grid / navigation map (serialize, vertex + edge building).
#include <vector>
#include <new>
#include <math.h>
#include "rgl.h"
using rgl::vector4;

extern int g_tick_lo;                           // 0x5e2958
extern int g_tick_hi;                           // 0x5e295c

struct tstring {
    int pos; char* buf; int cap; int len;
    tstring();                                  // 0x42bbd0
    ~tstring();                                 // 0x42bc40
    tstring(const tstring& o);                  // 0x42c110
    tstring& operator=(const tstring& o);       // 0x42bc50
};

struct IfaceB {
    int f4, f8, fc;
    IfaceB() {}
    virtual ~IfaceB() {}
    virtual void slot1() {}
    virtual void slot2() {}
};

// Agent: 0x1c8-byte record, vtable 0x590b84 (declaration from s004375b0, verified there).
struct vector4f { float x, y, z, w; };
struct Quad {
    int a, b, c, d;
    Quad() : a(2), c(g_tick_lo), d(g_tick_hi) {}
};
struct Agent : IfaceB {
    char body1[0x170 - 0x10];
    Quad q170;                                  // +0x170
    char body2[0x190 - 0x180];
    tstring name;                               // +0x190
    char body3[0x1c8 - 0x1a0];
    Agent() { reset(); }                        // 0x437830 (inlined)
    void reset();                               // 0x4352f0
    Agent(const Agent& o);                      // 0x437bb0
    Agent& operator=(const Agent& o);           // 0x437d60
};

// @ 0x477ea0
template void std::vector<Agent>::resize(size_t);

// ---- id pools: records handed out from pages of 16 (layout verified in s00440a30, Pool<0xb0>::alloc) ----
// 0xb0-byte record (RecJ of s004375b0): +4 in-use flag, +8 id, +0xc serial number.
struct RecJ {
    int f0, used, id, serial;
    char pad[0xb0 - 0x10];
    RecJ();                                     // 0x4377d0
};
// 0x5928-byte IfaceB-derived record (RecZ of s00431960); constructor 0x477a30.
struct RecZ {
    int f0, used, id, serial;
    char pad[0x5928 - 0x10];
    RecZ();                                     // 0x477a30
    ~RecZ();                                    // (non-trivial: array new stores a count cookie)
};
template<class T> struct Pool {
    int next_id;                                // +0
    std::vector<int> free_ids;                  // +4
    std::vector<T*> pages;                      // +0x14 (first at +0x18)
    int serial;                                 // +0x24
    void reset();
    int alloc();
};

// @ 0x477f00
template<> void Pool<RecZ>::reset()
{
    free_ids.resize(0, 0);
    for (int i = 0; i < next_id; i++) {
        pages[(unsigned)i >> 4][i & 0xf].used = 0;
        free_ids.push_back(i);
    }
}
// @ 0x477f90
template<> void Pool<RecJ>::reset()
{
    free_ids.resize(0, 0);
    for (int i = 0; i < next_id; i++) {
        pages[(unsigned)i >> 4][i & 0xf].used = 0;
        free_ids.push_back(i);
    }
}
// @ 0x478020
template<> int Pool<RecZ>::alloc()
{
    if (!(free_ids.size() > 0)) {
        RecZ* blk = new RecZ[16];
        pages.push_back(blk);
        for (int j = 0; j < 16; j++) {
            blk[j].used = 0;
            blk[j].id = next_id + j;
            free_ids.push_back(next_id + j);
        }
        next_id += 16;
    }
    unsigned id = free_ids.back();
    free_ids.pop_back();
    RecZ* r = &pages[id >> 4][id & 0xf];
    r->used = 1;
    r->serial = serial;
    serial++;
    return id;
}

// ---- height-grid navigation map ---------------------------------------------------------------------------

// Serialization stream (slot layout from s0040cfe0): slot 14 (+0x38) reads a float and returns the stream,
// slot 19 (+0x4c) reads an int.
struct stream {
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4(); virtual void s5();
    virtual void s6(); virtual void s7(); virtual void s8(); virtual void s9(); virtual void s10(); virtual void s11();
    virtual void s12(); virtual void s13();
    virtual stream* read_f(float* p);                   // slot 14
    virtual void s15(); virtual void s16(); virtual void s17(); virtual void s18();
    virtual void read_i(void* p);                       // slot 19
};

// vertex (0x18 bytes); only x, y, z and the flag byte are ever set here
struct NavVert { vector4 pos; char flag; int pad; };
// edge (0x34 bytes)
struct NavEdge { int a, b, c, d; int e; char flag; float g, h; int tail[5]; };
// face (0x64 bytes): up to 4 vertex ids and 4 edge ids
struct NavFace { int m0; char pad4[0x28 - 4]; int v[4]; int e[4]; int n; char pad4c[0x5c - 0x4c]; int m5c; char flag; char pad61[3]; };

struct HeightField {
    vector4** cols;                             // +0 (column pointers, 16-byte cells)
    int pad[4];
    int w, h;                                   // +0x14, +0x18
    float scale;                                // +0x1c
};

// one-argument resize (default-constructed fill value) lives out of line in the original
struct FaceVec : std::vector<NavFace> {
    void resize_default(unsigned n);            // 0x477ca0
};
struct EdgeVec : std::vector<NavEdge> {
    void resize_default(unsigned n);            // 0x477db0
};

struct NavCore {
    std::vector<int> v0;                        // +0
    std::vector<NavVert> verts;                 // +0x10
    FaceVec faces;                              // +0x20
    EdgeVec edges;                              // +0x30
    int m40;
    vector4 e44, f54;
    int m64;
    NavCore();                                  // 0x478150
};

// @ 0x478150
NavCore::NavCore()
{
    e44 = vector4(0.0f, 0.0f, 0.0f);
    f54 = vector4(0.0f, 0.0f, 0.0f);
    m64 = 0;
}

struct NavMap {
    int pad0, pad4;
    NavCore c;                                  // +8 (verts +0x18, faces +0x28, edges +0x38)
    char pad70[0x90 - 0x70];
    int step;                                   // +0x90
    void load(stream* s);                       // 0x4781d0
    int add_edge(int a, int b);                 // 0x478450
    int add_face(int n, int v0, int v1, int v2, int v3, int e0, int e1, int e2, int e3);   // 0x4784d0
    void clear();                               // 0x478580
    void build(HeightField* hf, int step_);     // 0x4785e0
    void finish();                              // 0x470920
};

// @ 0x4781d0
void NavMap::load(stream* s)
{
    int n1, n2, n3;
    s->read_i(&n1);
    NavVert v;
    v.flag = 0;
    c.verts.resize(n1, v);
    for (int i = 0; i < n1; i++) {
        NavVert* p = &c.verts[i];
        s->read_f(&p->pos.x)->read_f(&p->pos.y)->read_f(&p->pos.z);
    }
    s->read_i(&n2);
    NavEdge e;
    e.a = -10000;
    e.b = -10000;
    e.c = -10000;
    e.d = -10000;
    e.flag = 0;
    e.e = 0;
    e.g = 1.0f;
    e.h = 1.0f;
    c.edges.resize(n2, e);
    for (int i = 0; i < n2; i++) {
        s->read_i(&c.edges[i].e);
        s->read_i(&c.edges[i].a);
        s->read_i(&c.edges[i].b);
        s->read_i(&c.edges[i].c);
        s->read_i(&c.edges[i].d);
    }
    s->read_i(&n3);
    c.faces.resize_default(n3);
    for (int i = 0; i < n3; i++) {
        s->read_i(&c.faces[i].n);
        for (int j = 0; j < c.faces[i].n; j++)
            s->read_i(&c.faces[i].v[j]);
        for (int j = 0; j < c.faces[i].n; j++)
            s->read_i(&c.faces[i].e[j]);
        s->read_i(&c.faces[i].m5c);
    }
}

// @ 0x478450
int NavMap::add_edge(int a, int b)
{
    NavEdge e;
    e.c = -10000;
    e.d = -10000;
    e.a = a;
    e.flag = 0;
    e.e = 0;
    e.g = 1.0f;
    e.h = 1.0f;
    e.b = b;
    c.edges.push_back(e);
    return (int)c.edges.size() - 1;
}

// @ 0x4784d0
int NavMap::add_face(int n, int v0, int v1, int v2, int v3, int e0, int e1, int e2, int e3)
{
    NavFace f;
    f.n = n;
    f.v[0] = v0; f.v[1] = v1; f.v[2] = v2; f.v[3] = v3;
    f.m0 = 0;
    f.e[0] = e0; f.e[1] = e1;
    f.m5c = 0;
    f.flag = 0;
    f.e[2] = e2; f.e[3] = e3;
    c.faces.push_back(f);
    return (int)c.faces.size() - 1;
}

// @ 0x478580
void NavMap::clear()
{
    NavVert v;
    v.flag = 0;
    c.verts.resize(0, v);
    c.edges.resize_default(0);
    c.faces.resize_default(0);
}

// @ 0x4785e0
void NavMap::build(HeightField* hf, int step_)
{
    step = step_;
    clear();
    int nx = (hf->w - 0x18) / step;
    int ny = (hf->h - 0x18) / step;
    for (int iy = 0; iy < ny; iy++) {
        for (int ix = 0; ix < nx; ix++) {
            NavVert v;
            v.flag = 0;
            v.pos = vector4((float)(step * ix + 0xc) * hf->scale, (float)(step * iy + 0xc) * hf->scale,
                            hf->cols[step * ix + 0xc][step * iy + 0xc].x + 0.4f);
            c.verts.push_back(v);
        }
    }
    for (int r = 0; r < ny; r++)
        for (int k = 0; k < nx - 1; k++)
            add_edge(r * nx + k, r * nx + k + 1);
    int nh = (nx - 1) * ny;
    for (int r = 0; r < ny - 1; r++)
        for (int k = 0; k < nx; k++)
            add_edge(r * nx + k, r * nx + k + nx);
    for (int i = 0; i < nx - 1; i++)
        for (int j = 0; j < ny - 1; j++)
            add_face(4, i + j * nx, i + 1 + j * nx, i + 1 + (j + 1) * nx, i + (j + 1) * nx,
                     j * (nx - 1) + i, nh + j * nx + i + 1, (j + 1) * (nx - 1) + i, nh + j * nx + i);
    finish();
}

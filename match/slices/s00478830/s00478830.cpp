// Slice s00478830: a world-space cell index (2D grid of per-cell id lists) and the object that owns it:
// grid construction, insertion of a box, per-face bounding boxes of a navigation map, wind-object spawn and
// the scene-record reader. Names provisional.
#include <vector>
#include <list>
#include <new>
#include "rgl.h"
using rgl::vector4;

// The engine's text stream (vtable slots verified in s00415c80). Only the slots used here matter.
struct tstring {
    int pos;            // +0
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring();                                  // 0x42bbd0
    tstring(const char* s);                     // 0x42bbe0
    ~tstring();                                 // 0x42bc40
};

struct stream_base {
    virtual void reset();                       // slot 0, 0x415220
    virtual void s1();
    virtual void s2();
    virtual void s3();
    virtual void s4();
    virtual void s5();
    virtual void s6();
    virtual void s7();
    virtual void s8();
    virtual void s9();
    virtual void s10();
    virtual void s11();
    virtual void s12();
    virtual stream_base& r_stream(stream_base* o);      // +0x34  slot 13, 0x415cf0
    virtual void s14();
    virtual void s15();
    virtual void s16();
    virtual void s17();
    virtual void s18();
    virtual stream_base& r_int(int* v);                 // +0x4c  slot 19, 0x415930
    virtual void s20();
    virtual void s21();
    virtual void s22();
    virtual bool at_end();                              // +0x5c  slot 23, 0x415250
};

// A 0x30-byte embedded stream object (two of them live in the scene: ground-paint and terrain blocks).
struct SubStream : stream_base {
    char body[0x30 - 4];
};

// 0x1c8-byte placement record (s00462e20 Placed / s004375b0 Agent; the vector instantiation is
// std::vector<Agent>, so the record keeps that name).
struct Agent {
    char pad0[0x10];
    int m10;                                    // +0x10
    char pad14[0x1c8 - 0x14];
    void read(stream_base* s, int version);     // 0x4631c0
};

// Cell of the grid (RecP of s00431960, 0x34 bytes): vector<int> at +0, list<int> at +0x10, vector<int> at +0x20.
struct Cell {
    std::vector<int> a;                         // +0
    std::list<int> l;                           // +0x10
    int pad1c;
    std::vector<int> b;                         // +0x20 (ids of the objects overlapping the cell)
    int tail;                                   // +0x30
    ~Cell();                                    // 0x431ab0
};

// Two 0x14-byte free lists (std::list<int> + a 0x400 capacity + count).
struct Pool {
    std::list<int> l;
    int cap;
    int n;
    Pool() : cap(0x400), n(0) {}
};

// Viewport/light parameter block; layout and ctor verified in s0046b6e0 (0x46b940).
struct frustum { char b[0x120]; frustum(); };   // 0x41ef60
struct mat4 { vector4 s, f, u, o; };
mat4* identity_pos(mat4* out);                  // 0x40cd00
struct ViewParams {
    float f0, f4, f8;
    int fc;
    char b10;
    float f14, f18, f1c;
    int f20;
    float f24, f28;
    char b2c;
    int f30;
    frustum fr;
    char pad154[0x1a4 - 0x154];
    mat4 m1a4;
    mat4 m1e4;
    ViewParams();                               // 0x46b940
};

struct Wind {
    char body[0x7c];
    Wind();                                     // 0x461480
};

// 100-byte navigation-map record: eight vertex indices at +0x28, their count at +0x48.
struct NavVert { float x, y, z, a, b, c; };     // 0x18
struct NavRec {
    char pad28[0x28];
    int idx[8];                                 // +0x28
    int n;                                      // +0x48
    char pad4c[0x64 - 0x4c];
};
// The engine's record vector: size() is 0 when the array is empty (null first pointer).
struct RecVec {
    int pad0;
    char* first;                                // +0x04
    char* last;                                 // +0x08
    char* end;                                  // +0x0c
    int size() const { return first ? (int)((last - first) / 100) : 0; }
};
struct NavMesh {
    char pad1c[0x1c];
    NavVert* verts;                             // +0x1c
    char pad20[8];
    RecVec recs;                                // +0x28
};

// 2D cell grid (provisional name) and the scene/terrain object that owns it.
struct CellGrid {
    int dim[2];                                 // +0 (log2 of the cell counts)
    int n[2];                                   // +8
    float lo[2];                                // +0x10
    float hi[2];                                // +0x18
    float step[2];                              // +0x20
    Cell** cells;                               // +0x28 (cells[x] -> array of n[1] cells)
    Pool pool0;                                 // +0x2c
    Pool pool1;                                 // +0x40
    std::vector<int> v54;                       // +0x54
    int f64;                                    // +0x64
    vector4 m68;                                // +0x68
    int f78;                                    // +0x78
    std::vector<int> v7c;                       // +0x7c
    std::vector<int> v8c;                       // +0x8c
    int f9c;                                    // +0x9c
    std::vector<int> va0;                       // +0xa0
    int fb0;                                    // +0xb0
    int fb4;                                    // +0xb4 (nav mesh the grid is built from)
    std::vector<Wind*> winds;                   // +0xb8
    std::vector<int> vc8;                       // +0xc8
    ViewParams view;                            // +0xd8
    char gap2fc[0x10];
    vector4 m30c;                               // +0x30c
    vector4 m31c;                               // +0x31c
    std::vector<int> v32c;                      // +0x32c
    int f33c;                                   // +0x33c (not touched by the ctor)
    int f340;                                   // +0x340
    void init(const float* lo_, const float* hi_, int dimx, int dimy);      // 0x478830
    void locate(const float* p, int* out) const;                            // 0x41ed00
    void insert(int id, const float* lo_, const float* hi_);                // 0x478a10
    NavMesh* navmesh() const { return (NavMesh*)fb4; }
    CellGrid(const float* lo_, const float* hi_, const int* dimx, const int* dimy, const int* v);  // 0x478ad0
    void build(NavMesh* m);                     // 0x478c50
    Wind* spawn_wind();                         // 0x478f30
};

// A tiny owner with a std::vector<int> at +0x30 (unrelated to the grid).
struct IdHolder {
    char pad30[0x30];
    std::vector<int> v;                         // +0x30
    void add(int x);                            // 0x478fb0
};

struct Logger {
    virtual void l0(); virtual void l1(); virtual void l2(); virtual void l3();
    virtual void l4(); virtual void l5(); virtual void l6(); virtual void l7();
    virtual void l8(); virtual void l9();
    virtual Logger* get(const char* msg);       // +0x28
    virtual void put(const tstring* s);         // +0x2c
};
struct LogHub {
    virtual void h0(); virtual void h1(); virtual void h2(); virtual void h3();
    virtual void h4(); virtual void h5(); virtual void h6(); virtual void h7();
    virtual void h8(); virtual void h9();
    virtual Logger* get(const char* msg);       // +0x28
};

// std::vector<Agent> whose one-argument resize stays out of line (0x477ea0 in the original).
struct AgentVector : std::vector<Agent> {
    __declspec(noinline) void resize(unsigned n);   // 0x477ea0
};

// The scene container: records at +0x104, two embedded streams at +0x414/+0x444, a name at +0x20
// and a loaded flag at +0x78.
struct Scene {
    char pad20[0x20];
    tstring name;                               // +0x20
    char pad30[0x78 - 0x30];
    char loaded;                                // +0x78
    char pad79[0x104 - 0x79];
    AgentVector props;                          // +0x104 (first at +0x108)
    char pad114[0x414 - 0x114];
    SubStream s414;                             // +0x414
    SubStream s444;                             // +0x444
    void load(stream_base* s, int id);          // 0x479000
};

// @ 0x477ea0 (resize kept out of line: the original calls this helper from the scene loader)
__declspec(noinline) void AgentVector::resize(unsigned n)
{
    std::vector<Agent>::resize(n);
}

extern LogHub g_log;                            // 0x5a9f60

// @ 0x478830
void CellGrid::init(const float* lo_, const float* hi_, int dimx, int dimy)
{
    lo[0] = lo_[0];
    lo[1] = lo_[1];
    hi[0] = hi_[0];
    hi[1] = hi_[1];
    dim[0] = dimx;
    dim[1] = dimy;
    for (int k = 0; k < 2; k++) {
        n[k] = 1;
        for (int i = 0; i < dim[k]; i++)
            n[k] *= 2;
    }
    cells = new Cell*[n[0]];
    for (int i = 0; i < n[0]; i++) {
        cells[i] = new Cell[n[1]];
        for (int j = 0; j < n[1]; j++)
            cells[i][j].tail = 0;
    }
    if (n[0] == 1) {
        lo[0] = 1e12f;
        hi[0] = 1e12f;
        step[0] = 1.0f;
    } else if (n[0] == 2) {
        lo[0] = (hi[0] + lo[0]) * 0.5f;
        hi[0] = 1e12f;
        step[0] = 1e12f - lo[0];
    } else {
        step[0] = (hi[0] - lo[0]) / (float)(n[0] - 2);
    }
    if (n[1] == 1) {
        lo[1] = 1e12f;
        hi[1] = 1e12f;
        step[1] = 1.0f;
    } else if (n[1] == 2) {
        lo[1] = (lo[1] + hi[1]) * 0.5f;
        hi[1] = 1e12f;
        step[1] = 1e12f - lo[1];
    } else {
        step[1] = (hi[1] - lo[1]) / (float)(n[1] - 2);
    }
}

// @ 0x478a10
void CellGrid::insert(int id, const float* lo_, const float* hi_)
{
    int a[2], b[2];
    locate(lo_, a);
    locate(hi_, b);
    for (int x = a[0]; x <= b[0]; x++)
        for (int y = a[1]; y <= b[1]; y++)
            cells[x][y].b.push_back(id);
}

// @ 0x478ad0
CellGrid::CellGrid(const float* lo_, const float* hi_, const int* dimx, const int* dimy, const int* v)
    : pool0(), pool1(), v54(), v7c(), v8c(), va0(), winds(), vc8(), view(), v32c()
{
    f340 = *v;
    f9c = 1;
    init(lo_, hi_, *dimx, *dimy);
    m30c = vector4(1.0f, 0.0f, 0.0f);
    fb0 = 0;
    fb4 = 0;
    f64 = 0;
    m31c = vector4(0.0f, 0.0f, 0.0f);
    m68 = vector4(0.0f, 0.0f, 0.0f);
    f78 = 0;
}

// @ 0x478c50
void CellGrid::build(NavMesh* m)
{
    fb4 = (int)m;
    if (m == 0)
        return;
    for (unsigned i = 0; i < navmesh()->recs.size(); i++) {
        NavRec* r = (NavRec*)(navmesh()->recs.first + i * 100);
        NavVert* verts = navmesh()->verts;
        float lo[3] = { 1e6f, 1e6f, 0.0f };
        float maxy = -1e6f;
        float hi[3] = { -1e6f, maxy, 0.0f };
        int n = r->n;
        int j;
        for (j = 0; j < n; j++) {
            NavVert* p = &verts[r->idx[j]];
            if (lo[0] > p->x) lo[0] = p->x;
            if (lo[1] > p->y) lo[1] = p->y;
            if (hi[0] < p->x) hi[0] = p->x;
            if (maxy < p->y) maxy = p->y;
        }
        hi[1] = maxy;
        insert((int)i, lo, hi);
    }
}

// @ 0x478f30
Wind* CellGrid::spawn_wind()
{
    Wind* w = new Wind();
    winds.push_back(w);
    return w;
}

// @ 0x478fb0
void IdHolder::add(int x)
{
    v.push_back(x);
}

// @ 0x479000
void Scene::load(stream_base* s, int id)
{
    int version = 0, count = 0;
    int v;
    s->r_int(&v);
    if (v == -717) {
        s->r_int(&version);
        s->r_int(&count);
    } else {
        count = v;
    }
    int n = (int)props.size();
    props.resize(n + count);
    for (int i = 0; i < count; i++) {
        props[n + i].read(s, version);
        props[n + i].m10 = id;
    }
    if (version >= 4) {
        s444.reset();
        s->r_stream(&s444);
    }
    if (!s->at_end()) {
        int magic;
        s->r_int(&magic);
        if (magic == -0xb52e5a) {
            s414.reset();
            s->r_stream(&s414);
        } else {
            LogHub* hub = &g_log;
            hub->get("WARNING unable to find ground_paint_saved_magic for sco file")->put(&name);
        }
    }
    loaded = 1;
}

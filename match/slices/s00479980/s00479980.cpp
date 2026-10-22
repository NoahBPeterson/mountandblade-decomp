// Slice s00479980: the nav-mesh store. `build` (0x479980) turns every cell of a navigation map (0x88-byte
// records: vertex-id list +0, translation +0x54, scale +0x64, created face +0x84) into a scene face object
// (0xb4 bytes, vtable 0x54a588): the vertices are looked up in the map's shared vertex array (0x24-byte
// records), every vertex normal is accumulated from all cell translations and normalized, and the face is
// pushed into the store's object list and posted into the scene. `spawn` (0x479d20) builds an emitter from
// the definition table, `spawn_bodies` (0x479da0) instantiates one collision shape per spawn record, and
// `rebuild_nav` (0x479e20) builds the store for the active level (bounding box either from the play area or
// from the level body's collision shape). Names are provisional; only layouts confirmed by the disassembly.
#include <math.h>
#include <new>
#include "rgl.h"

using rgl::vector4;
using rgl::matrix;

// ---- the game object (mission / level host): the active level slot index, the play-area rectangle and the
// registry pointer (offsets from s0043db60 / s0048a420). ----------------------------------------------------
struct Registry { void reg(void* o); };                                             // 0x424170
struct NavStore;
struct SpawnRec;
struct Mission {
    NavStore* nav;                      // +0x000000 (the active nav store)
    char pad1[0x5df4 - 4];
    int cur;                            // +0x005df4 (index of the active level slot)
    char pad2[0x653c - 0x5df8];
    float min_x, min_y, max_x, max_y;   // +0x00653c (play-area rectangle)
    char pad3[0xc3b0dc - 0x654c];
    Registry* registry;                 // +0xc3b0dc
    void spawn_bodies(SpawnRec* r);
    void rebuild_nav(int which, int);
};
extern Mission* g_game;                 // 0x5e2930 (pointer to the mission object)

// ---- scene shapes: rgl objects with a virtual frame/flags/bbox interface (slots from s00408110) -----------
// vtable 0x54a588: slot 8 = set_frame, slot 10 = bbox_min, slot 11 = bbox_max, slot 13 = set_flags.
struct shape_base {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    shape_base() : m4(-1), m8(8), mc(-1), min_x(100000000), min_y(100000000),
                   max_x(-100000000), max_y(-100000000) {}
    virtual ~shape_base();
    virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4();
    virtual void s5(); virtual void s6(); virtual void s7();
    virtual void set_frame(const matrix* m);            // slot 8  (+0x20)
    virtual void s9();
    virtual void bbox_min(float* out);                  // slot 10 (+0x28)
    virtual void bbox_max(float* out);                  // slot 11 (+0x2c)
    virtual void s12();
    virtual void set_flags(unsigned f);                 // slot 13 (+0x34)
};
struct face : shape_base {                              // 0xb4 bytes, vtable 0x54a588
    vector4 m20, m30, m40, m50;         // +0x20
    vector4 v[5];                       // +0x60
    int count;                          // +0xb0
    face() {}
};

// ---- resources -------------------------------------------------------------------------------------------
struct res_shape;                                       // object returned by resmgr::get_object60 (0x418c10)
struct res_shape {
    char pad[0x18];
    shape_base* shape;                                  // +0x18
    shape_base* clone_shape();                          // 0x4285e0
};
struct tstring { int pos; char* buf; int cap; int len; };
struct res_mgr {
    res_shape* get_object60(const tstring& name);       // 0x418c10
};
extern res_mgr g_resmgr;                                // 0x5d6fa0
void mb_error(const char* fmt, ...);                    // 0x42cc80

// ---- std::vector look-alike (VC7.1: 4-byte allocator pad, first, last, end); size() null-guards first ---
template<class T> struct vec {
    char pad[4];
    T* first, *last, *end;
    unsigned size() const { return first == 0 ? 0 : (unsigned)(last - first); }
    unsigned capacity() const { return first == 0 ? 0 : (unsigned)(end - first); }
    T& operator[](unsigned i) const { return first[i]; }
};

// ---- the navigation map (NavCore, 0x68 bytes, ctor 0x478150 ICF-folded with other 4-vector types) -------
struct face;
struct NavVert {                                        // 0x24 bytes
    vector4 p;                                          // +0x00
    vector4 n;                                          // +0x10 (accumulated normal)
    int f20;                                            // +0x20
};
struct NavCell {                                        // 0x88 bytes
    int npts;                                           // +0x00
    int ids[0x14];                                      // +0x04 (vertex indices)
    vector4 trans;                                      // +0x54
    float scale;                                        // +0x64
    int f68;                                            // +0x68
    char pad6c[0x84 - 0x6c];
    face* obj;                                          // +0x84
};
struct NavCore {
    vec<NavVert> verts;                                 // +0x00
    vec<NavCell> cells;                                 // +0x10
    vec<int> v20;                                       // +0x20
    vec<int> v30;                                       // +0x30
    int m40;                                            // +0x40
    vector4 e44;                                        // +0x44
    vector4 f54;                                        // +0x54
    int m64;                                            // +0x64
    NavCore();                                          // 0x478150
    ~NavCore();                                         // 0x41f5d0 (four_vecs::~four_vecs)
};

// ---- the nav store (NavStore, 0x344 bytes, ctor 0x478ad0): an object list at +0x54 and the active map ----
struct ObjVec {
    char pad0[4];
    void** first, **last, **end;
    unsigned size() const { return first == 0 ? 0 : (unsigned)(last - first); }
    unsigned capacity() const { return first == 0 ? 0 : (unsigned)(end - first); }
    void insert_n(void** where, unsigned count, void* const& val);   // 0x41f720
};
struct NavStore {
    char pad0[0x54];
    ObjVec items;                                       // +0x54
    char pad1[0xb0 - 0x54 - sizeof(ObjVec)];
    NavCore* core;                                      // +0xb0
    char pad2[0x344 - 0xb4];
    NavStore(const vector4* lo, const vector4* hi, const int* lod1, const int* lod2, const int* param);
                                                        // 0x478ad0
    void build(NavCore* map);
    void post(void* o);                                 // 0x425240
    void reg(void* o);                                  // 0x424170
};

struct mat3 { vector4 s, f, u; };
mat3* __cdecl identity3(mat3* out);                     // 0x40cd60
matrix* __cdecl identity_pos(matrix* out);              // 0x40cd00

// @ 0x479980
void NavStore::build(NavCore* map)
{
    if (core != 0)
        delete core;
    core = map;

    for (unsigned k = 0; k < map->cells.size(); k++) {
        face* f = new face;
        NavCell* c = &map->cells[k];
        f->count = c->npts;
        for (int i = 0; i < c->npts; i++)
            f->v[i] = map->verts[c->ids[i]].p;
        f->m4 = k;
        f->m8 = c->f68;
        mat3 rot;
        identity3(&rot);
        matrix m;
        m.s = rot.s;
        m.f = rot.f;
        m.u = rot.u;
        m.o = vector4(0.0f, 0.0f, 0.0f);
        f->set_frame(&m);
        if (items.size() < items.capacity()) {
            *items.last = f;
            items.last++;
        } else {
            items.insert_n(items.last, 1, f);
        }
        post(f);
        c->obj = f;
    }

    for (unsigned i = 0; i < map->verts.size(); i++)
        map->verts[i].n = vector4(0.0f, 0.0f, 0.0f);

    for (unsigned i = 0; i < map->cells.size(); i++) {
        NavCell* c = &map->cells[i];
        for (int j = 0; j < c->npts; j++) {
            NavVert* v = &map->verts[c->ids[j]];
            float s = c->scale;
            v->n.x += c->trans.x * s;
            v->n.y += c->trans.y * s;
            v->n.z += c->trans.z * s;
        }
    }

    for (unsigned i = 0; i < map->verts.size(); i++) {
        vector4& n = map->verts[i].n;
        float len = (float)sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
        if (len <= 1e-10f) {
            n.x = 0.0f;
            n.y = 1.0f;
            n.z = 0.0f;
        } else {
            float inv = 1.0f / len;
            n.x *= inv;
            n.y *= inv;
            n.z *= inv;
        }
    }
}

// ---- particle emitters: the definition table (defs +0, count +4) and one 0x1bc-byte emitter -------------
struct Emit {
    char pad0[0x10];
    int f10, f14, f18;                                  // +0x10
    char pad1c[0x1b8 - 0x1c];
    int owner;                                          // +0x1b8
    Emit(const int* o) { f10 = 0; f14 = 0; f18 = 0; owner = *o; init(); }
    void init();                                        // 0x420b40
};
struct EmitDef;
struct EmitTable {
    EmitDef* defs;                                      // +0
    int count;                                          // +4
    void setup(Emit* e, int idx, const vector4* pos);   // 0x4424c0
    Emit* spawn(int* idx, const vector4* pos, const int* owner);
};

// @ 0x479d20
Emit* EmitTable::spawn(int* idx, const vector4* pos, const int* owner)
{
    if (*idx < 0 || *idx >= count)
        return 0;
    Emit* e = new Emit(owner);
    setup(e, *idx, pos);
    return e;
}

// ---- spawn records (0xa0 bytes each) --------------------------------------------------------------------
struct TypeElem {
    char pad[0x20];
    res_shape* res;                                     // +0x20
    char pad2[0x48 - 0x24];
};
struct PType {
    char pad[0x18];
    int count;                                          // +0x18
    TypeElem* arr;                                      // +0x1c
};
extern PType* g_types;                                  // 0x5e28f8

struct SpawnEntry {
    int type;                                           // +0x00
    char pad4[0x10];
    matrix m;                                           // +0x14
    char pad54[0x10];
    int counter;                                        // +0x64
    char pad68[0xa0 - 0x68];
};
struct SpawnRec {
    int count;                                          // +0x00
    SpawnEntry entries[1];                              // +0x04
};

// @ 0x479da0
void Mission::spawn_bodies(SpawnRec* r)
{
    for (int i = 0; i < r->count; i++) {
        SpawnEntry* e = &r->entries[i];
        PType* ty = &g_types[e->type];
        int c = e->counter % ty->count;
        e->counter = c;
        res_shape* rs = ty->arr[c].res;
        if (rs) {
            shape_base* o = rs->clone_shape();
            o->set_frame(&e->m);
            o->set_flags(0x200);
            registry->reg(o);
        }
    }
}

// ---- level slots (0x494 bytes) and the nav-store construction -------------------------------------------
extern int g_nav_param;                                 // 0x59132c

// @ 0x479e20
void Mission::rebuild_nav(int which, int)
{
    char* slots = *(char**)((char*)g_game + 0x13a50);
    char* sl = slots + which * 0x494;
    char* cs = slots + cur * 0x494;
    NavCore* core_ = 0;
    res_shape* r = 0;
    vector4 hi, lo;
    if (*(int*)(sl + 0x30) & 0x100) {
        hi.x = max_x + 1.0f;
        hi.y = max_y + 1.0f;
        lo.x = min_x - 1.0f;
        lo.y = min_y - 1.0f;
    } else {
        core_ = new NavCore;
        r = g_resmgr.get_object60(*(tstring*)(cs + 0x44));
        if (r == 0) {
            tstring* nm = (tstring*)(cs + 0x44);
            const char* s = nm->buf ? nm->buf : (const char*)&nm->buf;
            mb_error("Unable to find body %s", s);
        }
        r->shape->bbox_max(&hi.x);
        hi.x += 1.0f;
        hi.y += 1.0f;
        r->shape->bbox_min(&lo.x);
        lo.x -= 1.0f;
        lo.y -= 1.0f;
    }
    double dx = (double)(hi.x - lo.x) * (1.0f / 3.0f);
    double lx = (double)(float)log(dx) * 1.4426950408889634;
    int lod1 = (int)(ceil(lx) + (lx > 0.0 ? 1e-8f : -1e-8f));
    double dy = (double)(hi.y - lo.y) * (1.0f / 3.0f);
    double ly = (double)(float)log(dy) * 1.4426950408889634;
    int lod2 = (int)(ceil(ly) + (ly > 0.0 ? 1e-8f : -1e-8f));
    NavStore* s = new NavStore(&lo, &hi, &lod1, &lod2, &g_nav_param);
    nav = s;
    if ((*(int*)(sl + 0x30) & 0x100) == 0) {
        s->build(core_);
        shape_base* o = r->clone_shape();
        matrix m;
        identity_pos(&m);
        o->set_frame(&m);
        s->reg(o);
    }
}

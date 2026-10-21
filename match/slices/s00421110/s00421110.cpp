// Slice s00421110: scene/world containers around 0x421110-0x421a50: the entity-list world class, the mesh entity
// class (vtable 0x54b460), a cell grid and the effect entity (vtable 0x54b5d0). Names and layouts provisional.
#include "fx.h"

using rgl::vector4;

struct View;
struct Ent;

// 0x20-byte clip plane
struct Plane {
    char pad[0x20];
    int test(const vector4* p, float radius);                   // 0x41e470
};
// frustum (embedded at Obj+0x30): [0] valid, [4] plane count, planes from +8
struct Frustum {
    char valid; char pad0[3];
    int nplanes;
    Plane planes[8];
    void reset();                                               // 0x41e4f0
};
// an attached object in the world (a camera/view): frustum at +0x30, ambient at +0x17c, owner at +0x1c4
struct World;
struct View {
    char pad0[0x30];
    Frustum frustum;                                            // +0x30
    char pad1[0x17c - 0x30 - sizeof(Frustum)];
    vector4 ambient;                                            // +0x17c
    char pad2[0x1c4 - 0x18c];
    World* world;                                               // +0x1c4
};
extern Scene g_scene;                                           // 0x5fe898

// world entity (vtable slot 5 = radius, slot 9 = notify, slots 13/14 = bounds)
struct CellRef;
struct Ent {
    char pad0[4];
    int id;                                                     // +8
    char pad1[8];
    CellRef* refs;                                              // +0x14  cells this entity is registered in
    char pad2[0x58 - 0x18];
    vector4 s, f, u, o;                                         // +0x58 matrix
    char pad3[0xc4 - 0x98];
    float cx, cy, cz;                                           // +0xc4 local centre
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual float radius();                                     // slot 5
    virtual void v6();
    virtual void v7();
    virtual void v8();
    virtual void notify(View* view, bool* flag);                // slot 9
    virtual void v10();
    virtual void v11();
    virtual void v12();
    virtual const float* max_pos();                             // slot 13
    virtual const float* min_pos();                             // slot 14
};

struct T1;
struct T2;
struct CellNode { CellNode* next; CellNode* prev; Ent* obj; };           // 12 bytes
struct CellRef { CellRef* next; CellNode* node; int x, y; };             // 16 bytes
struct Cell { char pad[0x30]; CellNode* head; };               // 0x34 bytes
template <class T> struct Pool {                                           // grow: 0x4206a0 (CellNode), 0x420720 (CellRef)
    char pad[0x10];
    T* freelist;
    void grow();
    T* alloc() { if (!freelist) grow(); T* n = freelist; freelist = n->next; return n; }
};
struct World {
    char pad0[0x28];
    Cell** cols;                                                // +0x28
    Pool<CellNode> nodes;                                             // +0x2c
    Pool<CellRef> refs;                                               // +0x40 (free list at +0x50)
    char pad1[0x64 - 0x40 - sizeof(Pool<CellRef>)];
    T2* p64;                                                    // +0x64
    vector4 ambient;                                            // +0x68
    char pad2[0x7c - 0x78];
    std::vector<Ent*> ents;                                     // +0x7c (first +0x80)
    std::vector<Ent*> ents2;                                    // +0x8c (first +0x90)
    char pad3[0xb0 - 0x9c];
    T1* p_b0;                                                   // +0xb0
    int f_b4;                                                   // +0xb4
    char pad4[0x104 - 0xb8];
    bool loaded;                                                // +0x104
    void attach(View* view, int rem, int div);                   // 0x421110
    void reset();                                               // 0x421390
    void clear_ents();                                          // 0x4207e0
    void clear_cells();                                         // 0x420a70
    void insert(Ent* o, int x, int y);                          // 0x421560
    void insert_obj(Ent* o);                                    // 0x421600
    void to_cell(const float* p, int* cell);                    // 0x41ed00
};
struct CollisionEntity { rgl::string type_name(); };            // 0x421500
struct Stub {                                                   // default virtual stubs (0x421520 / 0x421540 / 0x421550)
    void get_vec(float* out, int);
    void nop(int, int);
    float zero(int, int);
};

// @ 0x421110
void World::attach(View* view, int rem, int div)
{
    view->world = this;
    view->ambient = ambient;
    view->frustum.reset();
    g_scene.register_obj((Obj*)view);
    struct Flag { bool v; char pad[7]; } flag;
    flag.v = false;
    for (std::vector<Ent*>::iterator it = ents.begin(); it < ents.end(); ++it) {
        Ent* e = *it;
        if (e->id % div == rem) {
            vector4 c;
            c.x = e->f.x * e->cy + e->u.x * e->cz + e->s.x * e->cx + e->o.x;
            c.y = e->s.y * e->cx + e->f.y * e->cy + e->u.y * e->cz + e->o.y;
            c.z = e->s.z * e->cx + e->f.z * e->cy + e->u.z * e->cz + e->o.z;
            float r = e->radius();
            int n = view->frustum.nplanes;
            int ok = 1;
            for (int i = 0; i < n; ++i) {
                if (!ok) break;
                ok = view->frustum.planes[i].test(&c, r);
            }
            if (!ok) continue;
            (*it)->notify(view, &flag.v);
        }
    }
    for (std::vector<Ent*>::iterator it = ents2.begin(); it < ents2.end(); ++it) {
        Ent* e = *it;
        if (e->id % div == rem) {
            vector4 c;
            c.x = e->f.x * e->cy + e->u.x * e->cz + e->s.x * e->cx + e->o.x;
            c.y = e->f.y * e->cy + e->u.y * e->cz + e->s.y * e->cx + e->o.y;
            c.z = e->f.z * e->cy + e->u.z * e->cz + e->s.z * e->cx + e->o.z;
            float r = e->radius();
            int n = view->frustum.nplanes;
            int ok = 1;
            for (int i = 0; i < n; ++i) {
                if (!ok) break;
                ok = view->frustum.planes[i].test(&c, r);
            }
            if (!ok) continue;
            (*it)->notify(view, &flag.v);
        }
    }
}

// ---- World: the spatial container (cells, pools, entity lists) -------------------------------------
struct T1 { ~T1(); };                                           // 0x41f5d0
struct T2 { ~T2(); };                                           // 0x41ecc0
// @ 0x421390
void World::reset()
{
    clear_ents();
    clear_cells();
    if (p_b0) {
        delete p_b0;
        p_b0 = 0;
    }
    f_b4 = 0;
    if (p64) {
        delete p64;
        p64 = 0;
    }
    loaded = false;
}

// @ 0x4213f0
MeshEntity::~MeshEntity()
{
    if (p98) delete p98;
    cleanup_psys();
    if (p138) { delete p138; p138 = 0; }
    if (p13c) { delete p13c; p13c = 0; }
}
// 0x4214e0 is the compiler-generated scalar deleting destructor ??_GMeshEntity (calls ~MeshEntity above, then
// operator delete if bit 0 of the flag is set); it is emitted from the virtual dtor and has no source of its own.
// @ 0x4214c0
rgl::string MeshEntity::type_name() { return rgl::string("mesh_entity"); }

// @ 0x421500
rgl::string CollisionEntity::type_name() { return rgl::string("collision_entity"); }
// @ 0x421520
void Stub::get_vec(float* out, int)
{
    out[0] = 0.0f; out[1] = 0.0f; out[2] = 0.0f;
}
// @ 0x421540
void Stub::nop(int, int) {}
// @ 0x421550
float Stub::zero(int, int) { return 0.0f; }

// @ 0x421560
void World::insert(Ent* o, int x, int y)
{
    CellNode* n = nodes.alloc();
    n->obj = o;
    n->prev = 0;
    n->next = cols[x][y].head;
    if (cols[x][y].head) cols[x][y].head->prev = n;
    cols[x][y].head = n;
    CellRef* r = refs.alloc();
    r->node = n;
    r->x = x;
    r->y = y;
    r->next = o->refs;
    o->refs = r;
}

// @ 0x421600
void World::insert_obj(Ent* o)
{
    int hi[2];
    int lo[2];
    to_cell(o->min_pos(), lo);
    to_cell(o->max_pos(), hi);
    for (int x = lo[0]; x <= hi[0]; ++x)
        for (int y = lo[1]; y <= hi[1]; ++y)
            insert(o, x, y);
}

// @ 0x421680
FxDef::~FxDef()
{
    clear_defs();
    if (count && elems) delete[] elems;
    if (env) {
        delete[] env;
        delete[] ints;
    }
}

// @ 0x421720
StrategicEntity::StrategicEntity()
    : f2a0(0), t2a8(g_time[0]), f2b0(0), t2b8(g_time[0]), f2c0(0)
{
    f168 = 0;
    matrix64 tmp;
    pos = *identity(&tmp);
    f160 = 0.5f;
    f164 = 0.5f;
    last_pos = rgl::vector4(-10000.0f, -10000.0f, -10000.0f);
    f16c = 0;
    fx = 0;
    f274 = 0;
    p288 = 0;
    f28c = 0;
    f290 = 0;
    f15c = 0;
    radius = 0.0f;
    f170 = 0;
    f31c = 0;
    f278 = -100000000.0f;
    f298 = -1;
    f294 = 1.0f;
    f2ec = 1;
    thresh = 2.0f;
}

// @ 0x421870
rgl::string StrategicEntity::type_name() { return rgl::string("strategic_entity"); }

// @ 0x421890
float StrategicEntity::get_radius() { return radius; }

// @ 0x4218a0
void StrategicEntity::set_color(int c)
{
    for (std::vector<Psys*>::iterator it = psys.begin(); it < psys.end(); ++it)
        (*it)->color = c;
    if (fx) fx->set_color(c);
}

// @ 0x4218e0
__declspec(noinline) void FxDef::set_color(int c)
{
    for (std::vector<Psys*>::iterator it = psys.begin(); it < psys.end(); ++it)
        (*it)->color = c;
    for (std::vector<PsysGroup*>::iterator g = groups.begin(); g < groups.end(); ++g) {
        PsysGroup* grp = *g;
        for (int i = 0; i < grp->nlists; ++i)
            grp->lists[i].set_color(c);
    }
    for (int i = 0; i < count; ++i)
        elems[i].set_color(c);
}

// @ 0x4219c0
void FxElem::set_color(int c)
{
    for (std::vector<Psys*>::iterator it = psys.begin(); it < psys.end(); ++it)
        (*it)->color = c;
    for (std::vector<PsysGroup*>::iterator g = groups.begin(); g < groups.end(); ++g) {
        PsysGroup* grp = *g;
        for (int i = 0; i < grp->nlists; ++i)
            grp->lists[i].set_color(c);
    }
}

// @ 0x421a50
int StrategicEntity::find_emitter(int tag)
{
    int r = -1;
    for (unsigned i = 0; i < emitters.size(); ++i)
        if (emitters[i]->tag == tag)
            r = i;
    return r;
}

// Stand-in definition of PsysList::set_color (0x421990): fx.h declares PsysList::set_color without a body because the
// original callers (FxElem/FxDef::set_color) were compiled without seeing it; a same-TU body would let cl keep ecx
// live across the call. This copy has the identical body under another class name.
struct PsysListBody : std::vector<Psys*> {
    void set_color(const int& c);
};
// @ 0x421990
void PsysListBody::set_color(const int& c)
{
    for (std::vector<Psys*>::iterator it = begin(); it < end(); ++it)
        (*it)->color = c;
}

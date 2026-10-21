// Slice s0044f3c0: rgl::mesh::transform (matrix by value), Font::draw_glyph (one glyph quad into a mesh),
// Agent::make_ranged_mesh (builds the carried-weapon display mesh), Mission::add_placed (appends a 0x1c8-byte
// scene-object record), vector<E56>::push_back and PhysSet::create_world (Newton world + material defaults).
// Names provisional.
#include <vector>
#include "s0044f3c0.h"


// ---- Font (see s0043db60) ---------------------------------------------------------------------------------
struct Glyph {                                       // 0x1c bytes: quad uv rectangle + metrics
    float u0, v0, u1, v1;
    float xoff, yoff, xadv;
};
struct GlyphNode { GlyphNode *left, *parent, *right; Glyph val; };
struct GlyphMap {                                    // std::map<int, glyph> (comparator, head, size)
    int pad0[2];
    GlyphNode* head;                                 // +8
    void lookup(GlyphNode** out, const int* key);    // 0x43b210 (ret 8)
};
struct vec2f { float x, y; };
struct Font {
    char pad0[0x28];
    Glyph dflt;                                      // +0x28 (returned for unknown characters)
    GlyphMap map;                                    // +0x44
    char pad50[0x6c - 0x50];
    Glyph ascii[0x200];                              // +0x6c
    void draw_glyph(int ch, const vector4* pos, const vec2f* scale, mesh* m, int arg);   // 0x44f5d0 (ret 0x14)
    __forceinline const Glyph* get(int c)
    {
        if (c >= 0x200) {
            GlyphNode* it;
            map.lookup(&it, &c);
            if (it != map.head)
                return &it->val;
            return &dflt;
        }
        return &ascii[c];
    }
};

// @ 0x44f3c0
void rgl::mesh::transform(mat4 m)
{
    ensure_local();
    unsigned i;
    for (i = 0; i < v70.size(); i++) {
        vector4 ta, tb;
        point20* p = &(&*v70.begin())[i];
        ta.x = m.u.x * p->a.z + m.f.x * p->a.y + m.s.x * p->a.x + m.o.x;
        ta.y = m.u.y * p->a.z + m.f.y * p->a.y + m.s.y * p->a.x + m.o.y;
        ta.z = m.u.z * p->a.z + m.f.z * p->a.y + m.s.z * p->a.x + m.o.z;
        p->a = ta;
        p = &(&*v70.begin())[i];
        tb.x = m.u.x * p->b.z + m.f.x * p->b.y + m.s.x * p->b.x + m.o.x;
        tb.y = p->b.x * m.s.y + m.u.y * p->b.z + m.f.y * p->b.y + m.o.y;
        tb.z = p->b.x * m.s.z + m.u.z * p->b.z + m.f.z * p->b.y + m.o.z;
        p->b = tb;
    }
    for (i = 0; i < v50.size(); i++) {
        vector4 tb;
        vertex60* v = &(&*v50.begin())[i];
        tb.x = m.u.x * v->normal.z + m.f.x * v->normal.y + m.s.x * v->normal.x;
        tb.y = m.u.y * v->normal.z + m.f.y * v->normal.y + m.s.y * v->normal.x;
        tb.z = m.u.z * v->normal.z + m.f.z * v->normal.y + m.s.z * v->normal.x;
        v->normal = tb;
    }
    slots[0].f28 = 1;
    finish_load();
}

// @ 0x44f5d0
void Font::draw_glyph(int ch, const vector4* pos, const vec2f* scale, mesh* m, int arg)
{
    const Glyph* g = get(ch);
    float dv = g->v1 - g->v0;
    vector4 a = *pos;
    float h = dv * scale->y;
    a.y = (g->yoff * scale->y - h) + a.y;
    float w = (g->u1 - g->u0) * scale->x;
    vector4 b(a.x + w, a.y + h, a.z);
    m->add_quad(&a, &b, &g->u0, &g->u1, arg);
}

// item flag field at ItemKind::flags: (flags & 0xff000) selects a visual class (module-system itp_* bits 12..19)
enum { ITEM_CLASS_MASK = 0xff000, ITEM_CLASS_A = 0x10000, ITEM_CLASS_B = 0x20000, ITEM_CLASS_C = 0x30000 };

static inline tstring item_mesh_name(ItemKind* items, const ItemRef& r)
{
    if (r.item >= 0 && r.item < g_item_count)
        return items[r.item].mesh_name_for((int)r.m4 >> 24);
    return tstring("Error: Invalid item");
}
static __forceinline mesh* build_ranged(int i, ItemRef r)
{
    mesh* result = 0;
    if (i >= 0) {
        mesh* obj = g_resmgr.get_object(item_mesh_name(g_items, r));
        float s = (float)g_items[i].f20c * 0.01f;
        if (obj) {
            result = obj->clone();
            vector4 d;
            d.y = -s;
            d.x = 0.0f;
            d.z = 0.0f;
            result->translate(&d);
            mat4 rot;
            rgl::identity(&rot);
            float angle = 1.5707964f;
            rot.rotate_su(&angle);
            if ((char)g_items[i].type_flags == 10) {
                if ((g_items[i].flags & ITEM_CLASS_MASK) == ITEM_CLASS_C) {
                    angle = frand() * 0.4f + 0.5f;
                    rot.rotate_sf(&angle);
                }
                if ((g_items[i].flags & ITEM_CLASS_MASK) == ITEM_CLASS_B) {
                    angle = frand() * 0.4f + 3.2415926f;
                    rot.rotate_sf(&angle);
                }
            }
            result->transform(rot);
        }
    }
    return result;
}
// @ 0x44f6a0
mesh* Agent::make_ranged_mesh()
{
    ItemKind* items = g_items;
    int i = -1;
    int a = slots_a.item;
    if (a >= 0) {
        i = a;
        if ((char)items[a].type_flags != 10)
            i = slots_b.item;
    }
    return build_ranged(i, ranged_ref());
}

// ---- scene-object records (0x1c8 bytes, see s004375b0 "Agent" / s00435490 PlacedObj) ------------------------
extern int g_tick_lo;                           // 0x5e2958 (g_time[2], 1e-5 s ticks)
extern int g_tick_hi;                           // 0x5e295c
struct IfaceB {
    int f4, f8, fc;
    IfaceB() {}
    virtual ~IfaceB() {}
    virtual void slot1() {}
    virtual void slot2() {}
};
struct Quad {
    int a, b, c, d;
    Quad() : a(2), c(g_tick_lo), d(g_tick_hi) {}
};
struct PlacedObj : IfaceB {                     // vtable 0x590b84 (??_7Rec1c8@@6B@)
    int m10, m14;                               // +0x10
    matrix a18, a58, a98, ad8, a118;            // +0x18 .. +0x157
    vector4 v158;                               // +0x158
    int m168;                                   // +0x168
    float f16c;                                 // +0x16c
    Quad q170;                                  // +0x170
    int m180, m184, m188, m18c;                 // +0x180 (m188 = kind, m1a0 = slot number)
    tstring name;                               // +0x190
    int m1a0, m1a4, m1a8, m1ac, m1b0, m1b4, m1b8, m1bc, m1c0, m1c4;   // +0x1a0
    PlacedObj() { reset(); }                    // 0x437830 (inlined here)
    void reset();                               // 0x4352f0
};

struct PropVec : std::vector<PlacedObj> {
    void push_back(const PlacedObj& x);         // 0x440b80
};
struct Mission {
    char pad0[0x6688];
    PropVec props;                              // +0x6688 (first at +0x668c)
    void spawn_prop(int* idx, int* id);         // 0x44c210
    void init_placed(PlacedObj* o, const int* kind, const int* id, const int* value, const matrix* m);   // 0x435860 (ret 0x14, ecx unused)
    int add_placed(const int* kind, const int* id, const int* value, const matrix* m, int* spawn_id);   // 0x44f870
};

// @ 0x44f870
int Mission::add_placed(const int* kind, const int* id, const int* value, const matrix* m, int* spawn_id)
{
    int cnt = 0;
    int prev = -1;
    int idx = (int)props.size();
    {
        PlacedObj rec;
        props.push_back(rec);
        int off = idx * 0x1c8;
        init_placed((PlacedObj*)((char*)&props[0] + off), kind, id, value, m);
        if (*kind == 7 || *kind == 6)
            ((PlacedObj*)((char*)&props[0] + off))->m10 = 2;
        if (*kind == 1) {
            cnt = 0;
            prev = -1;
            for (int k = 0; k < 0x28; k++) {
                if (prev >= cnt)
                    break;
                prev = cnt;
                for (unsigned j = 0; j < props.size(); j++) {
                    if (props[j].m188 == 1 && props[j].m1a0 == cnt)
                        cnt++;
                }
            }
            if (cnt < 0x40)
                ((PlacedObj*)((char*)&props[0] + off))->m1a0 = cnt;
        }
        spawn_prop(&idx, spawn_id);
    }
    return idx;
}

// ---- Newton physics set -------------------------------------------------------------------------------------
struct NewtonWorld; struct NewtonBody; struct NewtonCollision; struct NewtonRagDoll; struct NewtonRagDollBone;
extern "C" {
__declspec(dllimport) NewtonWorld* __cdecl NewtonCreate(void* alloc, void* free_);                              // 0x54921c
__declspec(dllimport) void __cdecl NewtonSetSolverModel(const NewtonWorld* w, int model);                       // 0x549220
__declspec(dllimport) void __cdecl NewtonSetWorldSize(const NewtonWorld* w, const float* mn, const float* mx);   // 0x549224
__declspec(dllimport) int __cdecl NewtonMaterialGetDefaultGroupID(const NewtonWorld* w);                         // 0x549228
__declspec(dllimport) void __cdecl NewtonMaterialSetDefaultSoftness(const NewtonWorld* w, int a, int b, float v);   // 0x54922c
__declspec(dllimport) void __cdecl NewtonMaterialSetDefaultElasticity(const NewtonWorld* w, int a, int b, float v); // 0x549230
__declspec(dllimport) void __cdecl NewtonMaterialSetDefaultFriction(const NewtonWorld* w, int a, int b, float s, float k);   // 0x549234
}

struct PhysRec {                                // 0x38 bytes (see s004413e0)
    int f0;
    const NewtonWorld* world;                   // +4
    const NewtonRagDoll* ragdoll;               // +8
    std::vector<const NewtonRagDollBone*> bones;   // +0xc
    std::vector<const NewtonBody*> bodies;      // +0x1c
    const NewtonCollision* collision;           // +0x2c
    const NewtonBody* body;                     // +0x30
    int material;                               // +0x34
};
struct E56 {                                    // the same 0x38-byte record as the vector sees it (see s0043edf0)
    int a, b, c;
    std::vector<int> v1;                        // +0x0c
    std::vector<int> v2;                        // +0x1c
    int d, e, f;
    E56(const E56& o);                          // 0x43f510
};
// the freshly created record as the original builds it: the two vectors are zeroed in place and no cleanup follows
struct RawVec { int pad; int *first, *last, *end; RawVec() : first(0), last(0), end(0) {} };
struct PhysRecInit {
    int f0;
    const NewtonWorld* world;
    const NewtonRagDoll* ragdoll;
    RawVec bones;
    RawVec bodies;
    const NewtonCollision* collision;
    const NewtonBody* body;
    int material;
};
struct RecVec : std::vector<PhysRec> {
    void push_back(const PhysRec& x);           // 0x44fa40
};
struct PhysSet {
    RecVec recs;                                // +0 (first at +4)
    char pad10[0x3c - 0x10];
    int nworlds;                                // +0x3c
    unsigned create_world();                    // 0x44fad0
};

// @ 0x44fa40
template void std::vector<E56>::push_back(const E56&);

// @ 0x44fad0
unsigned PhysSet::create_world()
{
    unsigned idx = 0;
    PhysRec* p = &recs[0];
    while (idx < recs.size() && p->world != 0) {
        idx++;
        p++;
    }
    NewtonWorld* w = NewtonCreate(0, 0);
    NewtonSetSolverModel(w, 6);
    {
        vector4 mx, mn;
        mn.x = -1000.0f; mn.y = -1000.0f; mn.z = -100.0f;
        mx.x = 1000.0f; mx.y = 1000.0f; mx.z = 1000.0f;
        NewtonSetWorldSize(w, &mn.x, &mx.x);
    }
    if (idx == recs.size()) {
        PhysRecInit r;
        r.ragdoll = 0;
        r.collision = 0;
        r.body = 0;
        r.world = w;
        recs.push_back(*(const PhysRec*)&r);
    } else {
        recs[idx].world = w;
    }
    recs[idx].material = NewtonMaterialGetDefaultGroupID(w);
    recs[idx].f0 = 0;
    NewtonMaterialSetDefaultSoftness(w, recs[idx].material, recs[idx].material, 0.1f);
    NewtonMaterialSetDefaultElasticity(w, recs[idx].material, recs[idx].material, 0.5f);
    NewtonMaterialSetDefaultFriction(w, recs[idx].material, recs[idx].material, 1.1f, 0.9f);
    nworlds++;
    return idx;
}

// @ 0x4520b0 is in slice s00451710

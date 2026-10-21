// Slice s0044c210: Mission::spawn_prop (0x44c210), instantiates one scene-prop record (a 0x1c8-byte record of the
// scene's prop table) as a world entity: picks the mesh list / collision resource by prop kind (item mesh, table
// entry, scene-object record, entry arrow, mesh passage, ...), creates the entity, attaches meshes and a collision
// shape, and plays the "created" sound events. Names are provisional; layouts reuse the neighbouring slices
// (world.h of s004291c0, s00434ad0.h, s00435490.h, s0043ac40.cpp, s004333c0.h, s00433cb0.h, s00432a50.cpp).
#include <stdlib.h>
#include <vector>
#include "rgl.h"
using rgl::vector4;
using rgl::matrix;

// inline vector helpers (the original inlines rgl's operator* with its `const float&` scale; the scale stays in memory)
static inline vector4 vmul(const float& s, const vector4& v) { return vector4(s * v.x, s * v.y, s * v.z); }
static inline vector4 vadd(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }

// rgl string with a read cursor (16 bytes)
struct tstring {
    int pos; char* buf; int cap; int len;
    tstring();                                  // 0x42bbd0
    tstring(const char* s);                     // 0x42bbe0
    ~tstring();                                 // 0x42bc40
    void operator=(const tstring& o);           // 0x42bc50
    void assign(const char* s);                 // 0x42bcb0
    bool ne(const char* s) const;               // 0x42bdb0 (!equals)
};

struct mesh {                                    // render mesh object (mesh.h of s00413320)
    char pad0[0x4c];
    int f4c;                                     // +0x4c (mirrored flag)
    char pad50[0x10c - 0x50];
    int f10c;                                    // +0x10c (collision mask)
    mesh* clone();                               // 0x41c6d0
};

// std::vector<mesh*> whose assignment and push_back stay out of line (pointer vectors fold with vector<int>)
struct MeshVec : std::vector<mesh*> {
    MeshVec& operator=(const std::vector<mesh*>& o);    // 0x43f0a0
    void push_back(mesh* const& x);                     // 0x4119d0
};

struct shape {                                   // collision shape (shape_base of world.h)
    virtual ~shape();                            // 0
    virtual int shape_type();                    // 1
    virtual void v2(); virtual void v3(); virtual void v4(); virtual void v5(); virtual void v6();
    virtual void set_scale(const vector4* s);    // 7  (+0x1c)
    virtual void set_frame(const matrix* m);     // 8  (+0x20)
    virtual void v9(); virtual void v10(); virtual void v11();
    virtual void set_flags(int f);               // 12 (+0x30)
};
struct shape_base {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    shape_base() : m4(-1), m8(8), mc(-1), min_x(100000000), min_y(100000000), max_x(-100000000), max_y(-100000000) {}
    virtual ~shape_base();
};
struct sphere_shape : shape_base {               // vtable 0x54a4f0, size 0x44
    vector4 v20, v30;
    float radius;
    sphere_shape(vector4 center, const float* r);          // 0x4332a0
};
struct capsule : shape_base {                    // vtable 0x54a540, size 0x64
    vector4 v20, v30, v40, v50;
    float radius;
    capsule(float r, const vector4* a, const vector4* b);  // 0x433310
};

struct res_shape {                               // resource holding a collision shape (world.h)
    char pad[0x18];
    shape* sh;                                   // +0x18
    shape* clone_shape();                        // 0x4285e0
    shape* copy_shape();                         // 0x439100 (body_res::copy_shape)
};
struct resmgr {                                  // global at 0x5d6fa0
    std::vector<mesh*> find_meshes(const tstring* prefix);   // 0x41b960
    res_shape* get_object60(const tstring* name);            // 0x418c10
};
extern resmgr g_resmgr;                          // 0x5d6fa0

struct Entity {                                  // vtable 0x54b460 and derived (world.h of s004291c0)
    virtual ~Entity();                           // 0
    virtual void v1(); virtual void v2(); virtual void v3(); virtual void v4(); virtual void v5();
    virtual void v6(); virtual void v7(); virtual void v8(); virtual void v9(); virtual void v10();
    virtual void v11(); virtual void v12(); virtual void v13(); virtual void v14(); virtual void v15();
    virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19(); virtual void v20();
    virtual void v21();
    virtual void set_frame(const matrix* m, const vector4* scale);   // 22 (+0x58)
    virtual void v23(); virtual void v24(); virtual void v25(); virtual void v26(); virtual void v27();
    virtual void set_shape(shape* s);            // 28 (+0x70)
    int pad4, pad8;
    int id;                                      // +0x0c
    int kind;                                    // +0x10
    char pad14[0x58 - 0x14];
    matrix local;                                // +0x58
    shape* shp;                                  // +0x98
    char pad9c[0x118 - 0x9c];
    std::vector<mesh*> meshes;                   // +0x118 (first at +0x11c)
    char pad128[0x15c - 0x128];
    float f15c;                                  // +0x15c
    char pad160[0x170 - 0x160];
    float f170;                                  // +0x170
    char pad174[0x19c - 0x174];
    vector4 v19c;                                // +0x19c
    vector4 v1ac;                                // +0x1ac
    char pad1bc[0x1d4 - 0x1bc];
    char f1d4;                                   // +0x1d4
    void add_mesh(mesh* m);                      // 0x442110
};

// lights / collision registry / entity factory (all the same World object, see s0043ac40, s00423ed0, s00428240)
struct World {
    Entity* create_entity(int type_id);          // 0x428440
    void light_at(vector4 p, vector4 n, vector4* out_dir, vector4* out_col, unsigned flags, float scale);   // 0x43ac40
    void reg(shape* s);                          // 0x424170
};

// plays a list of sound events (count + array of 0x30-byte records)
struct SoundSet {
    int count; char* items;
    void play(int id);                           // 0x48add0
};

struct ItemKind {                                // 0x238-byte item record (s00434ad0.h)
    char pad0[0x1c8];
    unsigned type_flags;                         // +0x1c8
    char pad1cc[0x228 - 0x1cc];
    SoundSet snd;                                // +0x228
    char pad230[0x238 - 0x230];
    tstring mesh_name_for(int bit);              // 0x433bf0
    mesh* mesh_for(unsigned __int64 cls, int bit);   // 0x433cb0
    bool is_ranged();                            // 0x433d90
    bool is_melee();                             // 0x433dc0
};
extern ItemKind* g_items;                        // 0x5e29d0
extern int g_item_count;                         // 0x5e29d4

struct Rec20Entry {                              // 0x48-byte named entry
    tstring name; char pad10[0x20 - 0x10];
    res_shape* shape;                            // +0x20
    char pad24[0x48 - 0x24];
};
struct Rec20 {                                   // 0x20-byte record (table at 0x5e28f8)
    char pad0[0x18];
    int count;                                   // +0x18
    Rec20Entry* entries;                         // +0x1c
};
extern Rec20* g_tbl20;                           // 0x5e28f8
struct Rec78 {                                   // 0x78-byte scene-object record (table at 0x5e29dc)
    char pad0[0x10];
    unsigned flags;                              // +0x10 (low byte kind; bit 0x4000 / 0x8000 / 0x2000 ...)
    char pad14[0x40 - 0x14];
    std::vector<mesh*> meshes;                   // +0x40
    res_shape* shape;                            // +0x50
    char pad54[0x6c - 0x54];
    SoundSet snd;                                // +0x6c
    char pad74[4];
};
extern Rec78* g_tbl78;                           // 0x5e29dc
extern int g_tbl78_count;                        // 0x5e29e0
extern int g_flag5a9fb0;                         // 0x5a9fb0

struct GameData {                                // pointer at 0x5e2930
    char pad0[0x1a1f0];
    int f1a1f0, f1a1f4, f1a1f8, f1a1fc, f1a200, f1a204;
    void set_matrix(const matrix* m);            // 0x435650
};
extern GameData* g_game;                         // 0x5e2930
extern __int64 g_a70448;                         // 0xa70448
extern __int64 g_a70450;                         // 0xa70450

int find_item(const tstring* name);              // 0x434ad0 (cdecl)
int find_in_table20(const tstring* name);        // 0x435260
int find_in_table78(const tstring* name);        // 0x4352a0

struct ItemRef {                                 // item index + packed word (s00434ad0.h)
    int item;
    unsigned m4;
    tstring mesh_name();                         // 0x434b20
};

struct PropRec {                                 // 0x1c8-byte scene-prop record
    char pad0[0x18];
    matrix frame;                                // +0x18
    matrix frame2;                               // +0x58
    matrix frame0;                               // +0x98 (initial frame)
    char padd8[0x158 - 0xd8];
    vector4 scale;                               // +0x158
    char pad168[0x188 - 0x168];
    int type;                                    // +0x188 (0 object, 1 entry, 2/6 item, 3, 4 table, 5 passage, 7 shield)
    int ref;                                     // +0x18c (resolved item / object index)
    tstring name;                                // +0x190
    int param;                                   // +0x1a0
    char pad1a4[0x1b4 - 0x1a4];
    Entity* ent;                                 // +0x1b4
    Entity* ent2;                                // +0x1b8
    float f1bc, f1c0;
    char pad1c4[0x1c8 - 0x1c4];
    void update_1c0();                           // 0x461200
};

// std::vector<Entity*> as VC7.1 compiles push_back: inline fast path, out-of-line _Insert_n (shared with vector<int>)
struct EntVec {
    int alloc; Entity** first; Entity** last; Entity** end;
    unsigned size() const { return first == 0 ? 0 : last - first; }
    unsigned capacity() const { return first == 0 ? 0 : end - first; }
    void _Insert_n(Entity** where, unsigned n, Entity* const& v);       // 0x43e7b0
    void push_back(Entity* const& x)
    {
        if (size() < capacity()) {
            *last = x;
            ++last;
        } else {
            _Insert_n(last, 1, x);
        }
    }
};

struct Agent {                                   // 0x5928-byte agent record
    char pad0[0x88];
    int f88;                                     // +0x88
    char pad8c[0x5928 - 0x8c];
};

struct Mission {
    World* world;                                // +0
    char pad4[0x18 - 4];
    std::vector<Agent*> agents;                  // +0x18 (16 agents per page)
    char pad28[0x63c0 - 0x28];
    EntVec ents;                                 // +0x63c0
    char pad63d0[0x668c - 0x63d0];
    PropRec* props;                              // +0x668c
    Agent& agent(unsigned id) { return agents[id >> 4][id & 0xf]; }
    void spawn_prop(int* prop_idx, int* agent_id);          // 0x44c210
};

// mesh of an item for the prop display class, null for an invalid item index (bounds check inlined)
static inline mesh* item_mesh(int item, int packed)
{
    if (item < 0 || item >= g_item_count)
        return 0;
    return g_items[item].mesh_for(0x3000000000000000ui64, packed >> 24);
}

#include <stddef.h>
#define OFFSET_CHECK(T, m, off) typedef char T##_##m##_at_##off[(offsetof(T, m) == (off)) ? 1 : -1]
OFFSET_CHECK(PropRec, frame, 0x18);
OFFSET_CHECK(PropRec, frame2, 0x58);
OFFSET_CHECK(PropRec, frame0, 0x98);
OFFSET_CHECK(PropRec, scale, 0x158);
OFFSET_CHECK(PropRec, type, 0x188);
OFFSET_CHECK(PropRec, name, 0x190);
OFFSET_CHECK(PropRec, param, 0x1a0);
OFFSET_CHECK(PropRec, ent, 0x1b4);
OFFSET_CHECK(PropRec, f1c0, 0x1c0);
OFFSET_CHECK(Mission, agents, 0x18);
OFFSET_CHECK(Mission, ents, 0x63c0);
OFFSET_CHECK(Mission, props, 0x668c);
OFFSET_CHECK(Entity, meshes, 0x118);
OFFSET_CHECK(Entity, v19c, 0x19c);
OFFSET_CHECK(Entity, v1ac, 0x1ac);
OFFSET_CHECK(Rec78, meshes, 0x40);
OFFSET_CHECK(Rec78, snd, 0x6c);
OFFSET_CHECK(Rec20Entry, shape, 0x20);

// @ 0x44c210
void Mission::spawn_prop(int* idx, int* id)
{
    PropRec* r = &props[*idx];
    r->frame = r->frame0;
    MeshVec meshes;
    res_shape* shape_res = 0;
    tstring name;
    tstring s2("0");
    int tbl = -1;
    int t = r->type;
    if (t == 2) {
        int ii = find_item(&r->name);
        name = g_items[ii].mesh_name_for(0);
    } else if (t == 6) {
        int item = find_item(&r->name);
        int param = r->param;
        unsigned k = g_items[item].type_flags & 0xff;
        ItemRef ref;
        ref.item = item;
        ref.m4 = param;
        mesh* m;
        if (!((k == 2 || k == 3 || k == 4 || k == 6 || k == 5 || k == 0x12) && (m = item_mesh(ref.item, (int)ref.m4)) != 0))
            name = ref.mesh_name();
        else
            meshes.push_back(m);
    } else if (t == 4) {
        int ti = find_in_table20(&r->name);
        r->param = r->param % g_tbl20[ti].count;
        name = g_tbl20[ti].entries[r->param].name;
        res_shape* rs = g_tbl20[ti].entries[r->param].shape;
        if (rs)
            shape_res = rs;
    } else if (t == 7) {
        int ri = r->ref;
        name = g_items[ri].mesh_name_for(r->param);
        s2.assign("boshield");
    } else if (t == 0) {
        tbl = find_in_table78(&r->name);
        if (tbl < 0)
            tbl = r->ref;
        if (tbl < 0 || tbl >= g_tbl78_count) {
            tbl = -1;
        } else {
            Rec78* rec = &g_tbl78[tbl];
            if (g_flag5a9fb0 != 0 || !(rec->flags & 0x4000))
                meshes = rec->meshes;
            shape_res = rec->shape;
        }
    } else if (t != 3) {
        if (t == 1) {
            if (g_flag5a9fb0)
                name.assign("entry_arrow");
            else
                name.assign("");
        } else if (t == 5) {
            if (g_flag5a9fb0)
                name.assign("_mesh_passage");
            else
                name.assign("");
            s2.assign("bo_mesh_passage");
        }
    }
after:
    if (name.len > 0)
        meshes = g_resmgr.find_meshes(&name);
    if (s2.ne("0") && s2.len != 0)
        shape_res = g_resmgr.get_object60(&s2);

    if (r->type == 7) {
        r->ref = find_item(&r->name);
        Entity* e = world->create_entity(7);
        mesh** first = meshes._Myfirst;
        for (unsigned i = 0; first != 0 && i < (unsigned)(meshes._Mylast - first); i++)
            e->add_mesh(first[i]->clone());
        if (shape_res)
            e->shp = shape_res->clone_shape();
        e->local = r->frame0;
        e->kind = 0xc;
        e->id = *idx;
        r->ent2 = e;
        e->f170 = 2.0f;
        float a = (float)(rand() % 0x3dc9) * (float)(1.0 / 15817);
        float b = (float)(rand() % 0x3dc9) * (float)(2.0 / 15817) + 1.0f;
        float c = (float)(rand() % 0x3dc9) * (float)(2.0 / 15817);
        e->v19c = vadd(vadd(vmul(c, e->local.f), vector4(0.0f, 0.0f, b)), vmul(a, e->local.u));
        float a2 = (float)(rand() % 0x3dc9) * (float)(9.0 / 15817);
        float c2 = (float)(rand() % 0x3dc9) * (float)(4.0 / 15817);
        float d = (float)(rand() % 0x3dc9) * (float)(6.0 / 15817);
        e->f15c = 0.4f;
        e->f1d4 = 0;
        e->v1ac = vadd(vadd(vmul(d, r->frame.s), vmul(c2, r->frame.f)), vmul(a2, r->frame.u));
    } else {
        Entity* e = world->create_entity(9);
        for (unsigned i = 0; i < meshes.size(); i++) {
            mesh* m = meshes[i]->clone();
            if (r->type == 4) {
                vector4 dir, col;
                world->light_at(r->frame.o, vector4(0.0f, 0.0f, 1.0f), &dir, &col, 1, 1.0f);
            }
            if (r->type == 0 && tbl >= 0 && tbl < g_tbl78_count && (g_tbl78[tbl].flags & 0x12000))
                m->f10c = 0xffff;
            else
                m->f10c = 0x1ffff;
            if (r->scale.z * r->scale.y * r->scale.x < 0.0f)
                m->f4c = 1;
            e->add_mesh(m);
        }
        if (shape_res) {
            if (r->type == 3 || r->type == 5 ||
                (r->type == 0 && tbl >= 0 && ((g_tbl78[tbl].flags & 0xff) == 5 || (g_tbl78[tbl].flags & 0xff) == 10)))
                e->shp = shape_res->copy_shape();
            shape* sh = shape_res->clone_shape();
            int v = 8;
            if (r->type == 0 && tbl >= 0) {
                unsigned fl = g_tbl78[tbl].flags;
                if ((char)fl == 8)
                    v = 0x10c;
                else if (fl & 0x4000)
                    v = 0xc;
                else if (fl & 0x8000)
                    v = 0;
            }
            sh->set_flags(v);
            sh->set_scale(&r->scale);
            sh->set_frame(&r->frame);
            e->set_shape(sh);
        } else if (r->type == 2) {
            r->ref = find_item(&r->name);
            unsigned k = g_items[r->ref].type_flags & 0xff;
            shape* sh;
            if (k == 1) {
                sh = (shape*)new capsule(0.4f, &vector4(0.0f, -0.3f, 1.05f), &vector4(0.0f, 0.67f, 1.15f));
            } else if (k == 0xb) {
                sh = (shape*)new sphere_shape(vector4(0.0f, 0.0f, 0.0f), &(const float&)0.3f);
            } else {
                goto done_shape;
            }
            if (sh) {
                sh->set_scale(&r->scale);
                sh->set_frame(&r->frame);
                world->reg(sh);
            }
        } else if (r->type == 6) {
            unsigned k = g_items[r->ref].type_flags & 0xff;
            ItemKind* ik = &g_items[r->ref];
            shape* sh;
            if (k == 1) {
                sh = (shape*)new capsule(0.4f, &vector4(0.0f, -0.3f, 1.05f), &vector4(0.0f, 0.67f, 1.15f));
            } else if (k == 0xb) {
                sh = (shape*)new sphere_shape(vector4(0.0f, 0.0f, 0.0f), &(const float&)0.3f);
            } else if (ik->is_melee() || ik->is_ranged() || k == 7 || k == 5 || k == 6 || k == 0x12) {
                sh = (shape*)new sphere_shape(vector4(0.0f, 0.0f, 0.0f), &(const float&)0.5f);
            } else {
                goto done_shape;
            }
            if (sh) {
                sh->set_scale(&r->scale);
                e->shp = sh;
            }
        }
done_shape:
        e->set_frame(&r->frame, &r->scale);
        r->frame2 = r->frame;
        e->kind = 0xc;
        e->id = *idx;
        r->ent = e;
        ents.push_back(e);
    }

    int ty = r->type;
    if (ty == 0) {
        int ri = r->ref;
        if (ri >= 0 && ri < g_tbl78_count && g_tbl78[ri].snd.count > 0) {
            SoundSet* ss = &g_tbl78[ri].snd;
            g_game->set_matrix(&r->frame);
            g_game->f1a1f0 = *idx;
            Entity* en = r->ent;
            if (en->meshes.size() > 0)
                g_game->f1a200 = (int)en->meshes[0];
            else
                g_game->f1a200 = 0;
            g_game->f1a1fc = 0;
            g_a70448 = *idx;
            ss->play(-0x28);
        }
    } else if (ty == 2 || ty == 6 || ty == 7) {
        int ri = r->ref;
        if (ri >= 0 && ri < g_item_count && g_items[ri].snd.count > 0) {
            SoundSet* ss = &g_items[ri].snd;
            int who = *id;
            int aid = -1;
            if (who >= 0)
                aid = agent(who).f88;
            g_game->f1a1f4 = who;
            g_game->f1a1f8 = aid;
            g_game->f1a204 = -1000;
            Entity* en = r->ent;
            if (en && en->meshes.size() > 0) {
                g_game->f1a200 = (int)en->meshes[0];
            } else {
                Entity* e2 = r->ent2;
                if (e2 && e2->meshes.size() > 0)
                    g_game->f1a200 = (int)e2->meshes[0];
                else
                    g_game->f1a200 = 0;
            }
            g_game->f1a1fc = 0;
            g_a70448 = *id;
            g_a70450 = aid;
            ss->play(-0x32);
        }
    }
    r->update_1c0();
}

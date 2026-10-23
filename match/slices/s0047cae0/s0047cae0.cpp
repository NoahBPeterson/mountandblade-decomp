// Slice s0047cae0: the scene-object ("entity") assembly / lighting side of the carried-item holder.
// All five functions are members of the big module object (`terrain_obj`, the Mission/terrain/SceneGen
// object; its tail members sit at +0x845e784.. so the class is one giant global).  Layout evidence:
//   +0x00            SceneRegistry* (== *this; create_entity + light_mesh live on it, see s0047a0c0)
//   +0x270 / +0x274  map extents (floats; see s004661e0 terrain: +0x278 = h, +0x27c = w)
//   +0x6688          std::vector of 0x1c8-byte placed-object records (Mission::add_placed, s0044f3c0)
//   +0xc3b0dc        SceneRegistry* again (same object; s00480db0 terrain.h World* world)
//   +0x845e784       tstring name (see s004c46d0 world_obj::~world_obj: name at +0x845e784)
//   +0x845e794       entity* arr[9] (3x3 grid by position sign, filled here)
//   +0x845e7b8       std::vector<mesh*> (VC7.1: allocator then _Myfirst at +0x845e7bc)
//   cl 13.10 folds the 3x3 index arithmetic into the displacement 0xb07df7 (12 * 0xb07df7 == 0x845e794).
// Callees (declared, defined in their own slices): resmgr::find_meshes (0x41b960), vector assign
// (0x43f0a0), World::create_entity (0x428440), mesh::clone (0x41c6d0), mesh::set_color (0x47af30),
// mesh::translate (0x44f240), ground::snap (0x47c9b0), mesh::finish_load (0x419300),
// Scene::light_mesh (0x47b7b0), aabb reset/extend/update (0x416e40/0x416ef0/0x416f70).
#include <vector>
#include <new>
#include <stddef.h>
#include <string.h>
#include "tstring.h"
#include "mesh.h"

using rgl::mesh;
using rgl::mat4;
using rgl::vector4;
using rgl::aabb;
using rgl::res_t;
using rgl::g_resmgr;

// push_back with the inlined fast path (VC7.1 std::vector<mesh*>)
struct MeshVec : std::vector<mesh*> {
    MeshVec& operator=(const std::vector<mesh*>& o);               // 0x43f0a0
    void insert_n(iterator where, size_t n, mesh* const& v);       // 0x410610
};

// ---- the scene entity (vtable 0x54b460, size 0x15c, ctor 0x41fba0; see s004291c0 world.h) ---------------
struct ebox {                                    // 0x30-byte collision box (reset 0x416e40, extend 0x416ef0, update 0x416f70)
    vector4 min, max, center;
    void reset();                                // 0x416e40
    void extend(const vector4* p);               // 0x416ef0
    void update();                               // 0x416f70
};
struct entity {
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3();
    virtual void refresh();                      // vtable +0x10 (no arguments)
    virtual float s5();                          // vtable +0x14 (returns a float)
    virtual void s6(vector4* p);                 // vtable +0x18
    char pad04[0x2c - 4];
    res_t* res;                                  // 0x2c
    char pad30[0x58 - 0x30];
    mat4 frame;                                  // 0x58
    char pad98[0xc4 - 0x98];
    vector4 origin;                              // 0xc4
    ebox box;                                    // 0xd4
    vector4 f104;                                // 0x104
    char pad114[0x118 - 0x114];
    MeshVec meshes;                              // 0x118 (_Myfirst at +0x11c)
    char pad128[0x140 - 0x128];
    char f140;                                   // 0x140 dirty flag
};

// The scene object at terrain_obj+0xc3b0dc (entity factory + light pass; == *terrain_obj).
struct SceneRegistry {
    entity* create_entity(int id);               // 0x428440
    void light_mesh(mesh* m, const mat4* frame, unsigned flags);   // 0x47b7b0
};

extern int g_use_shaders;                        // 0x5e28fc

// 0x1c8-byte placed-object record (Mission props vector, s0044f3c0)
struct PropRec {
    char pad0[0x188];
    int kind;                                    // 0x188 (compared to 4)
    char pad18c[0x1b4 - 0x18c];
    entity* ent;                                 // 0x1b4
    char pad1b8[0x1c8 - 0x1b8];
};
struct PropVec : std::vector<PropRec> {
};

struct terrain_obj {
    SceneRegistry* registry;                     // 0x00 (== *this)
    char pad04[0x270 - 4];
    float map_x;                                 // 0x270
    float map_y;                                 // 0x274
    char pad278[0x6688 - 0x278];
    PropVec props;                               // 0x6688 (_Myfirst at +0x668c)
    char pad6698[0xc3b0dc - 0x6698];
    SceneRegistry* registry2;                    // 0xc3b0dc
    char pad_e0[0x845e784 - 0xc3b0e0];
    tstring name;                                // 0x845e784
    entity* arr[9];                              // 0x845e794
    MeshVec meshes;                              // 0x845e7b8

    void build_entity();                         // 0x47cae0
    void snap(mesh* m);                          // 0x47c9b0 (ground::snap)
    void light_entities(entity** list, int count);   // 0x47ce00
    void light_props();                          // 0x47ced0
};

// @ 0x47cae0
void terrain_obj::build_entity()
{
    if (name.len < 4)
        return;
    meshes = g_resmgr.find_meshes(&name);

    vector4 center;
    center.x = map_x - 10.0f;
    center.y = map_y - 10.0f;
    center.z = (map_x + map_y) * 0.5f;
    vector4 tv(10.0f, 10.0f, 0.0f);

    entity** e = arr;
    int i = 3;
    do {
        int j = 3;
        do {
            *e = registry2->create_entity(9);
            (*e)->box.reset();
            e++;
        } while (--j);
    } while (--i);

    for (unsigned n = 0; n < meshes.size(); n++) {
        mesh* m = meshes[n]->clone();
        m->set_color(center);
        m->translate(&tv);
        snap(m);
        m->finish_load();

        int ix;
        if (m->v22c.center.x < 0.0f)
            ix = 0;
        else if (m->v22c.center.x > map_x)
            ix = 2;
        else
            ix = 1;
        int iy;
        if (m->v22c.center.y < 0.0f)
            iy = 0;
        else if (m->v22c.center.y > map_y)
            iy = 2;
        else
            iy = 1;
        entity** ep = &arr[ix * 3 + iy];

        m->f214 = 10;
        entity* e0 = *ep;
        e0->meshes.push_back((mesh*)m);
        e0->refresh();

        if (g_use_shaders == 0)
            registry2->light_mesh(m, &(*ep)->frame, 1);
        else if ((((char*)m->res)[0x1c] & 0x40) != 0)
            registry2->light_mesh(m, &(*ep)->frame, 1);

        (*ep)->box.extend((const vector4*)((char*)m + 0x22c));
        (*ep)->box.extend((const vector4*)((char*)m + 0x23c));
    }

    entity** q = arr;
    int k = 3;
    do {
        int j = 3;
        do {
            (*q)->box.update();
            (*q)->origin = *(vector4*)((char*)*q + 0xf4);
            (*q)->s6(&(*q)->f104);
            q++;
        } while (--j);
    } while (--k);
}

// ---- 0x47ce00 / 0x47ced0 helpers -------------------------------------------------------------------------
struct mat3 { float m[12]; };                    // 0x30 (3x3 + pad), filled by identity3
mat3* __cdecl identity3(mat3* out);              // 0x40cd60

// material flags of a mesh's resource record: res+0x1c bit 6 (per-mesh light pass) and res->mat->f20 bit 7
static inline bool mesh_wants_light(res_t* r)
{
    return ((r->mat->f20 >> 7) & 1) != 0 && (*(unsigned*)((char*)r + 0x1c) & 0x700) != 0x300;
}

// the light pass takes a 0x40-byte matrix; the origin sits at +0x34 in this variant
// @ 0x47ce00
// Light every mesh of every entity in `list` whose material wants the per-mesh pass, with an identity frame.
void terrain_obj::light_entities(entity** list, int count)
{
    for (int i = 0; i < count; i++) {
        entity* e = list[i];
        res_t* r = e->res;
        bool k = (((unsigned)r->mat->f20 >> 7) & 1) != 0;
        if (k && (*(unsigned*)((char*)r + 0x1c) & 0x700) != 0x300) {
            vector4 o(0.0f, 0.0f, 0.0f);
            mat3 ident;
            mat4 frame;
            memcpy(&frame, identity3(&ident), 0x30);
            frame.o = o;
            registry->light_mesh((mesh*)e, &frame, 1);
        }
    }
}

// @ 0x47ced0
// Light every mesh of the entity of every placed object whose record is not kind 4.
void terrain_obj::light_props()
{
    for (unsigned i = 0; i < props.size(); i++) {
        if (props[i].kind == 4 || props[i].ent == 0)
            continue;
        for (unsigned j = 0; j < props[i].ent->meshes.size(); j++) {
            res_t* r = props[i].ent->meshes[j]->res;
            bool k = (((unsigned)r->mat->f20 >> 7) & 1) != 0;
            if (k && (*(unsigned*)((char*)r + 0x1c) & 0x700) != 0x300)
                registry->light_mesh(props[i].ent->meshes[j], &props[i].ent->frame, 1);
        }
    }
}

// ---- item mesh setup (0x47cfe0) ---------------------------------------------------------------------------
struct ItemRef {                                 // 8 bytes: item index + modifier word
    int item; unsigned m4;
    tstring mesh_name();                                     // 0x434b20 (hidden return pointer)
    tstring mesh_name(unsigned __int64 cls);                 // 0x462900 (hidden return pointer)
};
struct ItemSound { int count; void play(int id); };   // 0x48add0 (thiscall on kind+0x228)
struct ItemKind {                                // 0x238 bytes (array at 0x5e29d0)
    char pad0[0x1c8];
    int type;                                    // 0x1c8 (low byte = kind)
    char pad1cc[0x1d0 - 0x1cc];
    unsigned flags1d0;                           // 0x1d0
    unsigned flags1d4;                           // 0x1d4
    char pad1d8[0x228 - 0x1d8];
    ItemSound snd;                               // 0x228
    char pad22c[0x238 - 0x22c];
    tstring mesh_name_for(int bit);              // 0x433bf0
    void get_carry_pos(mat4* out, int* bone_flags, int* bone);   // 0x433e40
};
extern ItemKind* g_items;                        // 0x5e29d0
extern int g_item_count;                         // 0x5e29d4

struct lod_mesh {
    char pad0[0x8c];
    mat4 frame;                                  // 0x8c
    int fcc;                                     // 0xcc
    void transform(const mat4& m);               // 0x47b610
};
struct lod_mesher {                              // resmgr::make_lod_mesh (0x41ca60)
    lod_mesh* make_lod_mesh(const tstring& pat, bool copy);
};

struct Skeleton { char pad0[4]; void* elems; };
struct AgentBody { char pad0[0x304]; Skeleton* skel; };
struct FxElem { void add_group(void* m); char pad[0x1dc - 4]; };   // 0x4423f0, element stride 0x1dc

struct GameData { char pad0[0x1a1f4]; };
extern GameData* g_game;                         // 0x5e2930
extern __int64 g_spawn_a;                        // 0xa70448
extern __int64 g_spawn_b;                        // 0xa70450

static inline tstring item_mesh_name(ItemKind* kind, int item, unsigned m4)
{
    if (item < 0 || item >= g_item_count)
        return tstring("Error: Invalid item");
    return kind->mesh_name_for((int)m4 >> 24);
}

// @ 0x47cfe0
// Build the carried-item display meshes of an agent body: one lod mesh per equipment slot, plus the
// hand-carry mesh for the active item; publishes the ids/sounds for the item drop code.
// cdecl; the 5th argument is unused.
void setup_item_meshes(AgentBody* body, int flag, int troop, char* outbase, int unused, ItemRef* items, int id)
{
    if (flag != 0)
        return;
    int i = 0;
    ItemRef* it = items;
    int* p = (int*)(outbase + 0x10);
    do {
        ItemRef ir = *it;
        if (ir.item >= 0) {
            ItemKind* kind = g_items + ir.item;
            unsigned t = (unsigned)kind->type & 0xff;
            if (t == 8 || t == 9 || t == 10 || t == 0x10 || t == 0x11 ||
                t == 2 || t == 3 || t == 4 || t == 7) {
                {
                    tstring name = item_mesh_name(kind, ir.item, ir.m4);
                    *p = (int)((lod_mesher*)&g_resmgr)->make_lod_mesh(name, false);
                }
                ((FxElem*)((char*)body->skel->elems + 0x2354))->add_group((void*)*p);
                ((lod_mesh*)*p)->fcc = 0;
                if (kind->snd.count > 0) {
                    *(int*)((char*)g_game + 0x1a1f4) = id;
                    *(int*)((char*)g_game + 0x1a1f8) = troop;
                    *(int*)((char*)g_game + 0x1a204) = i;
                    *(int*)((char*)g_game + 0x1a1fc) = *p;
                    *(int*)((char*)g_game + 0x1a200) = 0;
                    g_spawn_a = id;
                    g_spawn_b = troop;
                    kind->snd.play(-0x32);
                }
            } else if (t == 5 || t == 6) {
                *p = (int)((lod_mesher*)&g_resmgr)->make_lod_mesh(ir.mesh_name(), false);
                if ((char)kind->type == 6) {
                    mat4 mtx;
                    identity(&mtx);
                    mtx.o.x = -0.01f;
                    mtx.o.z = -0.03f;
                    float angle = -1.5707964f;
                    mtx.rotate_su(&angle);
                    ((lod_mesh*)*p)->transform(mtx);
                }
                ((FxElem*)((char*)body->skel->elems + 0x2354))->add_group((void*)*p);
                ((lod_mesh*)*p)->fcc = 0;
            }
            if ((((kind->flags1d0 & 0xf0000000) | (kind->flags1d4 & 7)) != 0) &&
                ((unsigned)kind->type & 0xff) != 7 && ((unsigned)kind->type & 0xff) != 9) {
                tstring name = ir.mesh_name(0x3000000000000000ull);
                if (name.len > 1) {
                    lod_mesh* m = ((lod_mesher*)&g_resmgr)->make_lod_mesh(name, false);
                    mat4 carry;
                    int bone, other;
                    kind->get_carry_pos(&carry, &bone, &other);
                    m->frame = carry;
                    p[8] = (int)m;
                    ((FxElem*)((char*)body->skel->elems + bone * 0x1dc))->add_group(m);
                }
            }
        }
        i++;
        it++;
        p++;
    } while (i < 4);
}

// ---- the agent's carried-ranged-weapon display mesh (0x47d320) ---------------------------------------------
struct agent_view {
    char pad0[0x50];
    entity* ent;                                 // 0x50
    mesh* make_ranged_mesh();                    // 0x44f6a0
    void update_ranged_mesh();                   // 0x47d320
};

// @ 0x47d320
void agent_view::update_ranged_mesh()
{
    delete ent->meshes.begin()[0];
    mesh* m = make_ranged_mesh();
    if (m != 0) {
        ent->meshes.begin()[0] = m;
        ent->origin = vector4(0.0f, -ent->s5(), 0.0f);
        ent->f140 = 1;
    } else {
        std::vector<mesh*>& v = ent->meshes;
        if (v._Myfirst != 0)
            operator delete(v._Myfirst);
        v._Myfirst = 0;
        v._Mylast = 0;
        v._Myend = 0;
    }
}

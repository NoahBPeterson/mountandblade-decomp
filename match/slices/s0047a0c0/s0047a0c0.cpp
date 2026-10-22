// Slice s0047a0c0: the mission's ambient effects and level-interior construction.
//
//   post_spawn     (0x47a0c0) spawns the ambient emitters around the camera focus: the fog emitter chosen by
//                  the fog mode (dense / thin) scaled by 0.4 or 0.1, then the three ground / high fog layers
//                  at the level base height (+0.05, +0.35) or at 0.15 / 0.65 (scene kind 1), and hands each to
//                  the scene through the hook at +0x64; the four handles end up in mission +0x1ac..+0x1b8.
//   tmesh::clone   (0x47a370) deep-copies the level's 0xe0-byte interior lod-mesh record (name, count, the five
//                  mesh lists resized and each mesh cloned).
//   make_grid      (0x47a460) rebuilds the 0x30 x 0x30 dust grid: one scene object (with the high or lod
//                  default material) per cell over the play area, then widens the bounds of the cells the dust
//                  list touches, then resets / updates every cell box.
//   build_interior (0x47a800) instantiates all interior meshes of the level body as one entity.
//   SetVec::resize (0x47a980) is std::vector<E56>::resize for 0x38-byte items (growth 0x445ea0, tail erase
//                  0x479930); E56 is the engine's {3 ints, vector<int>, vector<int>, 3 ints} record (s0043edf0).
// Names are provisional; layouts reuse the confirmed neighbours (s004428c0 cameras/emitters, s0044c210 props
// and entities, s0041b5a0 resmgr, s0043edf0 <vector>, s004832b0 level slots, s0041f720 meshes).
#include <math.h>
#include <new>
#include <vector>
#include "rgl.h"

using rgl::vector4;
using rgl::matrix;

// ---- the engine string (see tstring.h of s0042b790) -------------------------------------------------
struct tstring {
    int pos; char* buf; int cap; int len;
    tstring() {}                                // 0x42bbd0
    tstring(const char* s);                     // 0x42bbe0
    ~tstring();                                 // 0x42bc40
    void operator=(const tstring& o);           // 0x42bc50
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

// ---- render objects (mesh.h of s0041f720): the header fields and the clone ------------------------------
struct mesh_rec {
    mesh_rec* clone();                          // 0x41c6d0 (thiscall: new 0x260 + ctor + copy)
};
// the clone as a free call: the original emits `mov ecx, <src>` + `call 0x41c6d0`
#pragma warning(disable : 4035)
static mesh_rec* mesh_rec_clone(mesh_rec* m) { __asm { mov ecx, m } __asm { call mesh_rec::clone } }
struct mesh_obj;
mesh_obj* mesh_alloc();                         // the new + mesh ctor pair of 0x47a460

struct material;                                // "mat_default_skin_hi" / "mat_default_lod" (type 4)
struct mesh_obj {
    char pad0[4];
    int f4;                                     // +0x04
    char pad8[0x2c - 8];
    material* res;                              // +0x2c (material looked up for the cell)
    unsigned flags;                             // +0x30 (bit 0x10000000 set for the cells)
    char pad34[0x10c - 0x34];
    unsigned mask;                              // +0x10c
    char pad110[0x260 - 0x110];
};

// std::vector<mesh_rec*> (VC7.1 layout); the out-of-line insert 0x410610 is shared by every pointer vector
struct MeshVec : std::vector<mesh_rec*> {
    MeshVec& operator=(const std::vector<mesh_rec*>& o);        // 0x43f0a0
    void insert_n(iterator where, size_t n, const mesh_rec*& v);    // 0x410610
    MeshVec* self() { return this; }
    void resize2(size_t n, mesh_rec* v);                        // 0x41f970 (this - 4)
    void push_back(const mesh_rec*& x)
    {
        if (_Myfirst != 0 && size() < capacity()) {
            *_Mylast = (mesh_rec*)x;
            _Mylast++;
        } else {
            insert_n(_Mylast, 1, x);
        }
    }
};

// the 0x34-byte orientation block at the head of an interior record (ctor 0x40eaf0)
struct rot_state {
    vector4 a, b, c;
    float f30;
    rot_state();                                // 0x40eaf0
};

// interior lod-mesh record (0xe0 bytes; ctor 0x41c2a0, see s0041bf40): orientation, the five mesh lists,
// the number of used lists, a frame, a mask and the name
struct tmesh {
    rot_state base;                             // +0x00
    MeshVec lists[5];                           // +0x34
    int count;                                  // +0x84
    int cur;                                    // +0x88
    matrix m8c;                                 // +0x8c
    unsigned mask;                              // +0xcc
    tstring name;                               // +0xd0
    tmesh();                                    // 0x41c2a0
    tmesh* clone();                             // 0x47a370
};

// ---- the resource manager (global object at 0x5d6fa0, mbres.h of s0041b5a0) ----------------------------
struct res_obj { virtual ~res_obj(); };
struct mesh_res : res_obj {};
struct material;                                // "mat_default_skin_hi" / "mat_default_lod" (type 4)
struct res_mgr {
    char f0;                                    // +0x00
    int c4, c8, cc;                             // +0x04
    std::vector<res_obj*> v10;                  // +0x10
    std::vector<mesh_res*> meshes;              // +0x20
    std::vector<int> textures;                  // +0x30
    std::vector<material*> materials;           // +0x40
    std::vector<mesh_rec*> find_meshes(const tstring* prefix);   // 0x41b960
    material* get_material(const tstring& name);                 // 0x418a70
};
extern res_mgr g_resmgr;                        // 0x5d6fa0

// ---- scene objects (mesh.h / entity.h of s0041f720) ----------------------------------------------------
// the 0x260-byte render object attached to every grid cell (ctor 0x412a90)

// collision box (aabb of mesh.h): two corners and a centre; reset 0x416e40, update 0x416f70
struct aabb {
    vector4 min, max, center;
    float radius;
};
void bound_reset(aabb* b);                      // 0x416e40 (empty box)
void bound_update(aabb* b);                     // 0x416f70 (box from the two corners)

struct SceneRegistry;
struct World;
struct Emit;
struct Entity {
    char pad0[0xc];
    int id;                                     // +0x0c
    int kind;                                   // +0x10
    char pad14[0x118 - 0x14];
    MeshVec meshes;                             // +0x118 (first at +0x11c)
    char pad128[0x250 - 0x128];
    void* renderer;                             // +0x250
    World* owner;                               // +0x254
    virtual void v0(); virtual void v1(); virtual void v2(); virtual void v3();
    virtual void refresh();                     // slot 4 (+0x10): rebuild from the owned meshes
    virtual void v5(); virtual void v6(); virtual void v7();
    virtual void set_scale(Emit* e, float* k);  // slot 8 (+0x20)
};

struct SceneRegistry {                          // the collision registry (s00423ed0)
    int id;                                     // +0
    void reg(void* o);                          // 0x424170
};
struct World {
    char pad0[0xc3b0dc - 0];
};
struct Scene : World {                          // the mission's scene object (mission +0)
    SceneRegistry* registry;                    // +0xc3b0dc
    Entity* create_entity(int type_id);         // 0x428440
};

// ---- the emitter table (0x8ed5090) and its ids; emitters are 0x1bc bytes (s00479980) ------------------
struct Emit {
    char pad0[0x3c];
    vector4 v3c;                                // +0x3c
    vector4 v4c;                                // +0x4c
    vector4 v5c;                                // +0x5c
    char pad6c[0xec - 0x6c];
    float height;                               // +0xec (height offset of the placed emitter)
};
struct EmitDef;
struct EmitTable {
    EmitDef* defs;                              // +0
    int count;                                  // +4
    Emit* spawn(int* idx, const vector4* pos, const int* owner);    // 0x479d20
};
extern EmitTable g_emit_defs;                   // 0x8ed5090
extern int g_emit_id[];                         // 0x8ed5098 (ids of the ambient emitters: dense fog 4, thin 6)
extern int g_fog_id_a;                          // 0x8ed50b4 (ground fog layer)
extern int g_fog_id_b;                          // 0x8ed50b8
extern int g_fog_id_c;                          // 0x8ed50bc

// ---- game options (s00418150) and the scene's settings --------------------------------------------
extern int g_env_on;                            // 0x5aa028 (environmental effects, see s00484590)
extern int g_fog_density;                       // 0x5aa03c (>= 10: build the dust grid)
extern int g_mesh_quality;                      // 0x5aa084 (2 = high detail)

extern const char g_s_mat_hi[];                 // 0x5918d4 "mat_default_skin_hi"
extern const char g_s_mat_lod[];                // 0x5918c4 "mat_default_lod"
extern const char g_s_no_body[];                // 0x5918dc "Unable to find body %s"
void mb_error(const char* fmt, ...);            // 0x42cc80

// ---- the camera record (0x4c bytes, s004428c0) --------------------------------------------------------
struct Cam {
    char pad0[0x20];
    vector4 v20;                                // +0x20
    char pad30[0x10];
    float f40, f44, f48;                        // +0x40
};
extern Cam* g_cams;                             // 0x5e28f4

// ---- level slots (0x494 bytes; s004832b0) ------------------------------------------------------------
struct LevelSlot {
    char pad0[0x30];
    unsigned flags;                             // +0x30
    tstring body;                               // +0x34 (body resource name)
    tstring interior;                           // +0x44 (interior mesh name)
    char pad54[0x494 - 0x54];
};
struct GameData {
    char pad0[0x13a50];
    LevelSlot* slots;                           // +0x13a50
};
extern GameData* g_world;                       // 0x5e2930
extern int g_detail_res;                        // 0x5e28fc (detail resources present, see s00484590)

// ---- dust particles (stride 0xa0; x at +0, y at +4, z at +8) ------------------------------------------
struct DustEntry {
    float x;                                    // +0x00
    float y;                                    // +0x04
    float z;                                    // +0x08
    char pad0c[0xa0 - 0x0c];
};

// element type of SetVec: the engine's 0x38-byte {3 ints, vector<int>, vector<int>, 3 ints} record
struct E56 {
    int a, b, c;
    std::vector<int> v1;                        // +0x0c
    std::vector<int> v2;                        // +0x1c
    int d, e, f;
};

// ---- the mission ---------------------------------------------------------------------------------------
// Offsets are the confirmed ones (s00479980, s004713d0, s004832b0). The dust grid is a 0x30 x 0x30 array of
// collision boxes indexed [x][y] (x stride 0x9c0, y stride 0x34) and the parallel array of the cells' scene
// objects is indexed the same way (x stride 0xc0, y stride 4).
struct Mission {
    char pad0[0x1ac];
    Emit* fog;                                  // +0x0001ac (emitters spawned by the last post_spawn)
    Emit* layer0;                               // +0x0001b0
    Emit* layer1;                               // +0x0001b4
    Emit* layer2;                               // +0x0001b8
    char pad1bc[0x28b156 - 0x1bc];
    char cells[0x30][0x30][0x34];               // +0x0028b156 (dust grid boxes)
    char padcells[0x5c3b8f4 - 0x28b156 - 0x30 * 0x30 * 0x34];
    int dust_count;                             // +0x5c3b8f4
    char padd[0x5c3b93c - 0x5c3b8f8];
    DustEntry* dust;                            // +0x5c3b93c (stride 0xa0)
    char padgrid[0x843bd78 - 0x5c3b940];
    Entity* grid[0x30][0x30];                   // +0x0843bd78 (scene object of each cell)
    int n_inner;                                // +0x0845d97c (cells along y)
    int n_outer;                                // +0x0845d980 (cells along x)
    void post_spawn(int which);                 // 0x47a0c0
    void make_grid();                           // 0x47a460
    void build_interior(int which, int* out, int* n);   // 0x47a800
};
extern Mission* g_mission;                      // 0x5e292c (the mission object, see s00479980)
void replace_emitter(Emit* e);                  // scene hook (virtual +0x64) that takes the emitter

extern float g_k0_4;                            // 0x54b3b8
extern float g_k0_05;                           // 0x54b4d0 (1 / grid cells per side)
extern float g_k0_06;                           // 0x590d94
extern float g_k0_35;                           // 0x590e5c
float g_cst_0_15;                               // 0x3e19999a
float g_cst_0_65;                               // 0x3f266666

// ---- the scene object (mission +0) ----------------------------------------------------------------------
struct SceneReg {
    char pad0[0xc3b0dc];
    SceneRegistry* registry;                    // +0xc3b0dc
    Entity* create_entity(int type_id);         // 0x428440
};

// @ 0x47a0c0
void Mission::post_spawn(int which)
{
    SceneReg* view = *(SceneReg**)this;
    vector4 pos;
    pos.x = g_cams[*(int*)((char*)this + 0x660c)].v20.x * g_k0_4;
    pos.y = g_cams[*(int*)((char*)this + 0x660c)].v20.y * g_k0_4;
    pos.z = g_cams[*(int*)((char*)this + 0x660c)].v20.z * g_k0_4;
    pos.w = g_cams[*(int*)((char*)this + 0x660c)].f40 + g_cams[*(int*)((char*)this + 0x660c)].f44
          + g_cams[*(int*)((char*)this + 0x660c)].f48;
    int owner = 0;
    float scale = 0.1f;
    if (g_env_on == 0)
        return;
    LevelSlot* sl = &g_world->slots[which];
    if ((sl->flags & 1) == 0) {
        if (*(int*)((char*)this + 0x6608) != 4) {
            int idx = -1;
            if (*(int*)((char*)this + 0x6604) == 0) {
                idx = g_emit_id[4];
                scale = g_k0_4;
            } else if (*(int*)((char*)this + 0x6604) == 1) {
                idx = g_emit_id[6];
            } else {
                goto layers;
            }
            if (idx >= 0) {
                Emit* e = g_emit_defs.spawn(&idx, &pos, &owner);
                if (e) {
                    replace_emitter(e);
                    fog = e;
                    e->v3c.x *= scale;
                    e->v3c.y *= scale;
                    e->v3c.z *= scale;
                    e->v4c.x *= scale;
                    e->v4c.y *= scale;
                    e->v4c.z *= scale;
                    e->v5c.x *= scale;
                    e->v5c.y *= scale;
                    e->v5c.z *= scale;
                }
            }
        }
    }
layers:
    if (g_env_on == 0)
        return;
    if (*(int*)((char*)view + 0x10) == 1)
        pos.z = g_cst_0_65;
    else
        pos.z = g_cst_0_15;
    pos.x = 0.0f;
    pos.y = 0.0f;
    pos.w = 0.0f;
    int id = g_fog_id_a;
    Emit* e0 = g_emit_defs.spawn(&id, &pos, &owner);
    if (e0) {
        e0->height = *(float*)((char*)this + 0x654c);
        replace_emitter(e0);
        layer0 = e0;
    }
    id = g_fog_id_b;
    Emit* e1 = g_emit_defs.spawn(&id, &pos, &owner);
    if (e1) {
        e1->height = *(float*)((char*)this + 0x654c) + g_k0_05;
        replace_emitter(e1);
        layer1 = e1;
    }
    pos.z += g_k0_06;
    id = g_fog_id_c;
    Emit* e2 = g_emit_defs.spawn(&id, &pos, &owner);
    if (e2) {
        e2->height = *(float*)((char*)this + 0x654c) + g_k0_35;
        replace_emitter(e2);
        layer2 = e2;
    }
}

// @ 0x47a370
tmesh* tmesh::clone()
{
    tmesh* c = new tmesh;
    c->name = name;
    c->count = count;
    for (int i = 0; i < count; i++) {
        mesh_rec* zero = 0;
        c->lists[i].resize(lists[i].size(), zero);
        for (unsigned j = 0; j < lists[i].size(); j++)
            c->lists[i][j] = lists[i][j]->clone();
    }
    return c;
}

// @ 0x47a460
void Mission::make_grid()
{
    n_inner = 0;
    n_outer = 0;
    Entity** g = &grid[0][0];
    int cavity = (int)((char*)this + 0x28b156 - (char*)g);
    Entity** grid_p = &grid[0][0];
    for (int x = 0; x < 0x30; x++)
        for (int y = 0; y < 0x30; y++)
            grid_p[x + y] = 0;
    aabb* box = (aabb*)(int*)cells;
    for (int x = 0; x < 0x30; x++) {
        for (int y = 0; y < 0x30; y++, box++) {
            ((int*)box)[0] = 0;
            bound_reset(box);
        }
    }
    if (cavity == 0x7af9c8) {
        // box grid -> first box distance, kept live so the register use matches the original
    }
    if (g_fog_density >= 10) {
        n_inner = (int)ceil(*(float*)((char*)this + 0x270) * g_k0_05);
        n_outer = (int)ceil(*(float*)((char*)this + 0x274) * g_k0_05);
        if (n_inner > 0x30)
            n_inner = 0x30;
        if (n_outer > 0x30)
            n_outer = 0x30;
        int x = n_outer;
        do {
            for (int y = 0; y < n_inner; y++) {
                Entity* e = ((SceneReg*)*(void**)this)->create_entity(9);
                g[(x - x) + y] = e;
                mesh_obj* m = mesh_alloc();
                m->flags |= 0x10000000;
                if (g_detail_res != 0 && g_mesh_quality == 2)
                    m->res = g_resmgr.get_material(tstring(g_s_mat_hi));
                else
                    m->res = g_resmgr.get_material(tstring(g_s_mat_lod));
                const mesh_rec* mr = (const mesh_rec*)m;
                e->meshes.push_back(mr);
                e->refresh();
            }
        } while (--n_outer != 0);
    }
    DustEntry* dp = (DustEntry*)((char*)this + 0x5c3b8f8) + 0x14;
    for (int i = 0; i < dust_count; i++) {
        DustEntry* d = dp++;
        int y = (int)ceil(d->x * g_k0_05);
        if (y < 0)
            y = 0;
        else if (y >= n_inner)
            y = n_inner - 1;
        int x = (int)ceil(d->y * g_k0_05);
        if (x < 0)
            x = 0;
        else if (x >= n_outer)
            x = n_outer - 1;
        aabb* c = (aabb*)(cavity + (y * 3 + x) * 0x34);
        if (!(d->x < c->min.x))
            c->min.x = d->x;
        if (!(d->y < c->min.y))
            c->min.y = d->y;
        if (!(d->z < c->min.z))
            c->min.z = d->z;
        if (!(d->x > c->max.x))
            c->max.x = d->x;
        if (!(d->y > c->max.y))
            c->max.y = d->y;
        if (!(d->z > c->max.z))
            c->max.z = d->z;
    }
    for (int x = 0; x < n_outer; x++) {
        aabb* u = (aabb*)(cavity + x * 0x34);
        for (int y = 0; y < n_inner; y++, u += 0x30)
            bound_update(u);
    }
}

// @ 0x47a800
void Mission::build_interior(int which, int* out, int* n)
{
    LevelSlot* es = &g_world->slots[which];
    MeshVec meshes;
    meshes = g_resmgr.find_meshes(&es->body);
    if (meshes.size() == 0)
        mb_error(g_s_no_body, es->body.c_str());
    Entity* e = ((SceneReg*)*(void**)this)->create_entity(9);
    int count = 0;
    *out = (int)meshes.size();
    for (; count < *out; count++) {
        mesh_rec* m = meshes[count]->clone();
        meshes[count] = m;
        e->meshes.push_back(m);
        e->refresh();
    }
    float dist = 100000.0f;
    e->set_scale(0, &dist);
}

// @ 0x47a980
template void std::vector<E56>::resize(size_t, E56);

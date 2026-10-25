//-----------------------------------------------------------------------------
// File: mission.cpp  (0x478830..0x47aa40)
//
// Desc: the mission part of the terrain object's translation unit (TU 62 of work/migrate/tu_map.csv): the scene
//       container's grid (construction, insertion of a box, the nav mesh's per-face boxes, wind objects), the scene-file
//       reader, the mission's reset and ambient-wind setup, the nav store built from the nav mesh, the particle
//       emitter table, the spawned props and the interior meshes, the ambient fog emitters around the camera, the
//       dust-cell grid, and the vector<MRow56> erase / resize. Names provisional. Functions in ascending address
//       order. Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include <math.h>
#include <new>
#include "../../rgl/mesh.h"
#include "../../rgl/stream.h"
#include "../tstring.h"
#include "./frustum.h"
#include "./pool.h"
#include "./agent_rec.h"
#include "./mission.h"
#include "./cell_grid.h"
#include "./cell_world.h"
#include "./poly_mesh.h"
#include "./records.h"
#include "./game_data.h"
#include "./terrain.h"
#include "./emit.h"
#include "./scene_file.h"
#include "./tmesh.h"
#include "./face.h"
#include "./entity_rec.h"

using rgl::vector4;
using rgl::matrix;

// ---- calls into other objects -----------------------------------------------------------------------------------------
void mb_error(const char* fmt, ...);            // 0x42cc80
extern int g_use_shaders;                       // 0x5e28fc
extern int g_mesh_quality;                      // 0x5aa084
extern int g_nav_param;                         // 0x59132c
extern __int64 g_ticks[];                       // 0x5e2948
extern ptype* g_ptypes;                         // 0x5e28f8
#include "../../include/mat4.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../arr_set.h"
#include "../../include/frustum.h"
#include "../../include/keyframes.h"
#include "../../include/mesh_prims.h"
#include "../../include/ptr_tag.h"
#include "../../include/timer.h"
#include "../../include/vec.h"
#include "../../include/shape.h"      // (the shared class headers named here too: the resolution audit reads the annotations of directly included headers)
#include "../terrain_desc.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../view_params.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../view_state.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../include/mesh_parts.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../terrain_a/terrain.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../mission.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../mode_rec.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/iface.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
using rgl::mat3;
mat3* __cdecl identity3(mat3* out);             // 0x40cd60
matrix* __cdecl identity_pos(matrix* out);      // 0x40cd00
// resource-manager members this object calls that rgl/resource.h does not declare (no new fields)
struct resmgr : resource_manager {
    std::vector<rgl::mesh*> find_meshes(const rgl::string* prefix);   // 0x41b960 (hidden return pointer)
};
struct mesh_x : rgl::mesh {
    mesh_x* clone();                            // 0x41c6d0 (a copy of the mesh)
};
struct lod_group {
    char data[0xe0];
    lod_group();   // 0x41c2a0 (the interior record's constructor: tmesh() is the same code)
};
static __forceinline const rgl::string& rs(const tstring& t) { return (const rgl::string&)t; }   // tstring is the 16-byte string the engine's code constructs
template<> void std::vector<Rec1c8>::resize(size_t);   // 0x477ea0 (terrain_b.cpp)
// vector<int>::_Insert_n (0x410610) and vector<rgl::Obj*>::_Insert_n (0x41f720) are the instances the pointer vectors share (rgl/particles
// and rgl/scene): declared here, never instantiated; the views reach them through classes that make the protected member public
namespace std { template<> void vector<int>::_Insert_n(iterator, size_type, const int&); }   // 0x410610
struct IntVecPub : std::vector<int> { using std::vector<int>::_Insert_n; };
namespace rgl { struct Obj; }
namespace std { template<> void vector<rgl::Obj*>::_Insert_n(iterator, size_type, rgl::Obj* const&); }   // 0x41f720
struct ObjPtrVecPub : std::vector<rgl::Obj*> { using std::vector<rgl::Obj*>::_Insert_n; };
__forceinline void MeshVec::insert_n(iterator where, size_t n, rgl::mesh* const& v)
{ ((IntVecPub*)this)->_Insert_n(std::vector<int>::iterator((int*)&*where), n, *(const int*)&v); }
__forceinline void ObjVec::insert_n(iterator where, size_t count, void* const& val)
{ ((ObjPtrVecPub*)this)->_Insert_n(std::vector<rgl::Obj*>::iterator((rgl::Obj**)&*where), count, *(rgl::Obj* const*)&val); }
template<> void Pool<MAgent>::reset();          // 0x477f00 (terrain_b.cpp)
template<> void Pool<RecJ>::reset();           // 0x477f90

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
    to_cell(lo_, a);
    to_cell(hi_, b);
    for (int x = a[0]; x <= b[0]; x++)
        for (int y = a[1]; y <= b[1]; y++)
            cells[x][y].b.push_back(id);
}

// @ 0x478ad0
CellGrid::CellGrid(const float* lo_, const float* hi_, const int* dimx, const int* dimy, const int* v)
    : v7c(), v8c(), va0(), winds(), vc8(), view(), v32c()
{
    f340 = *v;
    f9c = 1;
    init(lo_, hi_, *dimx, *dimy);
    m30c = vector4(1.0f, 0.0f, 0.0f);
    core = 0;
    navmesh = 0;
    hf = 0;
    m31c = vector4(0.0f, 0.0f, 0.0f);
    amb = vector4(0.0f, 0.0f, 0.0f);
    f78 = 0;
}

// @ 0x478c50
void CellGrid::build(PolyMesh* m)
{
    navmesh = m;
    if (m == 0)
        return;
    for (unsigned i = 0; i < navmesh->c.faces.size(); i++) {
        MPoly* r = &vat(navmesh->c.faces, i);
        MVert* verts = &*navmesh->c.verts.begin();
        float lo[3] = { 1e6f, 1e6f, 0.0f };
        float maxy = -1e6f;
        float hi[3] = { -1e6f, maxy, 0.0f };
        int n = r->n;
        int j;
        for (j = 0; j < n; j++) {
            MVert* p = &verts[r->lim[j]];
            if (lo[0] > p->pos.x) lo[0] = p->pos.x;
            if (lo[1] > p->pos.y) lo[1] = p->pos.y;
            if (hi[0] < p->pos.x) hi[0] = p->pos.x;
            if (maxy < p->pos.y) maxy = p->pos.y;
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
void SceneFile::load(stream_base* s, int id)
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

// @ 0x479160
void Mission::reset()
{
    if (scene) {
        delete scene;
        scene = 0;
    }

    // vector<int>::resize(0, 0) below is the 2-argument overload; the original materialises the
    // value temporary in a stack slot and passes its address (cl 13.10 elides the store and passes
    // whatever is in the slot), so the source-level form is kept but cl builds its own temp.
    dead.resize(0, 0);
    blend_time = 0.0f;
    blend2_time = 0.0f;
    f67e8 = -1;
    agent_pool(this).reset();
    pool_j(this).reset();
    f5c = -1;
    f60 = -1;
    // The store order below is the original's (the compiler keeps it), so it is reproduced verbatim.
    f65ac = 0;
    f65a8 = 0;
    f65b8 = 0;
    f65bc = 0;
    f65c0 = 0;
    f65c4 = 0;
    f65c8 = 0;
    f65d0 = 0;
    f65d4 = 0;
    f65d8 = 0;
    f65dc = 0;
    f65e4 = 0;
    f65e0 = 0;
    f65cc = 0;
    f65b0 = 0;
    f6664 = -1;
    f6598 = vector4(10.0f, 10.0f, 10.0f);
    terrain.init();
    f672c = 0;
    f6730 = 0;
    f6734 = 0;
    f673c = 0;
    f6738 = 0;
    special = 0;
    f6748 = 0;
    blend2_from = 1.0f;
    {
        matrix tmp;
        frame_a = *identity_pos(&tmp);
    }
    str.clear();
    v5dfc.resize(0, 0);
}

// @ 0x4792d0
void Mission::setup_ambient(int level_idx, int spec_idx, float t)
{
    Level494* lv = g_game->levels + level_idx;
    EnvRow* sp = g_env_rows + spec_idx;

    {
        vector4 col;
        unpack_color(&col, (const unsigned*)&sp->f44);

        float k;
        if (t * 60.0f < 0.0f)
            k = 0.0f;
        else if (t * 60.0f >= 1.0f)
            k = 1.0f;
        else
            k = t * 60.0f;

        // ---- first wind object: a horizontal "sheet" whose normal comes from the hourly table
        f65b0 = scene->spawn_wind();
        f65b0->f0 = 2;

        float hour = (float)fmod((double)((float)g_ticks[1] * 1e-5f), 24.0);
        bool day = (4.0f <= hour && hour < 20.0f);

        int slot = (int)((float)(g_ticks[g_game->f13b18] - *(__int64*)&g_game->f13b20) * 1e-5f) % 24;
        {
            vector4 dir = sp->get_dir(slot, day);
            f65b0->f44 = dir;
            f65b0->f24 = vector4(dir.x * -10000.0f, dir.y * -10000.0f, dir.z * -10000.0f);
        }
        f65b0->f54 = 20000.0f;
        f65b0->f6c = 0;

        if (lv->flags & 1) {
            // flat variant: the wind object just sits at the origin, the cell gets a uniform colour
            f65b0->pos = vector4(0.0f, 0.0f, 0.0f);
            vector4 c;
            if (g_use_shaders) {
                c.x = 0.03f;
                c.y = 0.03f;
                c.z = 0.03f;
            } else {
                c.x = 0.15f;
                c.y = 0.15f;
                c.z = 0.15f;
            }
            scene->amb = c;
            scene->amb.w = 1.0f;
            f65b0->f14 = f65b0->pos;
            return;
        }

        // ---- variant B: an anisotropically scaled wind gust
        float p = (float)pow((double)(1.0f - k), 1.5);
        vector4 u(p * sp->v20.x, p * sp->v20.y, p * sp->v20.z);
        if ((sp->f10 & 0x10000000) || sqrtf(u.x * u.x + (u.y * u.y + u.z * u.z)) < 0.1f)
            f65b0->f6c |= 2;
        if (!g_use_shaders) {
            u.x = sqrtf(u.x);
            u.y = sqrtf(u.y);
            u.z = sqrtf(u.z);
        }
        float cx = col.x * k;
        float cy = col.y * k;
        col.z *= k;
        f65b0->pos = u;
        f65b0->f14 = f65b0->pos;

        // the cell colour: the unpacked table colour faded by k, offset by the table base position
        u.x = cx * 0.2f + sp->base.x;
        u.y = cy * 0.2f + sp->base.y;
        u.z = col.z * 0.2f + sp->base.z;
        if (!g_use_shaders) {
            u.x = sqrtf(u.x);
            u.y = sqrtf(u.y);
            u.z = sqrtf(u.z);
        }
        scene->amb = u;
        scene->amb.w = 1.0f;
    }

    // ---- second wind object: the table's direction rotated by 3.5 rad, normalised, half-size
    {
    f65b4 = scene->spawn_wind();
    f65b4->f0 = 2;

    float hour2 = (float)fmod((double)((float)g_ticks[1] * 1e-5f), 24.0);
    bool day2 = (4.0f <= hour2 && hour2 < 20.0f);
    int slot2 = (int)((float)(g_ticks[g_game->f13b18] - *(__int64*)&g_game->f13b20) * 1e-5f) % 24;
    const vector4& d2 = sp->get_dir(slot2, day2);
    float dx = d2.x, dy = d2.y;

    float len = sqrtf(dx * dx + dy * dy);
    float cx, cy;
    if (len > 0.0f) {
        float inv = 1.0f / len;
        cx = inv * dx;
        cy = inv * dy;
    } else {
        cx = 0.0f;
        cy = 1.0f;
    }
    vector4 rd(cos(3.5) * cx - sin(3.5) * cy, sin(3.5) * cx + cos(3.5) * cy, -0.9f);
    rd.normalize();
    f65b4->f44 = rd;

    f65b4->pos = sp->base;
    f65b4->pos.x *= 0.5f;
    f65b4->pos.y *= 0.5f;
    f65b4->pos.z *= 0.5f;
    f65b4->f14 = f65b4->pos;
    f65b4->f24 = vector4(rd.x * -10000.0f, rd.y * -10000.0f, rd.z * -10000.0f);
    f65b4->f54 = 20000.0f;
    f65b4->f6c = 1;
    }
}

// @ 0x479930
template std::vector<MRow56>::iterator std::vector<MRow56>::erase(std::vector<MRow56>::iterator, std::vector<MRow56>::iterator);

// @ 0x479980
void CellGrid::build_faces(NavData* map)
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
        f->transform((const pos*)&m);
        if (items.size() < items.capacity()) {
            *items._Mylast = f;
            items._Mylast++;
        } else {
            items.insert_n(items._Mylast, 1, f);
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

// @ 0x479d20
Emit* EmitTable::spawn(int* idx, const vector4* pos, const int* owner)
{
    if (*idx < 0 || *idx >= count)
        return 0;
    Emit* e = new Emit(owner);
    setup(e, *idx, pos);
    return e;
}

// @ 0x479da0
void Terrain::spawn_bodies(SpawnSet* r)
{
    for (int i = 0; i < r->count; i++) {
        SpawnEntry* e = &r->entries[i];
        ptype* ty = &g_ptypes[e->type];
        int c = e->counter % ty->nframes;
        e->counter = c;
        res_shape* rs = ty->boxes[c].res;
        if (rs) {
            shape_calls* o = (shape_calls*)rs->clone_shape();
            o->transform(&e->m);
            o->or_m8(0x200);
            grid->reg_shape(o);
        }
    }
}

// the stock VC7.1 vector::push_back for the mesh pointers (separate null tests in size() and capacity()); its growth path is the
// shared out-of-line _Insert_n at 0x410610 (declared with a plain pointer for the iterator)
struct MeshPush : std::vector<rgl::mesh*> {
    __forceinline void ins(rgl::mesh** where, size_t n, rgl::mesh* const& v)     // 0x410610
    { ((IntVecPub*)this)->_Insert_n(std::vector<int>::iterator((int*)where), n, *(const int*)&v); }
    void push(rgl::mesh* const& x)
    {
        if (size() < capacity()) {
            *_Mylast = x;
            _Mylast++;
        } else {
            ins(_Mylast, 1, x);
        }
    }
};
// V2 (a vec2 with constructors): ../../include/vec.h
#define LGF(v) ((float)log((double)(v)) * 1.442695f)
#define LOG2UP(v) ((int)(LGF(v) > 0.0f ? (float)ceil(LGF(v)) + 1e-8f : (float)ceil(LGF(v)) - 1e-8f))

// @ 0x479e20
void Mission::rebuild_nav(int which, int)
{
    Level494* sl = g_game->levels + which;
    Level494* cs = g_game->levels + f5df4;
    NavData* core_ = 0;
    res_shape* r = 0;
    V2 hi, lo;
    if (sl->flags & 0x100) {
        lo = V2(terrain.min_x - 1.0f, terrain.min_y - 1.0f);
        hi = V2(terrain.max_x + 1.0f, terrain.max_y + 1.0f);
    } else {
        core_ = (NavData*)new NavCore;   // 0x478150 (NavData() is the same constructor, ICF-folded)
        r = (res_shape*)g_resmgr.get_object60(*(rgl::string*)&cs->name44);
        if (r == 0) {
            tstring* nm = &cs->name44;
            mb_error("Unable to find body %s", nm->c_str());
        }
        ((shape_calls*)r->sh)->get_max(&hi.x);
        hi = V2(hi.x + 1.0f, hi.y + 1.0f);
        ((shape_calls*)r->sh)->get_min(&lo.x);
        lo = V2(lo.x - 1.0f, lo.y - 1.0f);
    }
    int lod1 = LOG2UP((hi.x - lo.x) * (1.0f / 3.0f));
    int lod2 = LOG2UP((hi.y - lo.y) * (1.0f / 3.0f));
    CellGrid* s = new CellGrid(&lo.x, &hi.x, &lod1, &lod2, &g_nav_param);
    scene = s;
    if ((sl->flags & 0x100) == 0) {
        s->build_faces(core_);
        shape_calls* o = (shape_calls*)r->clone_shape();
        matrix m;
        identity_pos(&m);
        o->transform(&m);
        s->reg_shape(o);
    }
}

// @ 0x47a0c0
void Mission::post_spawn(MAgent* a)
{
    EnvRow* row = &g_env_rows[cam];
    float sz = row->v20.z * 0.4f;
    vector4 sc;
    sc.x = row->base.x + row->v20.x * 0.4f;
    sc.y = row->base.y + row->v20.y * 0.4f;
    sc.z = row->base.z + sz;
    vector4 pos;
    pos.x = 0.0f;
    pos.y = 0.0f;
    pos.z = 0.1f;
    int idx;
    int owner = 0;
    if (g_env_on == 0)
        return;
    if ((g_game->levels[f5df4].flags & 1) == 0 && f6608 != 4) {
        if (fog_dense == 0) {
            idx = g_emit_id[4];
            pos.z = 0.4f;
        } else if (fog_dense == 1) {
            idx = g_emit_id[6];
        } else {
            goto layers;
        }
        if (idx >= 0) {
            owner = 0;
            Emit* e = g_emit_defs.spawn(&idx, &pos, &owner);
            if (e) {
                a->f250->add_emitter(e);
                a->f1ac = (int)e;
                e->v3c.x *= sc.x;
                e->v3c.z *= sc.x;
                e->v4c.x *= sc.y;
                e->v4c.z *= sc.y;
                e->v5c.x *= sc.z;
                e->v5c.z *= sc.z;
            }
        }
    }
layers:
    if (g_env_on != 0) {
        pos.x = 0.0f;
        pos.z = 0.0f;
        pos.y = 0.15f;
        if (a->f10 == 1)
            pos.y = 0.65f;
        idx = g_fog_id_a;
        owner = 0;
        Emit* e0 = g_emit_defs.spawn(&idx, &pos, &owner);
        if (e0) {
            e0->height = terrain.base_z;
            a->f250->add_emitter(e0);
            a->f1b0 = (int)e0;
        }
        idx = g_fog_id_b;
        owner = 0;
        Emit* e1 = g_emit_defs.spawn(&idx, &pos, &owner);
        if (e1) {
            e1->height = terrain.base_z + 0.05f;
            a->f250->add_emitter(e1);
            a->f1b4 = (int)e1;
        }
        pos.y = pos.y + 0.6f;
        idx = g_fog_id_c;
        owner = 0;
        Emit* e2 = g_emit_defs.spawn(&idx, &pos, &owner);
        if (e2) {
            e2->height = terrain.base_z + 0.06f;
            a->f250->add_emitter(e2);
            a->f1b8 = (int)e2;
        }
    }
}

// @ 0x47a370
tmesh* tmesh::clone()
{
    tmesh* c = (tmesh*)new lod_group;   // 0x41c2a0 (tmesh() is the same constructor)
    c->name = name;
    c->count = count;
    for (int i = 0; i < count; i++) {
        rgl::mesh* zero = 0;
        c->lists[i].resize(lists[i].size(), zero);
        for (unsigned j = 0; j < lists[i].size(); j++)
            c->lists[i][j] = ((mesh_x*)lists[i][j])->clone();
    }
    return c;
}

// @ 0x47a460
void Terrain::make_grid()
{
    n_inner = 0;
    n_outer = 0;
    for (int y = 0; y < 0x30; y++) {
        for (int x = 0; x < 0x30; x++) {
            gridA[y][x] = 0;
            gridB[y][x] = 0;
            boxes[y][x].reset();
        }
    }
    if (g_fog_density >= 10) {
        n_inner = (int)ceil(wx * 0.05f);
        n_outer = (int)ceil(wy * 0.05f);
        if (n_inner > 0x30)
            n_inner = 0x30;
        if (n_outer > 0x30)
            n_outer = 0x30;
        for (int x = 0; x < n_outer; x++) {
            for (int y = 0; y < n_inner; y++) {
                gridA[y][x] = grid->create_entity(9);
                rgl::mesh* m = new rgl::mesh;
                if (g_use_shaders != 0 && g_mesh_quality == 2)
                    m->res = g_resmgr.get_material(rs(tstring("grass")));
                else
                    m->res = g_resmgr.get_material(rs(tstring("grass_no_shadow")));
                m->flags |= 0x10000000;
                ((MeshPush*)&gridA[y][x]->meshes)->push(m);
                gridA[y][x]->refresh();
            }
        }
        for (int i = 0; i < list_c.count; i++) {
            DustEntry* d = (DustEntry*)((char*)&list_c.items[i] + 0x44);
            int iy = (int)(d->x * 0.05f);
            if (iy < 0)
                iy = 0;
            else if (iy >= n_inner)
                iy = n_inner - 1;
            int ix = (int)(d->y * 0.05f);
            if (ix < 0)
                ix = 0;
            else if (ix >= n_outer)
                ix = n_outer - 1;
            rgl::aabb* c = &boxes[iy][ix];
            if (d->x < c->min.x)
                c->min.x = d->x;
            if (d->y < c->min.y)
                c->min.y = d->y;
            if (d->z < c->min.z)
                c->min.z = d->z;
            if (d->x > c->max.x)
                c->max.x = d->x;
            if (d->y > c->max.y)
                c->max.y = d->y;
            if (d->z > c->max.z)
                c->max.z = d->z;
        }
        for (int x = 0; x < n_outer; x++)
            for (int y = 0; y < n_inner; y++)
                boxes[y][x].update();
    }
}

// @ 0x47a800
void Mission::build_interior(int which, rgl::mesh** out, int* n)
{
    Level494* es = &g_game->levels[which];
    MeshVec meshes;
    meshes = ((resmgr*)&g_resmgr)->find_meshes((const rgl::string*)&es->name34);
    if (meshes.size() == 0)
        mb_error("Error unable to find interior mesh %s", es->name34.c_str());
    Entity* e = scene->create_entity(9);
    *n = (int)meshes.size();
    for (int i = 0; i < *n; i++) {
        rgl::mesh* m = ((mesh_x*)meshes[i])->clone();
        out[i] = m;
        e->meshes.push_back(m);
        e->refresh();
    }
    float dist = 100000.0f;
    e->set_distance(&dist);
}

// @ 0x47a980
template void std::vector<MRow56>::resize(size_t, MRow56);

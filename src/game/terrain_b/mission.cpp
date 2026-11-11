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
#include "../../rgl/options.h"   // g_options
#include "../env_row.h"                // EnvRow, g_env_rows (0x5e28f4)
#include <new>
#include "../../rgl/mesh.h"
#include "../../rgl/stream.h"
#include "../tstring.h"
#include "./frustum.h"
#include "./pool.h"
#include "./agent_rec.h"
#include "./mission.h"
#include "./cell_grid.h"

#include "./poly_mesh.h"
#include "./records.h"
#include "./game_data.h"
#include "./terrain.h"
#include "./emit.h"
#include "./scene_file.h"
#include "./tmesh.h"
#include "./face.h"
#include "./tmesh.h"

using rgl::vector4;
using rgl::matrix;

// ---- calls into other objects -----------------------------------------------------------------------------------------
void mb_error(const char* fmt, ...);            // 0x42cc80
extern int g_use_shaders;                       // 0x5e28fc
#include "../../include/clock_table.h"   // g_ticks (the clock table)
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
#include "../../include/view_params.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../view_state.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../include/mesh_parts.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../terrain_a/terrain.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../mission.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../mode_rec.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/iface.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../ptype.h"
#include "../../include/clock_table.h"
#include "../../include/render_flags.h"

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
Mission* g_mission;                                         // 0x5e292c the mission (world) object
extern const int g_nav_param = 2;                  // 0x59132c (rdata)

using rgl::mat3;
struct lod_group {
    char data[0xe0];
    lod_group();   // 0x41c2a0 (the interior record's constructor: tmesh() is the same code)
};
static __forceinline const rgl::string& rs(const tstring& t) { return t; }   // tstring is the 16-byte string the engine's code constructs
template<> void std::vector<PlacedObj>::resize(size_t);   // 0x477ea0 (terrain_b.cpp)
// vector<int>::_Insert_n (0x410610) and vector<rgl::Obj*>::_Insert_n (0x41f720) are the instances the pointer vectors share (rgl/particles
// and rgl/scene): declared here, never instantiated; the views reach them through classes that make the protected member public
namespace std { template<> void vector<int>::_Insert_n(iterator, size_type, const int&); }   // 0x410610
struct IntVecPub : std::vector<int> { using std::vector<int>::_Insert_n; };
namespace rgl { struct Obj; }
namespace std { template<> void vector<rgl::Obj*>::_Insert_n(iterator, size_type, rgl::Obj* const&); }   // 0x41f720
struct ObjPtrVecPub : std::vector<rgl::Obj*> { using std::vector<rgl::Obj*>::_Insert_n; };
__forceinline void MeshVec::insert_n(iterator where, size_t n, rgl::mesh* const& v)
{ ((IntVecPub*)this)->_Insert_n(std::vector<int>::iterator((int*)&*where), n, *(const int*)&v); }
template<> void Pool<MAgent>::reset();          // 0x477f00 (terrain_b.cpp)
template<> void Pool<RecJ>::reset();           // 0x477f90

// @ 0x478830
void rgl::world::init(const float* lo_, const float* hi_, int dimx, int dimy)
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
    cells = new rgl::cell*[n[0]];
    for (int i = 0; i < n[0]; i++) {
        cells[i] = new rgl::cell[n[1]];
        for (int j = 0; j < n[1]; j++)
            cells[i][j].head = 0;
    }
    if (n[0] == 1) {
        lo[0] = 1e12f;
        hi[0] = 1e12f;
        cell_size[0] = 1.0f;
    } else if (n[0] == 2) {
        lo[0] = (hi[0] + lo[0]) * 0.5f;
        hi[0] = 1e12f;
        cell_size[0] = 1e12f - lo[0];
    } else {
        cell_size[0] = (hi[0] - lo[0]) / (float)(n[0] - 2);
    }
    if (n[1] == 1) {
        lo[1] = 1e12f;
        hi[1] = 1e12f;
        cell_size[1] = 1.0f;
    } else if (n[1] == 2) {
        lo[1] = (lo[1] + hi[1]) * 0.5f;
        hi[1] = 1e12f;
        cell_size[1] = 1e12f - lo[1];
    } else {
        cell_size[1] = (hi[1] - lo[1]) / (float)(n[1] - 2);
    }
}

// @ 0x478a10
void rgl::world::insert(int id, const float* lo_, const float* hi_)
{
    int a[2], b[2], c[2];
    to_cell(lo_, a);
    to_cell(hi_, b);
    for (c[0] = a[0]; c[0] <= b[0]; c[0]++)
        for (c[1] = a[1]; c[1] <= b[1]; c[1]++)
            cells[c[0]][c[1]].c.push_back(id);
}

// @ 0x478ad0
rgl::world::world(const float* lo_, const float* hi_, const int* dimx, const int* dimy, const int* v)
    : ents(), ents2(), removed(), lights(), meshes(), view(), emitters()
{
    noise_counter = *v;
    next_id = 1;
    init(lo_, hi_, *dimx, *dimy);
    m30c = vector4(1.0f, 0.0f, 0.0f);
    ground = 0;
    nav = 0;
    hf = 0;
    m31c = vector4(0.0f, 0.0f, 0.0f);
    ambient = vector4(0.0f, 0.0f, 0.0f);
    sky = 0;
}

// @ 0x478c50
void rgl::world::build(PolyMesh* m)
{
    nav = m;
    if (m == 0)
        return;
    for (unsigned i = 0; i < nav->faces.size(); i++) {
        MPoly* r = &::vat(nav->faces, i);
        MVert* verts = &*nav->verts.begin();
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
Light* rgl::world::spawn_wind()
{
    Light* w = new Light();
    lights.push_back(w);
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
            stream_base& hub = g_log;
            hub.w_cstr("WARNING unable to find ground_paint_saved_magic for sco file").w_tstr(&name);
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
        vector4 col = rgl::unpack_color((const unsigned*)&sp->f44);

        float k;
        if (t * 60.0f < 0.0f)
            k = 0.0f;
        else if (t * 60.0f >= 1.0f)
            k = 1.0f;
        else
            k = t * 60.0f;

        // ---- first wind object: a horizontal "sheet" whose normal comes from the hourly table
        f65b0 = scene->spawn_wind();
        f65b0->type = 2;

        float hour = (float)fmod((double)((float)g_ticks[1] * 1e-5f), 24.0);
        bool day = (4.0f <= hour && hour < 20.0f);

        int slot = (int)((float)(g_ticks[g_game->f13b18] - *(__int64*)&g_game->f13b20) * 1e-5f) % 24;
        {
            vector4 dir = sp->get_dir(slot, day);
            vector4 p(dir.x * -10000.0f, dir.y * -10000.0f, dir.z * -10000.0f);
            f65b0->dir = dir;
            f65b0->pos = p;
        }
        f65b0->range = 20000.0f;
        f65b0->flags = 0;

        if (lv->flags & 1) {
            // flat variant: the wind object just sits at the origin, the cell gets a uniform colour
            f65b0->color = vector4(0.0f, 0.0f, 0.0f);
            rgl::world* sc = scene;
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
            sc->ambient = c;
            sc->ambient.w = 1.0f;
            f65b0->base_color = f65b0->color;
            return;
        }

        // ---- variant B: an anisotropically scaled wind gust
        float p = (float)pow((double)(1.0f - k), 1.5);
        vector4 u(p * sp->v20[0], p * sp->v20[1], p * sp->v20[2]);
        if ((sp->f10 & 0x10000000) || sqrtf(u.x * u.x + (u.y * u.y + u.z * u.z)) < 0.1f)
            f65b0->flags |= 2;
        if (!g_use_shaders) {
            u.x = sqrtf(u.x);
            u.y = sqrtf(u.y);
            u.z = sqrtf(u.z);
        }
        float cx = col.x * k;
        float cy = col.y * k;
        f65b0->color = u;
        col.z *= k;
        f65b0->base_color = f65b0->color;

        // the cell colour: the unpacked table colour faded by k, offset by the table base position
        u.x = cx * 0.2f + sp->base[0];
        u.y = cy * 0.2f + sp->base[1];
        u.z = col.z * 0.2f + sp->base[2];
        if (!g_use_shaders) {
            u.x = sqrtf(u.x);
            u.y = sqrtf(u.y);
            u.z = sqrtf(u.z);
        }
        scene->ambient = u;
        scene->ambient.w = 1.0f;
    }

    // ---- second wind object: the table's direction rotated by 3.5 rad, normalised, half-size
    {
    f65b4 = scene->spawn_wind();
    f65b4->type = 2;

    float hour2 = (float)fmod((double)((float)g_ticks[1] * 1e-5f), 24.0);
    bool day2 = (4.0f <= hour2 && hour2 < 20.0f);
    int slot2 = (int)((float)(g_ticks[g_game->f13b18] - *(__int64*)&g_game->f13b20) * 1e-5f) % 24;
    vector4 d2 = sp->get_dir(slot2, day2);
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
    f65b4->dir = rd;

    f65b4->color = *(vector4*)sp->base;
    f65b4->color.x *= 0.5f;
    f65b4->color.y *= 0.5f;
    f65b4->color.z *= 0.5f;
    f65b4->base_color = f65b4->color;
    f65b4->pos = vector4(rd.x * -10000.0f, rd.y * -10000.0f, rd.z * -10000.0f);
    f65b4->range = 20000.0f;
    f65b4->flags = 1;
    }
}

// @ 0x479930
template std::vector<MRow56>::iterator std::vector<MRow56>::erase(std::vector<MRow56>::iterator, std::vector<MRow56>::iterator);

// @ 0x479980
void rgl::world::build_faces(NavData* map)
{
    if (ground != 0)
        delete ground;
    ground = map;

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
        if (objs.size() < objs.capacity()) {
            *objs._Mylast = (rgl::Obj*)f;
            objs._Mylast++;
        } else {
            objs.insert(objs.end(), (rgl::Obj*)f);
        }
        add_tree((shape_base*)f);
        c->obj = f;
    }

    for (unsigned i = 0; i < map->verts.size(); i++)
        map->verts[i].n = vector4(0.0f, 0.0f, 0.0f);

    // the three products are read before any store and the cell is re-fetched each step: a vertex may overlap a
    // cell record, so the order of these loads is behaviour
    for (unsigned i = 0; i < map->cells.size(); i++) {
        for (int j = 0; j < map->cells[i].npts; j++) {
            NavCell* c = &map->cells[i];
            float x = c->trans.x * c->scale;
            float y = c->trans.y * c->scale;
            float z = c->trans.z * c->scale;
            NavVert* v = &map->verts[c->ids[j]];
            v->n.x = x + v->n.x;
            v->n.y = y + v->n.y;
            v->n.z = z + v->n.z;
        }
    }

    for (unsigned i = 0; i < map->verts.size(); i++) {
        vector4& n = map->verts[i].n;
        float len = (float)sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
        if (!(len <= 1e-10f)) {
            float inv = 1.0f / len;
            n.x *= inv;
            n.y *= inv;
            n.z *= inv;
        } else {
            n.x = 0.0f;
            n.y = 1.0f;
            n.z = 0.0f;
        }
    }
}

// @ 0x479d20
rgl::psys* EmitTable::spawn(int* idx, const vector4* pos, const int* owner)
{
    if (*idx < 0 || *idx >= count)
        return 0;
    rgl::psys* e = new rgl::psys(*owner);
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
        body_part* rs = ty->boxes[c].res;
        if (rs) {
            shape_base* o = rs->clone_shape();
            o->transform(&e->m);
            o->or_m8(0x200);
            world->reg_shape(o);
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
    Level494* cs = g_game->levels + f5df4;
    NavData* core_ = 0;
    body_part* r = 0;
    vector4 hi, lo;
    if (g_game->levels[which].flags & 0x100) {
        *(V2*)&lo = V2(terrain.min_x - 1.0f, terrain.min_y - 1.0f);
        *(V2*)&hi = V2(terrain.max_x + 1.0f, terrain.max_y + 1.0f);
    } else {
        core_ = (NavData*)new NavCore;   // 0x478150 (NavData() is the same constructor, ICF-folded)
        tstring& nm = cs->name44;
        r = (body_part*)g_resmgr.get_object60(nm);
        if (r == 0) {
            const char* s = nm.buf;
            if (!s) s = (const char*)&nm.buf;
            mb_error("Unable to find body %s", s);
        }
        r->shape->get_max(&hi);
        *(V2*)&hi = V2(hi.x + 1.0f, hi.y + 1.0f);
        r->shape->get_min(&lo);
        *(V2*)&lo = V2(lo.x - 1.0f, lo.y - 1.0f);
    }
    int lod1 = LOG2UP((hi.x - lo.x) * (1.0f / 3.0f));
    int lod2 = LOG2UP((hi.y - lo.y) * (1.0f / 3.0f));
    scene = new rgl::world(&lo.x, &hi.x, &lod1, &lod2, &g_nav_param);
    if ((g_game->levels[which].flags & 0x100) == 0) {
        scene->build_faces(core_);
        shape_base* o = r->clone_shape();
        matrix m;
        o->transform(identity_pos(&m));
        scene->reg_shape(o);
    }
}

// @ 0x47a0c0
void Mission::post_spawn(MAgent* a)
{
    EnvRow* row = &g_env_rows[cam];
    float sz = row->v20[2] * 0.4f;
    vector4 sc;
    sc.x = row->base[0] + row->v20[0] * 0.4f;
    sc.y = row->base[1] + row->v20[1] * 0.4f;
    sc.z = row->base[2] + sz;
    vector4 pos;
    pos.x = 0.0f;
    pos.y = 0.0f;
    pos.z = 0.1f;
    int idx;
    int owner = 0;
    if (g_options.i20 == 0)
        return;
    if ((g_game->levels[f5df4].flags & 1) == 0 && f6608 != 4) {
        if (fog_dense == 0) {
            idx = g_emit_defs.used[4];
            pos.z = 0.4f;
        } else if (fog_dense == 1) {
            idx = g_emit_defs.used[6];
        } else {
            goto layers;
        }
        if (idx >= 0) {
            owner = 0;
            rgl::psys* e = g_emit_defs.spawn(&idx, &pos, &owner);
            if (e) {
                a->f250->add_emitter(e);
                a->f1ac = (int)e;
                e->curve[2].v *= sc.x;       // the r / g / b curves' end values (curve4 r 0x38, g 0x48, b 0x58: v0 at +4, v1 at +0xc)
                e->curve[3].v *= sc.x;
                e->curve[4].v *= sc.y;
                e->curve[5].v *= sc.y;
                e->curve[6].v *= sc.z;
                e->curve[7].v *= sc.z;
            }
        }
    }
layers:
    if (g_options.i20 != 0) {
        pos.x = 0.0f;
        pos.z = 0.0f;
        pos.y = 0.15f;
        if (a->f10 == 1)
            pos.y = 0.65f;
        idx = g_emit_defs.used[7];
        owner = 0;
        rgl::psys* e0 = g_emit_defs.spawn(&idx, &pos, &owner);
        if (e0) {
            e0->height = terrain.base_z;
            a->f250->add_emitter(e0);
            a->f1b0 = (int)e0;
        }
        idx = g_emit_defs.used[8];
        owner = 0;
        rgl::psys* e1 = g_emit_defs.spawn(&idx, &pos, &owner);
        if (e1) {
            e1->height = terrain.base_z + 0.05f;
            a->f250->add_emitter(e1);
            a->f1b4 = (int)e1;
        }
        idx = g_emit_defs.used[9];
        pos.y = pos.y + 0.6f;
        owner = 0;
        rgl::psys* e2 = g_emit_defs.spawn(&idx, &pos, &owner);
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
            c->lists[i][j] = ((rgl::mesh*)lists[i][j])->clone();
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
            emitted[y][x] = 0;
            boxes[y][x].reset();
        }
    }
    if (g_options.i34 >= 10) {
        n_inner = (int)ceil(wx * 0.05f);
        n_outer = (int)ceil(wy * 0.05f);
        if (n_inner > 0x30)
            n_inner = 0x30;
        if (n_outer > 0x30)
            n_outer = 0x30;
        for (int x = 0; x < n_outer; x++) {
            for (int y = 0; y < n_inner; y++) {
                gridA[y][x] = world->create_entity(9);
                rgl::mesh* m = new rgl::mesh;
                if (g_use_shaders != 0 && g_options.i7c == 2)
                    m->res = g_resmgr.get_material(rs(tstring("grass")));
                else
                    m->res = g_resmgr.get_material(rs(tstring("grass_no_shadow")));
                m->flags |= 0x10000000;
                ((MeshPush*)&gridA[y][x]->meshes)->push(m);
                gridA[y][x]->recalc();
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
    rgl::world* sc = scene;
    Level494* es = &g_game->levels[which];
    MeshVec meshes;
    meshes = g_resmgr.find_meshes(&es->name34);
    if (meshes.size() == 0)
        mb_error("Error unable to find interior mesh %s", es->name34.c_str());
    rgl::entity* e = sc->create_entity(9);
    *n = (int)meshes.size();
    rgl::mesh** o = out;
    for (int i = 0; i < *n; i++) {
        rgl::mesh* m = ((rgl::mesh*)meshes[i])->clone();
        *o++ = m;
        e->meshes.push_back(m);
        e->recalc();
    }
    float dist = 100000.0f;
    e->set_fbc(&dist);
}

// @ 0x47a980
template void std::vector<MRow56>::resize(size_t, MRow56);

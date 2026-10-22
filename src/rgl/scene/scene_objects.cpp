// scene_objects.cpp (0x4205b0..0x4218e0): the scene-object bookkeeping around the world and its entities: LOD
// selection, the cell/ref pools, the world's entity/mesh/emitter teardown and cell insertion, the entity base
// destructor and clear(), the emitter defaults, the effect definition's destructor and the strategic entity's
// small members. Migrated from slices s0041ffe0 (0x4205b0..0x4207e0), s00420960 (0x420960..0x420ec0),
// s00421110 (0x421110..0x421a50 minus 0x4218e0, 0x421990, 0x4219c0) and s00421aa0's FxDef::clear_defs.
// Boundary: start 0x4205b0 (entity::render 0x420310 calls lod_mesh2::select forward: high); end 0x4218e0
// (StrategicEntity::set_color 0x4218a0 tail-jumps to FxDef::set_color 0x4218e0 forward: high). Inside the range no
// forward call crosses a function, so the TU map's single object is kept.
#include <vector>
#include <list>
#include <new>
#include <stddef.h>
#include "../rgl_string.h"
#include "../rgl_math.h"
#include "../resource.h"
#include "./scene_math.h"
#include "./frustum.h"
#include "./scene.h"
#include "./emitter.h"
#include "./scene_entity.h"
#include "./world.h"
#include "./fx.h"

using namespace rgl;

extern __int64 g_time[];                                        // 0x5e2948 clock table (64-bit ticks)

namespace rgl {

// @ 0x4205b0
void lod_mesh2::select(const float* d)
{
    cur = 0;
    if (*d < 0.1f && count > 1) {
        cur = 1;
        if (*d < 0.05f && count > 2) {
            cur = 2;
            if (*d < 0.025f && count > 3) {
                cur = 3;
                if (*d < 0.0125f && count > 4)
                    cur = 4;
            }
        }
    }
}

} // namespace rgl

// @ 0x420650
template void std::vector<Obj*>::push_back(Obj* const&);

namespace rgl {

// List-backed object pools: a std::list of blocks plus a free list threaded through the cells of the newest block
// (two instantiations: 12-byte nodes and 16-byte refs).
// @ 0x4206a0
template <> void pool<cellnode>::grow()
{
    cellnode* blk = new cellnode[count];
    chunks.push_back(blk);
    for (int i = 0; i < count - 1; i++)
        blk[i].next = &blk[i + 1];
    ((int*)blk)[count * 3 - 3] = 0;
    freelist = blk;
}
// @ 0x420720
template <> void pool<cellref>::grow()
{
    cellref* blk = new cellref[count];
    chunks.push_back(blk);
    for (int i = 0; i < count - 1; i++)
        blk[i].next = &blk[i + 1];
    ((int*)blk)[count * 4 - 4] = 0;
    freelist = blk;
}

// @ 0x4207a0
void world::add_pending(entity* o)
{
    if (o) {
        o->id = next_id;
        next_id++;
        ents2.push_back(o);
        o->owner = this;
    }
}

// @ 0x4207e0
void world::clear_ents()
{
    for (std::vector<entity*>::iterator it = ents.begin(); it < ents.end(); ++it)
        delete *it;
    ents.resize(0, 0);
    for (std::vector<entity*>::iterator it = ents2.begin(); it < ents2.end(); ++it)
        delete *it;
    ents2.resize(0, 0);
    for (unsigned i = 0; i < lights.size(); i++)
        operator delete((void*)lights[i]);
    lights.resize(0, 0);
    for (unsigned i = 0; i < meshes.size(); i++) {
        mesh* p = meshes[i];
        if (p)
            delete p;
    }
    meshes.resize(0, 0);
    for (unsigned i = 0; i < emitters.size(); i++) {
        psys* r = emitters[i];
        if (r)
            delete r;
    }
    emitters.resize(0, 0);
    sky = 0;
    live = 0;
    next_id = 1;
}

// @ 0x420960
void entity::clear()
{
    for (unsigned i = 0; i < meshes.size(); i++) {
        delete meshes[i];
    }
    meshes.resize(0, 0);
    for (unsigned i = 0; i < lods.size(); i++) {
        delete lods[i];
    }
    lods.resize(0, 0);
}

typedef std::vector<int> int_vector;

// @ 0x420a70
void world::clear_cells()
{
    for (std::vector<Obj*>::iterator it = objs.begin(); it < objs.end(); ++it)
        delete *it;
    objs.resize(0, 0);
    for (int i = 0; i < nrows; i++) {
        for (int j = 0; j < ncols; j++) {
            cell* c = &rows[i][j];
            c->a.resize(0, 0);
            c->b.resize(0, 0);
            rows[i][j].c.~int_vector();
        }
    }
    nodes.link_chunks();
    refs.link_chunks();
}

// the emitter's defaults: every field re-initialised in turn, then the particle vector emptied
// @ 0x420b40
void psys::init()
{
    mat4 t1;
    E100 e;
    cache = 0;
    res = 0;
    ff0 = 0;
    v184 = vector4(0.0f, 0.0f, 1.0f);
    f194 = 1.0f;
    f198 = 0; spawn_left = 0; f1a0 = 0; f1a4 = 0;
    fec = 0;
    m134 = *(mat4*)identity_pos(&t1);
    mf4 = *(mat4*)identity_pos(&t1);
    v174 = vector4(0.0f, 0.0f, 0.0f);
    va4 = vector4(1.0f, 0.0f, 0.0f);
    vb4 = vector4(0.0f, 0.0f, 1.0f);
    fe8 = 1;
    half = pair2(0.5f, 0.5f);
    curve[0] = pair2(0.0f, 1.0f);
    curve[1] = pair2(1.0f, 0.0f);
    curve[2] = pair2(0.0f, 1.0f);
    curve[3] = pair2(1.0f, 1.0f);
    curve[4] = pair2(0.0f, 1.0f);
    curve[5] = pair2(1.0f, 1.0f);
    curve[6] = pair2(0.0f, 1.0f);
    curve[7] = pair2(1.0f, 1.0f);
    curve[8] = pair2(0.0f, 1.0f);
    curve[9] = pair2(1.0f, 1.0f);
    vc4 = vector4(0.0f, 0.0f, -9.8f);
    fd4 = 1.0f;
    quad[0] = pair2(1.0f, 1.0f);
    quad[1] = pair2(0.0f, 1.0f);
    quad[2] = pair2(0.0f, 0.0f);
    quad[3] = pair2(1.0f, 0.0f);
    f20 = 0.0f; f1c = 1.0f; f24 = 0.5f;
    f8 = 0;
    tag = -1; radius = 0.0f; f1b0 = 0; f1b4 = 0;
    fd8 = 1.0f; fdc = 1.0f; fe0 = 1.0f;
    particles.resize(0, e);
}

} // namespace rgl

// @ 0x420e90
void FxElem::add_to_first(rgl::psys* o)
{
    if (psys.size() != 0)
        psys[0]->kids.push_back(o);
}

// The clear() of the element array: per element the particle meshes (+0x19c) and lod records (+0x1b0), then the
// definition's own two lists (+0x20 and +0x30).
// @ 0x420ec0
void FxDef::clear_defs()
{
    for (int i = 0; i < count; i++) {
        for (std::vector<rgl::mesh*>::iterator it = elems[i].psys.begin(); it != elems[i].psys.end(); ++it)
            delete *it;
        elems[i].psys.resize(0, 0);
    }
    for (std::vector<rgl::mesh*>::iterator it = psys.begin(); it < psys.end(); ++it)
        delete *it;
    psys.resize(0, 0);
    for (int i = 0; i < count; i++) {
        for (std::vector<rgl::lod_mesh2*>::iterator it = elems[i].groups.begin(); it < elems[i].groups.end(); ++it)
            delete *it;
        elems[i].groups.resize(0, 0);
    }
    for (std::vector<rgl::lod_mesh2*>::iterator it = groups.begin(); it < groups.end(); ++it)
        delete *it;
    groups.resize(0, 0);
}

namespace rgl {

// An attached view / camera is brought up to date for the entities whose id falls in its share (id % div == rem):
// each one inside the view frustum draws itself.
// @ 0x421110
void world::render_part(scene* view, int rem, int div)
{
    view->owner = this;
    view->ambient = ambient;
    view->fr.build_planes();
    g_scene_reg.register_obj(view);
    render_info flag;
    flag.valid = 0;
    for (std::vector<entity*>::iterator it = ents.begin(); it < ents.end(); ++it) {
        entity* e = *it;
        if (e->id % div == rem) {
            vector4 c;
            c.x = e->local.f.x * e->vc4.y + e->local.u.x * e->vc4.z + e->local.s.x * e->vc4.x + e->local.o.x;
            c.y = e->local.s.y * e->vc4.x + e->local.f.y * e->vc4.y + e->local.u.y * e->vc4.z + e->local.o.y;
            c.z = e->local.s.z * e->vc4.x + e->local.f.z * e->vc4.y + e->local.u.z * e->vc4.z + e->local.o.z;
            float r = e->get_radius();
            int n = view->fr.nplanes;
            int ok = 1;
            for (int i = 0; i < n; ++i) {
                if (!ok) break;
                ok = view->fr.planes[i].in_front(&c, r);
            }
            if (!ok) continue;
            (*it)->render(view, &flag);
        }
    }
    for (std::vector<entity*>::iterator it = ents2.begin(); it < ents2.end(); ++it) {
        entity* e = *it;
        if (e->id % div == rem) {
            vector4 c;
            c.x = e->local.f.x * e->vc4.y + e->local.u.x * e->vc4.z + e->local.s.x * e->vc4.x + e->local.o.x;
            c.y = e->local.f.y * e->vc4.y + e->local.u.y * e->vc4.z + e->local.s.y * e->vc4.x + e->local.o.y;
            c.z = e->local.f.z * e->vc4.y + e->local.u.z * e->vc4.z + e->local.s.z * e->vc4.x + e->local.o.z;
            float r = e->get_radius();
            int n = view->fr.nplanes;
            int ok = 1;
            for (int i = 0; i < n; ++i) {
                if (!ok) break;
                ok = view->fr.planes[i].in_front(&c, r);
            }
            if (!ok) continue;
            (*it)->render(view, &flag);
        }
    }
}

// @ 0x421390
void world::reset()
{
    clear_ents();
    clear_cells();
    if (ground) {
        delete ground;
        ground = 0;
    }
    fb4 = 0;
    if (p64) {
        delete p64;
        p64 = 0;
    }
    loaded = false;
}

// @ 0x4213f0
entity::~entity()
{
    if (shape) delete shape;
    entity::clear();
    if (p138) { delete p138; p138 = 0; }
    if (p13c) { delete p13c; p13c = 0; }
}
// 0x4214e0 is the compiler-generated scalar deleting destructor ??_Gentity (calls ~entity above, then operator
// delete if bit 0 of the flag is set); it has no source of its own.

// @ 0x4214c0
string mesh_entity::type_name() { return string("mesh_entity"); }

// @ 0x421500
string collision_entity::type_name() { return string("collision_entity"); }

// the default rigid-body virtuals of a collision entity (a non-dynamic entity has no velocity or impulse response)
// @ 0x421520
vector4* collision_entity::point_velocity(vector4* out, const vector4* p)
{
    out->x = 0.0f; out->y = 0.0f; out->z = 0.0f;
    return out;
}
// @ 0x421540
void collision_entity::apply_impulse(const vector4* p, const vector4* j) {}
// @ 0x421550
float collision_entity::impulse_response(const vector4* p, const vector4* n) { return 0.0f; }

// @ 0x421560
void world::insert(entity* o, int x, int y)
{
    cellnode* n = nodes.alloc();
    n->obj = o;
    n->prev = 0;
    n->next = rows[x][y].head;
    if (rows[x][y].head) rows[x][y].head->prev = n;
    rows[x][y].head = n;
    cellref* r = refs.alloc();
    r->node = n;
    r->x = x;
    r->y = y;
    r->next = (cellref*)o->refs;
    o->refs = r;
}

// @ 0x421600
void world::insert_obj(entity* o)
{
    int hi[2];
    int lo[2];
    to_cell((const float*)o->get_v9c(), lo);
    to_cell((const float*)o->get_vac(), hi);
    for (int x = lo[0]; x <= hi[0]; ++x)
        for (int y = lo[1]; y <= hi[1]; ++y)
            insert(o, x, y);
}

} // namespace rgl

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

namespace rgl {

// @ 0x421720
strategic_entity::strategic_entity()
    : f2a0(0), t2a8(g_time[0]), f2b0(0), t2b8(g_time[0]), f2c0(0)
{
    f168 = 0;
    matrix tmp;
    *(matrix*)&local = *identity_pos(&tmp);
    f160 = 0.5f;
    f164 = 0.5f;
    last_pos = vector4(-10000.0f, -10000.0f, -10000.0f);
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
string strategic_entity::type_name() { return string("strategic_entity"); }

// @ 0x421890
float strategic_entity::get_radius() { return radius; }

// @ 0x4218a0
void strategic_entity::set_color(int c)
{
    for (std::vector<mesh*>::iterator it = meshes.begin(); it < meshes.end(); ++it)
        (*it)->color = c;
    if (fx) fx->set_color(c);
}

} // namespace rgl

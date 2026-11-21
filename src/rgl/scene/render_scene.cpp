// render_scene.cpp (0x429670..0x42a090): the transposed-rotation helper, the skinning iterator and group loop of
// FxDef, the render scene's constructor, and world::render_scene (entities, meshes and particle systems culled
// against the view frustum, plus the water-reflection pass). Migrated from slices s004291c0 (0x429670, 0x4296d0,
// 0x4296f0, 0x429740) and s00429890 (0x429890).
// Boundary: start 0x429670 (FxDef::skin 0x4291c0 calls mat4::rot_t and skin_iter::operator+ forward: high); end
// 0x42a090 is the TU map's (first byte of strategic_entity::render, whose class differs: low). No forward call or
// shared literal ties 0x429740 / 0x429890 to the helpers before them, so the single object is kept.
#include <vector>
#include "../../game/config/config_block.h"   // g_cfg
#include <new>
#include "../rgl_string.h"
#include "../rgl_math.h"
#include "../resource.h"
#include "./scene_math.h"
#include "./frustum.h"
#include "../../game/render_batch.h"   // g_batch (0x5fe898): register_scene 0x412040
#include "./scene.h"
#include "./emitter.h"
#include "./scene_entity.h"
#include "./world.h"
#include "./fx.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../include/frustum.h"
#include "../../include/keyframes.h"
#include "../../include/mat4.h"
#include "../../include/mesh_prims.h"
#include "../../include/vec.h"
#include "../../include/mesh_parts.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../mesh.h"      // (listed directly: the resolution audit scans only the source's own includes)

using namespace rgl;

// logging globals of another object
#include "../stream.h"                                  // text_stream g_debug_log (0x5a9f30)


namespace rgl {

// @ 0x429670
mat3* matrix::rot_t(mat3* out) const
{
    mat3 t;
    t.s.x = s.x; t.s.y = f.x; t.s.z = u.x;
    t.f.x = s.y; t.f.y = f.y; t.f.z = u.y;
    t.u.x = s.z; t.u.y = f.z; t.u.z = u.z;
    *out = t;
    return out;
}

} // namespace rgl

// @ 0x4296d0
skin_iter skin_iter::operator+(int n) const
{
    skin_iter t = *this;
    t.cur += n;
    return t;
}

// @ 0x4296f0
void FxDef::skin_group(lod_mesh* g)
{
    if (g->cur < g->count) {
        for (std::vector<mesh*>::iterator it = g->lists[g->cur].begin(); it < g->lists[g->cur].end(); it++)
            skin(*it, 0);
    }
}

namespace rgl {

// @ 0x429740
scene::scene()
{
    mask = 1;
    vector4 t(1.0f, 1.0f, 1.0f);
    ambient = t;
    rt = 0;
    refl = 0;
    clip_on = 0;
    clip_plane[0] = 0.0f;
    clip_plane[1] = 0.0f;
    clip_plane[2] = 0.0f;
    clip_plane[3] = 0.0f;
    lights_active = 0;
    depth_pass = 0;
    mask_pass = 0;
    clear_color = 0;
    ambient.w = 1.0f;
    depth_target = 0;
    owner = 0;
    flags = 0;
    cull_flip = 0;
    fog = 0;
    use_alt_vp = 0;
    fog_invert = 0;
    msaa = 0;
    active = true;
    reset();
}

// the frustum test inlined in the original: every plane must see the bounding sphere
static __forceinline int in_frustum(scene* sc, const vector4* p, float r)
{
    int inside = 1;
    for (int i = 0; i < sc->fr.nplanes; i++) {
        if (!inside)
            break;
        inside = sc->fr.planes[i].in_front(p, r);
    }
    return inside;
}

static __forceinline int in_frustum_n(scene* sc, const vector4* p, float r)
{
    int inside = 1;
    int n = sc->fr.nplanes;
    for (int i = 0; i < n; i++) {
        if (!inside)
            break;
        inside = sc->fr.planes[i].in_front(p, r);
    }
    return inside;
}

// world position of an entity's bounding-sphere centre. The original inlines this three times and the sums are
// grouped differently per copy and per component (float addition is not associative, so each copy is spelled out):
// the ents loop (0x4298f2: x = f,u,s; y and z = f,s,u), the ents2 loop (0x429a35: f,u,s) and the reflection loop
// (0x429f80: x = u,f,s; y and z = u,s,f).
static __forceinline void centre(vector4& c, entity* e)
{
    const mat4& l = e->local;
    const vector4& v = e->center;
    float x = l.f.x * v.y;
    x = x + l.u.x * v.z;
    x = x + v.x * l.s.x;
    c.x = x + l.o.x;
    float y = l.f.y * v.y;
    y = y + l.s.y * v.x;
    y = y + l.u.y * v.z;
    c.y = y + l.o.y;
    float z = l.f.z * v.y;
    z = z + l.s.z * v.x;
    z = z + l.u.z * v.z;
    c.z = z + l.o.z;
}

static __forceinline void centre2(vector4& c, entity* e)
{
    const mat4& l = e->local;
    const vector4& v = e->center;
    float x = l.f.x * v.y;
    x = x + l.u.x * v.z;
    x = x + v.x * l.s.x;
    c.x = x + l.o.x;
    float y = l.f.y * v.y;
    y = y + l.u.y * v.z;
    y = y + l.s.y * v.x;
    c.y = y + l.o.y;
    float z = l.f.z * v.y;
    z = z + l.u.z * v.z;
    z = z + l.s.z * v.x;
    c.z = z + l.o.z;
}

static __forceinline void centre3(vector4& c, entity* e)
{
    const mat4& l = e->local;
    const vector4& v = e->center;
    float x = l.u.x * v.z;
    x = x + l.f.x * v.y;
    x = x + l.s.x * v.x;
    c.x = x + l.o.x;
    float y = l.u.y * v.z;
    y = y + l.s.y * v.x;
    y = y + l.f.y * v.y;
    c.y = y + l.o.y;
    float z = l.u.z * v.z;
    z = z + l.s.z * v.x;
    z = z + l.f.z * v.y;
    c.z = z + l.o.z;
}

// @ 0x429890
void world::render_scene(scene* sc, bool reflect)
{
    sc->owner = this;
    sc->ambient = ambient;
    sc->fr.build_planes();
    g_batch.register_scene(sc);
    for (unsigned i = 0; i < meshes.size(); i++)
        ((mesh*)meshes[i])->clear_geometry();
    render_info info;
    info.valid = 0;
    for (std::vector<entity*>::iterator it = ents.begin(); it < ents.end(); ++it) {
        vector4 c;
        centre(c, *it);
        float r = (*it)->get_radius();
        if (in_frustum_n(sc, &c, r))
            (*it)->render(sc, &info);
    }
    for (std::vector<entity*>::iterator it = ents2.begin(); it < ents2.end(); ++it) {
        vector4 c;
        centre2(c, *it);
        float r = (*it)->get_radius();
        if (in_frustum_n(sc, &c, r))
            (*it)->render(sc, &info);
    }
    for (std::vector<psys*>::iterator it = emitters.begin(); it != emitters.end(); ++it) {
        psys* p = *it;
        if (in_frustum(sc, &p->world.o, p->radius))
            p->update(sc);
    }
    for (std::vector<mesh*>::iterator it = meshes.begin(); it < meshes.end(); ++it) {
        (*it)->slots[0].f28 = 1;
        (*it)->slots[1].f28 = 1;
        mat3 id;
        mat4 m;
        *(mat3*)&m = *identity3(&id);
        m.o = vector4(0.0f, 0.0f, 0.0f);
        mesh_render(*it, sc, m);
    }
    if (!reflect || !info.valid)
        return;
    if (sc->refl == 0) {
        scene* r = new scene;
        sc->refl = r;
        sc->refl->mask = sc->mask;
        sc->refl->vp_local.X = 0;
        sc->refl->vp_local.Y = 0;
        sc->refl->vp_local.Width = 0x100;
        sc->refl->vp_local.Height = 0x100;
        sc->refl->vp_local.MinZ = 0.0f;
        sc->refl->vp_local.MaxZ = 1.0f;
        sc->refl->clip_on = 1;
        sc->refl->clip_plane[0] = 0.0f;
        sc->refl->clip_plane[1] = 0.0f;
        sc->refl->clip_plane[2] = 1.0f;
        sc->refl->clip_plane[3] = -(info.depth - 0.25f);
        sc->refl->flags = sc->flags;
        frustum f = sc->fr;
        sc->refl->fog_invert = 1;
        sc->refl->msaa = 0;
        sc->refl->fr = f;
        sc->rt = g_resmgr.get_render_target(0x100, 0x100, 0x16, 1, 0);
        if (sc->rt == 0)
            return;
        sc->rt->flags = 0x300000;
    }
    if (!sc->rt->loaded)
        return;
    sc->refl->reset();
    if (g_cfg.enable_edit_mode) {
        stream_base& log = g_debug_log;
        log.w_cstr("RENDERING REFLECTIONS;");
    }
    if (!g_cfg.fake_reflections)
        sc->refl->flags |= 0x200;
    matrix cam = sc->fr.m;
    cam.o.z = (info.depth + info.depth) - cam.o.z;
    cam.s.z = -cam.s.z;
    cam.u.z = -cam.u.z;
    cam.f = vector4(-cam.f.x, -cam.f.y, -cam.f.z);
    sc->refl->fr.m = cam;
    sc->refl->fr.build_planes();
    if (sc->rt == 0)
        return;
    sc->refl->ambient = ambient;
    sc->refl->target = sc->rt;
    sc->refl->owner = this;
    sc->refl->fog = &view;
    sc->refl->list = (const ptr_list&)lights;
    {
    render_info rinfo;
    rinfo.valid = 0;
    if (g_cfg.fake_reflections) {
        if (sky)
            sky->render(sc->refl, &rinfo);
        return;
    }
    for (std::vector<entity*>::iterator it = ents.begin(); it < ents.end(); ++it) {
        vector4 c;
        centre3(c, *it);
        float r = (*it)->get_radius();
        if (in_frustum_n(sc->refl, &c, r))
            (*it)->render(sc->refl, &rinfo);
    }
    }
}

} // namespace rgl

// render_scene.cpp (0x429670..0x42a090): the transposed-rotation helper, the skinning iterator and group loop of
// FxDef, the render scene's constructor, and world::render_scene (entities, meshes and particle systems culled
// against the view frustum, plus the water-reflection pass). Migrated from slices s004291c0 (0x429670, 0x4296d0,
// 0x4296f0, 0x429740) and s00429890 (0x429890).
// Boundary: start 0x429670 (FxDef::skin 0x4291c0 calls mat4::rot_t and skin_iter::operator+ forward: high); end
// 0x42a090 is the TU map's (first byte of strategic_entity::render, whose class differs: low). No forward call or
// shared literal ties 0x429740 / 0x429890 to the helpers before them, so the single object is kept.
#include <vector>
#include <new>
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
#include "../../include/logger.h"                       // Logger (shared with game/terrain_b/scene_file.h)
extern char g_log_obj[];                                // 0x5a9f30 (a Logger)
extern int g_log_on;                                    // 0x5a9fb0
extern int g_simple_reflect;                            // 0x5a9fc4: the reflection pass renders only the backdrop entity

namespace rgl {

// @ 0x429670
mat3* mat4::rot_t(mat3* out) const
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
void FxDef::skin_group(lod_mesh2* g)
{
    if (g->cur < g->count) {
        for (mesh** it = g->lists[g->cur].v._Myfirst; it < g->lists[g->cur].v._Mylast; it++)
            skin(*it, 0);
    }
}

namespace rgl {

// @ 0x429740
scene::scene()
    : f0(0), f4(0), f8(0), fc(0), f10(0), f14(1.0f), f18(0), f1c(0), f20(0), f24(0), f28(0), f2c(1.0f)
{
    vector4* a = &ambient;
    *a = vector4(1.0f, 1.0f, 1.0f);
    mask = 1;
    rt = 0;
    refl = 0;
    f161 = 0;
    f16c = 0;
    f170 = 0;
    f174 = 0;
    f178 = 0;
    f160 = 0;
    f162 = 0;
    f164 = 0;
    f190 = 0;
    ambient.w = 1.0f;
    f15c = 0;
    owner = 0;
    flags = 0;
    f168 = 0;
    f1b0 = 0;
    f1a5 = 0;
    f169 = 0;
    f16a = 0;
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

// world position of an entity's bounding-sphere centre
static __forceinline void centre(vector4& c, entity* e)
{
    c.x = e->local.s.x * e->vc4.x + (e->local.f.x * e->vc4.y + e->local.u.x * e->vc4.z) + e->local.o.x;
    c.y = e->local.s.y * e->vc4.x + (e->local.f.y * e->vc4.y + e->local.u.y * e->vc4.z) + e->local.o.y;
    c.z = e->local.s.z * e->vc4.x + (e->local.f.z * e->vc4.y + e->local.u.z * e->vc4.z) + e->local.o.z;
}

// @ 0x429890
void world::render_scene(scene* sc, bool reflect)
{
    sc->owner = this;
    sc->ambient = ambient;
    sc->fr.build_planes();
    g_scene_registry.register_scene(sc);
    for (unsigned i = 0; i < meshes.size(); i++)
        ((mesh_ext*)meshes[i])->clear_geometry();
    render_info info;
    info.valid = 0;
    for (entity** it = ents._Myfirst; it < ents._Mylast; ++it) {
        float r;
        vector4 c;
        centre(c, *it);
        r = (*it)->get_radius();
        if (in_frustum_n(sc, &c, r))
            (*it)->render(sc, &info);
    }
    for (entity** it = ents2._Myfirst; it < ents2._Mylast; ++it) {
        float r;
        vector4 c;
        centre(c, *it);
        r = (*it)->get_radius();
        if (in_frustum(sc, &c, r))
            (*it)->render(sc, &info);
    }
    for (std::vector<psys*>::iterator it = emitters.begin(); it != emitters.end(); ++it) {
        psys* p = *it;
        if (in_frustum_n(sc, &p->mf4.o, p->radius))
            p->update(sc);
    }
    for (unsigned i = 0; i < meshes.size(); i++) {
        meshes[i]->slots[0].f28 = 1;
        meshes[i]->slots[1].f28 = 1;
        mat3 id;
        vector4 zero(0.0f, 0.0f, 0.0f);
        mat4 m;
        *(mat3*)&m = *identity3(&id);
        m.o = zero;
        mesh_render(meshes[i], sc, m);
    }
    if (!reflect || !info.valid)
        return;
    if (sc->refl == 0) {
        scene* r = new scene;
        sc->refl = r;
        sc->refl->mask = sc->mask;
        sc->refl->f0 = 0;
        sc->refl->f4 = 0;
        sc->refl->f8 = 0x100;
        sc->refl->fc = 0x100;
        sc->refl->f10 = 0;
        sc->refl->f14 = 1.0f;
        sc->refl->f161 = 1;
        sc->refl->f16c = 0;
        sc->refl->f170 = 0;
        sc->refl->f174 = 0x3f800000;
        sc->refl->f178 = -(info.depth - 0.25f);
        sc->refl->flags = sc->flags;
        frustum f = sc->fr;
        sc->refl->f169 = 1;
        sc->refl->f16a = 0;
        sc->refl->fr = f;
        sc->rt = resmgr().get_render_target(0x100, 0x100, 0x16, 1, 0);
        if (sc->rt == 0)
            return;
        sc->rt->flags = 0x300000;
    }
    if (!sc->rt->loaded)
        return;
    sc->refl->reset();
    if (g_log_on)
        ((Logger*)g_log_obj)->get("RENDERING REFLECTIONS;");
    if (!g_simple_reflect)
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
    sc->refl->f1b0 = env;
    sc->refl->list = (const ptr_list&)lights;
    render_info rinfo;
    rinfo.valid = 0;
    if (g_simple_reflect) {
        if (sky)
            sky->render(sc->refl, &rinfo);
        return;
    }
    for (entity** it = ents._Myfirst; it < ents._Mylast; ++it) {
        float r;
        vector4 c;
        centre(c, *it);
        r = (*it)->get_radius();
        if (in_frustum_n(sc->refl, &c, r))
            (*it)->render(sc->refl, &rinfo);
    }
}

} // namespace rgl

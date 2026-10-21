// Slice s00429890: the world's scene render (entities, meshes and particle systems culled against the view
// frustum, plus the water-reflection pass), the strategic_entity's per-frame render, and the light finder.
// Names provisional.
#include "strat.h"

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
static __forceinline void centre(vector4& c, MeshEnt* e)
{
    c.x = e->pos.s.x * e->off.x + (e->pos.f.x * e->off.y + e->pos.u.x * e->off.z) + e->pos.o.x;
    c.y = e->pos.s.y * e->off.x + (e->pos.f.y * e->off.y + e->pos.u.y * e->off.z) + e->pos.o.y;
    c.z = e->pos.s.z * e->off.x + (e->pos.f.z * e->off.y + e->pos.u.z * e->off.z) + e->pos.o.z;
}

// @ 0x429890
void Ctx::render_scene(scene* sc, bool reflect)
{
    sc->owner = this;
    sc->ambient = amb;
    sc->fr.build_planes();
    g_scene_reg.register_obj(sc);
    for (unsigned i = 0; i < meshes.size(); i++)
        meshes[i]->clear_all();
    render_info info;
    info.valid = 0;
    for (MeshEnt** it = ents_a._Myfirst; it < ents_a._Mylast; ++it) {
        float r;
        vector4 c;
        centre(c, *it);
        r = (*it)->get_radius();
        if (in_frustum_n(sc, &c, r))
            (*it)->render(sc, &info);
    }
    for (MeshEnt** it = ents_b._Myfirst; it < ents_b._Mylast; ++it) {
        float r;
        vector4 c;
        centre(c, *it);
        r = (*it)->get_radius();
        if (in_frustum(sc, &c, r))
            (*it)->render(sc, &info);
    }
    for (std::vector<Psys*>::iterator it = psyss.begin(); it != psyss.end(); ++it) {
        Psys* p = *it;
        if (in_frustum_n(sc, (const vector4*)((char*)p + 0x124), *(float*)((char*)p + 0x1ac)))
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
        meshes[i]->render(sc, m);
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
        sc->refl->f18c = sc->f18c;
        frustum f = sc->fr;
        sc->refl->f169 = 1;
        sc->refl->f16a = 0;
        sc->refl->fr = f;
        sc->rt = g_resmgr.get_render_target(0x100, 0x100, 0x16, 1, 0);
        if (sc->rt == 0)
            return;
        sc->rt->f24 = 0x300000;
    }
    if (!sc->rt->loaded)
        return;
    sc->refl->reset();
    if (g_log_on)
        ((Logger*)g_log_obj)->print("RENDERING REFLECTIONS;");
    if (!g_simple_reflect)
        sc->refl->f18c |= 0x200;
    matrix cam = sc->fr.m;
    cam.o.z = (info.depth + info.depth) - cam.o.z;
    cam.s.z = -cam.s.z;
    cam.u.z = -cam.u.z;
    cam.f = vector4(-cam.f.x, -cam.f.y, -cam.f.z);
    sc->refl->fr.m = cam;
    sc->refl->fr.build_planes();
    if (sc->rt == 0)
        return;
    sc->refl->ambient = amb;
    sc->refl->target = sc->rt;
    sc->refl->owner = this;
    sc->refl->f1b0 = env;
    sc->refl->list = (const PtrVec&)lights;
    render_info rinfo;
    rinfo.valid = 0;
    if (g_simple_reflect) {
        if (sky)
            sky->render(sc->refl, &rinfo);
        return;
    }
    for (MeshEnt** it = ents_a._Myfirst; it < ents_a._Mylast; ++it) {
        float r;
        vector4 c;
        centre(c, *it);
        r = (*it)->get_radius();
        if (in_frustum_n(sc->refl, &c, r))
            (*it)->render(sc->refl, &rinfo);
    }
}

// @ 0x42a090
void StratEnt::render(scene* sc, render_info* out)
{
    if (f140) {
        if (fx)
            fx->draw(sc, &pos, &f144);
        MeshEnt::render(sc, out);
        if ((sc->mask & 1) && (mode & 3) && g_shadows_on) {
            Light* l = ctx->first_dir_light();
            if (l && !(l->flags & 2)) {
                bool a = f160 * f294 > 0.05f;
                bool b = f164 * f294 > 0.05f;
                if (a || b)
                    update_shadows(sc, a, b);
            }
        }
        for (Psys** p = emitters_first; p != emitters_last; ++p)
            (*p)->update(sc);
        f31c++;
    }
}

// @ 0x42a1a0
Light* Ctx::first_dir_light()
{
    Light* found = 0;
    unsigned i = 0;
    while (i < lights.size()) {
        if (lights[i]->type == 2 && !(lights[i]->flags & 1))
            found = lights[i];
        i++;
        if (found)
            break;
    }
    return found;
}

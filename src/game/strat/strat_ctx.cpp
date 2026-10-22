//-----------------------------------------------------------------------------
// File: strat_ctx.cpp  (0x42b6d0..0x42bac0)
//
// Desc: the scene-world (Ctx) ambient colour, the strategic entity's shadow-map render (shadow_tick: renders the
//       entity into its 128x128 "shadow_shader" render target) and the entity lookup by id. Separate object from
//       strat_entity.cpp: create_shadow_mesh (0x42b080) calls ambient() forward, and only with ambient's body
//       NOT visible does cl allocate create_shadow_mesh's locals as the original does (see MIGRATION.md).
//       Flags: /O2 /MT /Gy /TP. Names provisional.
//-----------------------------------------------------------------------------
#include "./strat_ext.h"       // (listed directly: the resolution audit scans only the source's own includes)
#include "./strat_entity.h"

// @ 0x42b6d0
// The ambient light colour of the scene: the colour of the last enabled ambient light (type 2, flags bit 0),
// scaled by 0.7 and added to the base ambient at +0x68.
vector4 Ctx::ambient()
{
    vector4 c(0.0f, 0.0f, 0.0f);
    for (unsigned i = 0; i < lights.size(); i++) {
        Light* l = lights[i];
        if (l->type == 2 && (l->flags & 1))
            c = l->color;
    }
    vector4 t(c.x * 0.7f, c.y * 0.7f, c.z * 0.7f);
    return vector4(t.x + amb.x, t.y + amb.y, t.z + amb.z);
}

// @ 0x42b790
// Renders the shadow-map view of the entity into its 128x128 shadow render target (created on first use):
// an orthographic camera along the main light, then the entity and its linked entity are drawn into it.
void StratEnt::shadow_tick()
{
    if (p288 == 0) {
        p288 = g_resmgr.get_shadow_skin(0x80, 0x80, 0x1c, 0);
        if (p288 == 0) return;
        p288->file.assign("shadow_shader");
        skin* sk = p288;
        sk->f30 = g_resmgr.lookup_file(&sk->file);
        p288->id = 0x108;
        p288->refs[0]->f24 = 0x300000;
    }
    if (sc28c == 0) {
        sc28c = new scene;
        sc28c->mask = 0x2000;
        sc28c->f0 = 1;
        sc28c->f4 = 1;
        sc28c->f8 = 0x7e;
        sc28c->fc = 0x7e;
        sc28c->f10 = 0;
        sc28c->f14 = 1.0f;
        sc28c->f18 = 0;
        sc28c->f1c = 0;
        sc28c->f20 = 0x80;
        sc28c->f24 = 0x80;
        sc28c->f28 = 0;
        sc28c->f2c = 1.0f;
        sc28c->f1a5 = 1;
        sc28c->f164 = 1;
        sc28c->f18c = 0x2101;
        sc28c->f190 = 0;
    }
    if (p288 != 0 && p288->refs[0]->loaded != 0) {
        frustum f;
        render_info ri;
        matrix m;
        vector4 amb;
        f.farz = radius * 5.0f;
        f.perspective = 0;
        f.nearz = 0.01f;
        f.right = radius;
        f.left = -radius;
        f.top = radius;
        f.bottom = -radius;
        f.build_planes();
        build_shadow_frame(&m, &amb);
        f.m = m;
        f.build_planes();
        sc28c->fr = f;
        sc28c->target = p288->refs[0];
        if (fx) fx->draw(sc28c, &pos, &f144);
        MeshEnt::render(sc28c, &ri);
        if (f298 >= 0) {
            StratEnt* e = ctx->find_entity(f298);
            if (e != 0 && e->type() == 9 && e->fx != 0)
                e->fx->draw(sc28c, &e->pos, &f144);
        }
    }
}

// @ 0x42ba50
// Looks an entity up by id: the first list is searched until a hit (the first match wins), the second list
// is searched to the end (the last match wins).
StratEnt* Ctx::find_entity(int id)
{
    MeshEnt** p = ents_a._Myfirst;
    MeshEnt* r = 0;
    bool found = false;
    if (p < ents_a._Mylast) {
        do {
            if (found) return (StratEnt*)r;
            if ((*p)->id == id) {
                r = *p;
                found = true;
            }
            p++;
        } while (p < ents_a._Mylast);
        if (found) return (StratEnt*)r;
    }
    for (p = ents_b._Myfirst; p < ents_b._Mylast; p++)
        if ((*p)->id == id) r = *p;
    return (StratEnt*)r;
}

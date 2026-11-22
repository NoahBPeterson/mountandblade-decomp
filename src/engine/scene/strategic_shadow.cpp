//-----------------------------------------------------------------------------
// File: strategic_shadow.cpp  (0x42b6d0..0x42bac0)
//
// Desc: the scene-world (Ctx) ambient colour, the strategic entity's shadow-map render (shadow_tick: renders the
//       entity into its 128x128 "shadow_shader" render target) and the entity lookup by id. Separate object from
//       strategic_entity.cpp: create_shadow_mesh (0x42b080) calls ambient() forward, and only with ambient's body
//       NOT visible does cl allocate create_shadow_mesh's locals as the original does (see MIGRATION.md).
//       Flags: /O2 /MT /Gy /TP. Names provisional.
//-----------------------------------------------------------------------------
#define GM_RGL_NS                 // gui_manager.h: the scene / mesh stand-ins in namespace rgl (the defined symbols)
#include "engine/scene/strat_ext.h"       // (listed directly: the resolution audit scans only the source's own includes)
#include "engine/scene/strategic_entity.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "engine/scene/frustum.h"
#include "engine/fx/keyframes.h"
#include "engine/math/mat4_include.h"
#include "engine/render/mesh_prims.h"
#include "engine/math/vec.h"

// @ 0x42b6d0
// The ambient light colour of the scene: the colour of the last enabled ambient light (type 2, flags bit 0),
// scaled by 0.7 and added to the base ambient at +0x68.
vector4 rgl::world::get_ambient()
{
    vector4 c(0.0f, 0.0f, 0.0f);
    for (unsigned i = 0; i < lights.size(); i++) {
        Light* l = lights[i];
        if (l->type == 2 && (l->flags & 1))
            c = l->color;
    }
    vector4 t(c.x * 0.7f, c.y * 0.7f, c.z * 0.7f);
    return vector4(t.x + ambient.x, t.y + ambient.y, t.z + ambient.z);
}

// @ 0x42b790
// Renders the shadow-map view of the entity into its 128x128 shadow render target (created on first use):
// an orthographic camera along the main light, then the entity and its linked entity are drawn into it.
void rgl::strategic_entity::shadow_tick()
{
    if (shadow_skin == 0) {
        shadow_skin = g_resmgr.get_shadow_skin(0x80, 0x80, 0x1c, 0);
        if (shadow_skin == 0) return;
        shadow_skin->shader_name.assign("shadow_shader");
        skin* sk = shadow_skin;
        sk->shader = g_resmgr.resolve_shader(sk->shader_name);
        shadow_skin->flags = 0x108;
        shadow_skin->tex[0]->flags = 0x300000;
    }
    if (shadow_scene == 0) {
        shadow_scene = new scene;                 // the ctor 0x429740 is rgl::scene's
        shadow_scene->mask = 0x2000;
        shadow_scene->vp_local.X = 1;
        shadow_scene->vp_local.Y = 1;
        shadow_scene->vp_local.Width = 0x7e;
        shadow_scene->vp_local.Height = 0x7e;
        shadow_scene->vp_local.MinZ = 0.0f;
        shadow_scene->vp_local.MaxZ = 1.0f;
        shadow_scene->vp_alt.X = 0;
        shadow_scene->vp_alt.Y = 0;
        shadow_scene->vp_alt.Width = 0x80;
        shadow_scene->vp_alt.Height = 0x80;
        shadow_scene->vp_alt.MinZ = 0.0f;
        shadow_scene->vp_alt.MaxZ = 1.0f;
        shadow_scene->use_alt_vp = 1;
        shadow_scene->mask_pass = 1;
        shadow_scene->flags = 0x2101;
        shadow_scene->clear_color = 0;
    }
    if (shadow_skin != 0 && shadow_skin->tex[0]->loaded != 0) {
        frustum f;
        render_info ri;
        matrix m;
        vector4 amb;
        f.farz = shadow_radius * 5.0f;
        f.perspective = 0;
        f.nearz = 0.01f;
        f.right = shadow_radius;
        f.left = -shadow_radius;
        f.top = shadow_radius;
        f.bottom = -shadow_radius;
        f.build_planes();
        build_shadow_frame(&m, &amb);
        f.m = m;
        f.build_planes();
        shadow_scene->fr = f;
        shadow_scene->target = shadow_skin->tex[0];
        if (fx) fx->render(shadow_scene, &local, &lod_dist);
        rgl::entity::render(shadow_scene, &ri);
        if (shadow_caster >= 0) {
            rgl::entity* o = owner->find_entity(shadow_caster);
            if (o != 0 && o->type_id() == 9) {
                rgl::strategic_entity* e = static_cast<rgl::strategic_entity*>(o);
                if (e->fx != 0)
                    e->fx->render(shadow_scene, &e->local, &lod_dist);
            }
        }
    }
}

// @ 0x42ba50
// Looks an entity up by id: the first list is searched until a hit (the first match wins), the second list
// is searched to the end (the last match wins).
rgl::entity* rgl::world::find_entity(int id)
{
    std::vector<entity*>::iterator p = ents.begin();
    entity* r = 0;
    bool found = false;
    if (p < ents.end()) {
        do {
            if (found) return r;
            if ((*p)->id == id) {
                r = *p;
                found = true;
            }
            p++;
        } while (p < ents.end());
        if (found) return r;
    }
    for (p = ents2.begin(); p < ents2.end(); p++)
        if ((*p)->id == id) r = *p;
    return r;
}

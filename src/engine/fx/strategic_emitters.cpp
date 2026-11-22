// strategic_emitters.cpp (0x4219c0..0x421df0): FxElem::set_color, the strategic entity's emitter list and its
// per-frame update. Migrated from slices s00421110 (0x4219c0, 0x421a50) and s00421aa0 (0x421aa0, 0x421b00,
// 0x421b70).
// Boundary: start 0x4219c0: FxElem::set_color calls mesh_list::set_color (0x421990) as an opaque function (with the
// body visible cl inlines it: 153 differing bytes), so 0x421990 is another object; end 0x421df0 (strategic_entity::update calls FxDef::update 0x421df0 and the world's add_item 0x423c00
// forward: high).
#include <vector>
#include <math.h>
#include "engine/core/rgl_string.h"
#include "engine/math/rgl_math.h"
#include "engine/resource/resource.h"
#include "engine/math/scene_math.h"
#include "engine/scene/frustum_rgl_scene.h"
#include "engine/scene/scene.h"
#include "engine/scene/emitter.h"
#include "engine/scene/scene_entity.h"
#include "engine/scene/world.h"
#include "engine/fx/fx_rgl_scene.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "engine/scene/frustum.h"
#include "engine/fx/keyframes.h"
#include "engine/math/mat4_include.h"
#include "engine/render/mesh_prims.h"
#include "engine/math/vec.h"
#include "engine/render/mesh_parts.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "engine/render/mesh.h"      // (listed directly: the resolution audit scans only the source's own includes)

using namespace rgl;

// @ 0x4219c0
void FxElem::set_color(int c)
{
    // guarded do-while with the colour copied to a local inside the guard: c itself is address-taken by the second
    // loop (set_color(const int&)), so a store through *it could alias it; the copy stays in a register across the loop
    std::vector<rgl::mesh*>::iterator it = psys.begin();
    if (it < psys.end()) {
        int col = c;
        do { (*it)->color = col; ++it; } while (it < psys.end());
    }
    for (std::vector<rgl::lod_mesh*>::iterator g = groups.begin(); g < groups.end(); ++g) {
        rgl::lod_mesh* grp = *g;
        for (int i = 0; i < grp->count; ++i)
            grp->lists[i].set_color(c);
    }
}

namespace rgl {

// @ 0x421a50
int strategic_entity::find_emitter(int tag)
{
    int r = -1;
    for (unsigned i = 0; i < emitters.size(); ++i)
        if (emitters[i]->tag == tag)
            r = i;
    return r;
}

// @ 0x421aa0
void strategic_entity::add_emitter(psys* e)
{
    emitters.push_back(e);
}

// @ 0x421b00
void strategic_entity::clear_emitters()
{
    for (unsigned i = 0; i < emitters.size(); ++i) {
        psys* e = emitters[i];
        delete e;
    }
    emitters.resize(0);
}

// @ 0x421b70
void strategic_entity::update(float dt)
{
    radius = bound_radius;
    if (shape) {
        shape->transform(&local);
        vector4 t;
        bmax = *shape->get_max(&t);
        bmin = *shape->get_min(&t);
    }
    for (std::vector<psys*>::iterator it = emitters.begin(); it != emitters.end(); ++it) {
        (*it)->update(&dt, local);
        if (radius < (*it)->radius) radius = (*it)->radius;
    }
    float dx = last_pos.x - local.o.x;
    float dy = last_pos.y - local.o.y;
    float dz = last_pos.z - local.o.z;
    float len = (float)sqrt(dx * dx + dy * dy + dz * dz);
    if (len > thresh) {
        if (!f2ec) f2c0 = 0;
        last_pos = local.o;
    }
    if (fx) fx->update(dt, &local);
    prev = local;
    was_dirty = dirty;
    dirty = false;
    for (std::vector<mesh*>::iterator it = meshes.begin(); it < meshes.end(); ++it)
        ((mesh*)*it)->update_emitters(&dt, local);
    if ((char)is_dirty()) {                             // the original tests the flag byte (al)
        for (unsigned i = 0; i < items.size(); ++i) {
            matrix m = local;
            m.orthonormalize();
            items[i]->transform(&m);
            owner->add_item(items[i]);
        }
    }
    if (shadow_skin && render_count && !shadow_drawn) {
        g_resmgr.release(shadow_skin);   // 0x4175f0
        shadow_skin = 0;
    }
    render_count = 0;
    shadow_drawn = 0;
}

} // namespace rgl

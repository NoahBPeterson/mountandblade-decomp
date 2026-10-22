// strategic_emitters.cpp (0x4219c0..0x421df0): FxElem::set_color, the strategic entity's emitter list and its
// per-frame update. Migrated from slices s00421110 (0x4219c0, 0x421a50) and s00421aa0 (0x421aa0, 0x421b00,
// 0x421b70).
// Boundary: start 0x4219c0: FxElem::set_color calls mesh_list2::set_color (0x421990) as an opaque function (with the
// body visible cl inlines it: 153 differing bytes), so 0x421990 is another object; end 0x421df0 (strategic_entity::update calls FxDef::update 0x421df0 and the world's add_item 0x423c00
// forward: high).
#include <vector>
#include <math.h>
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

// @ 0x4219c0
void FxElem::set_color(int c)
{
    for (std::vector<rgl::mesh*>::iterator it = psys.begin(); it < psys.end(); ++it)
        (*it)->color = c;
    for (std::vector<rgl::lod_mesh2*>::iterator g = groups.begin(); g < groups.end(); ++g) {
        rgl::lod_mesh2* grp = *g;
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
    radius = fbc;
    if (shape) {
        shape->set_frame((const matrix*)&local);
        vac = shape->get_b();
        v9c = shape->get_a();
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
    f142 = f141;
    f141 = false;
    for (std::vector<mesh*>::iterator it = meshes.begin(); it < meshes.end(); ++it)
        ((mesh_ext*)*it)->update_emitters(&dt, local);
    if (is_dirty()) {
        for (unsigned i = 0; i < items.size(); ++i) {
            matrix m = local;
            m.orthonormalize();
            items[i]->set_frame(&m);
            owner->add_item(items[i]);
        }
    }
    if (p288 && f31c && !f170) {
        resmgr().release(p288);
        p288 = 0;
    }
    f31c = 0;
    f170 = 0;
}

} // namespace rgl

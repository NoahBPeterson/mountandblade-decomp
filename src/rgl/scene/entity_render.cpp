// entity_render.cpp (0x4201b0..0x4205b0): aabb::extend_box (the box grown by the eight transformed corners of
// another box) and rgl::entity::render (vtable slot 9: child meshes and LOD records). Migrated from slice
// s0041ffe0 (0x4201b0, 0x420310).
// Boundary: start 0x4201b0 (entity::recalc 0x41ffe0 calls extend_box forward: high); end 0x4205b0 (render calls
// lod_mesh2::select 0x4205b0 forward: high; render is the last function before it).
#include <vector>
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

namespace rgl {

// @ 0x4201b0
void box_ext::extend_box(const aabb* b, const matrix* m)
{
    if (b->min.x > b->max.x)
        return;
    {
        float c[8][4];
        c[0][0] = b->min.x; c[0][1] = b->min.y; c[0][2] = b->min.z;
        c[1][0] = b->max.x; c[1][1] = b->min.y; c[1][2] = b->min.z;
        c[2][0] = b->min.x; c[2][1] = b->max.y; c[2][2] = b->min.z;
        c[3][0] = b->max.x; c[3][1] = b->max.y; c[3][2] = b->min.z;
        c[4][0] = b->min.x; c[4][1] = b->min.y; c[4][2] = b->max.z;
        c[5][0] = b->max.x; c[5][1] = b->min.y; c[5][2] = b->max.z;
        c[6][0] = b->min.x; c[6][1] = b->max.y; c[6][2] = b->max.z;
        c[7][0] = b->max.x; c[7][1] = b->max.y; c[7][2] = b->max.z;
        for (int i = 0; i < 8; i++) {
            float* q = c[i];
            vector4 p;
            p.x = q[1] * m->f.x + q[2] * m->u.x + q[0] * m->s.x + m->o.x;
            p.y = m->u.y * q[2] + q[0] * m->s.y + q[1] * m->f.y + m->o.y;
            float t = m->s.z * q[0] + q[1] * m->f.z;   // the temporary keeps the (x, y) pair summed first
            p.z = t + q[2] * m->u.z + m->o.z;
            extend(&p);
        }
    }
}

// @ 0x420310
void entity::render(scene* sc, render_info* info)
{
    if (f140) {
        for (unsigned i = 0; i < meshes.size(); i++) {
            if (meshes[i]) {
                if (!meshes[i]->res) {
                    string n("default");
                    meshes[i]->res = g_resmgr.get_material(n);
                }
                if (meshes[i]->res->shader->flags20 & 0x100) {
                    if (meshes[i]->get70().size() > 0) {
                        info->valid = 1;
                        const vector4& p0 = (iter32(&*meshes[i]->get70().begin()) + 0)._Ptr->a;
                        info->depth = local.transform_point(((const mat4&)meshes[i]->m_cc).transform_point(p0)).z;
                    }
                }
                mesh_render(meshes[i], sc, local);
            }
        }
        for (unsigned i = 0; i < lods.size(); i++) {
            if (lods[i]) {
                lods[i]->select(&f144);
                if (lods[i]->count != 0 && lods[i]->lists[0].v.size() != 0 &&
                    (lods[i]->lists[0].v[0]->res->shader->flags20 & 0x100) != 0) {
                    if (((mesh_ext*)lods[i]->lists[0].v[0])->get70_().size() > 0) {
                        info->valid = 1;
                        info->depth = local.transform_point(((const mat4&)lods[i]->m8c).transform_point(((v70_view&)((mesh_ext*)lods[i]->lists[0].v[0])->get70_()).at(0).a)).z;
                    }
                }
                lods[i]->render(sc, local);
            }
        }
    }
}

} // namespace rgl

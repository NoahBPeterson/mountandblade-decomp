// entity_recalc.cpp (0x41fee0..0x4201b0): rgl::entity's scaled-frame setter, the row scaling it shares with
// set_local_scaled, and recalc (the bounds rebuilt from the child meshes). Migrated from slices s0041f720
// (0x41fee0, 0x41ff40) and s0041ffe0 (0x41ffe0).
// Boundary: start 0x41fee0 (entity.cpp's last function, 0x41fe90, calls scale_rows forward: high); end 0x4201b0
// (recalc calls aabb::extend_box forward: high; the only function between them is recalc).
#include <vector>
#include "../rgl_string.h"
#include "../rgl_math.h"
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

using namespace rgl;

namespace rgl {

// @ 0x41fee0
void smat::scale_rows(const vector4* k)
{
    s.x = k->x * s.x;
    s.y = s.y * k->x;
    s.z = s.z * k->x;
    f.x = f.x * k->y;
    f.y = k->y * f.y;
    f.z = k->y * f.z;
    u.x = k->z * u.x;
    u.y = u.y * k->z;
    u.z = u.z * k->z;
}

// @ 0x41ff40
void entity::set_local_scaled_dirty(const matrix* mm, const vector4* k)
{
    local = *(const smat*)mm;
    scale = *k;
    smat* m = &local;
    m->s.x = m->s.x * k->x;
    m->s.y = m->s.y * k->x;
    m->s.z = m->s.z * k->x;
    m->f.x = k->y * m->f.x;
    m->f.y = k->y * m->f.y;
    m->f.z = k->y * m->f.z;
    m->u.x = k->z * m->u.x;
    m->u.y = m->u.y * k->z;
    m->u.z = m->u.z * k->z;
    recalc();
    f141 = 1;
}

// identity_pos (0x40cd00) as inlined by its callers: the zero origin is built before the basis rows
static __forceinline matrix* ident4(matrix* out)
{
    vector4 o(0.0f, 0.0f, 0.0f);
    mat3 m;
    *(mat3*)out = *identity3(&m);
    out->o = o;
    return out;
}

// @ 0x41ffe0
void entity::recalc()
{
    box.reset();
    for (unsigned i = 0; i < meshes.size(); i++) {
        mesh** pm = &vat(meshes, i);
        if (*pm) {
            matrix a;
            ident4(&a);
            a.s.x = a.s.x * scale.x;
            a.s.y = a.s.y * scale.x;
            a.s.z = a.s.z * scale.x;
            a.f.x = a.f.x * scale.y;
            a.f.y = a.f.y * scale.y;
            a.f.z = a.f.z * scale.y;
            a.u.x = a.u.x * scale.z;
            a.u.y = a.u.y * scale.z;
            a.u.z = a.u.z * scale.z;
            box.extend_box(&(*pm)->bounds, &(a * (*pm)->m_cc));
        }
    }
    box.update();
    fbc = box.radius;
    vc4 = box.center;
    vc4.x = vc4.x / scale.x;
    vc4.y = vc4.y / scale.y;
    vc4.z = vc4.z / scale.z;
}

} // namespace rgl

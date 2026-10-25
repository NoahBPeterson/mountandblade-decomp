// shape_load.cpp (0x4285e0..0x428820): the shape resource's deep copy by shape type, and the load slots of the
// strategic and skeleton entities. Migrated from slice s00428240 (0x4285e0, 0x4286d0, 0x4286e0).
// Boundary: start 0x4285e0 (entity::load 0x4284e0 calls shape_resource::clone_shape forward: high); end 0x428820
// (skeleton_entity::load calls the FxDef constructor 0x428820 and FxDef::init 0x4288c0 forward: high).
#include <vector>
#include <new>
#include "../rgl_string.h"
#include "../../game/tstring.h"   // tstring: the string locals below (rgl::string's constructors are its 0x42bbd0 / 0x42bbe0)
#include "../rgl_math.h"
#include "../stream.h"
#include "../resource.h"
#include "./scene_math.h"
#include "./frustum.h"
#include "./scene.h"
#include "./emitter.h"
#include "./scene_entity.h"
#include "./world.h"
#include "./fx.h"
#include "./shape_clone.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../include/frustum.h"
#include "../../include/keyframes.h"
#include "../../include/mat4.h"
#include "../../include/mesh_prims.h"
#include "../../include/vec.h"
#include "../../include/shape.h"      // (the shared class headers named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/mesh_parts.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../mesh.h"      // (listed directly: the resolution audit scans only the source's own includes)

using namespace rgl;

// @ 0x4285e0
shape_base* shape_resource::clone_shape()
{
    shape_base* r;
    if (shape->shape_type() == 0) {
        return new sphere(*(sphere*)shape);
    } else if (shape->shape_type() == 1) {
        return new capsule(*(capsule*)shape);
    } else if (shape->shape_type() == 2) {
        return new face(*(face*)shape);
    } else if (shape->shape_type() == 3) {
        return new composite(*(composite*)shape);
    } else if (shape->shape_type() == 4) {
        return ((manifold*)shape)->build_composite();
    }
    return r;
}

namespace rgl {

// the element is pushed through a conversion from the derived type (a temporary pointer, as in the original)
struct particle_mesh : mesh {};

// @ 0x4286d0
void strategic_entity::load(stream_base* s) { entity::load(s); }

// @ 0x4286e0
void skeleton_entity::load(stream_base* s)
{
    entity::load(s);
    tstring a;
    tstring b;
    s->r_tstr((string*)&a);
    if (a.differs("none")) {
        fx = new FxDef(0);
        fx->init((FxRes*)g_resmgr.get_object70(*(string*)&a));
    }
    if (fx) {
        s->r_int(&nfx);
        for (int i = 0; i < nfx; i++) {
            int idx;
            mesh* m;
            s->r_int(&idx);
            s->r_tstr((string*)&a);
            if (a.differs("none"))
                m = ((mesh_x*)resmgr().get_object((string*)&a))->clone();
            fx->elems[idx].psys.push_back((particle_mesh*)m);
        }
    }
}

} // namespace rgl

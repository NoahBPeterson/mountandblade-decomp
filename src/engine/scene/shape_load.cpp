// shape_load.cpp (0x4285e0..0x428820): the shape resource's deep copy by shape type, and the load slots of the
// strategic and skeleton entities. Migrated from slice s00428240 (0x4285e0, 0x4286d0, 0x4286e0).
// Boundary: start 0x4285e0 (entity::load 0x4284e0 calls body_part::clone_shape forward: high); end 0x428820
// (skeleton_entity::load calls the FxDef constructor 0x428820 and FxDef::init 0x4288c0 forward: high).
#include <vector>
#include <new>
#include "engine/core/rgl_string.h"
#include "engine/core/tstring.h"   // tstring: the string locals below (rgl::string's constructors are its 0x42bbd0 / 0x42bbe0)
#include "engine/math/rgl_math.h"
#include "engine/io/stream.h"
#include "engine/resource/resource.h"
#include "engine/math/scene_math.h"
#include "engine/scene/frustum_rgl_scene.h"
#include "engine/scene/scene.h"
#include "engine/scene/emitter.h"
#include "engine/scene/scene_entity.h"
#include "engine/scene/world.h"
#include "engine/fx/fx_rgl_scene.h"
#include "engine/scene/shape_clone.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "engine/scene/frustum.h"
#include "engine/fx/keyframes.h"
#include "engine/math/mat4_include.h"
#include "engine/render/mesh_prims.h"
#include "engine/math/vec.h"
#include "engine/physics/shape.h"      // (the shared class headers named here too: the resolution audit reads the annotations of directly included headers)
#include "engine/render/mesh_parts.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "engine/render/mesh.h"      // (listed directly: the resolution audit scans only the source's own includes)

using namespace rgl;

// @ 0x4285e0
shape_base* body_part::clone_shape()
{
    shape_base* r;
    if (shape->shape_type() == 0) {
        r = new sphere(*(sphere*)shape);
    } else if (shape->shape_type() == 1) {
        r = new capsule(*(capsule*)shape);
    } else if (shape->shape_type() == 2) {
        r = new face(*(face*)shape);
    } else if (shape->shape_type() == 3) {
        r = new composite(*(composite*)shape);
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
        fx->init(g_resmgr.get_object70(*(string*)&a));
    }
    if (fx) {
        s->r_int(&nfx);
        for (int i = 0; i < nfx; i++) {
            int idx;
            mesh* m;
            s->r_int(&idx);
            s->r_tstr((string*)&a);
            if (a.differs("none"))
                m = ((rgl::mesh*)g_resmgr.get_object((string*)&a))->clone();
            fx->elems[idx].psys.push_back(m);
        }
    }
}

} // namespace rgl

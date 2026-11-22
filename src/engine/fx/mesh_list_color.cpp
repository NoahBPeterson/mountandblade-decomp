// mesh_list_color.cpp (0x421990..0x4219c0): mesh_list::set_color, the lod record's list tint (every mesh in the
// list gets the colour). Migrated from slice s00421110 (0x421990, there under the stand-in class PsysListBody).
// Boundary: a single function. FxDef::set_color (0x4218e0) calls it forward (high), and FxElem::set_color
// (0x4219c0) calls it out of line although the body is smaller than a call: with the body visible in the same
// object cl inlines it (153 differing bytes), so it is not in FxElem::set_color's object either.
#include <vector>
#include "engine/core/rgl_string.h"
#include "engine/math/rgl_math.h"
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

namespace rgl {

// @ 0x421990
void mesh_list::set_color(const int& c)
{
    for (iterator it = begin(); it < end(); ++it)
        (*it)->color = c;
}

} // namespace rgl

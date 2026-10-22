// mesh_list_color.cpp (0x421990..0x4219c0): mesh_list2::set_color, the lod record's list tint (every mesh in the
// list gets the colour). Migrated from slice s00421110 (0x421990, there under the stand-in class PsysListBody).
// Boundary: a single function. FxDef::set_color (0x4218e0) calls it forward (high), and FxElem::set_color
// (0x4219c0) calls it out of line although the body is smaller than a call: with the body visible in the same
// object cl inlines it (153 differing bytes), so it is not in FxElem::set_color's object either.
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

namespace rgl {

// @ 0x421990
void mesh_list2::set_color(const int& c)
{
    for (std::vector<mesh*>::iterator it = v.begin(); it < v.end(); ++it)
        (*it)->color = c;
}

} // namespace rgl

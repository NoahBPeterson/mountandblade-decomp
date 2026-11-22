// fx_def_set_color.cpp (0x4218e0..0x421990): FxDef::set_color, the effect definition's tint (every particle mesh,
// every lod record's lists, every element). Migrated from slice s00421110 (0x4218e0).
// Boundary: a single function. strategic_entity::set_color (0x4218a0) tail-jumps to it forward (high), and it calls
// mesh_list::set_color (0x421990) and FxElem::set_color (0x4219c0) forward (high): the callees' bodies are not
// visible to it (the slice kept them out of line for the same reason).
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

// @ 0x4218e0
void FxDef::set_color(int c)
{
    for (std::vector<rgl::mesh*>::iterator it = psys.begin(); it < psys.end(); ++it)
        (*it)->color = c;
    for (std::vector<rgl::lod_mesh*>::iterator g = groups.begin(); g < groups.end(); ++g) {
        rgl::lod_mesh* grp = *g;
        for (int i = 0; i < grp->count; ++i)
            grp->lists[i].set_color(c);
    }
    for (int i = 0; i < count; ++i)
        elems[i].set_color(c);
}

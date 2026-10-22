// fxdef_set_color.cpp (0x4218e0..0x421990): FxDef::set_color, the effect definition's tint (every particle mesh,
// every lod record's lists, every element). Migrated from slice s00421110 (0x4218e0).
// Boundary: a single function. strategic_entity::set_color (0x4218a0) tail-jumps to it forward (high), and it calls
// mesh_list2::set_color (0x421990) and FxElem::set_color (0x4219c0) forward (high): the callees' bodies are not
// visible to it (the slice kept them out of line for the same reason).
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

// @ 0x4218e0
void FxDef::set_color(int c)
{
    for (std::vector<rgl::mesh*>::iterator it = psys.begin(); it < psys.end(); ++it)
        (*it)->color = c;
    for (std::vector<rgl::lod_mesh2*>::iterator g = groups.begin(); g < groups.end(); ++g) {
        rgl::lod_mesh2* grp = *g;
        for (int i = 0; i < grp->count; ++i)
            grp->lists[i].set_color(c);
    }
    for (int i = 0; i < count; ++i)
        elems[i].set_color(c);
}

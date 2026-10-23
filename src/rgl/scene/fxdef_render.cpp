// fxdef_render.cpp (0x428ff0..0x4291c0): FxDef::render, the effect definition's draw: per element the particle
// meshes and lod records at the element's world frame, then the definition's own lists, skinned first unless frozen.
// Migrated from slice s00428b10 (0x428ff0).
// Boundary: a single function. skeleton_entity::render_fx (0x428f80) calls it forward (high) and it calls
// FxDef::skin (0x4291c0) and skin_group (0x4296f0) forward (high).
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

// @ 0x428ff0
void FxDef::render(scene* sc, const matrix* frame, const float* dist)
{
    if (count != 0) {
        for (int i = 0; i < count; i++) {
            matrix m;
            m = *frame * elems[i].m84;
            for (unsigned j = 0; j < elems[i].psys.size(); j++) {
                mesh_render(*(elems[i].psys.begin() + j), sc, m);
            }
            for (unsigned j = 0; j < elems[i].groups.size(); j++) {
                elems[i].groups[j]->select(dist);
                elems[i].groups[j]->render(sc, m);
            }
        }
        for (unsigned j = 0; j < psys.size(); j++) {
            if (!b58)
                skin((mesh*)psys[j], 0);
            mesh_render(psys[j], sc, *frame);
        }
        for (unsigned j = 0; j < groups.size(); j++) {
            groups[j]->select(dist);
            if (!b58)
                skin_group(groups[j]);
            groups[j]->render(sc, *frame);
        }
    }
}

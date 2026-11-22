// fx_def_render.cpp (0x428ff0..0x4291c0): FxDef::render, the effect definition's draw: per element the particle
// meshes and lod records at the element's world frame, then the definition's own lists, skinned first unless frozen.
// Migrated from slice s00428b10 (0x428ff0).
// Boundary: a single function. skeleton_entity::render_fx (0x428f80) calls it forward (high) and it calls
// FxDef::skin (0x4291c0) and skin_group (0x4296f0) forward (high).
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
                (elems[i].groups[j])->render(sc, m);   // 0x413b00
            }
        }
        for (unsigned j = 0; j < psys.size(); j++) {
            if (!frozen)
                skin((mesh*)psys[j], 0);
            mesh_render(psys[j], sc, *frame);
        }
        for (unsigned j = 0; j < groups.size(); j++) {
            groups[j]->select(dist);
            if (!frozen)
                skin_group(groups[j]);
            (groups[j])->render(sc, *frame);   // 0x413b00
        }
    }
}

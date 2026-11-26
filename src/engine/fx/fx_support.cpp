// fx_support.cpp (0x428b10..0x428ff0): the helpers of the effect definition's setup and rendering: quaternion from a
// rotation matrix, the envelope arrays, the FxElem constructor, a mesh's geometry reset, the render scene's reset and
// the skeleton entity's draw. Migrated from slice s00428b10 (0x428b10..0x428f80).
// Boundary: start 0x428b10 (FxDef::init 0x4288c0 calls quat_from_matrix, EnvRec::alloc and the FxElem constructor
// forward: high); end 0x428ff0 (skeleton_entity::render_fx 0x428f80 calls FxDef::render 0x428ff0 forward: high).
#include <vector>
#include <math.h>
#include <new>
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

// the rotation entry (row, column) of the frame: its rows s, f, u are consecutive vector4s
#define R(row, col) ((&(&m.s)[row].x)[col])
// @ 0x428b10
void quat_from_matrix(quaternion* out, const matrix& m)
{
    int nxt[3] = { 1, 2, 0 };
    float q[4];
    float tr = R(1, 1) + R(0, 0) + R(2, 2);
    if (tr > 0.0f) {
        float root = (float)sqrt(tr + 1.0f);
        out->w = root * 0.5f;
        root = 0.5f / root;
        out->x = (R(1, 2) - R(2, 1)) * root;
        out->y = (R(2, 0) - R(0, 2)) * root;
        out->z = (R(0, 1) - R(1, 0)) * root;
        return;
    }
    int i = 0;
    if (R(1, 1) > R(0, 0)) i = 1;
    if (R(2, 2) > R(i, i)) i = 2;
    int j = nxt[i];
    int k = nxt[j];
    float root = (float)sqrt((R(i, i) - (R(j, j) + R(k, k))) + 1.0f);
    q[i] = root * 0.5f;
    if (root != 0.0f) root = 0.5f / root;
    q[3] = (R(j, k) - R(k, j)) * root;
    q[j] = (R(j, i) + R(i, j)) * root;
    q[k] = (R(k, i) + R(i, k)) * root;
    out->x = q[0];
    out->y = q[1];
    out->z = q[2];
    out->w = q[3];
}
#undef R

// @ 0x428c60
void EnvRec::alloc(int n)
{
    if (n != 0) {
        keys_a = new quaternion[n];
        keys_b = new quaternion[n];
        ref = new quaternion[n];
    } else {
        keys_a = 0;
        keys_b = 0;
        ref = 0;
    }
}

// @ 0x428d10
FxElem::FxElem()
{
    qe4 = quaternion(0.0f, 0.0f, 0.0f, 1.0f);
    blend_weight = 1.0f;
    blend_rate = 1.0f;
    vf4 = vector4(0.0f, 0.0f, 0.0f);
    src = 0;
    nchild = 0;
    ngroups = 0;
    parent = 0;
    children = 0;
    ragdoll_bone = 0;
    quaternion t1 = quaternion(0.0f, 0.0f, 0.0f, 1.0f);
    quaternion t2 = quaternion(0.0f, 0.0f, 0.0f, 1.0f);
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            qa[i][j] = t1;
            qb[i][j] = t2;
            t[i][j] = -1000;
            c[i] = 0;
        }
    }
}

namespace rgl {

// @ 0x428e80
void mesh::clear_geometry()
{
    {
        skin_vert e;
        skin_verts.resize(0, e);
    }
    {
        point20 e;
        points.resize(0, e);
    }
    {
        face20 e;
        faces.resize(0, e);
    }
    {
        vertex60 e;
        vertices.resize(0, e);
    }
    influences.resize(0);
}

// @ 0x428f30
void scene::reset()
{
    target = 0;
    depth_target = 0;
    registered = false;
    for (unsigned i = 0; i < cache.size(); i++)
        ((mesh*)*(cache.begin() + i))->clear_geometry();
}

// @ 0x428f80
void skeleton_entity::render_fx(scene* sc)
{
    if (visible) {
        for (unsigned i = 0; i < meshes.size(); i++)
            mesh_render(*(meshes.begin() + i), sc, local);
        if (fx)
            fx->render(sc, &local, &lod_dist);
    }
}

} // namespace rgl

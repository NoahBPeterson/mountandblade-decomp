// fx_support.cpp (0x428b10..0x428ff0): the helpers of the effect definition's setup and rendering: quaternion from a
// rotation matrix, the envelope arrays, the FxElem constructor, a mesh's geometry reset, the render scene's reset and
// the skeleton entity's draw. Migrated from slice s00428b10 (0x428b10..0x428f80).
// Boundary: start 0x428b10 (FxDef::init 0x4288c0 calls quat_from_matrix, EnvRec::alloc and the FxElem constructor
// forward: high); end 0x428ff0 (skeleton_entity::render_fx 0x428f80 calls FxDef::render 0x428ff0 forward: high).
#include <vector>
#include <math.h>
#include <new>
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

// @ 0x428b10
void quat_from_matrix(quat4* out, const float* m)
{
    int nxt[3] = { 1, 2, 0 };
    float q[4];
    float tr = m[5] + m[0] + m[10];
    if (tr > 0.0f) {
        float root = (float)sqrt(tr + 1.0f);
        out->v[0] = root * 0.5f;
        root = 0.5f / root;
        out->v[1] = (m[6] - m[9]) * root;
        out->v[2] = (m[8] - m[2]) * root;
        out->v[3] = (m[1] - m[4]) * root;
        return;
    }
    int i = 0;
    if (m[5] > m[0]) i = 1;
    if (m[10] > m[i * 5]) i = 2;
    int j = nxt[i];
    int k = nxt[j];
    float root = (float)sqrt((m[i * 5] - (m[j * 5] + m[k * 5])) + 1.0f);
    q[i] = root * 0.5f;
    if (root != 0.0f) root = 0.5f / root;
    q[3] = (m[k + j * 4] - m[j + k * 4]) * root;
    q[j] = (m[i + j * 4] + m[j + i * 4]) * root;
    q[k] = (m[i + k * 4] + m[k + i * 4]) * root;
    out->v[1] = q[0];
    out->v[2] = q[1];
    out->v[3] = q[2];
    out->v[0] = q[3];
}

// @ 0x428c60
void EnvRec::alloc(int n)
{
    if (n != 0) {
        k0 = new vector4[n];
        k1 = new vector4[n];
        k2 = new vector4[n];
    } else {
        k0 = 0;
        k1 = 0;
        k2 = 0;
    }
}

// @ 0x428d10
FxElem::FxElem()
{
    qe4 = quat4(1.0f, 0.0f, 0.0f, 0.0f);
    f1d0 = 1.0f;
    f1d4 = 1.0f;
    vf4 = vector4(0.0f, 0.0f, 0.0f);
    src = 0;
    nchild = 0;
    f1ac = 0;
    parent = 0;
    children = 0;
    f1d8 = 0;
    quat4 t1 = quat4(1.0f, 0.0f, 0.0f, 0.0f);
    quat4 t2 = quat4(1.0f, 0.0f, 0.0f, 0.0f);
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
void mesh_ext::clear_geometry()
{
    {
        elem30 e;
        v60.resize(0, e);
    }
    {
        point20 e;
        v70.resize(0, e);
    }
    {
        face20 e;
        v90.resize(0, e);
    }
    {
        vertex60 e;
        v50.resize(0, e);
    }
    v80.resize(0);
}

// @ 0x428f30
void scene::reset()
{
    target = 0;
    f15c = 0;
    f1a4 = 0;
    for (unsigned i = 0; i < cache.size(); i++)
        ((mesh_ext*)*(cache.begin() + i))->clear_geometry();
}

// @ 0x428f80
void skeleton_entity::render_fx(scene* sc)
{
    if (f140) {
        for (unsigned i = 0; i < meshes.size(); i++)
            mesh_render(*(meshes.begin() + i), sc, local);
        if (fx)
            fx->render(sc, &local, &f144);
    }
}

} // namespace rgl

// Slice s00428b10: skeleton helpers: quaternion from a rotation matrix, envelope arrays, the FxElem ctor,
// resetting a mesh's geometry lists, skeleton_entity reset/render and FxDef::render.
#include <math.h>
#include "world.h"

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

// @ 0x428e80
void mesh::clear_geometry()
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
        (*(cache.begin() + i))->clear_geometry();
}

// @ 0x428f80
void skeleton_entity::render(scene* sc)
{
    if (f140) {
        for (unsigned i = 0; i < meshes.size(); i++)
            (*(meshes.begin() + i))->render(sc, *(mat4*)&local);
        if (fx)
            fx->render(sc, (mat4*)&local, &f144);
    }
}

// @ 0x428ff0
void FxDef::render(scene* sc, const mat4* frame, const float* dist)
{
    if (count != 0) {
        for (int i = 0; i < count; i++) {
            mat4 m;
            m = *frame * elems[i].m84;
            for (unsigned j = 0; j < elems[i].psys.size(); j++) {
                (*(elems[i].psys.begin() + j))->render(sc, m);
            }
            for (unsigned j = 0; j < elems[i].groups.size(); j++) {
                elems[i].groups[j]->select(dist);
                elems[i].groups[j]->render(sc, m);
            }
        }
        for (unsigned j = 0; j < psys.size(); j++) {
            if (!b58)
                skin(psys[j], 0);
            psys[j]->render(sc, *frame);
        }
        for (unsigned j = 0; j < groups.size(); j++) {
            groups[j]->select(dist);
            if (!b58)
                skin_group(groups[j]);
            groups[j]->render(sc, *frame);
        }
    }
}

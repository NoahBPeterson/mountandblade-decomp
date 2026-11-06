// fx_anim.cpp (0x421df0..0x422db0): the effect definition's per-frame animation. FxDef::update and its envelope and
// pose passes, the quaternion helpers, the animation record's phase and shape, the key-frame evaluators (vector and
// quaternion keys, slerp / nlerp, the bisecting key table) and the parent chain. Migrated from slices s00421aa0
// (0x421df0, 0x421ef0, 0x422080), s00422130, s00422db0 and s00423540 (its first six functions).
// Boundary: see MIGRATION.md.
#include <math.h>
#include "./fx_anim.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../include/frustum.h"
#include "../../include/keyframes.h"
#include "../../include/mat4.h"
#include "../../include/mesh_prims.h"
#include "../../include/quaternion.h"
#include "../../include/vec.h"
#include "../../include/mesh_parts.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../include/key_table.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../mesh.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../include/fx.h"
#include "../../game/slot.h"

using namespace rgl;

// FxAnim::value (0x423540) really returns its quaternion by value (the mangled name is
// ?bone_quat@FxAnim_x@@QAE?AUquaternion@@HHH@Z); the header spells it with an out-parameter. This local view
// gives the call its original signature so cl allocates the hidden return slot the way the original did.
struct FxAnimQ : FxAnim {
    __declspec(noinline) quaternion qvalue(int i, int j, int k);   // 0x423540
};

// local stand-ins for the vector4 operators the effect code inlines (rgl_math.h declares them out of line)

static inline float vnormalize(vector4& v)
{
    float len = (float)sqrt(v.x * v.x + (v.y * v.y + v.z * v.z));
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        v.x *= inv; v.y *= inv; v.z *= inv;
    } else {
        v.x = 0.0f; v.y = 1.0f; v.z = 0.0f;
    }
    return len;
}

// @ 0x421df0
void FxDef::update(float dt, const matrix* m)
{
    if (b58) return;
    FxAnim* self = (FxAnim*)this;
    if (!p64) self->update_envelopes(dt);
    f54 = dt + f54;
    b50 = false;
    if (p64) self->update_poses();
    else self->update_poses2();
    for (int i = 0; i < count; ++i) {
        for (std::vector<mesh*>::iterator it = elems[i].psys.begin(); it < elems[i].psys.end(); ++it) {
            if (!(*it)->kids.empty())
                ((mesh*)*it)->update_emitters(&dt, *m * elems[i].m84);
        }
    }
}

// @ 0x421ef0
void FxAnim::update_envelopes(float dt)
{
    float v = 1.0f;
    for (int i = 0; i < nenv; ++i) {
        if (env[i].target > 0.5f) v = env[i].attack;
        if (env[i].target > env[i].cur) {
            env[i].cur = dt / env[i].attack + env[i].cur;
            if (env[i].cur > env[i].target) env[i].cur = env[i].target;
        } else if (env[i].target < env[i].cur) {
            env[i].cur = env[i].cur - dt / env[i].release;
            if (env[i].cur < env[i].target) env[i].cur = env[i].target;
        }
    }
    for (int i = 0; i < count; ++i) {
        float w = 1.0f / (v + 0.5f);
        float s = (1.0f > w) ? 1.0f : w;
        elems[i].f1d0 = (1.0f - elems[i].f1d0) * elems[i].f1d4 * s * dt + elems[i].f1d0;
        if (elems[i].f1d0 > 1.0f) elems[i].f1d0 = 1.0f;
        else if (elems[i].f1d0 < 0.0f) elems[i].f1d0 = 0.0f;
    }
}

// @ 0x422080
void FxAnim::update_poses()
{
    for (int i = 0; i < count; ++i) {
        if (elems[i].parent == -1)
            *(matrix*)&elems[i].m84 = elems[i].m44;
        else
            *(matrix*)&elems[i].m84 = elems[elems[i].parent].m84 * elems[i].m44;
    }
    f54 = 0.0f;
    b50 = true;
}

// The slice wrote this function over the members of one object; the macros give the same field reads.
#define RC ((AnimRec*)env)
#define nrecs nenv
#define bones ((FxBone*)elems)
#define nbones count
#define BN(i) (*(FxBone*)((int)elems + (i) * sizeof(FxBone)))
#define dt f54

// @ 0x422130
void FxAnim::update_poses2()
{
    vector4 pos_old = v0c;
    for (int i = 0; i < nrecs; i++) {
        AnimRec& R = RC[i];
        float p = R.phase();
        R.f18 = R.shape(p);
    }
    for (int b = 0; b < nbones; b++) {
        quaternion saved = BN(b).rot();
        int found = -1;
        vector4 key0;
        quaternion q;
        for (int r = nrecs - 1; r >= 0; r--) {
            if (found != -1) break;
            if (!(RC[r].weight <= 0.0f) && ((unsigned)(RC[r].active_mask >> (unsigned char)b) & 1)) {
                if (b == 0) key0 = key_value(r);
                if (RC[r].bone == b) {
                    quaternion acc(0.0f, 0.0f, 0.0f, 0.0f);
                    for (int j = 0; j < 2; j++) {
                        if (RC[r].wt[j] > 0.001f) {
                            quaternion c;
                            c = ((FxAnimQ*)this)->qvalue(r, j, b);
                            const float* pw = &RC[r].wt[j];
                            quaternion t(c.x * *pw, c.y * *pw, c.z * *pw, c.w * *pw);
                            acc.add(t);
                        }
                    }
                    if (BN(b).parent == -1) {
                        q = acc;
                    } else {
                        quaternion pq; chain(&pq, r, BN(b).parent);
                        acc.normalize();
                        quaternion m = pq.mul(acc);
                        quaternion d = (BN(BN(b).parent).abs_rot()).mul_conj(m);
                        float* pa = &RC[r].blend_a;
                        float s = 1.0f - *pa;
                        quaternion t1(acc.x * s, acc.y * s, acc.z * s, acc.w * s);
                        quaternion t2(d.x * *pa, d.y * *pa, d.z * *pa, d.w * *pa);
                        q = t1 + t2;
                    }
                } else {
                    q = quaternion(0.0f, 0.0f, 0.0f, 0.0f);
                    for (int j = 0; j < 2; j++) {
                        if (RC[r].wt[j] > 0.001f) {
                            quaternion c;
                            c = ((FxAnimQ*)this)->qvalue(r, j, b);
                            const float* pw = &RC[r].wt[j];
                            quaternion t(c.x * *pw, c.y * *pw, c.z * *pw, c.w * *pw);
                            q.add(t);
                        }
                    }
                }
                found = r;
            }
        }
        if (found != -1) {
            q.normalize();
            if ((g_bit[b] & RC[found].flip_mask) != 0) {
                quaternion m = RC[found].ref[b].mul(q);
                if (q.dot_negative(m)) {
                    m.x = -m.x; m.y = -m.y; m.z = -m.z; m.w = -m.w;
                }
                q = m;
                if (b == 0) {
                    const float* ap = RC[found].add_pos;
                    key0.x = key0.x + ap[0];
                    key0.y = key0.y + ap[1];
                    key0.z = key0.z + ap[2];
                }
            }
            float wgt = RC[found].weight * BN(b).f1d0;
            float blend = wgt * dt * 30.0f;
            if (blend < 0.0f) blend = 0.0f;
            else if (blend > 1.0f) blend = 1.0f;
            if (wgt <= 0.99f) {
                quaternion t = BN(b).delta();
                if (t.w > 0.0f) t.w = t.w + 1.0f;
                else t.w = t.w - 1.0f;
                t.normalize();
                quaternion m = saved.mul(t);
                if (q.dot(m) < 0.0f) {
                    m.x = -m.x; m.y = -m.y; m.z = -m.z; m.w = -m.w;
                }
                float s = 1.0f - blend;
                quaternion qa;
                qa.set(m.x * s, m.y * s, m.z * s, m.w * s);
                quaternion qb;
                qb.set(q.x * blend, q.y * blend, q.z * blend, q.w * blend);
                BN(b).rot() = qa + qb;
                (BN(b).rot()).normalize();
            } else {
                BN(b).rot() = q;
            }
            if (b == 0) {
                float s = 1.0f - blend;
                v0c = vadd((key0 * blend), (pos_old * s));
            }
        }
        BN(b).delta() = saved.mul_conj(BN(b).rot());
        if (BN(b).parent == -1)
            BN(b).abs_rot() = BN(b).rot();
        else
            BN(b).abs_rot() = (BN(BN(b).parent).abs_rot()).mul(BN(b).rot());
        quat_to_matrix((matrix*)&BN(b).m44, &BN(b).rot());
        if (b == 0) {
            BN(0).m44.o = vadd(res->recs->m.o, v0c);
        }
        if (BN(b).parent == -1)
            *(matrix*)&BN(b).m84 = BN(b).m44;
        else
            *(matrix*)&BN(b).m84 = BN(BN(b).parent).m84 * BN(b).m44;
    }
    if (b68 && dt > 0.001f) {
        for (int b = 0; b < nbones; b++) {
            vector4 axis;
            float angle;
            quat_to_axis_angle(&axis, &angle, &BN(b).delta());
            vnormalize(axis);
            vector4 w;
            w = (axis * angle);
            BN(b).ang_vel() = (w * (1.0f / dt));
        }
        vector4 d = (v0c - pos_old);
        v6c = (d * (1.0f / dt));
    }
    dt = 0.0f;
    b50 = 1;
}
#undef dt
#undef RC
#undef nrecs
#undef bones
#undef nbones

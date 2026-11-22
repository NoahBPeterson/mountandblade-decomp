// fx_animation.cpp (0x421df0..0x422db0): the effect definition's per-frame animation. FxDef::update and its envelope and
// pose passes, the quaternion helpers, the animation record's phase and shape, the key-frame evaluators (vector and
// quaternion keys, slerp / nlerp, the bisecting key table) and the parent chain. Migrated from slices s00421aa0
// (0x421df0, 0x421ef0, 0x422080), s00422130, s00422db0 and s00423540 (its first six functions).
// Boundary: see MIGRATION.md.
#include <math.h>
#include "engine/fx/fx_animation.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "engine/scene/frustum.h"
#include "engine/fx/keyframes.h"
#include "engine/math/mat4_include.h"
#include "engine/render/mesh_prims.h"
#include "engine/math/quaternion.h"
#include "engine/math/vec.h"
#include "engine/render/mesh_parts.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "game/input/key_table.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "engine/render/mesh.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "engine/fx/fx.h"
#include "engine/resource/resource.h"                 // frame_set (FxDef::res)
#include "engine/resource/resource_io.h"         // Bone
#include "game/input/slot.h"

using namespace rgl;

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

// The pose pass picked by p64, as an inline helper: written in update's body the same if/else numbers the optimiser's
// expression entries one off and the loop's [index+elems] operands come out base/index swapped.
inline void update_pose_pass(FxDef* a) { if (a->pose_src) a->update_poses(); else a->update_poses2(); }

// @ 0x421df0
void FxDef::update(float dt, const matrix* m)
{
    if (frozen) return;
    FxDef* self = this;
    if (!pose_src) self->update_envelopes(dt);
    pose_dt = dt + pose_dt;
    pose_valid = false;
    update_pose_pass(self);
    for (int i = 0; i < count; ++i) {
        for (std::vector<mesh*>::iterator it = elems[i].psys.begin(); it < elems[i].psys.end(); ++it) {
            if (!(*it)->kids.empty())
                (*it)->update_emitters(&dt, *m * elems[i].m84);
        }
    }
}

// @ 0x421ef0
void FxDef::update_envelopes(float dt)
{
    float v = 1.0f;
    for (int i = 0; i < nenv; ++i) {
        if (env[i].target > 0.5f) v = env[i].attack;
        if (env[i].target > env[i].weight) {
            env[i].weight = dt / env[i].attack + env[i].weight;
            if (env[i].weight > env[i].target) env[i].weight = env[i].target;
        } else if (env[i].target < env[i].weight) {
            env[i].weight = env[i].weight - dt / env[i].release;
            if (env[i].weight < env[i].target) env[i].weight = env[i].target;
        }
    }
    for (int i = 0; i < count; ++i) {
        float w = 1.0f / (v + 0.5f);
        float s = (1.0f > w) ? 1.0f : w;
        elems[i].blend_weight = (1.0f - elems[i].blend_weight) * elems[i].blend_rate * s * dt + elems[i].blend_weight;
        if (elems[i].blend_weight > 1.0f) elems[i].blend_weight = 1.0f;
        else if (elems[i].blend_weight < 0.0f) elems[i].blend_weight = 0.0f;
    }
}

// @ 0x422080
void FxDef::update_poses()
{
    for (int i = 0; i < count; ++i) {
        if (elems[i].parent == -1)
            elems[i].m84 = elems[i].m44;
        else
            elems[i].m84 = elems[elems[i].parent].m84 * elems[i].m44;
    }
    pose_dt = 0.0f;
    pose_valid = true;
}

// the skeleton's root bone origin
static inline const rgl::vector4& root_origin(const frame_set* r) { return r->bones[0].local.o; }

// @ 0x422130
void FxDef::update_poses2()
{
    vector4 pos_old = root_pos;
    for (int i = 0; i < nenv; i++) {
        EnvRec& R = env[i];
        float p = R.phase();
        R.shaped_phase = R.shape(p);
    }
    for (int b = 0; b < count; b++) {
        quaternion saved = elems[b].q;
        int found = -1;
        vector4 key0;
        quaternion q;
        for (int r = nenv - 1; r >= 0; r--) {
            if (found != -1) break;
            if (!(env[r].weight <= 0.0f) && ((unsigned)(env[r].active_mask >> (unsigned char)b) & 1)) {
                if (b == 0) key0 = key_value(r);
                if (env[r].bone == b) {
                    quaternion acc(0.0f, 0.0f, 0.0f, 0.0f);
                    for (int j = 0; j < 2; j++) {
                        if (env[r].wt.w[j] > 0.001f) {
                            quaternion c;
                            c = value(r, j, b);
                            const float* pw = &env[r].wt.w[j];
                            quaternion t(c.x * *pw, c.y * *pw, c.z * *pw, c.w * *pw);
                            acc.add(t);
                        }
                    }
                    if (elems[b].parent == -1) {
                        q = acc;
                    } else {
                        quaternion pq = chain(r, elems[b].parent);
                        acc.normalize();
                        quaternion m = pq.mul(acc);
                        quaternion d = elems[elems[b].parent].abs_q.mul_conj(m);
                        float* pa = &env[r].blend_a;
                        float s = 1.0f - *pa;
                        quaternion t1(acc.x * s, acc.y * s, acc.z * s, acc.w * s);
                        quaternion t2(d.x * *pa, d.y * *pa, d.z * *pa, d.w * *pa);
                        q = t2 + t1;
                    }
                } else {
                    q = quaternion(0.0f, 0.0f, 0.0f, 0.0f);
                    for (int j = 0; j < 2; j++) {
                        if (env[r].wt.w[j] > 0.001f) {
                            quaternion c;
                            c = value(r, j, b);
                            const float* pw = &env[r].wt.w[j];
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
            if ((g_bit[b] & env[found].flip_mask) != 0) {
                quaternion m = env[found].ref[b].mul(q);
                if (q.dot_negative(m)) {
                    m.x = -m.x; m.y = -m.y; m.z = -m.z; m.w = -m.w;
                }
                q = m;
                if (b == 0) {
                    const vector4& ap = env[found].add_pos;
                    key0.x = key0.x + ap.x;
                    key0.y = key0.y + ap.y;
                    key0.z = key0.z + ap.z;
                }
            }
            float wgt = env[found].weight * elems[b].blend_weight;
            float blend = wgt * pose_dt * 30.0f;
            if (blend < 0.0f) blend = 0.0f;
            else if (blend > 1.0f) blend = 1.0f;
            if (wgt <= 0.99f) {
                quaternion t = elems[b].qe4;
                if (t.w > 0.0f) t.w = t.w + 1.0f;
                else t.w = t.w - 1.0f;
                t.normalize();
                quaternion m = saved.mul(t);
                if (q.dot(m) < 0.0f) {
                    m.x = -m.x; m.y = -m.y; m.z = -m.z; m.w = -m.w;
                }
                float s = 1.0f - blend;
                elems[b].q = quaternion().set(q.x * blend, q.y * blend, q.z * blend, q.w * blend)
                                       + quaternion().set(m.x * s, m.y * s, m.z * s, m.w * s);
                elems[b].q.normalize();
            } else {
                elems[b].q = q;
            }
            if (b == 0) {
                float s = 1.0f - blend;
                root_pos = vadd((key0 * blend), (pos_old * s));
            }
        }
        elems[b].qe4 = saved.mul_conj(elems[b].q);
        if (elems[b].parent == -1)
            elems[b].abs_q = elems[b].q;
        else
            elems[b].abs_q = elems[elems[b].parent].abs_q.mul(elems[b].q);
        quat_to_matrix(&elems[b].m44, &elems[b].q);
        if (b == 0) {
            elems[0].m44.o = root_pos + root_origin(res);
        }
        if (elems[b].parent == -1)
            elems[b].m84 = elems[b].m44;
        else
            elems[b].m84 = elems[elems[b].parent].m84 * elems[b].m44;
    }
    if (track_root_vel && pose_dt > 0.001f) {
        for (int b = 0; b < count; b++) {
            vector4 axis;
            float angle;
            quat_to_axis_angle(&axis, &angle, &elems[b].qe4);
            vnormalize(axis);
            vector4 w;
            w = (axis * angle);
            elems[b].vf4 = (w * (1.0f / pose_dt));
        }
        vector4 d = (root_pos - pos_old);
        root_vel = (d * (1.0f / pose_dt));
    }
    pose_dt = 0.0f;
    pose_valid = 1;
}

// fx_key_frames.cpp (0x422db0..0x423c00): the second object of fx_anim's range: the quaternion helpers, the animation record's
// phase and shape, the key-frame evaluators (vector and quaternion keys, slerp / nlerp, the bisecting key table) and the
// parent chain. update_poses2 (fx_animation.cpp, 0x422130) calls these as external functions. Migrated from slices
// s00422db0 and s00423540 (its first six functions).
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
#include "game/input/slot.h"

using namespace rgl;

// ---- quaternions ----------------------------------------------------------------------------------------------
// @ 0x422db0
quaternion& quaternion::set(const float& x_, const float& y_, const float& z_, const float& w_)
{
    x = x_; y = y_; z = z_; w = w_;
    return *this;
}
// @ 0x422de0
void quaternion::add(const quaternion& o)
{
    x += o.x; y += o.y; z += o.z; w += o.w;
}
// @ 0x422e10
quaternion quaternion::operator+(const quaternion& o) const
{
    return quaternion(o.x + x, o.y + y, o.z + z, o.w + w);
}
// @ 0x422e40
void quaternion::normalize()
{
    float len = (float)sqrt(w * w + x * x + y * y + z * z);
    if (len <= 1e-7f) {
        x = 1.0f; y = 0.0f; z = 0.0f; w = 0.0f;
    } else {
        float inv = 1.0f / len;
        x *= inv; y *= inv; z *= inv; w *= inv;
    }
}
// @ 0x422eb0
quaternion quaternion::mul(const quaternion& o) const
{
    quaternion r;
    float s = o.x * x;
    s += o.y * y;
    s += o.z * z;
    r.x = (y * o.z - o.y * z) + o.w * x + o.x * w;
    r.y = (o.x * z - o.z * x) + y * o.w + o.y * w;
    r.z = (o.y * x - o.x * y) + o.w * z + w * o.z;
    r.w = o.w * w - s;
    return r;
}
// @ 0x422f60
quaternion quaternion::mul_conj(const quaternion& o) const
{
    quaternion r;
    float s = x * o.x;
    s += o.z * z;
    s += o.y * y;
    s += o.w * w;
    r.x = (z * o.y - o.z * y) + w * o.x - x * o.w;
    r.y = (o.z * x - z * o.x) + w * o.y - o.w * y;
    r.z = (o.x * y - x * o.y) + o.z * w - z * o.w;
    r.w = s;
    return r;
}
// @ 0x423010
float quaternion::dot(const quaternion& o) const
{
    return o.z * z + o.y * y + o.x * x + o.w * w;
}
// @ 0x423040
bool quaternion::dot_negative(const quaternion& o) const
{
    return o.z * z + o.y * y + o.x * x + o.w * w < 0.0f;
}
// @ 0x423080
void quat_to_matrix(matrix* m, const quaternion* q)
{
    float x2 = q->x + q->x;
    float y2 = q->y + q->y;
    float z2 = q->z + q->z;
    float xx = x2 * q->x;
    float xy = y2 * q->x;
    float xz = z2 * q->x;
    float yy = y2 * q->y;
    float yz = z2 * q->y;
    float zz = z2 * q->z;
    float wx = x2 * q->w;
    float wy = y2 * q->w;
    float wz = z2 * q->w;
    m->s.x = 1.0f - (zz + yy);
    m->s.y = xy + wz;
    m->s.z = xz - wy;
    m->f.x = xy - wz;
    m->f.y = 1.0f - (zz + xx);
    m->f.z = wx + yz;
    m->u.x = wy + xz;
    m->u.y = yz - wx;
    m->u.z = 1.0f - (yy + xx);
}
// @ 0x423160
void quat_to_axis_angle(vector4* axis, float* angle, const quaternion* q)
{
    float w = q->w;
    if (w > 0.99999988f) {
        axis->x = 1.0f; axis->y = 0.0f; axis->z = 0.0f;
        *angle = 0.0f;
        return;
    }
    float s = (float)sqrt(1.0f - w * w);
    if (s < 1e-4f) s = 1.0f;
    float inv = 1.0f / s;
    axis->x = inv * q->x;
    axis->y = inv * q->y;
    axis->z = inv * q->z;
    *angle = (float)acos(w) * 2;
}

// ---- the animation record ---------------------------------------------------------------------------------------
// @ 0x4231f0
float EnvRec::phase()
{
    float t = (float)(g_ticks[tm.clock] - tm.start) * 1e-5f;
    if (mode == 20)
        return (float)fmod(t, period) / period;
    if (mode == 21) {
        if (!(t < period))
            return period / period;
    } else if (mode == 22) {
        if (!(t < period)) {
            flag = 0;
            return period / period;
        }
    }
    return t / period;
}
// @ 0x4232b0
float EnvRec::shape(float x)
{
    if (shape_mode == 1) return x;
    if (shape_mode == 2) return (float)sin(x * 1.5707964f);
    if (shape_mode == 5) return (float)sqrt(x);
    if (shape_mode == 4) return x * x;
    return 0.0f;
}

// @ 0x423300
vector4 FxDef::key_value(int i)
{
    vector4 v;
    if (env[i].type == 14) v = lerp_value(i);
    else if (env[i].type == 13) v = table_value(i);
    else if (env[i].type == 11) v = env[i].v7c[0];
    else v = env[i].v7c[1];
    return v;
}
// @ 0x423390
vector4 FxDef::lerp_value(int i)
{
    double t = 1.0f - env[i].shaped_phase;
    EnvRec* r = &env[i];
    vector4 v = vadd((r->v7c[1] * r->shaped_phase), (t * r->v7c[0]));
    return v;
}
// @ 0x423440
vector4 FxDef::table_value(int i)
{
    EnvRec* r = &env[i];
    if (!r->table) return vector4(0.0f, 0.0f, 0.0f);
    int a = r->key_begin;
    float key = (float)(r->key_end - a) * r->shaped_phase + a;
    float frac;
    r->table->tail.find(&key_idx, &frac, &key);
    double s = 1.0f - frac;
    const vector4* e = r->table->tail.data;
    vector4 v = vadd((frac * e[key_idx + 1]), (s * e[key_idx]));
    return v;
}

// ---- quaternion keys -------------------------------------------------------------------------------------------
// quaternion::dot_negative as value() has it: the body expanded inline
static __forceinline bool dot_neg(const quaternion& r, const quaternion& o) { return o.z * r.z + o.y * r.y + o.x * r.x + o.w * r.w < 0.0f; }

// @ 0x423540
quaternion FxDef::value(int i, int j, int k)
{
    int ofs = 0;
    if (env[i].table == 0)
        return quaternion(0.0f, 1.0f, 0.0f, 0.0f);   // x = 0, y = 1, z = 0, w = 0
    EnvRec* r = &env[i];
    if (j != 0)
        ofs = r->wt.offs[j];
    int span = r->key_end - r->key_begin;
    float key = (float)span * r->shaped_phase + (float)r->key_begin + (float)ofs;
    int* slot = &elems[k].t[i][j];
    int cur = *slot;
    float frac;
    r->table->keys[k].find(&cur, &frac, &key);
    quaternion q;
    if (cur != *slot || r->table != elems[k].c[i]) {
        *slot = cur;
        elems[k].c[i] = r->table;
        quaternion* va = r->table->keys[k].rots;
        elems[k].qa[i][j] = va[cur];
        elems[k].qb[i][j] = va[cur + 1];
        if (dot_neg(elems[k].q, elems[k].qa[i][j])) {
            elems[k].qa[i][j].x = -elems[k].qa[i][j].x;
            elems[k].qa[i][j].y = -elems[k].qa[i][j].y;
            elems[k].qa[i][j].z = -elems[k].qa[i][j].z;
            elems[k].qa[i][j].w = -elems[k].qa[i][j].w;
        }
        if (dot_neg(elems[k].q, elems[k].qb[i][j])) {
            elems[k].qb[i][j].x = -elems[k].qb[i][j].x;
            elems[k].qb[i][j].y = -elems[k].qb[i][j].y;
            elems[k].qb[i][j].z = -elems[k].qb[i][j].z;
            elems[k].qb[i][j].w = -elems[k].qb[i][j].w;
        }
    }
    if (g_keyboard.down[66] != 0) {
        quaternion* va = r->table->keys[k].rots;
        slerp(&q, &va[cur], &va[cur + 1], &frac);
    } else {
        nlerp(&q, &elems[k].qa[i][j], &elems[k].qb[i][j], &frac);
    }
    return q;
}

// @ 0x4238c0
void slerp(quaternion* out, const quaternion* a, const quaternion* b, const float* t)
{
    float c = a->x * b->x + a->z * b->z + a->y * b->y + b->w * a->w;
    float sgn;
    if (c < 0.0) {
        sgn = -1.0f;
        c = -c;
    } else {
        sgn = 1.0f;
    }
    float s0, s1;
    if (1.0 - c > 0.1f) {
        float om = (float)acos(c);
        float so = (float)sin(om);
        s0 = (float)sin((1.0f - *t) * om) * (1.0f / so);
        s1 = (float)sin(om * *t) * (1.0f / so);
    } else {
        s0 = 1.0f - *t;
        s1 = *t;
    }
    s1 *= sgn;
    out->x = s1 * b->x + s0 * a->x;
    out->y = s1 * b->y + s0 * a->y;
    out->z = s1 * b->z + s0 * a->z;
    out->w = s1 * b->w + s0 * a->w;
}

// @ 0x4239c0
void nlerp(quaternion* out, const quaternion* a, const quaternion* b, const float* t)
{
    float sgn;
    if (a->x * b->x + a->z * b->z + a->y * b->y + b->w * a->w < 0.0)
        sgn = -1.0f;
    else
        sgn = 1.0f;
    float s0 = 1.0f - *t;
    sgn = sgn * *t;
    out->x = sgn * b->x + s0 * a->x;
    out->y = sgn * b->y + s0 * a->y;
    out->z = sgn * b->z + s0 * a->z;
    out->w = sgn * b->w + s0 * a->w;
}

// @ 0x423a50
void key_array::find(int* cur, float* frac, float* key)
{
    float last = (float)keys[n - 1];
    float k = (float)keys[0];
    if (!(*key < k)) {
        if (*key >= last)
            k = last;
        else
            k = *key;
    }
    int c = *cur;
    if (c >= 0 && c < n - 1) {
        if ((float)keys[c] <= k) {
            if (k < (float)keys[c + 1])
                goto done;
            if (c < n - 2 && k < (float)keys[c + 2]) {
                *cur = ++c;
                goto done;
            }
        } else if (c >= 1 && (float)keys[c - 1] <= k) {
            *cur = --c;
            goto done;
        }
    }
    {
        int lo = 0;
        int hi = n - 1;
        while (lo < hi - 1) {
            int m = (hi + lo) >> 1;
            if (k < (float)keys[m])
                hi = m;
            else
                lo = m;
        }
        *cur = lo;
    }
done:
    int base = keys[*cur];
    *frac = (k - (float)base) / (float)(keys[*cur + 1] - base);
}

// @ 0x423b60
quaternion FxDef::chain(int i, int k)
{
    quaternion q = value(i, 0, k);
    if (elems[k].parent == -1)
        return q;
    quaternion m = chain(i, elems[k].parent).mul(q);
    return m;
}

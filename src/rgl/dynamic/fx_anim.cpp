// fx_anim.cpp (0x421df0..0x423c00): the effect definition's per-frame animation. FxDef::update and its envelope and
// pose passes, the quaternion helpers, the animation record's phase and shape, the key-frame evaluators (vector and
// quaternion keys, slerp / nlerp, the bisecting key table) and the parent chain. Migrated from slices s00421aa0
// (0x421df0, 0x421ef0, 0x422080), s00422130, s00422db0 and s00423540 (its first six functions).
// Boundary: see MIGRATION.md.
#include <math.h>
#include "./fx_anim.h"

using namespace rgl;

// The ext twins (fx_anim.h) are bound to the real functions here; the link has one function per address.
#pragma comment(linker, "/alternatename:??Hquaternion_x@@QBE?AUquaternion@@ABU1@@Z=??Hquaternion@@QBE?AU0@ABU0@@Z")
#pragma comment(linker, "/alternatename:?add@quaternion_x@@QAEXABUquaternion@@@Z=?add@quaternion@@QAEXABU1@@Z")
#pragma comment(linker, "/alternatename:?bone_quat@FxAnim_x@@QAE?AUquaternion@@HHH@Z=?value@FxAnim@@QAEPAUquaternion@@PAU2@HHH@Z")
#pragma comment(linker, "/alternatename:?dot@quaternion_x@@QBEMABUquaternion@@@Z=?dot@quaternion@@QBEMABU1@@Z")
#pragma comment(linker, "/alternatename:?dot_negative@quaternion_x@@QBE_NABUquaternion@@@Z=?dot_negative@quaternion@@QBE_NABU1@@Z")
#pragma comment(linker, "/alternatename:?find@KeyTable_x@@QAEXPAHPAM1@Z=?find@KeyTable@@QAEXPAHPAM1@Z")
#pragma comment(linker, "/alternatename:?key_value@FxAnim_x@@QAE?AUvector4@rgl@@H@Z=?key_value@FxAnim@@QAE?AUvector4@rgl@@H@Z")
#pragma comment(linker, "/alternatename:?mul@quaternion_x@@QBE?AUquaternion@@ABU2@@Z=?mul@quaternion@@QBE?AU1@ABU1@@Z")
#pragma comment(linker, "/alternatename:?mul_conj@quaternion_x@@QBE?AUquaternion@@ABU2@@Z=?mul_conj@quaternion@@QBE?AU1@ABU1@@Z")
#pragma comment(linker, "/alternatename:?mul_out@quaternion_x@@QBEPAUquaternion@@PAU2@PBU2@@Z=?mul@quaternion@@QBE?AU1@ABU1@@Z")
#pragma comment(linker, "/alternatename:?nlerp_x@@YAXPAUquaternion@@PBU1@1PBM@Z=?nlerp@@YAXPAUquaternion@@PBU1@1PBM@Z")
#pragma comment(linker, "/alternatename:?normalize@quaternion_x@@QAEXXZ=?normalize@quaternion@@QAEXXZ")
#pragma comment(linker, "/alternatename:?parent_quat@FxAnim_x@@QAE?AUquaternion@@HH@Z=?chain@FxAnim@@QAEPAUquaternion@@PAU2@HH@Z")
#pragma comment(linker, "/alternatename:?phase@AnimRec_x@@QAEMXZ=?phase@AnimRec@@QAEMXZ")
#pragma comment(linker, "/alternatename:?quat_to_axis_angle_x@@YAXPAUvector4@rgl@@PAMPBUquaternion@@@Z=?quat_to_axis_angle@@YAXPAUvector4@rgl@@PAMPBUquaternion@@@Z")
#pragma comment(linker, "/alternatename:?quat_to_matrix_x@@YAXPAUmatrix@rgl@@PBUquaternion@@@Z=?quat_to_matrix@@YAXPAUmatrix@rgl@@PBUquaternion@@@Z")
#pragma comment(linker, "/alternatename:?set@quaternion_x@@QAEAAUquaternion@@ABM000@Z=?set@quaternion@@QAEAAU1@ABM000@Z")
#pragma comment(linker, "/alternatename:?shape@AnimRec_x@@QAEMM@Z=?shape@AnimRec@@QAEMM@Z")
#pragma comment(linker, "/alternatename:?slerp_x@@YAXPAUquaternion@@PBU1@1PBM@Z=?slerp@@YAXPAUquaternion@@PBU1@1PBM@Z")

#define QX(e) ((quaternion_x&)(e))

// local stand-ins for the vector4 operators the effect code inlines (rgl_math.h declares them out of line)
static inline vector4 vmul(const vector4& a, const float& s) { return vector4(a.x * s, a.y * s, a.z * s); }
static inline vector4 vmul(const float& s, const vector4& a) { return vector4(s * a.x, s * a.y, s * a.z); }
static inline vector4 vmulv(const vector4& a, float s) { return vector4(a.x * s, a.y * s, a.z * s); }
static inline vector4 vadd(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
static inline vector4 vsub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }

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
                ((mesh_ext*)*it)->update_emitters(&dt, *m * elems[i].m84);
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
#define bones ((Bone*)elems)
#define nbones count
#define BN(i) (*(Bone*)((int)elems + (i) * sizeof(Bone)))
#define dt f54
#define X ((FxAnim_x*)this)

// @ 0x422130
void FxAnim::update_poses2()
{
    vector4 pos_old = v0c;
    for (int i = 0; i < nrecs; i++) {
        AnimRec& R = RC[i];
        float p = ((AnimRec_x&)R).phase();
        R.f18 = ((AnimRec_x&)R).shape(p);
    }
    for (int b = 0; b < nbones; b++) {
        quaternion saved = BN(b).rot();
        int found = -1;
        vector4 key0;
        quaternion q;
        for (int r = nrecs - 1; r >= 0; r--) {
            if (found != -1) break;
            if (!(RC[r].weight <= 0.0f) && ((unsigned)(RC[r].active_mask >> (unsigned char)b) & 1)) {
                if (b == 0) key0 = X->key_value(r);
                if (RC[r].bone == b) {
                    quaternion acc(0.0f, 0.0f, 0.0f, 0.0f);
                    for (int j = 0; j < 2; j++) {
                        if (RC[r].wt[j] > 0.001f) {
                            quaternion c; c = X->bone_quat(r, j, b);
                            const float* pw = &RC[r].wt[j];
                            quaternion t(c.x * *pw, c.y * *pw, c.z * *pw, c.w * *pw);
                            QX(acc).add(t);
                        }
                    }
                    if (BN(b).parent == -1) {
                        q = acc;
                    } else {
                        quaternion pq = X->parent_quat(r, BN(b).parent);
                        QX(acc).normalize();
                        quaternion m = QX(pq).mul(acc);
                        quaternion d = QX(BN(BN(b).parent).abs_rot()).mul_conj(m);
                        float* pa = &RC[r].blend_a;
                        float s = 1.0f - *pa;
                        quaternion t1(acc.x * s, acc.y * s, acc.z * s, acc.w * s);
                        quaternion t2(d.x * *pa, d.y * *pa, d.z * *pa, d.w * *pa);
                        q = QX(t1) + t2;
                    }
                } else {
                    q = quaternion(0.0f, 0.0f, 0.0f, 0.0f);
                    for (int j = 0; j < 2; j++) {
                        if (RC[r].wt[j] > 0.001f) {
                            quaternion c; c = X->bone_quat(r, j, b);
                            const float* pw = &RC[r].wt[j];
                            quaternion t(c.x * *pw, c.y * *pw, c.z * *pw, c.w * *pw);
                            QX(q).add(t);
                        }
                    }
                }
                found = r;
            }
        }
        if (found != -1) {
            QX(q).normalize();
            if ((g_bit[b] & RC[found].flip_mask) != 0) {
                quaternion m = QX(RC[found].ref[b]).mul(q);
                if (QX(q).dot_negative(m)) {
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
                QX(t).normalize();
                quaternion m = QX(saved).mul(t);
                if (QX(q).dot(m) < 0.0f) {
                    m.x = -m.x; m.y = -m.y; m.z = -m.z; m.w = -m.w;
                }
                float s = 1.0f - blend;
                quaternion qa;
                QX(qa).set(m.x * s, m.y * s, m.z * s, m.w * s);
                quaternion qb;
                QX(qb).set(q.x * blend, q.y * blend, q.z * blend, q.w * blend);
                BN(b).rot() = QX(qa) + qb;
                QX(BN(b).rot()).normalize();
            } else {
                BN(b).rot() = q;
            }
            if (b == 0) {
                float s = 1.0f - blend;
                v0c = vadd(vmul(key0, blend), vmul(pos_old, s));
            }
        }
        BN(b).delta() = QX(saved).mul_conj(BN(b).rot());
        if (BN(b).parent == -1)
            BN(b).abs_rot() = BN(b).rot();
        else
            BN(b).abs_rot() = QX(BN(BN(b).parent).abs_rot()).mul(BN(b).rot());
        quat_to_matrix_x((matrix*)&BN(b).m44, &BN(b).rot());
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
            quat_to_axis_angle_x(&axis, &angle, &BN(b).delta());
            vnormalize(axis);
            vector4 w;
            w = vmul(axis, angle);
            BN(b).ang_vel() = vmulv(w, 1.0f / dt);
        }
        vector4 d = vsub(v0c, pos_old);
        v6c = vmul(d, 1.0f / dt);
    }
    dt = 0.0f;
    b50 = 1;
}
#undef dt
#undef X
#undef RC
#undef nrecs
#undef bones
#undef nbones

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
    r.x = ((y * o.z - o.y * z) + o.w * x) + o.x * w;
    r.y = ((o.x * z - o.z * x) + y * o.w) + o.y * w;
    r.z = ((o.y * x - o.x * y) + o.w * z) + w * o.z;
    r.w = o.w * w - ((o.x * x + o.y * y) + o.z * z);
    return r;
}
// @ 0x422f60
quaternion quaternion::mul_conj(const quaternion& o) const
{
    quaternion r;
    r.x = ((z * o.y - o.z * y) + w * o.x) - x * o.w;
    r.y = ((o.z * x - z * o.x) + w * o.y) - o.w * y;
    r.z = ((o.x * y - x * o.y) + o.z * w) - z * o.w;
    r.w = ((x * o.x + o.z * z) + o.y * y) + o.w * w;
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
float AnimRec::phase()
{
    float t = (float)(g_time[clock] - start) * 1e-5f;
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
float AnimRec::shape(float x)
{
    if (shape_mode == 1) return x;
    if (shape_mode == 2) return (float)sin(x * 1.5707964f);
    if (shape_mode == 5) return (float)sqrt(x);
    if (shape_mode == 4) return x * x;
    return 0.0f;
}

// @ 0x423300
vector4 FxAnim::key_value(int i)
{
    vector4 v;
    if (rec(i)->type == 14) v = lerp_value(i);
    else if (rec(i)->type == 13) v = table_value(i);
    else if (rec(i)->type == 11) v = rec(i)->v7c;
    else v = rec(i)->v8c;
    return v;
}
// @ 0x423390
vector4 FxAnim::lerp_value(int i)
{
    float t = 1.0f - rec(i)->f18;
    AnimRec* r = rec(i);
    vector4 v = vadd(vmul(r->v8c, r->f18), vmul(t, r->v7c));
    return v;
}
// @ 0x423440
vector4 FxAnim::table_value(int i)
{
    AnimRec& r = *rec(i);
    if (!r.table) return vector4(0.0f, 0.0f, 0.0f);
    int a = r.key_begin;
    float key = (float)(r.key_end - a) * r.f18 + a;
    float frac;
    ((KeyTable_x&)r.table->tbl).find(&f1c, &frac, &key);
    float s = 1.0f - frac;
    const vector4* e = &r.table->tbl.v4()[f1c];
    vector4 v = vadd(vmul(frac, e[1]), vmul(s, e[0]));
    return v;
}

// ---- quaternion keys -------------------------------------------------------------------------------------------
// quaternion::dot_negative as value() has it: the body expanded inline
static __forceinline bool dot_neg(const quaternion& r, const quaternion& o) { return o.z * r.z + o.y * r.y + o.x * r.x + o.w * r.w < 0.0f; }

// @ 0x423540
quaternion* FxAnim::value(quaternion* out, int i, int j, int k)
{
    int ofs = 0;
    if (rec(i)->table == 0) {
        out->x = 0.0f;
        out->y = 1.0f;
        out->z = 0.0f;
        out->w = 0.0f;
        return out;
    }
    AnimRec* r = rec(i);
    if (j != 0)
        ofs = r->offs[j];
    int span = r->key_end - r->key_begin;
    float key = (float)span * r->f18 + (float)r->key_begin + (float)ofs;
    int slot = (k * 0x77 + 2 * i + j) * 4 + 0x104;
    int cur = *(int*)((char*)elems + slot);
    float frac;
    ((KeyTable_x&)r->table->tables[k]).find(&cur, &frac, &key);
    quaternion q;
#define BK bone(k)
    if (cur != *(int*)((char*)elems + slot) || r->table != BK->tab()[i]) {
        *(int*)((char*)elems + slot) = cur;
        BK->tab()[i] = r->table;
        quaternion* va = r->table->tables[k].qv();
        BK->ka()[2 * i + j] = va[cur];
        BK->kb()[2 * i + j] = va[cur + 1];
        if (dot_neg(BK->rot(), BK->ka()[2 * i + j])) {
            BK->ka()[2 * i + j].x = -BK->ka()[2 * i + j].x;
            BK->ka()[2 * i + j].y = -BK->ka()[2 * i + j].y;
            BK->ka()[2 * i + j].z = -BK->ka()[2 * i + j].z;
            BK->ka()[2 * i + j].w = -BK->ka()[2 * i + j].w;
        }
        if (dot_neg(BK->rot(), BK->kb()[2 * i + j])) {
            BK->kb()[2 * i + j].x = -BK->kb()[2 * i + j].x;
            BK->kb()[2 * i + j].y = -BK->kb()[2 * i + j].y;
            BK->kb()[2 * i + j].z = -BK->kb()[2 * i + j].z;
            BK->kb()[2 * i + j].w = -BK->kb()[2 * i + j].w;
        }
    }
    if (g_use_slerp != 0) {
        quaternion* va = r->table->tables[k].qv();
        slerp_x(&q, &va[cur], &va[cur + 1], &frac);
    } else {
        nlerp_x(&q, &BK->ka()[2 * i + j], &BK->kb()[2 * i + j], &frac);
    }
#undef BK
    *out = q;
    return out;
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
void KeyTable::find(int* cur, float* frac, float* key)
{
    int n = count;
    float last = (float)keys[n - 1];
    float k = (float)keys[0];
    if (!(*key < k)) {
        k = last;
        if (!(last <= *key))
            k = *key;
    }
    int c = *cur;
    if (c > -1 && c < n - 1) {
        if ((float)keys[c] <= k) {
            if (k < (float)keys[c + 1])
                goto done;
            if (c < n - 2 && k < (float)keys[c + 2]) {
                *cur = c + 1;
                goto done;
            }
        } else if (0 < c && (float)keys[c - 1] <= k) {
            *cur = c - 1;
            goto done;
        }
    }
    {
        int hi = n - 1;
        int lo = 0;
        if (n != 2 && -1 < n - 2) {
            int m = lo;
            do {
                m = (hi + lo) >> 1;
                if (k < (float)keys[m]) {
                    hi = m;
                    m = lo;
                }
                lo = m;
            } while (m < hi - 1);
        }
        *cur = lo;
    }
done:
    int base = keys[*cur];
    *frac = (k - (float)base) / (float)(keys[*cur + 1] - base);
}

// @ 0x423b60
quaternion* FxAnim::chain(quaternion* out, int i, int k)
{
    quaternion t, t3;
    quaternion* p = value(&t, i, 0, k);
    quaternion q = *p;
    int parent = bone(k)->parent;
    if (parent != -1) {
        chain(&t, i, parent);
        p = QX(t).mul_out(&t3, &q);
    }
    *out = *p;
    return out;
}

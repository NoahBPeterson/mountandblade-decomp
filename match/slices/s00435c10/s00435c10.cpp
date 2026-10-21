// Slice s00435c10: agent camera setup and steering helpers.
#include "s00435c10.h"

// 0x434b70 Pair::value (other slice)

static inline vector4 vsub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
static inline vector4 operator+(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }

static inline void cross_inl(vector4& r, const vector4& a, const vector4& b)
{ r.x = a.y * b.z - a.z * b.y; r.y = a.z * b.x - a.x * b.z; r.z = a.x * b.y - a.y * b.x; }

// @ 0x435c10
void Agent::camera_params(CamParams* p)
{
    if (f10 == 0) {
        vector4 a(f80 * 0.0f, f80 * 0.0f, f2c == -1 ? f80 * 0.95f : f80 * 1.5f);
        float dx = a.x - v98.x;
        float dy = a.y - v98.y;
        float dz = a.z - v98.z;
        p->v40 = vector4(f80 * 0.0f + dx, f80 * 0.0f + dy, f80 * -0.2f + dz);
        p->v50 = vector4(f80 * 0.0f + dx, f80 * 0.0f + dy, f80 * 0.5f + dz);
        p->f60 = 0.37f;
        if (f250 && (fe8 & 0x800000)) {
            p->v50 = f250->f304->f4->v1170;
            Z_* z = f250->f304->f4;
            p->v40.x = (z->f46c + z->fa00) * 0.5f;
            p->v40.y = (z->fa04 + z->f470) * 0.5f;
            p->v40.z = (z->fa08 + z->f474) * 0.5f;
        }
    } else {
        p->v40 = vector4(0.0f, -0.3f, 1.05f);
        p->v50 = vector4(0.0f, 0.67f, 1.15f);
        p->f60 = 0.4f;
    }
    if (f57d8) {
        p->v40.z = 2.0f;
        p->v50.z = 2.1f;
        p->f60 = 0.01f;
    }
    if (f10 == 0 && f5830 != 2) {
        p->v40.z = 4.0f;
        p->v50.z = 4.1f;
        p->f60 = 0.001f;
        return;
    }
    if (f10 == 1 && f14 != 1) {
        p->v40.z = 5.0f;
        p->v50.z = 5.1f;
        p->f60 = 0.001f;
    }
}
// @ 0x435f10
void Agent::steer(float dt)
{
    float k = 3.0f;
    if (f8 == g_mission->f5c) k = 6.0f;
    float* pv = &vel.x;
    float d = vel.z * dirz + diry * pv[1] + vel.x * dirx;
    if (!(d > 0.0f)) d = 0.0f;
    float s = (d * 5.0f + k) * dt;
    vector4 add(s * dirx, s * diry, s * dirz);
    pv[0] += add.x;
    pv[1] += add.y;
    pv[2] += add.z;
    vel.normalize();
}
// @ 0x435fc0
Pair Agent::best_weapon_slot() const
{
    for (int i = 3; i >= 0; i--) {
        const Pair& p = pairs[i];
        if (p.a < 0 || (char)g_items[p.a].type != 7) continue;
        if (slots[1] == i && slots[4] == 0) continue;
        if ((int)(p.b & 0xffff) <= 10) continue;
        return pairs[i];
    }
    Pair r;
    r.a = -1;
    return r;
}
// @ 0x436050
float Agent::equipment_value() const
{
    float sum = 0.0f;
    const Pair* p = pairs;
    for (int n = 8; n != 0; n--, p++) {
        if (p->a >= 0) {
            float v = g_items[p->a].f208;
            unsigned t = g_items[p->a].type & 0xff;
            if (t == 5 || t == 6 || t == 0x12 || t == 10) {
                int cur = p->b & 0xff;
                v = (float)(p->value() - cur) / (float)p->value() * v;
            } else if (t == 7 && (int)(p->b & 0xffff) <= 0) {
                v = 0.0f;
            }
            sum += v;
        }
    }
    return sum;
}
// @ 0x436130
void Agent::update_pitch(float dt)
{
    if (f10 != 0 && fbc != 0) {
        const Obj_a0* o = fbc;
        vector4 c; cross_inl(c, o->d, vel);
        vector4 u; cross_inl(u, o->d, c);
        u.normalize();
        f56cc = (float)((asin(u.z) - f56cc) * dt * 8.0f + f56cc);
    }
}
// @ 0x436210
void __cdecl rotate_scale_xy(vector4* out, const vector4* in, const float* angle, const vector4* scale)
{
    vector4 t = *in;
    float a = *angle;
    float c = (float)cos(a);
    double s = sin(a);
    t.z = 0.0f;
    float tx = t.x * c - t.y * s;
    float ty = t.x * s + t.y * c;
    t.y = ty; t.x = tx;
    t.normalize();
    float len = (float)sqrt(scale->x * scale->x + scale->y * scale->y);
    t.x = len * t.x; t.y = len * t.y; t.z = scale->z;
    *out = t;
}

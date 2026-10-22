// Slice s00432a50: Newton physics force callback, a 64-bit tick timer, random cone direction, a collision-response
// helper, weighted random pick over an actor's item records, std::copy instantiations for tstring and a 12-byte
// vector, and the sphere / capsule shape constructors that take arguments.  Names are provisional.
#include <stdlib.h>
#include <math.h>
#include <vector>
#pragma intrinsic(sin, cos)
#include "rgl.h"

using rgl::vector4;
using rgl::matrix;

// ---- Newton Game Dynamics imports (cdecl) -----------------------------------------------------------
struct NewtonBody;
extern "C" {
__declspec(dllimport) void __cdecl NewtonBodyGetMassMatrix(const NewtonBody* body, float* mass, float* ixx, float* iyy, float* izz);   // 0x549298
__declspec(dllimport) void __cdecl NewtonBodyAddForce(const NewtonBody* body, const float* force);       // 0x54929c
__declspec(dllimport) void __cdecl NewtonBodyAddTorque(const NewtonBody* body, const float* torque);     // 0x5492a0
}

// A force/torque the game wants applied to one body on the next physics step.
struct PendingForce {
    const NewtonBody* body;       // 0xa18210
    vector4 force;                // 0xa18214
    vector4 torque;               // 0xa18224
};
extern PendingForce g_pending;    // 0xa18210

// @ 0x432a50
void __cdecl apply_force_and_torque(const NewtonBody* body)
{
    float mass, ixx, iyy, izz;
    NewtonBodyGetMassMatrix(body, &mass, &ixx, &iyy, &izz);
    if (g_pending.body == body) {
        float f[3];
        f[0] = g_pending.force.x * mass;
        f[1] = g_pending.force.y * mass;
        f[2] = (g_pending.force.z - 9.8f) * mass;
        NewtonBodyAddForce(body, f);
        if (g_pending.torque.x * g_pending.torque.x + g_pending.torque.y * g_pending.torque.y +
                g_pending.torque.z * g_pending.torque.z > 0.0f) {
            f[0] = g_pending.torque.x * mass;
            f[1] = g_pending.torque.y * mass;
            f[2] = g_pending.torque.z * mass;
            NewtonBodyAddTorque(body, f);
        }
        g_pending.force = vector4(0.0f, 0.0f, 0.0f);
        g_pending.body = 0;
        g_pending.torque = vector4(0.0f, 0.0f, 0.0f);
    } else {
        float f[3];
        f[0] = 0.0f;
        f[1] = 0.0f;
        f[2] = mass * -9.8f;
        NewtonBodyAddForce(body, f);
    }
}

// ---- tick timer -------------------------------------------------------------------------------------
extern __int64 g_ticks[];      // 0x5e2948: 64-bit counters, 100 kHz units

struct Timer {
    int clock;                 // +0 index into g_ticks
    int pad4;
    __int64 start;             // +8
    char active;               // +0x10
    void restart();            // 0x432bf0
    float elapsed() const;     // 0x432c10
    void begin(float seconds); // 0x432da0
    bool expired();            // 0x432de0
};
// @ 0x432bf0
void Timer::restart() { start = g_ticks[clock]; }
// @ 0x432c10
float Timer::elapsed() const { return (float)(g_ticks[clock] - start) * 1e-5f; }
// @ 0x432da0
void Timer::begin(float seconds)
{
    if (!active) {
        start = g_ticks[clock] - (int)(seconds * -100000.0f);
        active = 1;
    }
}
// @ 0x432de0
bool Timer::expired()
{
    if (active) {
        float e = (float)(g_ticks[clock] - start) * 1e-5f;
        if (e >= 0.0f) {
            active = 0;
            return true;
        }
    }
    return false;
}

// ---- random direction inside a cone around `dir` ------------------------------------------------------
struct mat3 { vector4 s, f, u; };
// @ 0x432c40
void __cdecl rand_cone_dir(vector4* out, vector4 dir, float spread)
{
    float a = (float)(rand() % 0x3dc9) * 6.322312e-05f - 0.5f;
    float ang = a * spread;
    ang = ang + ang;
    float c = cos(ang);
    float s = sin(ang);
    float x1 = c * 1.0f - s * 0.0f;
    float y1 = c * 0.0f + s;
    float phi = (float)(rand() % 0x3dc9) * 0.00039724255f;
    float c2 = cos(phi);
    float s2 = sin(phi);
    float B = y1 * s2 + c2 * 0.0f;
    float A = y1 * c2 - s2 * 0.0f;
    mat3 m;
    m.f = dir;
    m.u = vector4(0.0f, 0.0f, 1.0f);
    ((matrix*)&m)->orthonormalize();
    float px = m.u.x * A + m.f.x * x1;
    out->x = px + m.s.x * B;
    out->y = m.u.y * A + m.s.y * B + m.f.y * x1;
    out->z = m.u.z * A + m.s.z * B + m.f.z * x1;
}

// ---- collision response: removes the approaching component of v along n, with friction ------------------
// 12-byte float triple with the engine's inline operators (the same type as Vec3 below).
struct Vec3 {
    float x, y, z;
    Vec3() {}
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    Vec3 operator-() const { return Vec3(-x, -y, -z); }
    Vec3 operator+(const Vec3& o) const { return Vec3(x + o.x, y + o.y, z + o.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
    Vec3& operator-=(const Vec3& o) { x = x - o.x; y = y - o.y; z = z - o.z; return *this; }
    float dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
};
// @ 0x432e30
void __cdecl bounce(Vec3* out, Vec3* v, const Vec3* n)
{
    float d = (-*n).dot(*v);
    if (d < 0.0f) d = 0.0f;
    Vec3 t = *n * d;
    Vec3 s;
    s.x = t.x + v->x; s.y = t.y + v->y; s.z = t.z + v->z;
    v->x = s.x; v->y = s.y; v->z = s.z;
    if (d > 0.0f) {
        Vec3 g = s * d;
        Vec3 f = g * 0.4f;
        v->x = s.x - f.x; v->y = s.y - f.y; v->z = s.z - f.z;
        t -= f;
    }
    out->x = -t.x; out->y = -t.y; out->z = -t.z;
}

// ---- weighted random pick among an actor's item records -------------------------------------------------
struct ItemRec { int pad0, pad4; unsigned flags; char pad_c[0x40 - 0xc]; };     // 0x40 bytes, flags at +8
struct Record { char pad0[0x10]; ItemRec* items; int count; char pad18[0x30 - 0x18]; };   // 0x30 bytes
struct RecordTable { Record* recs; };
struct ItemDef { char pad0[0x30]; __int64 flags; char pad38[0x508 - 0x38]; };    // 0x508 bytes, flags at +0x30
struct GameData { char pad[0x13a00]; ItemDef* items; };
extern GameData* g_game;       // 0x5e2930

struct Entry { char pad0[0xd8 - 0]; };
struct Actor {
    char pad0[0x2c];
    int f2c;
    char pad30[0x88 - 0x30];
    int f88;                   // +0x88 item index (-1 = none)
    char pad8c[0xd8 - 0x8c];
    struct { int rec; char pad[0x2c - 4]; } entries[1];   // +0xd8, stride 0x2c
    char pad_after[0x12c - 0xd8 - 0x2c];
    RecordTable* tbl;          // +0x12c
    unsigned char flags130;    // +0x130
    void fill_weights(float* out, int idx, int rec);      // 0x432f60 (idx unused)
    void fill_weights_x(float* out, int idx, int rec);    // 0x432f60 (declaration only: the callee is opaque to the caller)
    int pick(int idx);                                    // 0x433040
    int hand_kind() const;                                // 0x433140
};

// @ 0x432f60
void Actor::fill_weights(float* out, int idx, int rec)
{
    int best = -100;
    for (int i = 0; i < tbl->recs[rec].count; i++) {
        unsigned fl = tbl->recs[rec].items[i].flags;
        int score = 0;
        if (fl & 0x1000000) score = (flags130 & 4) ? 2 : -10;
        if (fl & 0x2000000) {
            if ((flags130 & 8) && f2c != -1) score += 2;
            else score -= 10;
        }
        if (score == best) {
            out[i] = 1.0f;
        } else if (score > best) {
            out[i] = 1.0f;
            for (int j = 0; j < i; j++) out[j] = 0.0f;
            best = score;
        } else {
            out[i] = 0.0f;
        }
    }
}

// @ 0x433040
int Actor::pick(int idx)
{
    float w[64];
    float r;
    float total;
    int rec = entries[idx].rec;
    total = 0.0f;
    int res = -1;
    fill_weights_x(w, idx, rec);
    for (int i = 0; i < tbl->recs[rec].count; i++) total += w[i];
    r = (float)(rand() % 0x3dc9) * total * 6.322312e-05f;
    int n = tbl->recs[rec].count;
    float acc = 0.0f;
    for (int i = 0; i < n; i++) {
        acc += w[i];
        if (r <= acc) { res = i; break; }
    }
    return res;
}

// @ 0x433140
int Actor::hand_kind() const
{
    int r = 0;
    if (f88 >= 0) {
        if (f2c >= 0) r = 2;
        else {
            if (g_game->items[f88].flags & 0x4000000i64) r = 1;
            else r = 0;
        }
    }
    return r;
}

// ---- std::copy / copy_backward instantiations (VC7.1 <xutility>) --------------------------------------
struct tstring {
    int pos; char* buf; int cap; int len;
    void operator=(const tstring& o);          // 0x42bc50
};

#include <algorithm>
namespace std {
// 0x4331e0 (forward copy of 16-byte strings), 0x433220 (backward), 0x433260 (backward copy of 12-byte records)
template tstring* _Copy_opt<tstring*, tstring*>(tstring*, tstring*, tstring*, _Nonscalar_ptr_iterator_tag);
template tstring* _Copy_backward_opt<tstring*, tstring*>(tstring*, tstring*, tstring*, _Nonscalar_ptr_iterator_tag);
template Vec3* _Copy_backward_opt<Vec3*, Vec3*>(Vec3*, Vec3*, Vec3*, _Nonscalar_ptr_iterator_tag);
}

// ---- collision shapes (layouts from s00408110 / s0041ac10; ctors that take arguments) -------------------
struct shape_base {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    shape_base() : m4(-1), m8(8), mc(-1), min_x(100000000), min_y(100000000), max_x(-100000000), max_y(-100000000) {}
    virtual ~shape_base();
};
struct sphere_shape : shape_base {             // vtable 0x54a4f0, size 0x44
    vector4 v20, v30;
    float radius;
    sphere_shape(vector4 center, const float* r);          // 0x4332a0
};
struct capsule : shape_base {                  // vtable 0x54a540, size 0x64
    vector4 v20, v30, v40, v50;
    float radius;
    capsule(float r, const vector4* a, const vector4* b);   // 0x433310
};
// @ 0x4332a0
sphere_shape::sphere_shape(vector4 center, const float* r)
{
    v20 = center;
    v30 = center;
    radius = *r;
}
// @ 0x433310
capsule::capsule(float r, const vector4* a, const vector4* b)
{
    radius = r;
    v40 = *a;
    v50 = *b;
    v20 = v40;
    v30 = v50;
}

// ---- mesh morph time clamp (mesh layout: s00413320/mesh.h; `times` is read as ints here) ----------------
struct keyframes12 { void* frames; int* times; int count; };
struct mesh_node {
    char pad0[0x18];
    mesh_node* parent;                         // +0x18
    char pad1c[0xa1 - 0x1c];
    char local_a1;                             // +0xa1
    char pada2[2];
    keyframes12 kf;                            // +0xa4
    char padb0[0xc8 - 0xb0];
    float c8;                                  // +0xc8
    keyframes12& get_kf();                     // 0x40ec00 (local_a1 ? kf : parent->get_kf())
    void clamp_c8(const float* t);             // 0x433190
};
// @ 0x433190
void mesh_node::clamp_c8(const float* t)
{
    keyframes12& k = local_a1 ? kf : parent->get_kf();
    if (k.count > 0) {
        if (*t > (float)k.times[k.count - 1]) c8 = (float)k.times[k.count - 1];
        else c8 = *t;
    }
}

//-----------------------------------------------------------------------------
// File: game_util.cpp  (0x4320b0..0x4338df)
//
// Desc: small game-logic helpers: key-code group tables, agent team-relation tests, scalar min/max/clamp/round/
//       rand helpers, 2-D rotate, 3x3 frame algebra, the Newton force callback, the tick timer, a random cone
//       direction, a collision-response helper, the actor's weighted item pick, std::copy instantiations, the
//       argument constructors of the sphere / capsule shapes, collision face helpers, the composite copy
//       constructor, a message ring, the height-field sampler and the scene object's placement setters.
//       Functions in ascending address order (cl's bottom-up emission).
//       Flags: /O2 /MT /Gy /TP. Names provisional.
//-----------------------------------------------------------------------------
#include <math.h>
#include <stdlib.h>
#include <algorithm>
#pragma intrinsic(sin, cos, fabs)
#include "game_util.h"
#include "key_groups.h"      // group_base: defined in key_groups.cpp (another object in the original)
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../include/mat4.h"
#include "../../include/quaternion.h"
#include "../../include/timer.h"
#include "../../include/vec.h"
#include "../../include/shape.h"      // (the shared class headers named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/height_field.h"
#include "../../game/placed_obj.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../game/skeleton.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../game/agent_obj.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../game/strat/gui_manager.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../game/channel.h"      // (Channel: the 0xd8-byte animation record whose busy() 0x433840 ends this object)
#include "../../game/mission.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/troop.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/clock_table.h"

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----


// ---- Newton Game Dynamics imports (cdecl) -----------------------------------------------------------
extern "C" {
__declspec(dllimport) void __cdecl NewtonBodyGetMassMatrix(const NewtonBody* body, float* mass, float* ixx, float* iyy, float* izz);   // 0x549298
__declspec(dllimport) void __cdecl NewtonBodyAddForce(const NewtonBody* body, const float* force);       // 0x54929c
__declspec(dllimport) void __cdecl NewtonBodyAddTorque(const NewtonBody* body, const float* torque);     // 0x5492a0
}

// ---- team relations ---------------------------------------------------------------------------------
// The mission object (g_mission, 0x5e292c) keeps the relation matrix (stride 0x5b ints) at +0x53c8: -1 enemy, 1 friend.
// @ 0x432350
bool AgentX::is_enemy(const Agent* o) const
{
    if (team >= 0 && team < 8 && o->team >= 0 && o->team < 8)
        return g_mission->rel_flat()[team * 0x5b + o->team] == -1;
    return false;
}
// @ 0x4323a0
__declspec(noinline) bool Agent::is_friend(const Agent* o) const
{
    if (team >= 0 && team < 8 && o->team >= 0 && o->team < 8)
        return g_mission->rel_flat()[team * 0x5b + o->team] == 1;
    return false;
}
// @ 0x4323f0
int Agent::is_neutral(const Agent* o) const
{
    if (!((const AgentX*)this)->is_enemy(o) && !is_friend(o)) return 1;
    return 0;
}
// @ 0x432450
float AgentX::speed_scale() const
{
    if (f10 == 0 && f2c != -1) return 2.4f;
    return 1.5f;
}
// @ 0x432470
bool AgentX::f100_is_0x48_and_charged() const
{
    if (*(const int*)((const char*)this + 0x100) != 0x48) return false;
    return ((const SkelX*)body->skel)->f40->charge > 0.1f;
}

// ---- scalar helpers -------------------------------------------------------------------------------
// @ 0x4324b0
float __cdecl fmin_ref(const float* a, const float* b) { return *a < *b ? *a : *b; }
// @ 0x4324d0
float __cdecl fmax_ref(const float* a, const float* b) { return *a > *b ? *a : *b; }
// @ 0x4324f0
int __cdecl imax_ref(const int* a, const int* b) { return *a > *b ? *a : *b; }
// @ 0x432510
int __cdecl clamp_index(const int* v, const int* lo, const int* hi)
{
    if (*v < *lo) return *lo;
    if (*v >= *hi) return *hi - 1;
    return *v;
}
// @ 0x432540
int __cdecl round_float(float* p)
{
    float fl = (float)floor((double)*p);
    int i = (int)fl;
    return (*p - fl <= 0.5f) ? i : i + 1;
}
// @ 0x432580
int __cdecl rand_below(const int* n)
{
    if (*n <= 1) return 0;
    return rand() % *n;
}
// @ 0x4325a0
float __cdecl closer_angle(const float* ref, const float* a, const float* b)
{
    float da = (float)fabs(*a - *ref);
    if (da > 3.1415927f) da = 6.2831855f - da;
    float db = (float)fabs(*b - *ref);
    if (db > 3.1415927f) db = 6.2831855f - db;
    if (db > da) return *a;
    return *b;
}

// @ 0x432600
void vec2x::rotate_a(float ang)
{
    float c = (float)cos((double)ang);
    float s = (float)sin((double)ang);
    float nx = c * x - s * y;
    float ny = s * x + c * y;
    y = ny;
    x = nx;
}

// @ 0x432640
int __stdcall in_id_set(int id)
{
    int r = 0;
    if (id == 0x44 || id == 0xa2 || id == 0xc0 || id == 0xa8 || id == 0xab || id == 0x9c || id == 0xc6 ||
        id == 0xae || id == 0x9f || id == 0xc3 || id == 0xb1 || id == 0xa5 || id == 0xbd || id == 0xb4 ||
        id == 0xba || id == 0xb7)
        r = 1;
    return r;
}

// @ 0x4326d0
int __stdcall id_matches_dir(int id, int dir)
{
    int res = 0;
    int g = group_base(id);
    if (g == 0xc7) {
        res = 1;
    } else if (g == 0xd9 || g == 0xcd || g == 0xe5) {
        if (dir == 3) res = 1;
    } else if (g == 0xdf || g == 0xd3 || g == 0xeb) {
        if (dir == 1) res = 1;
    } else if (g == 0xdc || g == 0xd0 || g == 0xe8) {
        if (dir == 2) res = 1;
    } else if (g == 0xd6 || g == 0xca || g == 0xe2) {
        if (dir == 0) res = 1;
    }
    return res;
}

// @ 0x432770
void Vec3::sub_assign(const Vec3& o) { x = x - o.x; y = y - o.y; z = z - o.z; }

// @ 0x432790
ItemRef Agent::slot_pair(int idx) const
{
    const ItemRef* p = 0;
    int i = slots[idx];
    if (i >= 0 && (idx == 0 || slots[4] == 0)) p = &pairs[i];
    ItemRef r;
    r.item = -1;
    if (p) r = *p;
    return r;
}

// @ 0x4327f0
void vec2x::rotate_b(float ang)
{
    float c = (float)cos((double)ang);
    float s = (float)sin((double)ang);
    float nx = c * x - s * y;
    float t = c * y;
    y = t + s * x;
    x = nx;
}
// @ 0x432830
void __cdecl vec3_sub(Vec3* out, const Vec3* a, const Vec3* b) { out->x = a->x - b->x; out->y = a->y - b->y; out->z = a->z - b->z; }

// ---- frame algebra ----------------------------------------------------------------------------------
// p in the rows of m (the rotation part of transform_point_to_local 0x407f70, written member-first like it): an
// inline helper of the original (its parameters' keys order the x87 operands; the hand-expanded sums could not)
static inline vector4 rot_to_local(const vector4& p, const rot3x* m)
{
    return vector4(m->s.x * p.x + m->s.y * p.y + m->s.z * p.z,
                   m->f.x * p.x + m->f.y * p.y + m->f.z * p.z,
                   m->u.x * p.x + m->u.y * p.y + m->u.z * p.z);
}
// @ 0x432860
rot3x rot3x::mul_t(const rot3x& b) const
{
    rot3x r;
    r.s = rot_to_local(b.s, this);
    r.f = rot_to_local(b.f, this);
    r.u = rot_to_local(b.u, this);
    return r;
}

// b expressed in this frame
// @ 0x4329d0
rgl::matrix rgl::matrix::relative(const rgl::matrix& b) const
{
    rgl::matrix m;
    m.o = transform_point_to_local(b.o);
    *(rot3x*)&m = ((const rot3x*)this)->mul_t(*(const rot3x*)&b);
    return m;
}

// ---- Newton force callback ---------------------------------------------------------------------------
// @ 0x432a50
void __cdecl apply_force_and_torque(const NewtonBody* body)
{
    float mass, ixx, iyy, izz;
    NewtonBodyGetMassMatrix(body, &mass, &ixx, &iyy, &izz);
    if (g_phys.pending.body == body) {
        float f[3];
        f[0] = g_phys.pending.force.x * mass;
        f[1] = g_phys.pending.force.y * mass;
        f[2] = (g_phys.pending.force.z - 9.8f) * mass;
        NewtonBodyAddForce(body, f);
        if (g_phys.pending.torque.x * g_phys.pending.torque.x + g_phys.pending.torque.y * g_phys.pending.torque.y +
                g_phys.pending.torque.z * g_phys.pending.torque.z > 0.0f) {
            f[0] = g_phys.pending.torque.x * mass;
            f[1] = g_phys.pending.torque.y * mass;
            f[2] = g_phys.pending.torque.z * mass;
            NewtonBodyAddTorque(body, f);
        }
        g_phys.pending.force = vector4(0.0f, 0.0f, 0.0f);
        g_phys.pending.body = 0;
        g_phys.pending.torque = vector4(0.0f, 0.0f, 0.0f);
    } else {
        float f[3];
        f[0] = 0.0f;
        f[1] = 0.0f;
        f[2] = mass * -9.8f;
        NewtonBodyAddForce(body, f);
    }
}

// ---- tick timer -------------------------------------------------------------------------------------
// @ 0x432bf0
void Timer::restart() { start = g_ticks[clock]; }
// @ 0x432c10
float Timer::elapsed() const { return (float)(g_ticks[clock] - start) * 1e-5f; }

// ---- random direction inside a cone around `dir` ------------------------------------------------------
// @ 0x432c40
void __cdecl rand_cone_dir(vector4* out, vector4 dir, float spread)
{
    float a = (float)(rand() % 0x3dc9) * 6.322312e-05f - 0.5f;
    float ang = a * spread;
    ang = ang + ang;
    float c = cos(ang);
    float s = sin(ang);
    vector4 v1 = vector4(c * 1.0f - s * 0.0f, c * 0.0f + s, 0.0f);
    float phi = (float)(rand() % 0x3dc9) * 0.00039724255f;
    float c2 = cos(phi);
    float s2 = sin(phi);
    float B = v1.y * s2 + c2 * 0.0f;
    float A = v1.y * c2 - s2 * 0.0f;
    rgl::rot3 m;
    m.f = dir;
    m.u = vector4(0.0f, 0.0f, 1.0f);
    ((matrix*)&m)->orthonormalize();
    float px = m.u.x * A;
    px += m.f.x * v1.x;
    float py = m.u.y * A;
    py += m.s.y * B;
    py += m.f.y * v1.x;
    float pz = m.u.z * A;
    pz += m.s.z * B;
    pz += m.f.z * v1.x;
    out->x = px + m.s.x * B;
    out->y = py;
    out->z = pz;
}

// @ 0x432da0
void ActiveTimer::begin(float seconds)
{
    if (!active) {
        start = g_ticks[clock] - (int)(seconds * -100000.0f);
        active = 1;
    }
}
// @ 0x432de0
bool ActiveTimer::expired()
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

// The vectors of 0x432e30 are 16 bytes on its frame (x, y, z and an unused lane: the engine's 4-float vector); Vec3w is
// that record with Vec3's inline operators. The moved velocity is three float locals, not a vector.
struct Vec3w {
    float x, y, z, w;
    Vec3w() {}
    Vec3w(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    Vec3w operator-() const { return Vec3w(-x, -y, -z); }
    Vec3w operator+(const Vec3w& o) const { return Vec3w(x + o.x, y + o.y, z + o.z); }
    Vec3w operator*(float s) const { return Vec3w(x * s, y * s, z * s); }
    Vec3w& operator-=(const Vec3w& o) { x = x - o.x; y = y - o.y; z = z - o.z; return *this; }
    float dot(const Vec3w& o) const { return x * o.x + y * o.y + z * o.z; }
};
// ---- collision response: removes the approaching component of v along n, with friction ------------------
// @ 0x432e30
void __cdecl bounce(Vec3* out, Vec3* v, const Vec3* n_)
{
    const Vec3w* n = (const Vec3w*)n_;
    float d = (-*n).dot(*(const Vec3w*)v);
    if (d < 0.0f) d = 0.0f;
    Vec3w t = *n * d;
    float sx = t.x + v->x, sy = t.y + v->y, sz = t.z + v->z;
    v->x = sx; v->y = sy; v->z = sz;
    if (d > 0.0f) {
        Vec3w g(sx * d, sy * d, sz * d);
        Vec3w f = g * 0.4f;
        v->x = sx - f.x; v->y = sy - f.y; v->z = sz - f.z;
        t -= f;
    }
    out->x = -t.x; out->y = -t.y; out->z = -t.z;
}

static unsigned char agent_flags130(const Agent* a) { return *((const unsigned char*)a + 0x130); }   // +0x130 (0x432f60)
// ---- weighted random pick among an actor's item records -------------------------------------------------
// @ 0x432f60
void Agent::fill_weights(float* out, int idx, int rec)
{
    int best = -100;
    for (int i = 0; i < agent_recs(this)[rec].count; i++) {
        ActorItemRec& item = agent_recs(this)[rec].items[i];
        int score = 0;
        if (item.flags & 0x1000000) score = (agent_flags130(this) & 4) ? 2 : -10;
        if (item.flags & 0x2000000) {
            if ((agent_flags130(this) & 8) && f2c != -1) score += 2;
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

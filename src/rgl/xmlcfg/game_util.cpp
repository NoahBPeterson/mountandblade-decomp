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
#include "../../include/msg_log.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../game/channel.h"      // (Channel: the 0xd8-byte animation record whose busy() 0x433840 ends this object)
#include "../../game/mission.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/troop.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)

// ---- Newton Game Dynamics imports (cdecl) -----------------------------------------------------------
extern "C" {
__declspec(dllimport) void __cdecl NewtonBodyGetMassMatrix(const NewtonBody* body, float* mass, float* ixx, float* iyy, float* izz);   // 0x549298
__declspec(dllimport) void __cdecl NewtonBodyAddForce(const NewtonBody* body, const float* force);       // 0x54929c
__declspec(dllimport) void __cdecl NewtonBodyAddTorque(const NewtonBody* body, const float* torque);     // 0x5492a0
}

// ---- control-id groups -------------------------------------------------------------------------------------------
// Key-code test: false for the first code of each triple group_base() returns, 0xc7 + 3k.
// @ 0x4320b0
int __cdecl not_group_base(int x)
{
    int r = 1;
    if (x == 0xc7 || x == 0xca || x == 0xcd || x == 0xd0 || x == 0xd3 || x == 0xd6 || x == 0xd9 ||
        x == 0xdc || x == 0xdf || x == 0xe2 || x == 0xe5 || x == 0xe8 || x == 0xeb)
        r = 0;
    return r;
}

// Key codes in the Module System's header_triggers numbering: key_home 0xc7, key_up 0xc8, key_page_up 0xc9,
// key_left 0xcb, key_right 0xcd, key_end 0xcf, key_down 0xd0, key_page_down 0xd1, key_insert 0xd2, key_delete 0xd3,
// mouse buttons 0xe0..0xe7.
// @ 0x432130
int __cdecl group_base(int id)
{
    int r = -1;
    if (id == 0xc7 || id == 0xc8 || id == 0xc9) {
        r = 0xc7;
    } else {
        if (id == 0x97 || id == 0x98 || id == 0x99) return 0x97;
        if (id == 0xca || id == 0xcb || id == 0xcc) return 0xca;
        if (id == 0xcd || id == 0xce || id == 0xcf) return 0xcd;
        if (id == 0xd0 || id == 0xd1 || id == 0xd2) return 0xd0;
        if (id == 0xd3 || id == 0xd4 || id == 0xd5) return 0xd3;
        if (id == 0xd6 || id == 0xd7 || id == 0xd8) return 0xd6;
        if (id == 0xd9 || id == 0xda || id == 0xdb) return 0xd9;
        if (id == 0xdc || id == 0xdd || id == 0xde) return 0xdc;
        if (id == 0xdf || id == 0xe0 || id == 0xe1) return 0xdf;
        if (id == 0xe2 || id == 0xe3 || id == 0xe4) return 0xe2;
        if (id == 0xe5 || id == 0xe6 || id == 0xe7) return 0xe5;
        if (id == 0xe8 || id == 0xe9 || id == 0xea) return 0xe8;
        if (id == 0xeb || id == 0xec || id == 0xed) return 0xeb;
    }
    return r;
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
bool AgentX::is_friend(const Agent* o) const
{
    if (team >= 0 && team < 8 && o->team >= 0 && o->team < 8)
        return g_mission->rel_flat()[team * 0x5b + o->team] == 1;
    return false;
}
// @ 0x4323f0
int AgentX::is_neutral(const Agent* o) const
{
    if (!is_enemy(o) && !is_friend(o)) return 1;
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

// The same function, declared (not visible as a definition) so cl does not assume its register use.
int __cdecl group_base_ext(int id);   // 0x432130
// @ 0x4326d0
int __stdcall id_matches_dir(int id, int dir)
{
    int res = 0;
    int g = group_base_ext(id);
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
// @ 0x432860
rot3x rot3x::mul_t(const rot3x& b) const
{
    rot3x r;
    vector4 a, c, d;
    a.x = (b.s.x*this->s.x + b.s.y*this->s.y) + b.s.z*this->s.z;
    a.y = (b.s.y*this->f.y + b.s.z*this->f.z) + b.s.x*this->f.x;
    a.z = (b.s.x*this->u.x + b.s.z*this->u.z) + b.s.y*this->u.y;
    r.s = a;
    c.x = (b.f.x*this->s.x + b.f.y*this->s.y) + b.f.z*this->s.z;
    c.y = (b.f.x*this->f.x + b.f.z*this->f.z) + b.f.y*this->f.y;
    c.z = (b.f.x*this->u.x + b.f.z*this->u.z) + this->u.y*b.f.y;
    r.f = c;
    d.x = (b.u.y*this->s.y + b.u.z*this->s.z) + b.u.x*this->s.x;
    d.y = (b.u.x*this->f.x + b.u.z*this->f.z) + b.u.y*this->f.y;
    d.z = (b.u.x*this->u.x + b.u.z*this->u.z) + b.u.y*this->u.y;
    r.u = d;
    return r;
}

// b expressed in this frame
// @ 0x4329d0
rgl::mat4 rgl::mat4::relative(const rgl::mat4& b) const
{
    rgl::mat4 m;
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
    float x1 = c * 1.0f - s * 0.0f;
    float y1 = c * 0.0f + s;
    float phi = (float)(rand() % 0x3dc9) * 0.00039724255f;
    float c2 = cos(phi);
    float s2 = sin(phi);
    float B = y1 * s2 + c2 * 0.0f;
    float A = y1 * c2 - s2 * 0.0f;
    rgl::rot3 m;
    m.f = dir;
    m.u = vector4(0.0f, 0.0f, 1.0f);
    ((matrix*)&m)->orthonormalize();
    float px = m.u.x * A + m.f.x * x1;
    out->x = px + m.s.x * B;
    out->y = m.u.y * A + m.s.y * B + m.f.y * x1;
    out->z = m.u.z * A + m.s.z * B + m.f.z * x1;
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

// ---- collision response: removes the approaching component of v along n, with friction ------------------
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
// @ 0x432f60
void AgentX::fill_weights(float* out, int idx, int rec)
{
    int best = -100;
    for (int i = 0; i < recs()[rec].count; i++) {
        unsigned fl = recs()[rec].items[i].flags;
        int score = 0;
        if (fl & 0x1000000) score = (flags130() & 4) ? 2 : -10;
        if (fl & 0x2000000) {
            if ((flags130() & 8) && f2c != -1) score += 2;
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
int AgentX::pick(int idx)
{
    float w[64];
    float r;
    float total;
    int rec = *(int*)((char*)this + 0xd8 + idx * 0x2c);
    total = 0.0f;
    int res = -1;
    fill_weights_x(w, idx, rec);
    for (int i = 0; i < recs()[rec].count; i++) total += w[i];
    r = (float)(rand() % 0x3dc9) * total * 6.322312e-05f;
    int n = recs()[rec].count;
    float acc = 0.0f;
    for (int i = 0; i < n; i++) {
        acc += w[i];
        if (r <= acc) { res = i; break; }
    }
    return res;
}

// @ 0x433140
int AgentX::agent_class() const
{
    int r = 0;
    int item = *(const int*)((const char*)this + 0x88);       // item index (-1 = none)
    if (item >= 0) {
        if (f2c >= 0) r = 2;
        else {
            if (g_game->troops[item].flags30s & 0x4000000i64) r = 1;
            else r = 0;
        }
    }
    return r;
}

namespace rgl {
struct keyframes;
struct mesh {
    keyframes& get_kf();   // 0x40ec00
};
}
// @ 0x433190
void mesh_node::set_time(const float* t)
{
    keyframes12& k = local_a1 ? kf : (keyframes12&)((rgl::mesh*)parent)->get_kf();   // 0x40ec00 (rgl::mesh::get_kf: the parent walk)
    if (k.count > 0) {
        if (*t > (float)k.times[k.count - 1]) c8 = (float)k.times[k.count - 1];
        else c8 = *t;
    }
}

// ---- std::copy / copy_backward instantiations (VC7.1 <xutility>) --------------------------------------
namespace std {
// 0x4331e0 (forward copy of 16-byte strings), 0x433220 (backward), 0x433260 (backward copy of 12-byte records)
template tstring* _Copy_opt<tstring*, tstring*>(tstring*, tstring*, tstring*, _Nonscalar_ptr_iterator_tag);
template tstring* _Copy_backward_opt<tstring*, tstring*>(tstring*, tstring*, tstring*, _Nonscalar_ptr_iterator_tag);
template Vec3* _Copy_backward_opt<Vec3*, Vec3*>(Vec3*, Vec3*, Vec3*, _Nonscalar_ptr_iterator_tag);
}

// ---- collision shapes -----------------------------------------------------------------------------------
// @ 0x4332a0
sphere::sphere(vector4 c, const float* r) : shape_base(inline_init())
{
    local_center = c;
    center = c;
    radius = *r;
}
// @ 0x433310
capsule::capsule(float r, const vector4* a, const vector4* b) : shape_base(inline_init())
{
    radius = r;
    l0 = *a;
    l1 = *b;
    p0 = l0;
    p1 = l1;
}

// @ 0x4333c0
bool face::contains_xy(const float* p) const
{
    bool inside = true;
    int i = 0;
    const vector4* a = w;
    do {
        if (i >= count) return inside;
        i++;
        int j = i % count;
        float e = w[j].x - a->x, f = w[j].y - a->y, g = p[0] - a->x, h = p[1] - a->y;
        if (h * e - g * f < 0.0f)
            inside = false;
        a++;
    } while (inside);
    return inside;
}
// @ 0x433440
float face::plane_z(const float* p) const
{
    return w[0].z - ((p[1] - w[0].y) * normal.y + (p[0] - w[0].x) * normal.x) / normal.z;
}
// @ 0x433470
composite::composite(const composite& o) : shape_base(inline_init()) { ((composite*)this)->copy_from(&o); }

// @ 0x4334b0
void MsgLog::push(const tstring& s, int v)
{
    if (enabled && ring.count < 32) {
        int idx = (ring.head + ring.count) % 32;
        ring.names[idx] = s;
        ring.vals[idx] = v;
        ring.count++;
    }
}

static inline int ifloor(float f) { return (int)(f > 0.0f ? f : f - 0.99999988f); }
// @ 0x433510
void HeightFieldX::sample(float x, float y, float* out_h, int* out_id)
{
    float inv = 1.0f / cell;
    float t[2];
    t[0] = x * inv;
    t[1] = y * inv;
    int ix = ifloor(t[0]);
    if (ix < 0) ix = 0;
    else { int r = w - 1; if (ix < w) r = ix; ix = r; }
    int iy = ifloor(t[1]);
    if (iy < 0) iy = 0;
    else { int r = h - 1; if (iy < h) r = iy; iy = r; }
    float fu = (float)fmod((double)t[0], 1.0);
    float fv = (float)fmod((double)t[1], 1.0);
    HCell** pc = &cols[ix];
    *out_id = pc[0][iy].id;
    HCell* a = &pc[0][iy];
    if (a->flags & 2) {
        if (fu > fv) {
            *out_h = (a->h - pc[1][iy].h) * (1.0f - fu) + (pc[1][iy + 1].h - pc[1][iy].h) * fv + pc[1][iy].h;
            return;
        }
        *out_h = (pc[0][iy].h - pc[0][iy + 1].h) * (1.0f - fv) + (pc[1][iy + 1].h - pc[0][iy + 1].h) * fu + pc[0][iy + 1].h;
    } else {
        if (1.0f - fu > fv) {
            *out_h = (pc[1][iy].h - pc[0][iy].h) * fu + (pc[0][iy + 1].h - pc[0][iy].h) * fv + pc[0][iy].h;
            return;
        }
        *out_h = (pc[1][iy].h - pc[1][iy + 1].h) * (1.0f - fv) + (a[1].h - pc[1][iy + 1].h) * (1.0f - fu) + pc[1][iy + 1].h;
    }
    if (pc[0][iy].flags & 1) *out_id = pc[0][iy].id2;
}

// @ 0x4336d0
bool __cdecl in_box(const vector4* lo, const vector4* hi, const vector4* p)
{
    return lo->x <= p->x && p->x <= hi->x && lo->y <= p->y && p->y <= hi->y && lo->z <= p->z && p->z <= hi->z;
}

// @ 0x433740
void Obj3::set_v184(const vector4* v) { v184 = *v; }
// @ 0x433770
void Obj3::set_placement(const matrix* m, const vector4* a, const vector4* b, int c)
{
    unsigned f = flags & ~1u;
    flags = f;
    m134 = *m;
    v174 = *a;
    v184 = *b;
    m194 = c;
    if (f & 2) {
        m19c = m1a0 = 1000000000;
        m1a4 = 0;
    }
}
// @ 0x433810
void Obj3::reset_pending(int v)
{
    flags &= ~1u;
    m19c = m1a0 = v;
    m1a4 = 0;
}

// @ 0x433840  (agent/MIGRATION.md calls it Curve::step_reached; Curve and Channel are the same 0xd8-byte record)
#include "../../include/fx.h"
extern float g_step_tbl[];      // 0x5a9c20 per-clock step (seconds)
bool Channel::busy()
{
    bool r;
    float t = (float)(g_ticks[tm.clock] - tm.start) * 1e-5f;
    if (mode == 20) {
        float inv = 1.0f / period;
        int a = (int)((t - g_step_tbl[tbl_idx]) * inv);
        int b = (int)(t * inv);
        if (a < b) return true;
        return false;
    }
    if (mode != 21 && mode != 22) return r;
    if (t >= period) return true;
    return false;
}

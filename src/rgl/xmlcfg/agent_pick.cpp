//-----------------------------------------------------------------------------
// File: agent_pick.cpp  (0x433040..0x4338df)
//
// Desc: the second object of game_util's range: the actor's weighted item pick (Agent::pick, which calls
//       Agent::fill_weights 0x432f60 as an external function), the agent class test, the mesh morph time clamp, the
//       std::copy instantiations, the argument constructors of the sphere / capsule shapes, collision face
//       helpers, the composite copy constructor, the message ring, the height-field sampler and the scene object's
//       placement setters. Functions in ascending address order. Flags: /O2 /MT /Gy /TP.
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
#include "../../game/strat/gui_manager.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../game/channel.h"      // (Channel: the 0xd8-byte animation record whose busy() 0x433840 ends this object)
#include "../../game/mission.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/troop.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/clock_table.h"

// @ 0x433040
int Agent::pick(int idx)
{
    float w[64];
    float r;
    float total;
    int rec = *(int*)((char*)this + 0xd8 + idx * 0x2c);
    total = 0.0f;
    int res = -1;
    fill_weights(w, idx, rec);
    for (int i = 0; i < agent_recs(this)[rec].count; i++) total += w[i];
    r = (float)(rand() % 0x3dc9) * total * 6.322312e-05f;
    int n = agent_recs(this)[rec].count;
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
    do {
        if (i >= count) break;
        struct V2 { float x, y; };
        int j = (i + 1) % count;
        V2 d = { w[j].x - w[i].x, w[j].y - w[i].y };
        V2 q = { p[0] - w[i].x, p[1] - w[i].y };
        if (q.y * d.x - q.x * d.y < 0.0f)
            inside = false;
        i++;
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
void gui_manager::push(const tstring& s, int v)
{
    if (enabled)
        ring.add(s, v);
}

static inline int ifloor(float f) { float fl = f; if (f > 0.0f) fl = f; else fl -= 0.99999988f; return (int)fl; }
// @ 0x433510
void HeightField::sample(float x, float y, float* out_h, int* out_id)
{
    float inv = 1.0f / cell;
    float tx = x * inv;
    float ty = y * inv;
    int ix = ifloor(tx);
    if (ix < 0) ix = 0;
    else { int r = w - 1; if (ix < w) r = ix; ix = r; }
    int iy = ifloor(ty);
    if (iy < 0) iy = 0;
    else { int r = h - 1; if (iy < h) r = iy; iy = r; }
    float fu = (float)fmod((double)tx, 1.0);
    *out_id = cols[ix][iy].id;
    float fv = (float)fmod((double)ty, 1.0);
    HCell** pc = &cols[ix];
    HCell* a = pc[0] + iy;
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
    if (cols[ix][iy].flags & 1) *out_id = cols[ix][iy].id2;
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
bool Channel::busy()
{
    bool r;
    float t = (float)(g_ticks[tm.clock] - tm.start) * 1e-5f;
    if (mode == 20) {
        float inv = 1.0f / period;
        int a = (int)((t - g_step_tbl[tbl_idx]) * inv);
        int b = (int)(t * inv);
        r = a < b;
    }
    else if (mode == 21 || mode == 22) {
        if (t >= period) r = true;
        else r = false;
    }
    return r;
}

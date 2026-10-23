//-----------------------------------------------------------------------------
// File: agent_ai.cpp  (0x43b100..0x43e190)
//
// Desc: the record table accessors, the agent table iteration, the int hash lookup, the nearest-enemy list and
//       target pickers, the aim-error update, the input queue, nav-face tracking and the movement animation
//       chooser, the animation blend update and the path goals. Functions in ascending address order.
//       Flags: /O2 /MT /Gy /TP. Names provisional.
//-----------------------------------------------------------------------------
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <new>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include "./agent_types.h"      // (named here too: the resolution audit reads the annotations of directly included headers)
#include "./agent_ai.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../include/mat4.h"
#include "../../include/ptr_tag.h"
#include "../../include/ptr_vec.h"
#include "../../include/timer.h"
#include "../../include/vec.h"
#include "../light.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../placed_obj.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../skeleton.h"      // (listed directly: the resolution audit scans only the source's own includes)

static __forceinline float clampf(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

enum { skl_horse_archery = 23, skl_riding = 24, skl_power_draw = 33, skl_power_throw = 34 };   // header_skills.py


// ---- 0x1f8-byte record table accessors --------------------------------------------------------------------------------------------------------------------
// @ 0x43b100
int Table1f8::get_f1f0(unsigned i)
{
    if (i < count) return recs[i].f1f0;
    return 0;
}
// @ 0x43b130
int Table1f8::get_f1f4(unsigned i)
{
    if (i < count) return recs[i].f1f4;
    return 0;
}

// ---- the agent table (agents are 0x5928-byte records in pages of 16) --------------------------------------------------------------------------------------------
// @ 0x43b160
inline void AgentTable::next_used(unsigned* i)
{
    ++*i;
    while ((int)*i < count) {
        unsigned v = *i;
        if (pages[v >> 4][v & 0xf].used != 0) break;
        ++*i;
    }
}
// @ 0x43b1b0
inline int AgentTable::first_used()
{
    unsigned v = 0;
    if (0 < count) {
        Agent** pg = &pages[0];
        do {
            if (pg[v >> 4][v & 0xf].used != 0) break;
            v++;
        } while ((int)v < count);
    }
    return v;
}
// @ 0x43b1f0
inline Agent* AgentTable::get(unsigned id)
{
    return &pages[id >> 4][id & 0xf];
}

// stdext::hash_map<int, HashVal>::lower_bound: bucket (key ^ 0xdeadbeef) & mask, the iterator returned through a hidden pointer
// @ 0x43b210
template IntHashBase::iterator IntHashBase::lower_bound(const int&);

// @ 0x43b2b0
tstring* vecstr::ufill(tstring* f, unsigned n, const tstring& v)
{
    ptr_tag t;
    uninit_fill_n_str(f, n, v, *this, t);
    return f + n;
}
// @ 0x43b2e0
E1c8* vecE1c8::ucopy(E1c8* f, E1c8* l, E1c8* d) { ptr_tag t; return uninit_copy1c8(f, l, d, *this, t); }

// ---- the agent's list of the 16 nearest enemies, sorted by distance (0x18-byte entries at agent+0x53ec) --------------------------------------------------------------
static __forceinline bool hostile_to(Mission* m, int a, Agent* o)
{
    if (a < 0 || a >= 8) return false;
    int b = o->team;
    if (b < 0 || b >= 8) return false;
    return m->teams[a].rel[b] == -1;
}
static __forceinline bool allied_to(Mission* m, int a, Agent* o)
{
    if (a < 0 || a >= 8) return false;
    int b = o->team;
    if (b < 0 || b >= 8) return false;
    return m->teams[a].rel[b] == 1;
}

// @ 0x43b300
void Agent::collect_enemies(int)
{
    nthreat = 0;
    Mission* m = g_mission;
    AgentTable* t = &m->agents;
    unsigned i = t->first_used();
    if (i < (unsigned)t->count) {
        do {
            Agent* o = &m->agents.pages[i >> 4][i & 0xf];
            if (o->f10 == 0 && o->f14 == 1 && (int)i != id && hostile_to(m, team, o)) {
                vector4 d;
                d.x = o->pos.x - pos.x;
                d.y = o->pos.y - pos.y;
                d.z = o->pos.z - pos.z;
                float len = d.normalize();
                int k = 0;
                int n = nthreat;
                if (0 < n) {
                    while (k < nthreat) {
                        if (!(threats[k].dist < len)) break;
                        k++;
                    }
                    if (k >= 16) goto next;
                }
                if (n < 16) nthreat = n + 1;
                for (int j = nthreat - 1; j > k; j--) threats[j] = threats[j - 1];
                threats[k].id = o->id;
                threats[k].dist = len;
                threats[k].dir = d;
                m = g_mission;
            }
        next:
            t = &m->agents;
            t->next_used(&i);
        } while (i < (unsigned)t->count);
    }
}

// @ 0x43b4d0
unsigned Agent::pick_target()
{
    int n = nthreat;
    unsigned best = (unsigned)-1;
    float bestv = -100.0f;
    unsigned tid = (unsigned)-1;
    if (0 < n) {
        Threat* r = threats;
        Mission* m = g_mission;
        do {
            tid = r->id;
            if (m->agents.pages[tid >> 4][tid & 0xf].f14 == 1) {
                const vector4& p = m->agents.pages[tid >> 4][tid & 0xf].pos;
                float dx = p.x - pos.x;
                float dy = p.y - pos.y;
                float dz = p.z - pos.z;
                float v = 3.0f / (float)sqrt(dy * dy + (dx * dx + dz * dz));
                if (!(v < 4.0f)) v = 4.0f;
                v = v / ((float)m->agents.pages[tid >> 4][tid & 0xf].f55e8 + 7.0f);
                if (v > bestv) { bestv = v; best = tid; }
            }
            r++;
            n--;
            tid = best;
        } while (n != 0);
    }
    return tid;
}

// @ 0x43b5d0
unsigned Agent::pick_random()
{
    unsigned best = (unsigned)-1;
    float bestv = -100.0f;
    unsigned tid = (unsigned)-1;
    int n = 0;
    if (0 < nthreat) {
        Threat* r = threats;
        do {
            tid = r->id;
            Agent* o = &g_mission->agents.pages[tid >> 4][tid & 0xf];
            if (o->f14 == 1) {
                float dx = o->pos.x - pos.x;
                float dy = o->pos.y - pos.y;
                float dz = o->pos.z - pos.z;
                float v = 3.0f / (float)sqrt(dx * dx + (dy * dy + dz * dz));
                if (!(v < 4.0f)) v = 4.0f;
                Agent* o2 = &g_mission->agents.pages[tid >> 4][tid & 0xf];
                ItemRef cur = o2->cur_pair();       // a by-value copy: its stack stores keep cl from reusing the page lookups below
                int item = cur.item;
                if (item >= 0) {
                    unsigned t = g_items[item].type_flags & 0xff;
                    if (t == 8 || t == 9 || t == 10 || t == 0x10 || t == 0x11) v = v * 1.5f;
                }
                if (g_mission->agents.pages[tid >> 4][tid & 0xf].f2c != -1) v = v * 0.85f;
                int w = g_mission->agents.pages[tid >> 4][tid & 0xf].f55e8;
                v = v / ((float)w + 4.0f);
                v = ((float)(rand() % 0x3dc9) * 1.2644623e-05f + 1.0f) * v;
                if (v > bestv) { bestv = v; best = r->id; }
            }
            n++;
            r++;
            tid = best;
        } while (n < nthreat);
    }
    return tid;
}

// the agent's (or its mount's) velocity rotated by the body frame
// @ 0x43b7b0
vector4* Agent::local_velocity(vector4* out)
{
    unsigned idx = f2c;
    vector4 v;
    if ((int)idx >= 0) {
        Agent* a = &g_mission->agents.pages[idx >> 4][idx & 0xf];
        v.x = a->vel_xy.x; v.y = a->vel_xy.y;
    } else {
        v.x = vel_xy.x; v.y = vel_xy.y;
    }
    v.z = 0.0f;
    if (body) {
        const rgl::matrix& m = body->m;
        float rz = (v.y * m.f.z + v.x * m.s.z) + m.u.z * 0.0f;
        float ry = (v.y * m.f.y + v.x * m.s.y) + m.u.y * 0.0f;
        float rx = (v.y * m.f.x + v.x * m.s.x) + m.u.x * 0.0f;
        out->x = rx; out->y = ry; out->z = rz;
        return out;
    }
    *out = v;
    return out;
}

// @ 0x43b8a0
unsigned Agent::find_talk_target()
{
    Mission* m = g_mission;
    AgentTable* t = &m->agents;
    unsigned best = (unsigned)-1;
    unsigned i = t->first_used();
    if (i < (unsigned)t->count) {
        do {
            Agent* o = &m->agents.pages[i >> 4][i & 0xf];
            if (o->f10 == 1 && o->f30 == -1 && o->f14 == 1 && o->f5830 == 2) {
                int a = g_game->skill_level(troop, 0x18);
                int b = o->f8c.stat_1ec();
                if (b <= a) {
                    vector4 d(o->pos.x - pos.x, o->pos.y - pos.y, o->pos.z - pos.z);
                    float len = d.normalize();
                    if (len < 1.0f && d.x * fx + d.y * fy > 0.1f)
                        best = i;
                }
            }
            m = g_mission;
            t = &m->agents;
            t->next_used(&i);
        } while (i < (unsigned)t->count);
    }
    return best;
}

// @ 0x43b9e0
unsigned Agent::pick_front()
{
    Mission* m = g_mission;
    AgentTable* t = &m->agents;
    unsigned best = (unsigned)-1;
    float bestv = 0.07f;
    unsigned i = t->first_used();
    unsigned n = t->count;
    if (i < n) {
        int myteam = team;
        do {
            Agent* o = &m->agents.pages[i >> 4][i & 0xf];
            if (!allied_to(m, myteam, o) && o->f14 == 1 && o->f10 != 1) {
                vector4 d(o->pos.x - pos.x, o->pos.y - pos.y, o->pos.z - pos.z);
                float len = d.normalize();
                float a = clampf((d.x * fx + d.y * fy) + 0.5f, 0.0f, 1.2f);
                float b = clampf(1.0f / (len + 0.5f), 0.0f, 1.0f);
                if (b * a > bestv) { best = i; bestv = b * a; }
            }
            t->next_used(&i);
        } while (i < n);
    }
    return best;
}

// @ 0x43bb80
unsigned Agent::pick_duel()
{
    Mission* m = g_mission;
    AgentTable* t = &m->agents;
    unsigned best = (unsigned)-1;
    float bestv = 0.0f;
    unsigned i = t->first_used();
    unsigned n = t->count;
    if (i < n) {
        Agent** pg = &m->agents.pages[0];
        int myteam = team;
        do {
            Agent* o = &pg[i >> 4][i & 0xf];
            if (hostile_to(g_mission, myteam, o) && o->f14 == 1 && o->active != 0) {
                vector4 d(o->pos.x - pos.x, o->pos.y - pos.y, o->pos.z - pos.z);
                float len = d.normalize();
                float a = clampf((d.x * fx + (d.y * fy + d.z * fz)) + 0.8f, 0.0f, 1.0f);
                float b = clampf(1.0f / (len + 0.5f), 0.0f, 1.0f);
                float od = -(d.x * o->fx + (d.y * o->fy + d.z * o->fz));
                float c = clampf(od + 0.5f, 0.0f, 1.0f);
                float v = (c * b) * a;
                if (v > bestv) { best = i; bestv = v; }
            }
            g_mission->agents.next_used(&i);
        } while (i < n);
    }
    return best;
}

// ---- the aim error of a ranged agent ----------------------------------------------------------------------------------------------------------------------------------
// Notes on the spelling (all found by diffing against the original; none changes behaviour):
//  - `(0.0f > x) ? 0.0f : x` is the inlined max(0, x) the original used for the shot-error floor; the second
//    `if (acc < 0.0f)` is real source too (the original tests twice).
//  - the speed term is `spd * 0.0f` in the original (a tuning constant that is zero in 1.011).
//  - `sp` (mount-less speed) is a float: the original compares/adds it with single-precision constants.
//  - tm is Timer::elapsed() inlined at its first use; the later call in the crossbow/firearm arm stays a call.
// @ 0x43bdb0
void Agent::update_aim_error()
{
    float v = 0.0f;
    ItemRef* r = 0;
    if (slots[0] >= 0) r = &pairs[slots[0]];
    ItemRef tmp;
    tmp.item = -1;
    if (r) tmp = *r;
    ItemRef cur = tmp;
    int item = cur.item;
    if (item >= 0) {
        unsigned t = g_items[item].type_flags & 0xff;
        if (t == itp_type_bow || t == itp_type_crossbow || t == itp_type_thrown || t == itp_type_pistol || t == itp_type_musket) {
            float tm = (float)(g_ticks[timer.clock] - timer.start) * 1e-5f;
            float a, b, nz;
            {
                vector4 p, n;
                p.x = tm; p.y = a = tm + 3.0f; p.z = b = tm * 1.5f;
                noise_gradient(0.7f, &p.x, &n.x);
                nz = (n.y + n.x) * 1.9f + 1.4f;
            }
            ItemKind* ik = &g_items[item];
            vector4 d;
            d.x = fx - prev_facing.x;
            d.y = fy - prev_facing.y;
            d.z = fz - prev_facing.z;
            float spd = 0.0f;
            if (g_step > 0.0f) spd = d.length() / g_step;
            spd = spd * 0.0f;
            if (spd > 1.0f) spd = 1.0f;
            prev_facing = *(vector4*)&fx;
            unsigned t2 = ik->type_flags & 0xff;
            if (t2 == itp_type_bow) {
                int tr = troop;
                float sk = (float)g_game->skill_level(tr, skl_power_draw) * 25.0f + (int)g_game->chars[tr].f80[3];
                float p21 = (float)cur.stat_21x(1) * 8.0f;
                float w = (float)cur.stat_1ec() * 40.0f + p21 + 50.0f;
                int c = g_items[item].f204;
                if (c == 0) c = 99;
                float cf = (float)c * 0.01f;
                float acc0 = ((w + 50.0f) - sk) * 6e-5f;
                float acc = (0.0f > acc0) ? 0.0f : acc0;
                if (acc < 0.0f) acc = 0.0f;
                if (active) {
                    int ag = f2c;
                    if (ag >= 0) {
                        AgentTable* tb = &g_mission->agents;
                        if (1.0f < fabs(tb->get(ag)->vel_xy.y)) {
                            float t5 = ((float)fabs(tb->get(ag)->vel_xy.y) + 3.0f - (sk + 200.0f) * (((float)g_game->skill_level(tr, skl_riding) + 5.0f) * (float)g_game->skill_level(troop, skl_horse_archery)) * 0.0004f) * 0.008f;
                            float zero = 0.0f;
                            float m = 4.0f / ((float)g_game->skill_level(troop, skl_horse_archery) + 4.0f);
                            acc = (float)(fmax_ref(&zero, &t5) * m * nz + acc);
                        }
                    } else {
                        float sp = (float)sqrt(vel_xy.y * vel_xy.y + vel_xy.x * vel_xy.x);
                        if (sp > 0.0f) acc = (((sp + 3.0f) / (sk + 200.0f)) * nz * 7.0f + acc);
                    }
                    float lvl = 1.25f / damage_factor(0);
                    float g = (sk - w * 0.5f) * 0.01f;
                    g = (0.3f > g) ? 0.3f : g;
                    float thr = g + lvl;
                    v = acc;
                    if (tm < lvl) {
                        float zero = 0.0f;
                        float x = (lvl - tm) / lvl - sk * 0.001f;
                        v = fmax_ref(&zero, &x) + acc;
                    } else if (thr < tm) {
                        vector4 q, n; q.x = tm; q.y = a; q.z = b;
                        noise_gradient(0.4f, &q.x, &n.x);
                        float e1 = (tm - thr) * 0.4f;
                        float e2 = 1.0f;
                        float w2 = (float)fabs(((n.y + n.x) + 1.0f) * 0.5f);
                        v = (w * 0.0006f) * (w2 * fmin_ref(&e1, &e2)) + acc;
                    }
                    float r2 = (100.0f / (sk + 100.0f)) * spd - sk * 0.002f;
                    r2 = (0.0f > r2) ? 0.0f : r2;
                    v = (1.0f - cf) * 0.08f + (r2 * 0.2f + v);
                } else {
                    v = 0.0;
                }
            } else if (t2 != itp_type_crossbow && t2 != itp_type_pistol && t2 != itp_type_musket) {
                int tr = troop;
                float sk = (float)g_game->skill_level(tr, skl_power_throw) * 20.0f + (int)g_game->chars[tr].f80[5];
                float cf = (float)cur.stat_204() * 0.01f;
                if (active) {
                    float p21 = (float)cur.stat_21x(1);
                    float fv = (float)cur.stat_1ec() * 5.0f + p21;
                    float zero = 0.0f;
                    float x = ((fv + 30.0f) * 8.0f - sk) * 0.00012f;
                    float acc = fmax_ref(&zero, &x);
                    int ag = f2c;
                    if (ag >= 0) {
                        AgentTable* tb = &g_mission->agents;
                        if (1.0f < fabs(tb->get(ag)->vel_xy.y)) {
                            float t5 = ((float)fabs(tb->get(ag)->vel_xy.y) + 5.0f - (sk + 200.0f) * (((float)g_game->skill_level(tr, skl_riding) + 5.0f) * (float)g_game->skill_level(troop, skl_horse_archery)) * 0.0004f) * 0.004f;
                            float zero2 = 0.0f;
                            float m = 4.0f / ((float)g_game->skill_level(troop, skl_horse_archery) + 4.0f);
                            acc = (float)(fmax_ref(&zero2, &t5) * m * nz + acc);
                        }
                    } else {
                        float sp = (float)sqrt(vel_xy.y * vel_xy.y + vel_xy.x * vel_xy.x);
                        if (sp > 0.0f) acc = (((sp + 3.0f) / (sk + 50.0f)) * nz + acc);
                    }
                    float zero3 = 0.0f;
                    float y = (100.0f / (sk + 100.0f)) * spd - sk * 0.0007f;
                    v = (1.0f - cf) * 0.08f + (fmax_ref(&zero3, &y) * 0.3f + acc);
                } else {
                    v = 0.0;
                }
            } else {
                float cf = (float)cur.stat_204() * 0.01f;
                float A, B;
                if (t2 == itp_type_crossbow) {
                    A = (float)(int)g_game->chars[troop].f80[4];
                    B = (float)cur.stat_21x(1);
                } else if (t2 == itp_type_pistol || t2 == itp_type_musket) {
                    A = (float)(int)g_game->chars[troop].f80[6];
                    B = (float)cur.stat_21x(1);
                }
                if (active && cur.max_amount() != (int)(cur.m4 & 0xff)) {
                    float zero = 0.0f;
                    float p1 = ((B + B + 100.0f) - A) * 0.00015f;
                    float q = A + 100.0f;
                    float p2 = 0.7f - timer.elapsed() * q * 0.01f;
                    float zero2 = 0.0f;
                    float r1 = fmax_ref(&zero, &p1);
                    float acc = fmax_ref(&zero2, &p2) + r1;
                    int ag = f2c;
                    if (ag >= 0) {
                        AgentTable* tb = &g_mission->agents;
                        if (1.0f < fabs(tb->get(ag)->vel_xy.y)) {
                            float t5 = ((float)fabs(tb->get(ag)->vel_xy.y) + 5.0f - (A + 150.0f) * (((float)g_game->skill_level(troop, skl_riding) + 5.0f) * ((float)g_game->skill_level(troop, skl_horse_archery) + 2.0f)) * 0.0004f) * 0.005f;
                            float zero3 = 0.0f;
                            float m = 4.0f / ((float)g_game->skill_level(troop, skl_horse_archery) + 4.0f);
                            acc = (float)(fmax_ref(&zero3, &t5) * m * nz + acc);
                        }
                    } else {
                        float sp = (float)sqrt(vel_xy.y * vel_xy.y + vel_xy.x * vel_xy.x);
                        if (sp > 0.0f) acc = (((sp + 3.0f) / q) * nz * 3.0f + acc);
                    }
                    float zero4 = 0.0f;
                    float y = (100.0f / q) * spd - A * 0.004f;
                    v = (1.0f - cf) * 0.08f + (fmax_ref(&zero4, &y) * 0.15f + acc);
                } else {
                    v = 0.0;
                }
            }
        }
    }
    double rate = g_step * 5.0f;
    if (!active) rate = g_step;
    if (aim_error < v) rate = rate + rate;
    if (rate > 1.0) rate = 1.0f;
    aim_error = (float)(rate * v + (1.0 - rate) * aim_error);
}

// ---- input queue, nav face tracking and the movement animation chooser -------------------------------------------------------------------------------------------------------
// @ 0x43c9c0
void Agent::poll_input(int unused, InputState* out)
{
    InputState orig = *out;
    *out = in_prev;
    if (q_count != 0) {
        unsigned now_lo = ((unsigned*)g_ticks)[4];
        int now_hi = ((int*)g_ticks)[5];
        while (q_count != 0) {
            unsigned i = q_head;
            if (q_cap <= i)
                i -= q_cap;
            InputRec* e = q_buf[i];
            if (e->hi > now_hi || (e->hi == now_hi && e->lo >= now_lo))
                break;
            i = q_head;
            if (q_cap <= i)
                i -= q_cap;
            out->v[0] |= q_buf[i]->bits;
            in_last = *out;
            if (q_count != 0) {
                q_head++;
                if (q_cap <= q_head)
                    q_head = 0;
                q_count--;
                if (q_count == 0)
                    q_head = 0;
            }
        }
    }
    out->v[0] |= in_last.v[0];
    if ((orig.v[0] & 0x100) == 0 && (out->v[0] & 0x100) != 0)
        out->v[0] |= 0x400;
}

// @ 0x43cb30
int Agent::update_nav_face()
{
    vector4 d(nav_pos.x - pos.x, nav_pos.y - pos.y, nav_pos.z - pos.z);
    if (d.y * d.y + (d.z * d.z + d.x * d.x) > 0.001f) {
        vector4* p = &pos;
        int face = nav_face;
        nav_pos = *p;
        if (face != -1) {
            if (g_mission->scene->nav->inside(&nav_face, &p->x))
                return nav_face;
            if (face != -1) {
                int mode = 2;
                nav_face = g_mission->scene->locate_near(p, &nav_face, &mode);
                if (nav_face != -1)
                    return nav_face;
                nav_face = g_mission->scene->nearest_face(&p->x);
                return nav_face;
            }
        }
        nav_face = g_mission->scene->nearest_face(&p->x);
    }
    return nav_face;
}

// Weapon-kind groups of the movement animation table (kind 9; 0 and 7; 1..6; everything else).
#define PICK(k9, k07, k16, kx, m9, m07, m16, mx, shared) \
    if (kind == 9) { \
        if (mode == 2) return m9; \
        return (mode != 1 ? -16 : 0) + k9; \
    } \
    if (kind == 0 || kind == 7) { \
        if (mode != 2) return (mode != 1 ? -16 : 0) + k07; \
    } else { \
        if (kind != 1 && kind != 4 && kind != 2 && kind != 5 && kind != 3 && kind != 6) { \
            if (mode == 2) return mx; \
            return (mode != 1 ? -16 : 0) + kx; \
        } \
        if (mode != 2) return (mode != 1 ? -16 : 0) + k16; \
    } \
    return shared;

// @ 0x43cc30
int Agent::move_anim(int mode)
{
    vector4 t;
    t.x = 0.0f; t.y = 0.0f; t.z = 0.0f;
    if (target_id >= 0) {
        unsigned tid = target_id;
        Agent** pp = &g_mission->agents.pages[tid >> 4];
        t = (*pp + (tid & 0xf))->pos;
    }
    vector4 d(t.x - pos.x, t.y - pos.y, t.z - pos.z);
    if (d.x * vel.x + d.y * vel.y + d.z * vel.z > 0.0f) {
        AgentBody* b = body;
        if (move_z * b->m.s.z + move_y * b->m.s.y + move_x * b->m.s.x > 0.5f) {
            int kind = weapon_kind;
            PICK(0xff, 0x107, 0x10b, 0x103, 0x10f, 0x116, 0x116, 0x113, 0x116)
        }
        if (!(-b->m.s.z * move_z + -b->m.s.y * move_y + -b->m.s.x * move_x > 0.5f)) {
            int kind = weapon_kind;
            PICK(0x100, 0x108, 0x10c, 0x104, 0x110, 0x117, 0x117, 0x114, 0x117)
        }
        int kind = weapon_kind;
        PICK(0xfe, 0x106, 0x10a, 0x102, 0x10e, 0x115, 0x115, 0x112, 0x115)
    }
    int kind = weapon_kind;
    PICK(0x101, 0x109, 0x10d, 0x105, 0x111, 0x118, 0x118, 0x118, 0x118)
}

// ---- animation blend update ---------------------------------------------------------------------------------------------------------------------------------------------------------
static inline Agent* AGENT(unsigned id)
{
    Agent** pp = &g_mission->agents.pages[id >> 4];
    return *pp + (id & 0xf);
}

// Bone-mask bits (64-bit values stored in .data; provisional names)
extern __int64 g_mask_a0;       // 0x5a8ea0
extern __int64 g_mask_a8;       // 0x5a8ea8
extern __int64 g_mask_e68;      // 0x5a8e68
extern __int64 g_mask_ed0;      // 0x5a8ed0
extern __int64 g_mask_ef8;      // 0x5a8ef8
extern __int64 g_mask_f18;      // 0x5a8f18

// per-tick update of the two animation slots: next sub-animation, bone masks, channel weights from the pitch,
// head / torso rotations from the look angles
// @ 0x43d040
void Agent::update_anims(int unused)
{
    int i;
    if (disabled != 0)
        return;

    i = 0;
    do {
        AnimSlot* s = &anim[i];
        int v = s->next;
        if (v == -1 || v == s->cur) {
            if (body->skel->chans[i].busy() && s->cur != -1) {
                if (anim[i].sub != -1 && v != -1 && ((defs->defs[s->cur].subs[anim[i].sub].flags & 0x10000000) != 0)) {
                    v = pick_slot_anim(i);
                    if (v != anim[i].sub)
                        goto start;
                }
            }
        } else {
            v = pick_slot_anim(i);
        start:
            if (v != -1 && s->next != -1) {
                unsigned f1 = 0;
                float f2 = 0;
                if (s->cur != -1 && anim[i].sub != -1) {
                    SubAnim* o = &defs->defs[s->cur].subs[anim[i].sub];
                    f1 = o->flags;
                    f2 = o->offset;
                }
                start_channel(i, &defs->defs[s->next].subs[v], &f1, &f2);
                anim[i].sub = v;
                s->f28 = defs->defs[s->next].f2c;
                if (i == 0)
                    f56e4 = 1.0f;
            }
        }
        s->cur = s->next;
        i++;
    } while (i < 2);

    char act[2];
    int a0 = (anim[0].cur == -1 || anim[0].sub == -1) ? 0 : 1;
    act[0] = a0;
    int a1 = (anim[1].cur == -1 || anim[1].sub == -1) ? 0 : 1;
    act[1] = a1;

    unsigned bits[2];
    unsigned m = 0xffffffff;
    if (act[0]) {
        unsigned fl = defs->defs[anim[0].cur].flags;
        if (fl & 0x100) { bits[0] = body->skel->sel[0x28 / 4]; m = ~bits[0]; }
        else if (fl & 0x200) { bits[0] = body->skel->sel[0x2c / 4]; m = ~bits[0]; }
        else if (fl & 0x400) { bits[0] = body->skel->sel[0x30 / 4]; m = ~bits[0]; }
        else { bits[0] = 0; m = ~bits[0]; }
    } else {
        bits[0] = 0;
    }
    if (act[1]) {
        unsigned fl = defs->defs[anim[1].cur].flags;
        unsigned x;
        if (fl & 0x100) x = body->skel->sel[0x28 / 4];
        else if (fl & 0x200) x = body->skel->sel[0x2c / 4];
        else if (fl & 0x400) x = body->skel->sel[0x30 / 4];
        else x = 0;
        bits[1] = (x & m) | (m & ~x);
        m = 0;
    } else {
        bits[1] = 0;
    }
    if (act[0])
        bits[0] |= m;

    i = 0;
    do {
        unsigned nb = bits[i];
        Skeleton* sk = body->skel;
        unsigned diff = anim[i].prev_mask ^ nb;
        unsigned bit = 1;
        int off = 0;
        for (int k = 0; k < sk->nbones; k++) {
            if ((bit & diff) != 0)
                *(int*)((char*)sk->recs + off + 0x1d0) = 0;
            bit <<= 1;
            off += 0x1dc;
        }
        Channel* c = &body->skel->chans[i];
        c->flags = nb;
        c->fc = 0;
        anim[i].prev_mask = nb;
        if (anim[i].f10 != -1)
            body->skel->chans[i].f3c = anim[i].f10;
        i++;
    } while (i < 2);

    i = 0;
    do {
        if (act[i]) {
            AnimDef* d = &defs->defs[anim[i].cur];
            if ((d->flags & 1) != 0 && f2c >= 0)
                body->skel->set_rec_phase(i, AGENT(f2c)->anim[0].scale);
            if ((d->flags & 0x1000) == 0) {
                body->skel->chans[i].w1 = 0.0f;
                body->skel->chans[i].w0 = 1.0f;
            } else {
                double v = asin(fz);
                if (v > 0.0) {
                    v = v * 1.0504225f;
                    body->skel->chans[i].rate = anim[i].f28;
                    body->skel->chans[i].w1 = 0.0f;
                    if (v < 0.0) v = 0.0; else if (v > 1.0) v = 1.0;
                    body->skel->chans[i].w1 = (float)v;
                    body->skel->chans[i].w0 = (float)(1.0 - v);
                } else {
                    v = v * -1.0504225f;
                    body->skel->chans[i].rate = -anim[i].f28;
                    body->skel->chans[i].w1 = 0.0f;
                    if (v < 0.0) v = 0.0; else if (v > 1.0) v = 1.0;
                    body->skel->chans[i].w1 = (float)v;
                    body->skel->chans[i].w0 = (float)(1.0 - v);
                }
            }
            if ((d->flags & 0x2000) != 0 && f2c != -1) {
                double h = cur * 0.5f * 0.5f;
                vector4 q;
                q.y = 0.0f;
                q.w = 0.0f;
                q.z = (float)sin(h);
                q.x = (float)cos(h);
                *(vector4*)(body->skel->chans[i].xform + 0x70 / 4) = q;
                body->skel->chans[i].mask_lo |= (unsigned)g_mask_a0;
                body->skel->chans[i].mask_hi |= (unsigned)(g_mask_a0 >> 32);
                *(vector4*)(body->skel->chans[i].xform + 0x80 / 4) = q;
                *(__int64*)&body->skel->chans[i].mask_lo |= g_mask_a8;
            }
            if (f10 != 0 && (d->flags & 0x8000) == 0) {
                double h = -f56cc * 0.5f;
                vector4 q;
                q.z = 0.0f;
                q.w = 0.0f;
                vector4 t;
                t.x = 0.0f;
                q.y = (float)sin(h);
                q.x = (float)cos(h);
                *(vector4*)body->skel->chans[i].xform = q;
                body->skel->chans[i].mask_lo |= (unsigned)g_mask_e68;
                body->skel->chans[i].mask_hi |= (unsigned)(g_mask_e68 >> 32);
                double a = -f56cc;
                double c1 = cos(a);
                if (f56cc < 0.0f) {
                    t.y = (float)((1.0 - c1) * 1.59f);
                    t.z = (float)(sin(a) * -0.79f);
                } else {
                    t.y = (float)((1.0 - c1) * -0.99f);
                    t.z = (float)(sin(a) * -0.89f);
                }
                *(vector4*)body->skel->chans[i].v6c = t;
                body->skel->chans[i].mask_lo |= (unsigned)g_mask_e68;
                body->skel->chans[i].mask_hi |= (unsigned)(g_mask_e68 >> 32);
                q.y = -q.y;
                *(vector4*)(body->skel->chans[i].xform + 0x80 / 4) = q;
                body->skel->chans[i].mask_lo |= (unsigned)g_mask_a8;
                body->skel->chans[i].mask_hi |= (unsigned)(g_mask_a8 >> 32);
                *(vector4*)(body->skel->chans[i].xform + 0xd0 / 4) = q;
                body->skel->chans[i].mask_lo |= (unsigned)g_mask_ed0;
                body->skel->chans[i].mask_hi |= (unsigned)(g_mask_ed0 >> 32);
                *(vector4*)(body->skel->chans[i].xform + 0x120 / 4) = q;
                body->skel->chans[i].mask_lo |= (unsigned)g_mask_ef8;
                body->skel->chans[i].mask_hi |= (unsigned)(g_mask_ef8 >> 32);
                *(vector4*)(body->skel->chans[i].xform + 0x160 / 4) = q;
                *(__int64*)&body->skel->chans[i].mask_lo |= g_mask_f18;
            }
        }
        i++;
    } while (i < 2);

    i = 0;
    do {
        AnimSlot* s = &anim[i];
        if ((s->events & 0x100000) != 0)
            body->skel->chans[i].set_phase(s->f1c);
        unsigned ev = s->events;
        if ((ev & 0x3000000) == 0 || (body->skel->chans[i].busy() && (ev & 0x2000000) == 0))
            s->events = 0;
        if (!(s->phase < 0.0f)) {
            body->skel->set_rec_phase(i, s->phase);
            s->phase = -1.0f;
        }
        i++;
    } while (i < 2);

    if (f10 == 0 && anim[0].cur >= 0) {
        AnimDef* d = &defs->defs[anim[0].cur];
        if ((d->flags & 0x4000) != 0) {
            float* p = &d->subs[anim[0].sub].v2c;
            float x = p[0], y = p[1], z = p[2], w = p[3];
            Channel* c = body->skel->chans;
            float sc = anim[0].scale;
            c->v6c[0] = -x * sc;
            c->v6c[1] = -y * sc;
            c->v6c[2] = -z * sc;
            c->v6c[3] = w;
            c->mask_lo |= (unsigned)g_mask_e68;
            c->mask_hi |= (unsigned)(g_mask_e68 >> 32);
        }
    }
}

// ---- path goals and the path follower ----------------------------------------------------------------------------------------------------------------------------------------------------
// @ 0x43db60
E1c8* vecE1c8::ufill(E1c8* f, unsigned n, const E1c8& v)
{
    ptr_tag t;
    uninit_fill_n1c8(f, n, v, *this, t);
    return f + n;
}

// @ 0x43db90
void Agent::set_goal_agent(unsigned aid)
{
    int a = AGENT(aid)->update_nav_face();
    if (update_nav_face() != a) {
        if (path_active != 0 && AGENT(aid)->update_nav_face() == goal_face) {
            goal_changed = 0;
            goto done;
        }
        goal_changed = 1;
    }
    path_active = 0;
done:
    f318 = aid;
    goal = AGENT(aid)->pos;
    goal_dir = AGENT(aid)->v56b4;
    goal_face = AGENT(aid)->update_nav_face();
    path_flag = 0;
    path_state = 2;
}

// @ 0x43dcb0
inline void Agent::set_goal_point(vector4 p, int face)
{
    if (update_nav_face() != face) {
        if (path_active != 0 && face == goal_face) {
            goal_changed = 0;
            goto done;
        }
        goal_changed = 1;
    }
    path_active = 0;
done:
    goal = p;
    p.x = 0.0f; p.y = 0.0f; p.z = 0.0f;
    goal_dir = p;
    f318 = -1;
    goal_face = face;
    path_state = 1;
    path_flag = 0;
}

// @ 0x43dd70
void Agent::set_goal_near(const vector4* center, const float* radius)
{
    int i = 0;
    float r = *radius;
    vector4 p;
    Mission* m;
    do {
        p = *center;
        p.x = ((rand() % 0x3dc9) * 6.322312e-05f - 0.5f) * r * 2.0f + p.x;
        p.y = ((rand() % 0x3dc9) * 6.322312e-05f - 0.5f) * r * 2.0f + p.y;
        m = g_mission;
        if (m->min_x <= p.x && p.x < m->max_x && m->min_y <= p.y && p.y < m->max_y)
            break;
        i++;
        r *= 0.9f;
    } while (i < 0x20);
    vector4 d(goal.x - p.x, goal.y - p.y, goal.z - p.z);
    if (d.z * d.z + (d.y * d.y + d.x * d.x) < 1e-06f) {
        path_state = 1;
        return;
    }
    int cur = goal_face;
    int face;
    if (cur >= 0 && m->nav.inside(&goal_face, &p.x)) {
        face = cur;
    } else {
        int mode = 2;
        if (cur >= 0 && (face = m->scene->locate_near(&p, &goal_face, &mode)) != -1) {
        } else {
            face = g_mission->scene->nearest_face(&p.x);
        }
    }
    set_goal_point(p, face);
}

// @ 0x43df60
void Agent::advance_path()
{
    if (path_active != 0 && (path_state & 3) != 0) {
        int face = update_nav_face();
        int n = path_count;
        if (face == path_face[n]) {
            path_active = 0;
            goal_changed = 0;
            return;
        }
        if (face >= 0) {
            NavMesh* nav = &g_mission->nav;
            if (nav->inside(&face, &pos.x)) {
                int next = path_cur + 1;
                int nface;
                vector4 np;
                if (next < n) {
                    nface = path_face[path_cur + 2];
                    np = path_pos[path_cur];
                } else {
                    nface = goal_face;
                    np = goal;
                }
                int steps = 10;
                vector4 d;
                d.x = pos.x - np.x;
                d.y = pos.y - np.y;
                d.z = pos.z - np.z;
                if (d.out_length_squared() < 900.0f) {
                    if (nav->trace(face, pos, nface, &np.x, &steps))
                        path_cur = next;
                }
            }
        }
        if (path_cur >= path_count) {
            path_active = 0;
            goal_changed = 0;
        }
    }
}

// deque<T>::_Xlen
// @ 0x43e0e0
void deque_xlen() { throw std::length_error("deque<T> too long"); }

// destroys the two int vectors of an E56 record (std::allocator<E56>::destroy)
// @ 0x43e150
void __stdcall tidy_vec_pair(VecPair2* p)
{
    p->b.tidy();
    p->a.tidy();
}

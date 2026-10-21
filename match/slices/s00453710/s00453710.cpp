// Slice s00453710: Agent::hit_damage (provisional name), the damage magnitude of a blow or missile hit from this
// agent on the agent `victim`: the base from the wielded weapon (melee swing stat x scale, or missile speed
// ratio ^ exponent), the couched-lance and swing-direction speed bonus with its "ui_speed_bonus" message, then
// the weapon-type skill/proficiency/strength modifiers, clamped to [0, 500].
#include <stdio.h>
#include "s00453710.h"

// Same function as 0x4350c0 (s00434ad0), defined here as a static so cl gives it the register convention
// (item in EAX, flag on the stack). Module System item types: header_items.py itp_type_*.
enum { itp_type_one_handed_wpn = 2, itp_type_two_handed_wpn = 3, itp_type_polearm = 4, itp_type_bow = 8,
       itp_type_crossbow = 9, itp_type_thrown = 10, itp_type_pistol = 0x10, itp_type_musket = 0x11 };
// @ 0x4350c0 (copy; not part of this slice)
static __declspec(noinline) int slot_class_impl(int item, int flag)
{
    unsigned t = g_items[item].type_flags & 0xff;
    if (t == itp_type_bow) return 3;
    if (t == itp_type_crossbow) return 4;
    if (t == itp_type_thrown) return 5;
    if (t == itp_type_pistol || t == itp_type_musket) return 6;
    if (t == itp_type_polearm) return 2;
    if (t == itp_type_one_handed_wpn) return 0;
    return flag == 0;
}

static inline Agent* AGENT(Mission* ms, unsigned id)
{
    return &ms->agents[id >> 4][id & 0xf];
}

static __forceinline vector4 vmul_vs(const vector4& v, const float& s)
{
    vector4 r;
    r.x = v.x * s; r.y = v.y * s; r.z = v.z * s;
    return r;
}
static __forceinline vector4 vmul_sv(const float& s, const vector4& v)
{
    vector4 r;
    r.x = s * v.x; r.y = s * v.y; r.z = s * v.z;
    return r;
}
static __forceinline vector4 vsub(const vector4& a, const vector4& b)
{
    vector4 r;
    r.x = a.x - b.x; r.y = a.y - b.y; r.z = a.z - b.z;
    return r;
}
static __forceinline vector4 rot_xy(const pos& M, Vec2 p)
{
    vector4 t;
    t.x = p.x * M.s.x + 0.0f * M.u.x + p.y * M.f.x;
    t.y = 0.0f * M.u.y + p.x * M.s.y + p.y * M.f.y;
    t.z = 0.0f * M.u.z + p.x * M.s.z + p.y * M.f.z;
    return t;
}
static __forceinline vector4 rot_inl(const pos& M, const vector4& p)
{
    vector4 t;
    t.x = p.x * M.s.x + p.z * M.u.x + p.y * M.f.x;
    t.y = p.z * M.u.y + p.x * M.s.y + p.y * M.f.y;
    t.z = p.z * M.u.z + p.x * M.s.z + p.y * M.f.z;
    return t;
}

// @ 0x453710
void Agent::hit_damage(int hand, Missile* missile, float* out_dmg, unsigned* out_flag, unsigned victim, float* scale)
{
    ItemRef sp = slot_pair(hand);
    *out_dmg = 0.0f;
    Mission* ms = g_mission;
    vector4 A;
    A.x = 0.0f; A.y = 0.0f; A.z = 0.0f;
    if ((int)victim >= 0) {
        unsigned mid = AGENT(ms, victim)->mount;
        Agent* v = AGENT(ms, victim);
        if ((int)mid >= 0) {
            A = vmul_vs(AGENT(ms, mid)->body->m.f, AGENT(ms, mid)->vel.y);
        } else {
            vector4 lv;
            lv.x = v->vel.x; lv.y = v->vel.y; lv.z = 0.0f;
            A = v->body->m.rot_ext(lv);
        }
    }
    vector4 B;
    if (mount >= 0) {
        B = vmul_sv(AGENT(ms, mount)->vel.y, AGENT(ms, mount)->body->m.f);
    } else {
        Vec2 lv = vel;
        B = rot_xy(body->m, lv);
    }
    if (missile) {
        *out_flag = 1;
        float spd = missile->speed;
        if ((int)victim >= 0) {
            float z = missile->dz * missile->speed;
            float x = A.x - missile->speed * missile->dx;
            float y = A.y - missile->dy * missile->speed;
            spd = (float)sqrt(x * x + y * y + (A.z - z) * (A.z - z));
        }
        *out_dmg = (float)pow((double)(spd / missile->ref_speed), (double)g_pow_a) * missile->dmg_scale * *scale;
    } else {
        int st = hand ? hand_b : hand_a;
        unsigned fl = (st == 0);
        *out_flag = fl;
        float base;
        if (sp.item >= 0)
            base = (float)(int)sp.stat_21x(fl) * *scale;
        else
            base = (float)g_game->chars[troop].f58 * 0.3f + 2.0f;
        *out_dmg = base;
        vector4 d = vsub(A, B);
        float len = d.normalize();
        if (couching()) {
            *out_dmg = (*out_dmg + 5.0f) * (float)pow((double)len, (double)1.8f);
            vector4 q = body->m.rot_ext(*(vector4*)(body->skel->rec_inl(19) + 0x10));
            q.normalize();
            *out_dmg = ((0.2f - (q.x * d.x + q.y * d.y + q.z * d.z)) * *out_dmg) * g_lance_scale * 0.2f;
            if (g_ui_enabled) {
                if (id == g_mission->local_id)
                    g_msglog.add(*g_strtable.lookup(tstring("ui_delivered_couched_lance_damage")), 0xffd03030);
                else if (victim == (unsigned)g_mission->local_id)
                    g_msglog.add(*g_strtable.lookup(tstring("ui_received_couched_lance_damage")), 0xffd03030);
            }
        } else {
            if (hand_a == 3)
                *out_dmg = *out_dmg * 1.05f;
            vector4 q = body->m.rot_ext(*(vector4*)(body->skel->rec_inl(19) + 0x10));
            q.normalize();
            int m = hand_a;
            vector4 up;
            up.x = 0.0f; up.y = 0.0f; up.z = 1.0f;
            if (m == 0) {
                A = q;
            } else {
                vector4* ax;
                if (m == 2) {
                    ax = &up;
                } else {
                    if (m == 1) {
                        vector4 c = rgl::cross(q, up);
                        up.x = -c.x; up.y = -c.y; up.z = -c.z;
                        A = up;
                        goto done_dir;
                    }
                    A = rgl::cross(q, up);
                    A.normalize();
                    ax = &A;
                }
                A = rgl::cross(q, *ax);
            }
        done_dir:
            A.normalize();
            float t = 1.0f;
            float h = f118;
            if (h < 0.33f) {
                t = h * 3.030303f;
                if (t < 0.0f)
                    t = 0.0f;
                else if (t >= 1.0f)
                    t = 1.0f;
                if (m != 0)
                    t = t * 0.7f + 0.3f;
            } else if (h > 0.61f) {
                float x = (h - 0.65f) * -25.000025f;
                t = 1.0f;
                float lo = 0.0f;
                t = clamp_float(&x, &lo, &t);
            }
            float w = (float)sp.stat_1f0a() * 0.154f;
            if (m == 0)
                w = w * 0.93f;
            if (t < 1e-06f) {
                *out_dmg = 0.0f;
            } else {
                if (t < 0.1f)
                    t = 0.1f;
                else if (t >= 1.2f)
                    t = 1.2f;
                t = (t * w - (A.x * d.x + A.y * d.y + A.z * d.z) * len) / w;
                if (t < 0.3f)
                    t = 0.3f;
                else if (t > 3.0f)
                    t = 3.0f;
                float p = (float)pow((double)t, (double)g_pow_b);
                *out_dmg = p * *out_dmg;
                float bonus = (p - 1.0f) * 100.0f;
                int n = round_float(&bonus);
                if (n != 0 && g_ui_enabled && id == g_mission->local_id) {
                    char msg[512];
                    if (n > 0)
                        sprintf(msg, g_strtable.lookup(tstring("ui_speed_bonus_plus"))->c_str(), n);
                    else
                        sprintf(msg, g_strtable.lookup(tstring("ui_speed_bonus"))->c_str(), n);
                    g_msglog.add(tstring(msg), -1);
                }
            }
        }
    }
    if (sp.item >= 0) {
        ItemRef w = slot_pair_inl(1);
        int cls = slot_class_impl(sp.item, w.item >= 0);
        int prof = (int)g_game->chars[troop].prof[cls];
        if ((g_items[sp.item].type_flags & 0xff) == itp_type_crossbow) {
            if (g_mission->mode6604 == 1)
                *out_dmg = *out_dmg * 0.75f;
        } else {
            float pct = (float)prof * 0.01f;
            *out_dmg = (pct * 0.15f + 0.85f) * *out_dmg;
            unsigned t = g_items[sp.item].type_flags & 0xff;
            if (t == itp_type_bow) {
                int sk = g_game->skill_level(troop, 0x21);
                int lim = sp.stat_1ec() + 4;
                if (!(lim < sk)) lim = sk;
                *out_dmg = ((float)lim * 0.14f + 1.0f) * *out_dmg;
                if (mount != -1) {
                    int hs = g_game->skill_level(troop, 0x17);
                    *out_dmg = ((float)hs * 0.019f + 0.8f) * *out_dmg;
                }
                if (g_mission->mode6604 == 1)
                    *out_dmg = *out_dmg * 0.9f;
            } else if (t == itp_type_thrown) {
                int sk = g_game->skill_level(troop, 0x22);
                *out_dmg = ((float)sk * 0.1f + 1.0f) * *out_dmg;
                if (mount != -1) {
                    int hs = g_game->skill_level(troop, 0x17);
                    *out_dmg = ((float)hs * 0.019f + 0.8f) * *out_dmg;
                }
            } else if (g_items[sp.item].is_melee()) {
                int sk = g_game->skill_level(troop, 0x23);
                *out_dmg = ((float)sk * 0.08f + 1.0f) * *out_dmg;
            }
            *out_dmg = (float)g_game->chars[troop].f58 * 0.2f + *out_dmg;
        }
        unsigned t2 = g_items[sp.item].type_flags & 0xff;
        if (t2 == itp_type_two_handed_wpn || t2 == itp_type_polearm) {
            ItemRef w2 = slot_pair_inl(1);
            if (w2.item >= 0 || mount >= 0)
                *out_dmg = *out_dmg * (0.75f * 0.9f);
        }
    }
    if (*out_dmg < 0.0f)
        *out_dmg = 0.0f;
    else if (*out_dmg > 500.0f)
        *out_dmg = 500.0f;
}

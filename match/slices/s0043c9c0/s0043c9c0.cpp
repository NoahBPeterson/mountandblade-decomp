// Slice s0043c9c0: Agent input queue poll, nav-face tracking and the movement-direction animation chooser
// (provisional names).
#include "agent_nav.h"

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
                nav_face = g_mission->scene->locate(p);
                return nav_face;
            }
        }
        nav_face = g_mission->scene->locate(p);
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
        unsigned id = target_id;
        Agent** pp = &g_mission->pages[id >> 4];
        t = (*pp + (id & 0xf))->pos;
    }
    vector4 d(t.x - pos.x, t.y - pos.y, t.z - pos.z);
    if (d.x * fwd_x + d.y * fwd_y + d.z * fwd_z > 0.0f) {
        AgentBody* b = body;
        if (move_z * b->dz + move_y * b->dy + move_x * b->dx > 0.5f) {
            int kind = weapon_kind;
            PICK(0xff, 0x107, 0x10b, 0x103, 0x10f, 0x116, 0x116, 0x113, 0x116)
        }
        if (!(-b->dz * move_z + -b->dy * move_y + -b->dx * move_x > 0.5f)) {
            int kind = weapon_kind;
            PICK(0x100, 0x108, 0x10c, 0x104, 0x110, 0x117, 0x117, 0x114, 0x117)
        }
        int kind = weapon_kind;
        PICK(0xfe, 0x106, 0x10a, 0x102, 0x10e, 0x115, 0x115, 0x112, 0x115)
    }
    int kind = weapon_kind;
    PICK(0x101, 0x109, 0x10d, 0x105, 0x111, 0x118, 0x118, 0x118, 0x118)
}

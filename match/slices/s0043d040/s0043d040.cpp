// Slice s0043d040: Agent::update_anims, the per-tick animation blend update of the two animation slots of an
// agent (provisional name): picks the next sub-animation, assigns bone masks to the skeleton's channels,
// blends the channel weights from the agent's pitch and builds the head/torso rotations from its look angles.
#include <math.h>
#include "agent_nav.h"

#define SUB(i) (*(int*)((char*)this + ((i) + 5) * 0x2c))     // == slot[i].sub
static inline Agent* AGENT(unsigned id)
{
    Agent** pp = &g_mission->pages[id >> 4];
    return *pp + (id & 0xf);
}

// Bone-mask bits (64-bit values stored in .data; provisional names)
extern __int64 g_mask_a0;       // 0x5a8ea0
extern __int64 g_mask_a8;       // 0x5a8ea8
extern __int64 g_mask_e68;      // 0x5a8e68
extern __int64 g_mask_ed0;      // 0x5a8ed0
extern __int64 g_mask_ef8;      // 0x5a8ef8
extern __int64 g_mask_f18;      // 0x5a8f18

// 0x43a980 (s0043a280): start a sub-animation on a channel
struct StartCmd;

// @ 0x43d040
void Agent::update_anims(int unused)
{
    int i;
    if (disabled != 0)
        return;

    i = 0;
    do {
        AnimSlot* s = &slot[i];
        int v = s->next;
        if (v == -1 || v == s->cur) {
            if (body->skel->chans[i].busy() && s->cur != -1) {
                if (SUB(i) != -1 && v != -1 && (((*lib)[s->cur].subs[SUB(i)].flags & 0x10000000) != 0)) {
                    v = pick_slot_anim(i);
                    if (v != SUB(i))
                        goto start;
                }
            }
        } else {
            v = pick_slot_anim(i);
        start:
            if (v != -1 && s->next != -1) {
                unsigned f1 = 0;
                float f2 = 0;
                if (s->cur != -1 && SUB(i) != -1) {
                    SubAnim* o = &(*lib)[s->cur].subs[SUB(i)];
                    f1 = o->flags;
                    f2 = o->offset;
                }
                start_channel(i, &(*lib)[s->next].subs[v], &f1, &f2);
                SUB(i) = v;
                s->f28 = (*lib)[s->next].f2c;
                if (i == 0)
                    f56e4 = 1.0f;
            }
        }
        s->cur = s->next;
        i++;
    } while (i < 2);

    char act[2];
    int a0 = (slot[0].cur == -1 || slot[0].sub == -1) ? 0 : 1;
    act[0] = a0;
    int a1 = (slot[1].cur == -1 || slot[1].sub == -1) ? 0 : 1;
    act[1] = a1;

    unsigned bits[2];
    unsigned m = 0xffffffff;
    if (act[0]) {
        unsigned fl = (*lib)[slot[0].cur].flags;
        if (fl & 0x100) { bits[0] = body->skel->sel[0x28 / 4]; m = ~bits[0]; }
        else if (fl & 0x200) { bits[0] = body->skel->sel[0x2c / 4]; m = ~bits[0]; }
        else if (fl & 0x400) { bits[0] = body->skel->sel[0x30 / 4]; m = ~bits[0]; }
        else { bits[0] = 0; m = ~bits[0]; }
    } else {
        bits[0] = 0;
    }
    if (act[1]) {
        unsigned fl = (*lib)[slot[1].cur].flags;
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
        unsigned diff = slot[i].prev_mask ^ nb;
        unsigned bit = 1;
        int off = 0;
        for (int k = 0; k < sk->nrecs; k++) {
            if ((bit & diff) != 0)
                *(int*)(sk->recs + off + 0x1d0) = 0;
            bit <<= 1;
            off += 0x1dc;
        }
        Channel* c = &body->skel->chans[i];
        c->flags = nb;
        c->fc = 0;
        slot[i].prev_mask = nb;
        if (slot[i].f10 != -1)
            body->skel->chans[i].f3c = slot[i].f10;
        i++;
    } while (i < 2);

    i = 0;
    do {
        if (act[i]) {
            AnimDef* d = &(*lib)[slot[i].cur];
            if ((d->flags & 1) != 0 && id >= 0)
                body->skel->set_rec_phase(i, AGENT(id)->slot[0].scale);
            if ((d->flags & 0x1000) == 0) {
                body->skel->chans[i].w1 = 0.0f;
                body->skel->chans[i].w0 = 1.0f;
            } else {
                double v = asin(dir_z);
                if (v > 0.0) {
                    v = v * 1.0504225f;
                    body->skel->chans[i].rate = slot[i].f28;
                    body->skel->chans[i].w1 = 0.0f;
                    if (v < 0.0) v = 0.0; else if (v > 1.0) v = 1.0;
                    body->skel->chans[i].w1 = (float)v;
                    body->skel->chans[i].w0 = (float)(1.0 - v);
                } else {
                    v = v * -1.0504225f;
                    body->skel->chans[i].rate = -slot[i].f28;
                    body->skel->chans[i].w1 = 0.0f;
                    if (v < 0.0) v = 0.0; else if (v > 1.0) v = 1.0;
                    body->skel->chans[i].w1 = (float)v;
                    body->skel->chans[i].w0 = (float)(1.0 - v);
                }
            }
            if ((d->flags & 0x2000) != 0 && id != -1) {
                double h = f56c8 * 0.5f * 0.5f;
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
        AnimSlot* s = &slot[i];
        if ((s->events & 0x100000) != 0)
            body->skel->chans[i].f433930(s->f1c);
        unsigned ev = s->events;
        if ((ev & 0x3000000) == 0 || (body->skel->chans[i].busy() && (ev & 0x2000000) == 0))
            s->events = 0;
        if (!(s->phase < 0.0f)) {
            body->skel->set_rec_phase(i, s->phase);
            s->phase = -1.0f;
        }
        i++;
    } while (i < 2);

    if (f10 == 0 && slot[0].cur >= 0) {
        AnimDef* d = &(*lib)[slot[0].cur];
        if ((d->flags & 0x4000) != 0) {
            float* p = &d->subs[slot[0].sub].v2c;
            float x = p[0], y = p[1], z = p[2], w = p[3];
            Channel* c = body->skel->chans;
            float sc = slot[0].scale;
            c->v6c[0] = -x * sc;
            c->v6c[1] = -y * sc;
            c->v6c[2] = -z * sc;
            c->v6c[3] = w;
            c->mask_lo |= (unsigned)g_mask_e68;
            c->mask_hi |= (unsigned)(g_mask_e68 >> 32);
        }
    }
}

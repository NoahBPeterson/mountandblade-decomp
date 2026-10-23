//-----------------------------------------------------------------------------
// File: mission_ui.cpp  (0x488560..0x48b2d0)
//
// Desc: the mission-UI widget base (track timers, flag / colour accessors, text placement, the constructor), its
//       list helpers, and the neighbouring records of the same object file: the item-pair sort, troop record and
//       party list helpers, selector records, a mission view, the world's party / item value queries, the replay
//       frame interpolation, per-team counters and combatant slots, and the first widget's key handler. Names
//       provisional. Functions in ascending address order (cl's bottom-up emission). Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "../tstring.h"
#include "../../rgl/rgl_math.h"
#include "../../rgl/mesh.h"
#include "./ui_common.h"
#include "./item_ref.h"
#include "./troop_inv.h"
#include "./party.h"
#include "./mission_ui.h"
#include "./ui_widgets.h"
#include "./world.h"
#include "./mission.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../include/keyframes.h"
#include "../../include/mat4.h"
#include "../../include/mesh_prims.h"
#include "../../include/vec.h"

// round to nearest, as cl inlines it (0x432540 is the out-of-line copy in another object)
static __forceinline int rnd(const float* p)
{
    float fl = floorf(*p);
    int i = (int)fl;
    return (*p - fl <= 0.5f) ? i : i + 1;
}

struct Rec80 { int v[32]; };                // the party's 0x80-byte companion records (0x489540)

extern "C" void _ReadWriteBarrier(void);    // compiler barrier (no code emitted): pins the period store ahead of the g_ticks load

// @ 0x488560
void MissionUi::set_a(float a, int b)
{
    periodA = a;
    _ReadWriteBarrier();
    t0 = g_ticks[clock0];
    valA = b;
    srcA = color14c;
    c0 = 1;
}

// @ 0x4885a0
void MissionUi::set_b(float a, int b)
{
    periodB = a;
    _ReadWriteBarrier();
    t1 = g_ticks[clock1];
    valB = b;
    srcB = color14c;
    c1 = 1;
}

// @ 0x4885e0
void MissionUi::set_c(float a, int b)
{
    periodC = a;
    _ReadWriteBarrier();
    t2 = g_ticks[clock2];
    valC = b;
    srcC = color150;
    c2 = 1;
}

// @ 0x488620
void MissionUi::set_d(float a, int b)
{
    periodD = a;
    _ReadWriteBarrier();
    t3 = g_ticks[clock3];
    valD = b;
    srcD = color150;
    c3 = 1;
}

// @ 0x488660
void MissionUi::set_e(float a, float* p)
{
    periodE = a;
    _ReadWriteBarrier();
    t4 = g_ticks[clock4];
    f88 = p[0];
    f8c = p[1];
    a8 = fD8;
    ac = fDC;
    c4 = 1;
}

// @ 0x4886c0
void MissionUi::set_f(float a, float* p)
{
    periodF = a;
    _ReadWriteBarrier();
    t5 = g_ticks[clock5];
    f80 = p[0];
    f84 = p[1];
    a0 = fE0;
    a4 = fE4;
    c5 = 1;
}

// @ 0x488720
// Per-frame track driver (ret 0x10: this + three stack words). `a` is forwarded to mesh::render, `t` is the
// caller's time accumulator, bumped by 1.0 for every child that advances. 0x40cd00 fills a 0x40-byte frame; the
// original keeps a snapshot of (fE0, fE4, fE8) and the advanced time in the words around it.
void MissionUi::render(rgl::scene* a, float* t, int, int)
{
    float xf[19];
    if (f104 & 1)
        return;
    if (tracks[4])
    {
        if (f104 & 2)
            tracks[4]->color = color14c & 0x77ffffffu;
        else
            tracks[4]->color = (b121 && d01()) ? color150 : color14c;
    }
    rgl::identity_pos((rgl::matrix*)xf);
    xf[0] = fE0;
    xf[1] = fE4;
    xf[16] = fE0;
    xf[17] = fE4;
    xf[18] = fE8;
    for (int i = 0; i < 5; i++)
    {
        if (!tracks[i])
            continue;
        if (i == 3)
        {
            if (!(b121 && d01() && !(tracks[1] && b11c[1])))
                ;
            else
            {
                xf[17] = *t + 1.0f;
                *t = xf[17];
                tracks[3]->render(a, *(rgl::matrix*)xf);
                continue;
            }
        }
        else if (i == 2)
        {
            if (b121 && d01() && tracks[3])
                continue;
        }
        if (!b11c[i])
            continue;
        xf[17] = *t + 1.0f;
        *t = xf[17];
        tracks[i]->render(a, *(rgl::matrix*)xf);
    }
}

// @ 0x488890
void MissionUi::set15c(int v)
{
    v15c = v;
}

// @ 0x4888a0
int MissionUi::get15c()
{
    return v15c;
}

// @ 0x4888b0
void MissionUi::set168(int v)
{
    *(int*)&v168 = v;
}

// @ 0x4888c0
float MissionUi::get168()
{
    return v168;
}

// @ 0x4888d0
bool MissionUi::test_c8(int m)
{
    return (unsigned char)((fC8 & m) != 0);
}

// @ 0x4888e0
void MissionUi::clear_c8()
{
    fC8 = 0;
}

// @ 0x4888f0
bool MissionUi::clear_mask(int m)
{
    bool had = (fC8 & m) != 0;
    fC8 &= ~m;
    return had;
}

// @ 0x488910
void MissionUi::or_flags(int m)
{
    fC8 |= m;
    fCC |= m;
}

// @ 0x488930
void MissionUi::set_d0(int v)
{
    fD0 = v;
}

// @ 0x488940
void MissionUi::set_d4(int v)
{
    fD4 = v;
}

// @ 0x488950
void MissionUi::or_d4(int m)
{
    fD4 |= m;
}

// @ 0x488970
// NOTE: 3 bytes differ from the original: identical instruction stream, but MSVC keeps `this`
// in edx and p in eax where the original keeps `this` in eax and p in edx.
void MissionUi::move_a(float* p)
{
    fE8 = p[0];
    fEC = p[1];
    if (p154)
    {
        p154->d23(this);
        return;
    }
    fE0 = p[0];
    fE4 = p[1];
}

// @ 0x4889b0
void MissionUi::move_f0(float* p)
{
    screen[0] = p[0];
    screen[1] = p[1];
    if (p154)
    {
        p154->d23(this);
        return;
    }
}

// @ 0x4889e0
void MissionUi::move_f8(float* p)
{
    screen[2] = p[0];
    screen[3] = p[1];
    if (p154)
    {
        p154->d23(this);
        return;
    }
}

// @ 0x488a10
void MissionUi::set138(float* p)
{
    if (f148 & 1)
        f148 &= ~1u;
    f138 = p[0];
    f13c = p[1];
}

// @ 0x488a40
void MissionUi::set148(float* p)
{
    f148 = *(unsigned*)p;
    float t[2];
    get_xy(t);
}

// @ 0x488a70
void MissionUi::or148(float* p)
{
    f148 |= *(unsigned*)p;
    float t[2];
    get_xy(t);
}

// @ 0x488aa0
void MissionUi::run(int a, int b)
{
    d33(b);
    d32(a, b);
}

// @ 0x488ad0
void MissionUi::set_track(int i, rgl::mesh* p)
{
    if (tracks[i])
        delete tracks[i];
    tracks[i] = p;
    d34(p);
}

// @ 0x488b10
void MissionUi::set14c(float* p)
{
    color14c = *(unsigned*)p;
}


// @ 0x488b20
void MissionUi::set_target(void* p)
{
    color150 = *(unsigned*)p;
}

// @ 0x488b30
void MissionUi::set_pos(int p)
{
    float v[2];
    f124 = p;
    get_xy(v);
}

// @ 0x488b50
void MissionUi::set_text(float* out, const tstring* s)
{
    if (f148 & 2) {
        float v = screen[1] * 0.95f;
        f144 = v;
        f140 = v;
    }
    name = *s;
    unsigned fl = f148;
    if (fl & 1) {
        f138 = 0.005f;
        f13c = screen[1] * 0.9f - f144 * 0.6f;
    }
    if ((fl & 0x200) && !(fl & 0x100)) {
        float c[2];
        c[0] = 1.0f;
        c[1] = 0.75f;
        set_xy(c);
    }
    float var[2];
    get_xy(var);
    fl = f148;
    if (fl & 0x200) {
        float v[2];
        float* src = (fl & 0x100) ? &screen[0] : &var[0];
        v[0] = *src;
        v[1] = var[1];
        set_xy(v);
        f13c = var[1] - f144 * 0.63f;
        get_xy(v);
    }
    b11c[4] = 1;
    *(vec2*)out = *(vec2*)var;
}

// @ 0x488c70
void MissionUi::put(tstring* out)
{
    *out = name;
}

// @ 0x488c90
void MissionUi::flag1_set()
{
    f104 |= 1;
}

// @ 0x488ca0
void MissionUi::flag1_clear()
{
    f104 &= ~1u;
}

// @ 0x488cb0
void MissionUi::flag2_set()
{
    f104 |= 2;
}

// @ 0x488cc0
void MissionUi::flag2_clear()
{
    f104 &= ~2u;
}

// Same fields and shape as renderer.h's envelope::set_fade (0x4624c0) / set_params (0x462540): the object's
// +0xdc..+0xec block is a fade envelope (target, 1/period, peak, dirty byte, smoothing); see MIGRATION.md.
// @ 0x488cd0
void MissionUi::set_speed(const float* ref, const float* lo, const float* hi)
{
    *(char*)&fE8 = 1;
    fDC = *ref;
    fE0 = *lo;
    fE4 = *hi;
    float f = (1.0f / fE0) * 3.0f;
    if (f < 0.001f) {
        fEC = 0.001f;
        return;
    }
    if (f >= 0.95f) {
        fEC = 0.95f;
        return;
    }
    fEC = f;
}

// @ 0x488d50
void MissionPanel::draw()
{
    for (int i = 0; i < 2; i++) {
        slot_on[i] = 0;
        if (slots[i]) {
            slots[i]->draw_begin();
            float d = host->screen[i] - host->screen[2 + i];
            if (d > screen[i]) {
                slot_on[i] = 1;
                slots[i]->draw_end();
                if (slot_on[i] != 0)
                    slots[i]->set_fill(screen[i] / d);
            }
        }
    }
    {
        float xy[2];
        xy[0] = 0.0f;
        xy[1] = 0.0f;
        host->d22(xy);
    }
}

// @ 0x488e20
void ElemList::call0(int i, int a)
{
    elems[i].s0(a);
}

// @ 0x488e40
void ElemList::call1(int i, int a)
{
    elems[i].s1(a);
}

// @ 0x488e60
void PairStack::push(int a, int b)
{
    if (count >= 0 && count < 0x80) {
        pairs[count * 2] = a;
        pairs[count * 2 + 1] = b;
        count++;
    }
}

// @ 0x488e90
void TimerChild::tick(int idx)
{
    if (period < -1.0f) {
        if (idx == rnd(&period)) {
            __int64 buf[16];
            int zero = 0;
            ch.emit(idx, 0, &zero, buf, -1);
        }
    } else {
        long long d = g_ticks[clock] - last;
        if (!((float)d * 1e-5f < period) && idx >= 0) {
            last = g_ticks[clock];
            ch.play(idx);
        }
    }
}

// @ 0x488f80
int __fastcall sort_value(ItemRef* e)
{
    int idx = e->item;
    if (idx >= 0) {
        float x = (float)g_items[idx].value;
        x = x * g_sort_val[e->b.mod * 5].v;
        return rnd(&x);
    }
    return 0;
}

// @ 0x488ff0
float __stdcall speed_scale(const float* base, const float* ref, float r)
{
    float d = fabsf(r - *ref);
    if ((r > 1.001f && *ref < 1.001f) || (r < 0.999f && *ref > 0.999f))
        d = d * 5.0f + 3.0f;
    return *base / (d + 1.0f);
}

// @ 0x489060
void __fastcall clear_slots(ItemRef* e)
{
    int i = 0;
    while (i < 0x60) {
        e[i].item = -1;
        i++;
    }
}

// @ 0x489070
void ItemRef::check_slots(int n)
{
    int i = 0;
    if (n > 0) {
        do {
            if (this[i].item == -1)
                break;
            i++;
        } while (i < n);
    }
}

// @ 0x489090
void ItemRef::sort_sel(int n)
{
    for (int j = 0; j < n - 1; j++) {
        int best = j;
        for (int k = j + 1; k < n; k++) {
            if (this[k].item > this[best].item ||
                (this[k].item == this[best].item && g_sort_val[this[k].b.mod * 5].v > g_sort_val[this[best].b.mod * 5].v))
                best = k;
        }
        ItemRef tmp = this[best];
        this[best] = this[j];
        this[j] = tmp;
    }
}
// @ 0x4892a0
void Troop::set_flags_low(int v)
{
    flags30 &= ~0xf;
    flags30 |= (__int64)v & 0xf;
}

// @ 0x4892d0
void Troop::add_stat(unsigned i, int amt)
{
    float v = stat[i] + (float)amt;
    stat[i] = v;
    if (v < 0.0f) {
        stat[i] = 0.0f;
        return;
    }
    if (v > 1000.0f)
        stat[i] = 1000.0f;
}

// @ 0x489320
void Troop::set_colors(int a, int b)
{
    f3b8 = (b & 0xff) << 16 | a;
}

// @ 0x489340
void Troop::sub_3b4(int amt)
{
    f3b4 -= amt;
    if (f3b4 < 0)
        f3b4 = 0;
}

// @ 0x489370
void Troop::set_alpha(int a)
{
    int v = a;
    *(unsigned char*)&f64 = 0;
    f64 |= v & 0xff;
}

// @ 0x489390
int __cdecl cursor_check(unsigned v)
{
    if ((v & 7) != 0 && v != 7)
        return 0;
    return 1;
}

// @ 0x4893b0
int sub_4893b0(unsigned x)
{
    int m = x & 7;
    if (m < 2 || m >= 7) {
        if (x != 7)
            return 0;
    }
    return 1;
}

// @ 0x4893e0
void RecA::reset()
{
    f0 = 0;
    f4 = -1;
    f8 = 0;
    name.cap = 0;
    name.pos = 0;
    name.buf = 0;
    name.len = 0;
}

// @ 0x489400
void RecA::set_name(const tstring* s)
{
    name = *s;
    flag1c = 1;
}

// @ 0x489420
void Party::set_id(int v)
{
    unsigned __int64* f = (unsigned __int64*)&flags_lo;
    ((char*)&flags_lo)[0] = 0;
    *f |= (long long)(v & 0xff);
}

// @ 0x489460
float Party::scale()
{
    unsigned u = (unsigned)flags_lo & 0x3000;
    if (u == 0)
        return 0.5f;
    if (u == 0x1000)
        return 0.75f;
    return 1.0f;
}

// @ 0x489490
int Party::sum_all()
{
    int s = 0;
    int n = nstacks;
    if (n > 0) {
        Stack20* p = stacks;
        do {
            s += p->count;
            p++;
        } while (--n);
    }
    return s;
}

// @ 0x4894b0
int Party::sum_head()
{
    int s = 0;
    int i = 0;
    int n = nstacks;
    if (n > 0) {
        Stack20* p = stacks;
        do {
            if (p->flags & 1)
                return s;
            s += p->count;
            i++;
            p++;
        } while (i < n);
    }
    return s;
}

// @ 0x4894e0
int Party::sum_flagged()
{
    int s = 0;
    int n = nstacks;
    if (n > 0) {
        Stack20* p = stacks;
        do {
            if (p->flags & 1)
                s += p->count;
            p++;
        } while (--n);
    }
    return s;
}

// @ 0x489510
int Party::count_tail()
{
    int n = nstacks;
    int lead = 0;
    int i = 0;
    if (n > 0) {
        char* q = (char*)stacks + 8;
        do {
            if (*q & 1)
                break;
            lead++;
            i++;
            q += 0x20;
        } while (i < n);
    }
    return n - lead;
}

// @ 0x489540
int* Party::remove_row(int* out, int index, int* arr)
{
    Stack20 local = stacks[index];

    int i = index;
    if (index < nstacks - 1) {
        do {
            stacks[i] = stacks[i + 1];
            i++;
        } while (i < nstacks - 1);
    }

    *(int*)((char*)this + nstacks * 0x20 + 0x8c) = -1;
    *(int*)((char*)this + nstacks * 0x20 + 0x88) = 0;
    int cnt = nstacks - 1;
    nstacks = cnt;
    dirty = 1;

    if (arr != 0 && index < cnt) {
        int j = index;
        do {
            *(Rec80*)((char*)arr + j * 0x80) = *(Rec80*)((char*)arr + (j + 1) * 0x80);
            j++;
        } while (j < nstacks);
    }

    *(Stack20*)out = local;
    return out;
}

// @ 0x489620
int Party::find_or_add(unsigned troop, unsigned flag, char force)
{
    int found = -1;
    if (*(unsigned __int64*)&flags_lo & 0x100000) {
        int have, cap;
        if (flag & 1) {
            have = 0;
            int m = nstacks;
            if (m > 0) {
                Stack20* p = stacks;
                do {
                    if (p->flags & 1)
                        have += p->count;
                    p++;
                } while (--m);
            }
            cap = max_prisoners();
        } else {
            have = sum_head();
            cap = max_members();
        }
        if (cap - have <= 0) {
            if (force)
                return found;
        }
    }

    int n = nstacks;
    int i = 0;
    if (n > 0) {
        Stack20* p = stacks;
        do {
            if (p->troop == (int)troop && ((p->flags ^ flag) & 1) == 0)
                found = i;
            i++;
            p++;
        } while (i < n);
        if (found != -1)
            return found;
    }
    if (n >= 0x40)
        return found;

    if (flag & 1) {
        if ((*(unsigned __int64*)&flags_lo & 0x100000) == 0)
            goto tail_count;
        int ct = count_tail();
        int mx = max_4b1700();
        if (ct < mx) {
        tail_count: ;
            int lead = 0;
            int m = nstacks;
            for (int k = 0; k < m; k++) {
                if (stacks[k].flags & 1)
                    break;
                lead++;
            }
            if (m - lead < 0x20)
                return nstacks;
        }
    }
    if (!(flag & 1)) {
        if (*(unsigned __int64*)&flags_lo & 0x100000) {
            int ff = first_flagged();
            int mx = max_4b1700();
            if (!(ff < mx))
                return found;
        }
        int lead = 0;
        int m = nstacks;
        for (int k = 0; k < m; k++) {
            if (stacks[k].flags & 1)
                break;
            lead++;
        }
        if (lead < 0x20)
            return nstacks;
    }
    return found;
}

// @ 0x4897a0
void MissionView::setup(void* src, int* b, int* c)
{
    ext_467c50(0);
    memcpy(v118, v18, sizeof(v18));
    memcpy(vd8, src, sizeof(vd8));
    f168 = *c;
    f16c = *b;
    int idx = f170;
    struct W2 { int lo, hi; };
    ((W2*)&f178)->lo = ((W2*)g_ticks)[idx].lo;
    ((W2*)&f178)->hi = ((W2*)g_ticks)[idx].hi;

    f180 = 0;
}

// @ 0x489820
void MissionView::set(void* src)
{
    memcpy(v18, src, sizeof(v18));
    if (obj1b4 != 0)
        obj1b4->slot23(src, &f158);
    ext_477b70();
}

// @ 0x489860
void Sel::set_v(int v)
{
    if (rec[0].present)
        rec[0].x = v;
    if (rec[1].present)
        rec[1].x = v;
    if (rec[2].present)
        rec[2].x = v;
}

// @ 0x489890
void Sel::first(int* out)
{
    int i = 0;
    int* p = &rec[0].present;
    do {
        if (*p != 0) {
            int k = i * 0x6c;
            V4* a = (V4*)((char*)this + k + 0x18);
            V4* o = (V4*)out;
            o->a = a->a;
            o->b = a->b;
            o->c = a->c;
            o->d = a->d;
            return;
        }
        i++;
        p += 0x1b;
    } while (i < 3);
    V4* zo = (V4*)out;
    zo->a = 0;
    zo->b = 0;
    zo->c = 0;
}

// @ 0x4898e0
void Sel::set_vec(const int* a, const int* b, const int* c)
{
    if (rec[0].present) {
        *(V4*)&rec[0].a[0] = *(const V4*)a;
        rec[0].b = *b;
        *(V4*)&rec[0].c[0] = *(const V4*)c;
    }
    if (rec[1].present) {
        *(V4*)&rec[1].a[0] = *(const V4*)a;
        rec[1].b = *b;
        *(V4*)&rec[1].c[0] = *(const V4*)c;
    }
    if (rec[2].present) {
        *(V4*)&rec[2].a[0] = *(const V4*)a;
        rec[2].b = *b;
        *(V4*)&rec[2].c[0] = *(const V4*)c;
    }
}

// @ 0x4899e0
void Sel::bump(int d)
{
    if (rec[0].present) {
        int v = rec[0].counter + d;
        rec[0].counter = v;
        if (v < 1)
            rec[0].counter = 1;
        else if (v > 4)
            rec[0].counter = 4;
    }
    if (rec[1].present) {
        int v = rec[1].counter + d;
        rec[1].counter = v;
        if (v < 1)
            rec[1].counter = 1;
        else if (v > 4)
            rec[1].counter = 4;
    }
    if (rec[2].present) {
        int v = rec[2].counter + d;
        rec[2].counter = v;
        if (v < 1)
            rec[2].counter = 1;
        else if (v > 4)
            rec[2].counter = 4;
    }
}

// @ 0x489a90
void RecA::set_sel(const int* src)
{
    int v = *src;
    if (v < 0 || v >= 8)
        return;
    sel = v;
}

// @ 0x489ab0
void Named16::set(const tstring* s)
{
    name = *s;
    flag10 = 1;
}

// @ 0x489ad0
void MissionView::quarter()
{
    if (g_day >= 0x3c) {
        v18[2] = v18[0];
        v18[3] = v18[1];
        return;
    }
    if (g_day >= 0x19) {
        v18[2] = v18[0] / 2;
        v18[3] = v18[1] / 2;
        return;
    }
    v18[2] = v18[0] / 4;
    v18[3] = v18[1] / 4;
}

// @ 0x489b20
void* MissionView::make(int* p)
{
    quarter();
    char* o = g_alloc.make(v18[2], v18[3], 0x15, 1);
    *(int*)(o + 0x64) = 0;
    *(int*)(o + 0x60) = *p;
    *(int*)(*(int*)(o + 0x48) + 0x4c) = 0;
    if (p64 != 0) {
        *(int*)(o + 0x30) = *(int*)((char*)p64 + 0x30);
        ((tstring*)(o + 0x20))->operator=(*(tstring*)((char*)p64 + 0x20));
        *(int*)(o + 0x1c) = *(int*)((char*)p64 + 0x1c);
        ((tstring*)(o + 0x04))->operator=(*(tstring*)((char*)p64 + 0x04));
        *(V4*)(o + 0x34) = *(V4*)((char*)p64 + 0x34);
        *(int*)(o + 0x44) = *(int*)((char*)p64 + 0x44);
        *(int*)(o + 0x50) = *(int*)((char*)p64 + 0x50);
        *(int*)(o + 0x4c) = *(int*)((char*)p64 + 0x4c);
        *(int*)(o + 0x54) = *(int*)((char*)p64 + 0x54);
        *(int*)(o + 0x58) = *(int*)((char*)p64 + 0x58);
    }
    return o;
}

// @ 0x489be0
void Ptr30::clear_all()
{
    int zero = 0, cnt = 0x63;
    int** e = &p[2];
    int i = 5;
    do {
        int* q;
        q = e[-2]; q[1] = zero; q[2] = cnt;
        q = e[-1]; q[1] = zero; q[2] = cnt;
        q = e[0];  q[1] = zero; q[2] = cnt;
        q = e[1];  q[1] = zero; q[2] = cnt;
        q = e[2];  q[1] = zero; q[2] = cnt;
        q = e[3];  q[1] = zero; q[2] = cnt;
        e += 6;
    } while (--i);
}

// @ 0x489c30
tstring World::get_name()
{
    return name;
}

// @ 0x489c50
int World::count_in(int id, int idx, char flag)
{
    Troop* base = troops;
    int c = 0;
    Troop* t = &base[idx];
    int n = t->inv_count();
    if (n > 0) {
        ItemRef* e = t->inv;
        do {
            if (e->item == id)
                c++;
            e++;
        } while (--n);
    }
    if (flag) {
        Troop* b = &base[idx];
        if (b->equip[0].item == id) c++;
        if (b->equip[1].item == id) c++;
        if (b->equip[2].item == id) c++;
        if (b->equip[3].item == id) c++;
        if (b->equip[4].item == id) c++;
        if (b->equip[5].item == id) c++;
        if (b->equip[6].item == id) c++;
        if (b->equip[7].item == id) c++;
        if (b->equip[8].item == id) c++;
        if (b->equip[9].item == id) c++;
    }
    return c;
}

// @ 0x489d10
int World::sl_take(int group, int item, int want, char extras)
{
    int total = 0;
    Troop* t = &troops[group];
    int n = t->inv_count();
    for (int i = 0; i < n; i++) {
        if (want <= 0)
            break;
        ItemRef* p = (ItemRef*)((char*)troops + 0xb4) + group * 0xa1 + i;
        if (p->item == item) {
            total += sort_value(p);
            p->item = -1;
            want--;
        }
    }
    if (extras) {
        int i = 0;
        while (i < 10) {
            if (want <= 0)
                break;
            ItemRef* p = (ItemRef*)((char*)troops + 0x4a4) + group * 0xa1 + i;
            if (p->item == item) {
                total += sort_value(p);
                p->item = -1;
                want--;
            }
            i++;
        }
    }
    return total;
}

// @ 0x489e70
int World::sl_roll_party(int lo, int hi)
{
    int found = -1;
    int tries = 0;
    do {
        if (tries >= 1000)
            return found;
        int span = hi - lo;
        int v;
        if (span <= 1)
            v = 0;
        else
            v = rand() % span;
        found = v + lo;
        PartyRec* pi = (PartyRec*)(party_recs + found * 0x138);
        tries++;
        if (!pi->f50_active) {
            found = -1;
            continue;
        }
        if (pi->f64_flag) {
            found = -1;
            continue;
        }
        if (found == -1)
            continue;
    } while (found == -1);
    return found;
}

// @ 0x489ee0
int World::sl_roll_other(int lo, int hi)
{
    int tries = 0;
    int span = hi - lo;
    int v;
    do {
        int r;
        if (span <= 1)
            r = 0;
        else
            r = rand() % span;
        v = r + lo;
        tries++;
        if (v == excluded)
            v = -1;
        if (tries > 100000)
            return 0;
    } while (v == -1);
    return v;
}

// @ 0x489f40
// Party strength factor from the difficulty and option settings, truncated to int (the x87 value is popped by the
// _ftol call). The constants are .rdata literals of this object (0.09, 0.15, 0.2, 1.2, 0.05, 1.05, 100.01, 0.75).
int World::sl_value_factor()
{
    float f = 1.0f;
    if (fb4)
        f = 0.75f;
    f = (1.0f - (float)g_difficulty * 0.09f) * (((g_speed_base - 0.5f) * 0.15f + 1.0f) * f);
    f = f * ((float)(g_int_5aa054 - 2) * 0.15f + 1.0f);
    f = f * ((float)(g_int_5aa058 - 2) * 0.2f + 1.0f);
    if (g_stick_mode == 1)
        f = f * 1.2f;
    return (int)(((float)(g_agent_count - 2) * 0.05f + 1.05f) * f * 100.01f);
}

// @ 0x48a000
void World::sl_update_avg()
{
    float now = (float)g_ticks[0] * 1e-05f;
    float dt = now - avg_prev_time;
    float add = sl_value_factor();
    avg_value = avg_value * avg_weight + add * dt;
    avg_prev_time = now;
    float wsum = dt + avg_weight;
    avg_weight = wsum;
    avg_value = avg_value / wsum;
}

// @ 0x48a080
int World::sl_pick_chain(int leader, char lancer)
{
    int chain[32];
    chain[0] = leader;
    int n = 0;
    if (troops[leader].f50 > 0) {
        do {
            chain[n + 1] = troops[chain[n]].f50;
            n++;
        } while (troops[chain[n]].f50 > 0);
    }
    int cnt = troops[cur_troop].fb0 / 3 + 1;
    int r;
    if (cnt > 1)
        r = rand() % cnt;
    else
        r = 0;
    r++;
    int lo = 1;
    if (lancer)
        lo = 0;
    if (r < lo)
        r = lo;
    if (r > n)
        return chain[n];
    return chain[r];
}

// @ 0x48a150
int World::sl_cell_coord(int party, int cell, int)
{
    char* base = g_cell_table + party * 0xb0;
    int roll = rand() % 0x3dc9;
    int lo = range_lo;
    int hi = range_hi;
    char* c = base + cell * 0x10;
    int span = *(int*)(c + 0x48) - *(int*)(c + 0x44);
    float f = (float)(hi - lo) * ((float)roll * 6.322312e-05f);
    f = (f + lo) * span;
    f = f * 0.01f;
    return *(int*)(c + 0x44) + rnd(&f);
}

// @ 0x48a210
void sl_item_flag_all()
{
    for (int i = 0; i < g_item_count; i++)
        g_items[i].f21c = 1.0f;
}

// @ 0x48a250
void __stdcall sl_item_set_all(float v)
{
    for (int i = 0; i < g_item_count; i++)
        g_items[i].f224 = v;
}

// @ 0x48a290
int World::sl_party_value(int party)
{
    if (party >= 0) {
        g_store0 = 0;
        g_store1 = 0;
        if (sl_query(7, party, cur_party) != 0)
            return g_store0;
        Troop* r = &troops[party];
        if (((__int64)r->flags30 & 0x10) != 0)
            return 0;
        return r->fb0 * r->fb0 / 20 + 25;
    }
    return 0;
}

// @ 0x48a320
int World::sl_party_price(int party)
{
    if (party < 0)
        return 0;
    g_store0 = 0;
    g_store1 = 0;
    if (sl_query(9, party, cur_party))
        return g_store0;
    int v = sl_party_value(party);
    return (int)((float)v * 5.0f + 2.0f);
}

// @ 0x48a380
void World::set_names(int slot, const tstring* a)
{
    ((tstring*)((char*)this + (slot + 0x1a24) * 16))->operator=(*a);
}

// @ 0x48a3a0
void NamedRec::setup(const tstring* a, const tstring* b)
{
    name68 = *a;
    name1c = *b;
    f4c = 0;
    f50 = 0;
    f04 = 0;
}

// @ 0x48a3d0
void NamedRec::setup2(const tstring* a, const tstring* b, const tstring* c, int v)
{
    name68.clear();
    name1c = *a;
    name78 = *b;
    name88 = *c;
    f4c = v;
    f50 = 0;
    f04 = 1;
}

// @ 0x48a420
void Mission::get_frame(rgl::matrix* out)
{
    if (blend_time > 0.0f) {
        float t = (g_ticks[origin] - start) * 1e-5f;
        t = t / blend_time;
        if (blend_n > 0) {
            t = atan((t - 0.5f) * (float)blend_n * 3.1415927f) /
                    (atan((float)blend_n * 1.5707964f) * 2.0f) + 0.5f;
        }
        if (t >= 1.0f) {
            *out = frame_b;
            return;
        }
        float s = 1.0f - t;
        rgl::matrix m;
        // each row is two scaled copies added lane by lane; the original rounds the y and z products to float
        // (stack temporaries, modelled with volatile) while the x lane stays in the x87 register. The 4th lane of
        // r is never written: the copy into the matrix carries a stale stack dword.
        {
            volatile float pay, paz, pby, pbz;
            rgl::vector4 r;
            float pax = frame_a.o.x * s; pay = frame_a.o.y * s; paz = frame_a.o.z * s;
            float pbx = frame_b.o.x * t; pby = frame_b.o.y * t; pbz = frame_b.o.z * t;
            r.x = pbx + pax; r.y = pby + pay; r.z = pbz + paz;
            m.o = r;
        }
        {
            volatile float pay, paz, pby, pbz;
            rgl::vector4 r;
            float pax = frame_a.f.x * s; pay = frame_a.f.y * s; paz = frame_a.f.z * s;
            float pbx = frame_b.f.x * t; pby = frame_b.f.y * t; pbz = frame_b.f.z * t;
            r.x = pbx + pax; r.y = pby + pay; r.z = pbz + paz;
            m.f = r;
        }
        {
            volatile float pay, paz, pby, pbz;
            rgl::vector4 r;
            float pax = frame_a.u.x * s; pay = frame_a.u.y * s; paz = frame_a.u.z * s;
            float pbx = frame_b.u.x * t; pby = frame_b.u.y * t; pbz = frame_b.u.z * t;
            r.x = pbx + pax; r.y = pby + pay; r.z = pbz + paz;
            m.u = r;
        }
        {
            volatile float pay, paz, pby, pbz;
            rgl::vector4 r;
            float pax = frame_a.s.x * s; pay = frame_a.s.y * s; paz = frame_a.s.z * s;
            float pbx = frame_b.s.x * t; pby = frame_b.s.y * t; pbz = frame_b.s.z * t;
            r.x = pbx + pax; r.y = pby + pay; r.z = pbz + paz;
            m.s = r;
        }
        m.orthonormalize();
        *out = m;
        return;
    }
    *out = frame_a;
}

// @ 0x48a740
float Mission::blend_param()
{
    if (blend_time > 0.0f) {
        float t = (g_ticks[origin] - start) * 1e-5f;
        t = t / blend_time;
        if (t >= 1.0f)
            return blend_to;
        return (1.0f - t) * blend_from + t * blend_to;
    }
    return blend_from;
}

// @ 0x48a7d0
float Mission::blend2_param()
{
    if (blend2_time > 0.0f) {
        float t = (g_ticks[origin2] - start2) * 1e-5f;
        t = t / blend2_time;
        if (t >= 1.0f)
            return blend2_to;
        return (1.0f - t) * blend2_from + t * blend2_to;
    }
    return blend2_from;
}

// @ 0x48a860
void Mission::start_clock(int v)
{
    clock_set = 1;
    int d = (int)((float)-v * 100000.0f);
    clock_start = g_ticks[clock_index] - d;
}

// @ 0x48a8b0
int Mission::team_count()
{
    int n = 0;
    if (0 < counts[0][1]) n = 1;
    if (0 < counts[1][1]) n++;
    if (0 < counts[2][1]) n++;
    if (0 < counts[3][1]) n++;
    if (0 < counts[4][1]) n++;
    if (0 < counts[5][1]) n++;
    if (0 < counts[6][1]) n++;
    if (0 < counts[7][1]) n++;
    return n;
}

// @ 0x48a910
int Mission::last_team()
{
    int r = -1;
    if (0 < counts[0][1]) r = 0;
    if (0 < counts[1][1]) r = 1;
    if (0 < counts[2][1]) r = 2;
    if (0 < counts[3][1]) r = 3;
    if (0 < counts[4][1]) r = 4;
    if (0 < counts[5][1]) r = 5;
    if (0 < counts[6][1]) r = 6;
    if (0 < counts[7][1]) r = 7;
    return r;
}

// @ 0x48a990
bool Mission::point_in_rect(const float* p)
{
    return rect_xmin <= p[0] && p[0] < rect_xmax &&
           rect_ymin <= p[1] && p[1] < rect_ymax;
}

// @ 0x48a9e0
void Mission::slot_finish(int i)
{
    CombatRec* r = rec(i);
    r->b[0].m4 = r->b[0].use;
    r->b[1].m4 = r->b[1].use;
    r->b[2].m4 = r->b[2].use;
}

// @ 0x48aa20
void Mission::slot_restore(int i)
{
    CombatRec* r = rec(i);
    r->b[0].use = r->b[0].m4;
    r->b[1].use = r->b[1].m4;
    r->b[2].use = r->b[2].m4;
}

// @ 0x48aa60
void Mission::slot_set(int i, int s)
{
    CombatRec* r = rec(i);
    if (s >= 0 && s < 3) {
        r->b[0].use = 0;
        r->b[1].use = 0;
        r->b[2].use = 0;
        r->b[s].use = 1;
    } else {
        r->b[0].use = 1;
        r->b[1].use = 1;
        r->b[2].use = 1;
    }
}

// @ 0x48aac0
int Mission::active_kind(int i)
{
    CombatRec* r = rec(i);
    for (int k = 0; k < 3; k++) {
        if (r->b[k].use != 0) {
            switch (r->b[k].k8) {
            case 0: return 2;
            case 2: return 1;
            case 1: return 0;
            case 3: return 11;
            default: return -1;
            }
        }
    }
    return -1;
}

// @ 0x48ab30
int Mission::slot_mode(int i)
{
    CombatRec* r = rec(i);
    int v = 0;
    while (v < 3) {
        if (r->b[v].use != 0) {
            v = r->b[v].kc;
            goto sw;
        }
        v++;
    }
    v = -1;
sw:
    switch (v) {
    case 0: return 0;
    case 1: return 1;
    case 2: return 2;
    }
    return -1;
}

// @ 0x48ab90
int Mission::slot_kind_a(int i)
{
    int v = -1;
    for (int k = 0; k < 3; k++) {
        if (rec(i)->b[k].use) {
            v = rec(i)->b[k].k10;
            break;
        }
    }
    switch (v) {
    case 0: return 0;
    case 1: return 1;
    }
    return -1;
}

// @ 0x48abf0
int Mission::slot_kind_b(int i)
{
    int v = -1;
    for (int k = 0; k < 3; k++) {
        if (rec(i)->b[k].use) {
            v = rec(i)->b[k].k14;
            break;
        }
    }
    switch (v) {
    case 0: return 0;
    case 1: return 1;
    }
    return -1;
}

// @ 0x48ac50
bool Mission::has_origin(int i)
{
    bool r = false;
    int a = slots[i].a;
    if (a >= 0 && g_origin_table[a] != 0)
        r = true;
    int b = slots[i].b;
    if (b >= 0 && g_origin_table[b] != 0)
        r = true;
    return r;
}

// @ 0x48ac90
bool Mission::has_free_origin(int i)
{
    bool r = false;
    int a = slots[i].a;
    if (a >= 0 && g_origin_table[a] != 0 && g_origin_state[a] == 0)
        r = true;
    int b = slots[i].b;
    if (b >= 0 && g_origin_table[b] != 0 && g_origin_state[b] == 0)
        r = true;
    return r;
}

// @ 0x48ace0
int Vec68::count()
{
    if (!begin)
        return 0;
    return end - begin;
}

// @ 0x48ad00
int Map10007::lookup(unsigned id)
{
    if (id < 0x2717)
        return tab[id].value;
    int h = id % 0x2717;
    int n = 0;
    if (tab[h].key != id) {
        do {
            if (n >= 1000)
                return -1;
            h = (h + 0x4f) % 0x2717;
            n++;
        } while (tab[h].key != id);
    }
    if (n < 1000)
        return tab[h].value;
    return -1;
}

// @ 0x48ad70
void fill8(int* first, int* last, const int* val)
{
    for (; first != last; first += 2) {
        first[0] = val[0];
        first[1] = val[1];
    }
}

// @ 0x48ada0
void move_backward8(__int64* first, __int64* last, __int64* dest)
{
    int n = (int)(last - first);
    memmove(dest - n, first, n * 8);
}

// @ 0x48add0
void TimerList::update(int flag)
{
    for (int i = 0; i < count; i++) {
        items[i].tick(flag);
        if (flag > 0 && (flag & 1) && g_world->mode == 0x12)
            return;
    }
}


// @ 0x48ae20
MissionUi::MissionUi()
{
    float a = 0.005f;
    float b = 0.024f;
    float c = 0.0f;
    float d = 1.0f;

    clock0 = 0; t0 = g_ticks[0];
    clock1 = 0; t1 = g_ticks[0];
    clock2 = 0; t2 = g_ticks[0];
    clock3 = 0; t3 = g_ticks[0];
    clock4 = 0; t4 = g_ticks[0];
    clock5 = 0; t5 = g_ticks[0];
    // name's tstring constructor runs implicitly (0x42bbd0)
    fE0 = 0; fE4 = 0; fE8 = 0; fEC = 0;
    f100 = 1;
    p154 = 0;
    f104 = 0;
    for (int i = 0; i < 5; i++) {
        tracks[i] = 0;
        b11c[i] = 0;
    }
    f138 = a;
    f140 = b;
    f13c = c;
    color14c = 0xff000000;
    color150 = 0xff000000;
    b11c[0] = 1;
    b11c[2] = 1;
    f164 = 1;
    screen[3] = c;
    f144 = b;
    screen[2] = c;
    periodA = periodB = periodC = periodD = periodE = periodF = -1.0f;
    screen[0] = c;
    fD8 = d;
    f124 = 0;
    fC8 = fCC = fD0 = fD4 = 0;
    f158 = v15c = f160 = 0;
    v168 = f16c = 0;
    b121 = 0;
    b122 = 0;
    c0 = c1 = c2 = c3 = c4 = c5 = 0;
    f148 = 0x11;
    f170 = d;
    screen[1] = c;
    fDC = d;
}

// @ 0x48b090
void MissionUi::set_text_pos(const float* p)
{
    float v[2];
    float o[2];
    if (f148 & 2)
        f148 &= ~2u;
    f140 = p[0];
    f144 = p[1];
    if ((f148 & 0x200) && !(f148 & 0x100)) {
        v[0] = 1.0f;
        v[1] = 0.75f;
        set_xy(v);
    }
    if (f148 & 0x200) {
        get_xy(v);
        float m = screen[0];
        if (v[0] < m)
            m = v[0];
        o[0] = m;
        o[1] = v[1];
        set_xy(o);
        f13c = v[1] - f144 * 0.63f;
        get_xy(o);
    }
}

// @ 0x48b170
char MissionUi::always1() { return 1; }

// @ 0x48b180
void MissionUi::flag2()
{
    b121 = 0;
    f104 |= 2;
}

// @ 0x48b1a0
void MissionUi::flag1()
{
    b121 = 0;
    f104 |= 1;
}

// @ 0x48b1c0
UiA::UiA()
{
    f178 = 0;
    b121 = 0;
    f180 = 0;
    f184 = 0;
}

// @ 0x48b1f0
int UiA::type() { return 1; }

// @ 0x48b200
char UiA::key(unsigned p)
{
    char b = 0;
    if (f184 & 1) {
        if (!(p & 0x10))
            goto tail;
        b = 1;
        if (!(f184 & 2) || v15c == 0) {
            ::play_sound(&g_sound);
            f180 = (f180 == 0);
            set_key_flags(0x10000);
            v15c = (v15c == 0);
        }
    } else {
        if (p & 4)
            goto clear;
        if (p & 0x10) {
            b = 1;
            ::play_sound(&g_sound);
            f180 = 1;
            goto tail;
        }
        if (!(p & 8) || f180 != 1)
            goto tail;
        b = 1;
        set_key_flags(0x10000);
clear:
        f180 = 0;
    }
tail:
    set_key_flags(p);
    refresh();
    return b;
}

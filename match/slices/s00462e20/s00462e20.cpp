// Slice s00462e20: Inv (stock / loot container: random item picker, slot helpers), the placed-object stream
// reader, the 0x5374-byte object constructor, the camera-direction helper and the ArrSet copy ctor / dtor.
// Names provisional.
#include <math.h>
#include "s00462e20.h"

// ---------------------------------------------------------------------------------------------------------
// Inv
// ---------------------------------------------------------------------------------------------------------

// @ 0x462e20
int Inv::pick(int seed, int p3, unsigned must, unsigned mustnot, unsigned type, unsigned t2, unsigned t3, char cheapest)
{
    int best = -1, cheap = -1;
    float bestv = -1.0f, cheapv = 1e9f;
    int n = count();
    int matching = 0;
    int total = 0;
    int i = 0;
    if (0 < n) {
        Slot* pp = slots;
        do {
            if (pp->item >= 0 && matches(i, p3, must, mustnot, type, t2, t3))
                matching++;
            i++;
            pp++;
        } while (i < n);
        total = matching;
        if (matching != 0 && cheapest == 0)
            total = matching + 3;
    }
    ItemKind* items = g_items;
    float ftotal = (float)total;
    float thr = 1.7f / ftotal;
    if (0 < n) {
    for (total = 0; total < n; total++) {
        Slot* p = &slots[total];
        if (p->item >= 0) {
                ItemKind* k = &items[p->item];
                if (fits(p, p3)) {
                    unsigned tf = k->type_flags;
                    if ((tf & must) == must && (mustnot & tf) == 0 &&
                        (type == 0xffffffff || (tf & 0xff) == type || (tf & 0xff) == t2 || (tf & 0xff) == t3)) {
                        int idx = p->item;
                        unsigned sd = idx * 3 + 7 + seed * 0x9d;
                        float v = (float)items[idx].value;
                        g_rand_seed = sd;
                        if (((flags & 0x10) != 0 ||
                             (g_rand_seed = (g_rand_seed * 0x1c13 - 0xd098556) * 0x1105,
                              (float)(g_rand_seed % 0x3dc9) * 6.322312e-05f < thr)) && v > bestv) {
                            best = total;
                            bestv = v;
                        }
                        if (cheapest != 0 && v < cheapv) {
                            cheap = total;
                            cheapv = v;
                        }
                    }
                }
        }
    }
    if (best != -1)
        return best;
    }
    return cheap;
}



// @ 0x463030
int Inv::find_free()
{
    int r = -1;
    bool found = false;
    int n = count();
    int i = 0;
    Slot* p = slots;
    do {
        if (i >= n)
            return r;
        if (p->item == -1) {
            r = i;
            found = true;
        }
        i++;
        p++;
    } while (!found);
    return r;
}

// @ 0x463070
bool Inv::add(Slot* s)
{
    int r = -1;
    bool found = false;
    int n = count();
    int i = 0;
    Slot* p = slots;
    do {
        if (i >= n)
            break;
        if (p->item == -1) {
            r = i;
            found = true;
        }
        i++;
        p++;
    } while (!found);
    if (r >= 0) {
        slots[r].item = s->item;
        slots[r].modifier = s->modifier;
        return true;
    }
    return false;
}

// @ 0x4630d0
void Inv::unequip(int i)
{
    Slot* p = &equip[i];
    if (p->item >= 0) {
        if (add(p))
            p->item = -1;
    }
}

// @ 0x463100
bool Inv::equip_best(int seed, int equip_slot, unsigned must, unsigned mustnot, unsigned type, unsigned t2, unsigned t3, char cheapest)
{
    int r = pick(seed, equip_slot, must, mustnot, type, t2, t3, cheapest);
    if (r >= 0) {
        Slot* d = &equip[equip_slot];
        if (d->item >= 0) {
            if (add(d))
                d->item = -1;
        }
        d->item = slots[r].item;
        d->modifier = slots[r].modifier;
        slots[r].item = -1;
    }
    return r >= 0;
}

// @ 0x463190
int RecHolder::first_flagged()
{
    int r = 0;
    int i = 0;
    if (0 < nrecs) {
        Rec* p = recs;
        do {
            if ((p->flags & 1) != 0)
                return r;
            r++;
            i++;
            p++;
        } while (i < nrecs);
    }
    return r;
}

// ---------------------------------------------------------------------------------------------------------
// Placed::read
// ---------------------------------------------------------------------------------------------------------

// three floats of one row, through a pointer
static inline void r3(stream_base& s, float* p) { s.r_float(&p[0]).r_float(&p[1]).r_float(&p[2]); }

// @ 0x4631c0
void Placed::read(stream_base* s, int version)
{
    int dummy;
    s->r_int(&f188);
    s->r_int(&f18c);
    s->r_int(&dummy);
    rgl::matrix* m = &m18;
    r3(r_rotation(*s, m), &m->o.x);
    m98 = *m;
    s->r_tstr(&name);
    f1bc = 100.0f;
    if (f188 == 0) {
        if (name.len < 4 || name.buf[0] != 's' || name.buf[1] != 'p' || name.buf[2] != 'r' || name.buf[3] != '_') {
            tstring t("spr_");
            t.append(name);
            name = t;
        }
        int idx = find_in_table78(&name);
        if (idx != -1) {
            f18c = idx;
        } else {
            if (f18c < 0 || f18c >= g_tbl78_count) {
                f18c = 0;
                name = g_tbl78[0].name;
            } else {
                name = g_tbl78[f18c].name;
            }
        }
        f1bc = (float)g_tbl78[f18c].f14;
    }
    s->r_int(&f1a0);
    if (version >= 2) {
        s->r_int(&f1a4);
        if (version >= 3)
            r3(*s, &f158);
    }
}

// ---------------------------------------------------------------------------------------------------------
// ArrSet
// ---------------------------------------------------------------------------------------------------------

// @ 0x4636d0
ArrSet::ArrSet(const ArrSet& o)
{
    name = o.name;
    for (int i = 0; i < 5; i++) {
        cnt[i] = o.cnt[i];
        arr[i] = 0;
        if (o.cnt[i] > 0) {
            arr[i] = new int[o.cnt[i]];
            for (int j = 0; j < o.cnt[i]; j++)
                arr[i][j] = o.arr[i][j];
        }
    }
}

// @ 0x463760
ArrSet::~ArrSet()
{
    for (int i = 0; i < 5; i++) {
        if (arr[i])
            delete[] arr[i];
    }
}

// ---------------------------------------------------------------------------------------------------------
// ViewState
// ---------------------------------------------------------------------------------------------------------
#define W(o)   (*(int*)((char*)this + (o)))
#define FL(o)  (*(float*)((char*)this + (o)))
#define VEC(o) (*(rgl::vector4*)((char*)this + (o)))
#define T64(o) (*(__int64*)((char*)this + (o)))

// @ 0x463370
void ViewState::init()
{
    W(0) = 0;
    W(0x517c) = 0;
    W(0x50d0) = 0;
    W(0x50d4) = 0;
    W(0x50cc) = 0;
    W(0x34) = 0;
    W(0x38) = 0;
    W(0x50) = 0;
    W(0x54) = 0;
    VEC(0x3c) = rgl::vector4(0.0f, 1.0f, 0.0f);
    FL(0x68) = 0.1f;
    FL(0x30) = 1.0f;
    W(0x24) = -1;
    T64(0x60) = g_ticks[W(0x58)];
    T64(0xb0) = g_ticks[W(0xa8)];
    T64(0xa0) = g_ticks[W(0x98)];
    T64(0x5120) = g_ticks[W(0x5118)];
    T64(0x5130) = g_ticks[W(0x5128)];
    FL(0xb8) = 0.1f;
    VEC(4) = rgl::vector4(-3.0f, 0.0f, 0.0f);
    W(0x6c) = 0;
    W(0x88) = 0;
    W(0x70) = 0;
    FL(0x94) = 1.0f;
    W(0x8c) = 0;
    W(0x90) = -100000;
    W(0xbc) = 0;
    W(0x50e0) = 0;
    W(0x50e4) = 0;
    VEC(0x14) = rgl::vector4(0.0f, 0.0f, 0.0f);
    VEC(0x50f8) = rgl::vector4(0.0f, 0.0f, 0.0f);
    W(0xc0) = -1;
    W(0xc4) = -1;
    W(0xc8) = -1;
    VEC(0x5154) = rgl::vector4(0.0f, 1.0f, 0.0f);
    W(0x5108) = 0;
    W(0x510c) = 0;
    W(0x5138) = 0;
    W(0x513c) = 0;
    W(0x5140) = 0;
    W(0x5144) = 0;
    W(0x514c) = 0;
    W(0x517c) = 0;
    W(0x5164) = 0;
    FL(0x4c) = 0.5f;
    FL(0x5318) = 1.0f;
    W(0x535c) = 0;
    rgl::matrix tmp;
    *(rgl::matrix*)((char*)this + 0x531c) = *identity_pos(&tmp);
    W(0x5360) = 0;
    W(0x5364) = 0;
    W(0x2c) = 0;
    W(0x5148) = 0;
    W(0x5314) = 0;
    W(0x5110) = 0;
    W(0x5114) = 0;
    W(0x5368) = -1;
    FL(0x5370) = 15.0f;
}

// @ 0x463610
rgl::vector4 ViewState::get_dir(int a, int b)
{
    rgl::vector4 r;
    float ang = FL(0x18) * 0.017453292f;
    float c = (float)cos(ang), s = (float)sin(ang);
    float x1 = c * -1.0f - s * 0.0f;
    r.z = c * 0.0f - s * 1.0f;
    double q = (float)(a - 12) * -0.017453292f * 15.0;
    float cb = cos(q);
    r.x = cb * 0.0f - sin(q) * x1;
    r.y = sin(q) * 0.0f + cb * x1;
    return r;
}

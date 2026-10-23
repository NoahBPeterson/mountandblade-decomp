//-----------------------------------------------------------------------------
// File: troop_inv.cpp  (0x462ac0..0x4636d0)
//
// Desc: the troop record's inventory: the item-to-slot mapping and skill-slot values, the "may this item go into
//       slot N" tests, the weighted random item picker, the free-slot search / add / unequip / equip helpers, the
//       party's first-prisoner search, the placed-object stream reader, the 0x5374-byte view-state constructor and
//       the camera-direction helper. Names provisional. Functions in ascending address order (cl's bottom-up
//       emission). Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include <math.h>
#include "../tstring.h"
#include "../../rgl/rgl_math.h"
#include "../../rgl/stream.h"
#include "./ui_common.h"
#include "./item_ref.h"
#include "./troop_inv.h"
#include "./party.h"
#include "../view_state.h"      // (listed directly: the resolution audit scans only the source's own includes)

#define MB_NOINLINE __declspec(noinline)    // (tools/difftest/slice.py reads the name before a '__declspec(' as the function name)

// register convention in the original: item in eax, outputs in ecx / edx / stack
// @ 0x462ac0
static MB_NOINLINE void item_slot_source(int item, int* f58_idx, int* nib_idx, int* stat_idx)
{
    *f58_idx = -1;
    *nib_idx = -1;
    *stat_idx = -1;
    if (item >= 0) {
        ItemKind* it = &g_items[item];
        unsigned t = *(volatile unsigned*)&it->type_flags & 0xff;     // (volatile read: keeps the original's add + load pair)
        if (t != itp_type_one_handed_wpn && t != itp_type_two_handed_wpn && t != itp_type_polearm && t != itp_type_crossbow) {
            if (t == itp_type_horse) { *nib_idx = 0x18; return; }
            if (t == itp_type_bow) { *nib_idx = 0x21; return; }
            if (t == itp_type_thrown) { *nib_idx = 0x22; return; }
            if (t == itp_type_shield) { *nib_idx = 0x1a; return; }
            if (t != itp_type_head_armor && t != itp_type_body_armor && t != itp_type_foot_armor && t != itp_type_hand_armor)
                return;
        }
        *f58_idx = 0;
    }
}

// @ 0x462b60
int Troop::inv_count()
{
    if (!(flags30 & 0x10)) return 0x5a;
    switch (nibble(12)) {
    case 0: return 0x1e;
    case 1: return 0x24;
    case 2: return 0x2a;
    case 3: return 0x30;
    case 4: return 0x36;
    case 5: return 0x3c;
    case 6: return 0x42;
    case 7: return 0x48;
    case 8: return 0x4e;
    case 9: return 0x54;
    case 10: return 0x60;
    default: return 0x60;
    }
}

// @ 0x462c00
int Troop::slot_value(int item)
{
    int result = 0;
    int nib_idx, stat_idx;
    item_slot_source(item, &item, &nib_idx, &stat_idx);
    if (item >= 0) result = f58[item];
    else if (nib_idx >= 0) result = nibble(nib_idx);
    else if (stat_idx >= 0) result = (int)stat[stat_idx];
    return result;
}

// @ 0x462c80
bool Troop::fits_slot(ItemRef* ref, int slot)
{
    int item = ref->item;
    unsigned tf = g_items[item].type_flags;
    ItemKind* it = &g_items[item];
    unsigned t = tf & 0xff;
    bool ok = false;
    if (flags30 & 0x10) {
        int need = slot_value(item);
        if (ref->stat_1ec() > need) { ok = false; goto done; }
    }
    if (slot < 0 || slot > 3) {
        if (slot == 4) { if (t != itp_type_head_armor) goto done; }
        else if (slot == 5) { if (t != itp_type_body_armor) goto done; }
        else if (slot == 6) { if (t != itp_type_foot_armor) goto done; }
        else if (slot == 7) { if (t != itp_type_hand_armor) goto done; }
        else {
            if (slot != 8) {
                if (slot != 9) goto done;
                if (t == itp_type_goods) {
                    if (tf & itp_food) { ok = true; goto done; }
                    goto done;
                }
            }
            if (t != itp_type_horse) goto done;
        }
    } else if (t != itp_type_bolts && t != itp_type_arrows && t != itp_type_bullets) {
        if (!it->is_ranged() && t != itp_type_shield) {
            if (it->is_melee()) { ok = true; goto done; }
            goto done;
        }
    }
    ok = true;
done:
    return ok;
}

// @ 0x462da0
bool Troop::item_matches(int idx, int slot, unsigned need, unsigned forbid, int t1, int t2, int t3)
{
    ItemKind* it = &g_items[inv[idx].item];
    if (fits_slot(&inv[idx], slot)) {
        unsigned tf = it->type_flags;
        if ((tf & need) == need && !(forbid & tf)) {
            if (t1 == -1 || (int)(tf & 0xff) == t1 || (int)(tf & 0xff) == t2 || (int)(tf & 0xff) == t3)
                return true;
        }
    }
    return false;
}

// @ 0x462e20
int Troop::pick(int seed, int p3, unsigned must, unsigned mustnot, unsigned type, unsigned t2, unsigned t3, char cheapest)
{
    int best = -1, cheap = -1;
    float bestv = -1.0f, cheapv = 1e9f;
    int n = inv_count();
    int matching = 0;
    int total = 0;
    int i = 0;
    if (0 < n) {
        ItemRef* pp = inv;
        do {
            if (pp->item >= 0 && item_matches(i, p3, must, mustnot, type, t2, t3))
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
        ItemRef* p = &inv[total];
        if (p->item >= 0) {
                ItemKind* k = &items[p->item];
                if (fits_slot(p, p3)) {
                    unsigned tf = k->type_flags;
                    if ((tf & must) == must && (mustnot & tf) == 0 &&
                        (type == 0xffffffff || (tf & 0xff) == type || (tf & 0xff) == t2 || (tf & 0xff) == t3)) {
                        int idx = p->item;
                        unsigned sd = idx * 3 + 7 + seed * 0x9d;
                        float v = (float)items[idx].value;
                        g_rand_seed = sd;
                        if (((flags30 & 0x10) != 0 ||
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
int Troop::find_free()
{
    int r = -1;
    bool found = false;
    int n = inv_count();
    int i = 0;
    ItemRef* p = inv;
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
bool Troop::add(ItemRef* s)
{
    int r = -1;
    bool found = false;
    int n = inv_count();
    int i = 0;
    ItemRef* p = inv;
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
        inv[r].item = s->item;
        inv[r].m4 = s->m4;
        return true;
    }
    return false;
}

// @ 0x4630d0
void Troop::unequip(int i)
{
    ItemRef* p = &equip[i];
    if (p->item >= 0) {
        if (add(p))
            p->item = -1;
    }
}

// @ 0x463100
bool Troop::equip_best(int seed, int equip_slot, unsigned must, unsigned mustnot, unsigned type, unsigned t2, unsigned t3, char cheapest)
{
    int r = pick(seed, equip_slot, must, mustnot, type, t2, t3, cheapest);
    if (r >= 0) {
        ItemRef* d = &equip[equip_slot];
        if (d->item >= 0) {
            if (add(d))
                d->item = -1;
        }
        d->item = inv[r].item;
        d->m4 = inv[r].m4;
        inv[r].item = -1;
    }
    return r >= 0;
}

// @ 0x463190
int Party::first_flagged()
{
    int r = 0;
    int i = 0;
    if (0 < nstacks) {
        Stack20* p = stacks;
        do {
            if ((p->flags & 1) != 0)
                return r;
            r++;
            i++;
            p++;
        } while (i < nstacks);
    }
    return r;
}



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
    s->r_tstr((rgl::string*)&name);
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

#define W(o)   (*(int*)((char*)this + (o)))
#define FL(o)  (*(float*)((char*)this + (o)))
#define VEC(o) (*(rgl::vector4*)((char*)this + (o)))
#define T64(o) (*(__int64*)((char*)this + (o)))

// @ 0x463370
void ViewBody::init()
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
    *(rgl::matrix*)((char*)this + 0x531c) = *rgl::identity_pos(&tmp);
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
rgl::vector4 ViewBody::get_dir(int a, int b)
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

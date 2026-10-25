//-----------------------------------------------------------------------------
// File: stat_panel_x.cpp  (0x48be00..0x48c330)
//
// Desc: the troop record's inventory compaction (ItemRef::compact), its attribute and skill setters (the character
//       sheet's points), the inventory sort, and the "Event" placeholder object. Names provisional. Functions in
//       ascending address order (cl's bottom-up emission). Flags: /O2 /MT /Gy /TP.
//
// Boundary: class changes only (UiE | ItemRef at 0x48be00, Troop | Party at 0x48c330); no forward call in or out.
//-----------------------------------------------------------------------------
#include <math.h>
#include <stdlib.h>
#include "../tstring.h"
#include "../ui/ui_common.h"
#include "../ui/item_ref.h"
#include "../skill_cap.h"
#include "./stat_panel_x.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../item_ref.h"
#include "../../include/item_kind.h"
#include "../../include/troop.h"

// round to nearest, as cl inlines it (0x432540 is the out-of-line copy in another object)
static __forceinline int rnd(const float* p)
{
    float fl = floorf(*p);
    int i = (int)fl;
    return (*p - fl <= 0.5f) ? i : i + 1;
}

// @ 0x48be00
void ItemRef::compact(int n)
{
    if (n <= 0)
        n = 0x60;
    int i = 0;
    while (i < n) {
        if (this[i].item == -1)
            break;
        i++;
    }
    int j = i + 1;
    if (j < n) {
        ItemRef* d = &this[i];
        while (j < n) {
            if (this[j].item != -1) {
                if (j >= n)
                    break;
                d->item = this[j].item;
                d->m4 = this[j].m4;
                i++;
                this[j].item = -1;
                d++;
            }
            j++;
        }
    }
    sort_sel(i);
}

// Views of members defined in this file as an object that does not know them (an undefined declaration): the original
// calls set_attr (from add_attr) and compact (from sort_inv) with `this` kept in a callee-saved register, as for a callee
// outside the translation unit; a same-file callee lets cl assume its register use and inline part of it.
#include "../troop_ext.h"      // TroopExt: the one definition (falloff 0x4375b0, set_attr 0x48be70)
struct ItemRefExt { void compact(int n); };               // 0x48be00

// the attribute words (+0x58) and the packed skill words (+0x68) as the setters index them
#define ATTR(t) ((int*)&(t)->f58[0])
#define F7A(t) (((unsigned char*)(t))[0x7a])

// @ 0x48be70
void Troop::set_attr(int idx, int val)
{
    if (val < 0)
        val = 0;
    else if (val > 100)
        val = 100;
    ATTR(this)[idx] = val;
    int v = F7A(this) & 0xf;
    int x = v;
    int base = ATTR(this)[0];
    int m = g_skill_tbl[36].cap;
    if (v < 0)
        x = 0;
    else if (v > m)
        x = m;
    if ((float)(base + 0x23 + x * 2) < hp) {
        if (v < 0)
            v = 0;
        else if (v > m)
            v = m;
        hp = (float)(base + 0x23 + v * 2);
    }
}

// @ 0x48bef0
void Troop::add_attr(int idx, int delta)
{
    int step = 1;
    if (delta < 0) {
        delta = -delta;
        step = -1;
    }
    if (idx < 0 || idx >= 4 || delta <= 0)
        return;
    int n = delta;
    do {
        int v = ATTR(this)[idx] + step;
        if (v >= 0 && v < 0x40) {
            if (idx == 2) {
                fa4 += step;
            } else if (idx == 0) {
                if (step == 1)
                    hp += 1.0f;
                else if (step == -1)
                    hp -= 1.0f;
            }
            int sv = ATTR(this)[idx] + step;
            ((TroopExt*)this)->set_attr(idx, sv);
        }
    } while (--n);
}

// @ 0x48bf90
void Troop::set_skill(int idx, int val)
{
    if (val < 0)
        val = 0;
    else if (val > g_skill_tbl[idx].cap)
        val = g_skill_tbl[idx].cap;
    int word = idx / 8;
    int shift = (idx % 8) * 4;
    int cur = ((unsigned)nib[word] >> shift) & 0xf;
    if (cur < 0)
        cur = 0;
    else if (cur > g_skill_tbl[idx].cap)
        cur = g_skill_tbl[idx].cap;
    if (val > cur && idx == 0x24)
        hp += (float)((val - cur) * 2);
    nib[word] = (nib[word] & ~(0xf << shift)) | ((val & 0xf) << shift);
    int v = F7A(this) & 0xf;
    int m;
    int base = ATTR(this)[0];
    m = g_skill_tbl[36].cap;
    int x = v;
    if (v < 0)
        x = 0;
    else if (v > m)
        x = m;
    if ((float)(base + 0x23 + x * 2) < hp) {
        if (v < 0)
            v = 0;
        else if (v > m)
            v = m;
        hp = (float)(base + 0x23 + v * 2);
    }
}

// @ 0x48c090
void Troop::add_skill(int id, int delta)
{
    if (id < 0 || id >= 0x2a)
        return;
    set_skill(id, nibble(id) + delta);
}

// @ 0x48c0c0
void Troop::sort_inv()
{
    int count = inv_count();
    ((ItemRefExt*)&inv[0])->compact(count);
    if (count > 0) {
        int passes = count;
        int limit = count - 1;
        do {
            if (limit > 0) {
                int* p = (int*)&inv[0];
                int i = limit;
                do {
                    int l, r;
                    if (*p >= 0) {
                        float v = (float)g_items[*p].value * g_sort_val[*((char*)p + 7) * 5].v;
                        float fl = (float)floor(v);
                        int n = (int)fl;
                        if (!(v - fl <= 0.5f))
                            n++;
                        l = n;
                    } else {
                        l = 0;
                    }
                    if (p[2] >= 0) {
                        float v = (float)g_items[p[2]].value * g_sort_val[*((char*)p + 0xf) * 5].v;
                        float fl = (float)floor(v);
                        int n = (int)fl;
                        if (!(v - fl <= 0.5f))
                            n++;
                        r = n;
                    } else {
                        r = 0;
                    }
                    if (l < r || (l == r && p[0] > p[2])) {
                        int t = p[0]; p[0] = p[2]; p[2] = t;
                        t = p[1]; p[1] = p[3]; p[3] = t;
                    }
                    p += 2;
                    i--;
                } while (i != 0);
            }
            passes--;
        } while (passes != 0);
    }
}

// @ 0x48c210
Event::Event()
{
    f60 = 1;
    f68 = ((int*)&g_ticks[1])[0];
    f6c = ((int*)&g_ticks[1])[1];
    f80 = 0;
    f88 = ((int*)&g_ticks[0])[0];
    f8c = ((int*)&g_ticks[0])[1];
}

// @ 0x48c260
void Event::init()
{
    f10 = 0;
    name.assign("An Event");
    f24 = 0;
    f28 = 0.0f;
    f50 = 0;
    float s = 0.0f;
    float c = 1.0f;
    f34 = s;
    f38 = c;
    float a = (float)(rand() % 15817) * 0.00039724255f;
    float ca = cos(a);
    float sa = sin(a);
    f38 = sa * f34 + ca * f38;
    f34 = ca * s - sa * c;
    f40 = 0.3f;
    f44 = -10000.0f;
    f90 = -1;
}

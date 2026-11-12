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
#include "../../include/clock_table.h"

// round to nearest, as cl inlines it (0x432540 is the out-of-line copy in another object)
static __forceinline int rnd(const float* p)
{
    float fl = floorf(*p);
    int i = (int)fl;
    return (*p - fl <= 0.5f) ? i : i + 1;
}

// ItemRef::compact (0x48be00) and Troop::set_attr (0x48be70) are defined further down, after their callers sort_inv / add_attr:
// the original calls both with `this` kept in a callee-saved register, as for a callee outside the translation unit, and cl
// assumes the register use of a callee defined earlier in the file (and inlines part of it). Declared by item_ref.h / troop.h.

// the original's add_attr saw only a declaration of set_attr (a different TU), so cl keeps `this` in a
// callee-saved register across the call; a stand-in class reproduces that (mapped in symbols/slices/stat_panel_x.txt)
struct TroopX { void set_attr(int idx, int val); };

// the attribute words (+0x58) and the packed skill words (+0x68) as the setters index them
#define ATTR(t) ((int*)&(t)->f58[0])
#define F7A(t) (((unsigned char*)(t))[0x7a])

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
        if (ATTR(this)[idx] + step >= 0 && ATTR(this)[idx] + step < 0x40) {   // no local: the sum is a CSE temporary
            if (idx == 2) {
                fa4 += step;
            } else if (idx == 0) {
                if (step == 1)
                    hp += 1.0f;
                else if (step == -1)
                    hp -= 1.0f;
            }
            TroopX* s = (TroopX*)this;
            s->set_attr(idx, ATTR(this)[idx] + step);
        }
    } while (--n);
}

static __forceinline int clamp_rank(int x, const int& hi) { if (x < 0) x = 0; else if (x > hi) x = hi; return x; }
// @ 0x48be70
void Troop::set_attr(int idx, int val)
{
    if (val < 0)
        val = 0;
    else if (val > 100)
        val = 100;
    ATTR(this)[idx] = val;
    int base = ATTR(this)[0];
    const int& m = g_skill_tbl[36].max_rank;
    if ((float)(base + 0x23 + clamp_rank(F7A(this) & 0xf, m) * 2) < hp)
        hp = (float)(base + 0x23 + clamp_rank(F7A(this) & 0xf, m) * 2);
}

// the 36th skill's value (nibble at +0x7a), clamped to its rank limit, and the hit points it gives
static inline int skill36(Troop* t)
{
    int v = F7A(t) & 0xf;
    if (v < 0)
        v = 0;
    else if (v > g_skill_tbl[36].max_rank)
        v = g_skill_tbl[36].max_rank;
    return v;
}
static inline int max_hp(Troop* t) { int base = ATTR(t)[0]; return base + 0x23 + skill36(t) * 2; }

// @ 0x48bf90
void Troop::set_skill(int idx, int val)
{
    if (val < 0)
        val = 0;
    else if (val > g_skill_tbl[idx].max_rank)
        val = g_skill_tbl[idx].max_rank;
    int word = idx / 8;
    int shift = (idx % 8) * 4;
    int cur = ((unsigned)nib[word] >> shift) & 0xf;
    if (cur < 0)
        cur = 0;
    else if (cur > g_skill_tbl[idx].max_rank)
        cur = g_skill_tbl[idx].max_rank;
    if (val > cur && idx == 0x24)
        hp += (float)((val - cur) * 2);
    nib[word] = (nib[word] & ~(0xf << shift)) | ((val & 0xf) << shift);
    if ((float)max_hp(this) < hp)
        hp = (float)max_hp(this);
}

// @ 0x48c090
void Troop::add_skill(int id, int delta)
{
    if (id < 0 || id >= 0x2a)
        return;
    set_skill(id, nibble(id) + delta);
}

// sort_value 0x488f80 and its round-to-nearest, as they inline here
static inline int rnd_val(const float* p)
{
    float fl = (float)floor(*p);
    int i = (int)fl;
    if (*p - fl <= 0.5f)
        return i;
    return i + 1;
}
static inline int sort_val(ItemRef* e)
{
    int idx = e->item;
    if (idx >= 0) {
        float v = (float)g_items[idx].value;
        int mod = (signed char)(e->m4 >> 24);     // = e->b.mod (the union's top byte), read from the word: TroopInv::sort_inv 0x48c0c0 is byte-exact only this way (68 aligned with b.mod)
        float x = v * g_sort_val[mod * 5].v;
        return rnd_val(&x);
    }
    return 0;
}

// @ 0x48c0c0
void Troop::sort_inv()
{
    int count = inv_count();
    inv[0].compact(count);
    if (count > 0) {
        int limit = count - 1;
        int passes = count;
        do {
            if (limit > 0) {
                ItemRef* p = &inv[0];
                int i = limit;
                do {
                    int l = sort_val(&p[0]);
                    int r = sort_val(&p[1]);
                    if (l < r || (l == r && p[0].item > p[1].item)) {
                        int t = p[0].item; p[0].item = p[1].item; p[1].item = t;
                        t = p[0].m4; p[0].m4 = p[1].m4; p[1].m4 = t;
                    }
                    p++;
                    i--;
                } while (i != 0);
            }
            passes--;
        } while (passes != 0);
    }
}

// @ 0x48be00
void ItemRef::compact(int n)
{
    if (n <= 0)
        n = 0x60;
    int i = 0;
    for (; i < n; i++) {
        if (this[i].item == -1)
            break;
    }
    int j = i + 1;
    if (j < n) {
        ItemRef* d = &this[i];
        for (; j < n; j++) {
            if (this[j].item != -1) {
                if (j >= n)
                    break;
                d->item = this[j].item;
                d->m4 = this[j].m4;
                i++;
                this[j].item = -1;
                d++;
            }
        }
    }
    sort_sel(i);
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

// The event's position (+0x24) and its direction (+0x34) are 2D vectors whose constructor takes its components by
// const reference: each argument is a stack temporary that the member copy reads back (mov [esp+4],0; mov eax,[esp+4]).
// The direction is then turned by a random angle through the vector's own rotate (it reads the stored components).
struct EvVec2 {
    float x, y;
    EvVec2() {}
    EvVec2(const float& a, const float& b) : x(a), y(b) {}
    void rotate(float a)
    {
        float c = cos(a);
        float s = sin(a);
        float nx = c * x - s * y;
        y = c * y + s * x;
        x = nx;
    }
};

// @ 0x48c260
void Event::slot1()
{
    f10 = 0;
    name.assign("An Event");
    f50 = 0;
    *(EvVec2*)&f24 = EvVec2(0.0f, 0.0f);
    *(EvVec2*)&f34 = EvVec2(0.0f, 1.0f);
    ((EvVec2*)&f34)->rotate((float)(rand() % 15817) * 0.00039724255f);
    f40 = 0.3f;
    f44 = -10000.0f;
    f90 = -1;
}

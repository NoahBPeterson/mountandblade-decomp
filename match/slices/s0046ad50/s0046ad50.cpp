// Slice s0046ad50: terrain triangle/vertex normals, an agent's item-slot resolver, an agent ray setup, and
// VC7.1 <memory>/<algorithm> instantiations (_Uninit_fill_n, copy, fill) for 0x24/0x18/0x64/0x34/0x38/0x1c8-byte
// records. Names provisional.
#include <string.h>
#include <stddef.h>
#include "s0046ad50.h"

// ---- 0x46ad50: per-triangle normals, summed per vertex, normalised ------------------------------------------
// @ 0x46ad50
void terrain_obj::compute_normals()
{
    int x, y, flip;
    vector4 tri[3];
    int ab[6];
    vector4 n;
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            for (flip = 0; flip < 2; flip++) {
                triangle_corners(x, y, flip, &ab[0], &ab[2], &ab[4]);
                tri[0] = at(ab[0], ab[1]).pos;
                tri[1] = at(ab[2], ab[3]).pos;
                tri[2] = at(ab[4], ab[5]).pos;
                quad(x, y).nrm[flip] = *tri_normal(&n, tri);
            }
        }
    }
    for (y = 0; y <= h; y++) {
        n.x = 0.0f; n.y = 0.0f; n.z = 0.0f;
        for (x = 0; x <= w; x++)
            at(x, y).nrm = n;
    }
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            for (flip = 0; flip < 2; flip++) {
                triangle_corners(x, y, flip, &ab[0], &ab[2], &ab[4]);
                const vector4& q = quad(x, y).nrm[flip];
                vector4* v;
                v = &at(ab[0], ab[1]).nrm; v->x = q.x + v->x; v->y = q.y + v->y; v->z = q.z + v->z;
                v = &at(ab[2], ab[3]).nrm; v->x = q.x + v->x; v->y = q.y + v->y; v->z = q.z + v->z;
                v = &at(ab[4], ab[5]).nrm; v->x = q.x + v->x; v->y = q.y + v->y; v->z = q.z + v->z;
            }
        }
    }
    for (y = 0; y <= h; y++) {
        for (x = 0; x <= w; x++) {
            vector4& v = at(x, y).nrm;
            float len = v.length();
            if (len > 1e-10f) {
                float inv = 1.0f / len;
                v.x *= inv; v.y *= inv; v.z *= inv;
            } else {
                v.x = 0.0f; v.y = 1.0f; v.z = 0.0f;
            }
        }
    }
}

// ---- 0x46b190: pick the item of a troop's slot, or the first of a candidate list that fits ---------------------
// @ 0x46b190
__int64 __cdecl pick_slot_item(int troop, int slot, int mask, int n, int* cand, char remove, char* found)
{
    *found = 0;
    if (((__int64)mask & g_bit[slot]) == 0 &&
        ((mask & 0x200) == 0 || (g_bit[slot] & 0x10) == 0 ||
         g_items[g_world->troops[troop].equip[slot].item].type_flags >= 0)) {
        *found = 1;
        return *(__int64*)&g_world->troops[troop].equip[slot];
    }
    int i = 0;
    ItemRef res;
    res.item = -1;
    res.m4 = 0;
    do {
        if (n <= i) break;
        int v = cand[i];
        ItemRef r;
        r.m4 = 0;
        if (v >= 0) {
            r.item = v;
            if (g_world->troops[troop].fits_slot(&r, slot)) {
                res.item = v;
                res.m4 = 0;
                res.reduce(0);
                if (remove) cand[i] = -1;
            }
        }
        i++;
    } while (res.item == -1);
    return *(__int64*)&res;
}

// ---- 0x46b2d0: resolve the ten equipment slots of a live agent ------------------------------------------------
struct AgentEq {
    char pad0[0x10];
    int f10;                    // +0x10 (nonzero: no equipment, every slot -1)
    char pad14[0x88 - 0x14];
    int troop;                  // +0x88
    char pad8c[0x1d0 - 0x8c];
    ItemRef sel[10];            // +0x1d0 chosen (item, modifier) per slot
    char found[10];             // +0x220
    char pad22a[0x56f0 - 0x22a];
    int mask;                   // +0x56f0 excluded-slot mask
    int cand[8];                // +0x56f4 candidate items
    int ncand;                  // +0x5714
};
// @ 0x46b2d0
void __stdcall AgentEq_pick_slots(AgentEq* a)
{
    if (a->f10 == 0) {
        int cand[8];
        for (int j = 0; j < a->ncand; j++)
            cand[j] = a->cand[j];
        for (int i = 0; i < 10; i++)
            *(__int64*)&a->sel[i] = pick_slot_item(a->troop, i, a->mask, a->ncand, cand, 1, &a->found[i]);
        return;
    }
    for (int i = 0; i < 10; i++)
        a->sel[i].item = -1;
}

// ---- 0x46b410: line-of-fire segment of an agent -----------------------------------------------------------------
struct FireAgent {
    char pad0[0x20];
    vector4 org;                // +0x20
    vector4 pos;                // +0x30
    vector4 dir;                // +0x40 (x, y, z)
    char pad50[0x88 - 0x50];
    int troop;                  // +0x88
    void ray(struct FireRay* r);    // 0x46b410
};
struct FireRay {
    char pad0[0x20];
    vector4 from;               // +0x20
    vector4 to;                 // +0x30
    char pad40[0x60 - 0x40];
    float width;                // +0x60
};
static inline vector4 vscale(const vector4& v, float f) { return vector4(v.x * f, v.y * f, v.z * f); }
static inline vector4 vadd(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
// @ 0x46b410
void FireAgent::ray(FireRay* r)
{
    r->width = 0.002f;
    r->from = pos;
    float s;
    if (troop >= 0) s = (float)g_items[troop].f20c * 0.01f;
    else s = 0.8f;
    vector4 d = vscale(dir, s);
    r->to = vadd(d, org);
}

// ---- 0x46b3a0: std::fill over 0x38-byte records (tstring name + ten ints) ----------------------------------------
struct elem56 { tstring name; int a[10]; };
// @ 0x46b3a0
void __cdecl fill56(elem56* f, elem56* l, const elem56& v) { for (; f != l; ++f) *f = v; }

// ---- 0x46b4c0: std::copy over 0x1c8-byte polymorphic records (operator= 0x437d60) ------------------------------------
struct E1c8 {
    virtual ~E1c8();
    char body[0x1c8 - 4];
    E1c8& operator=(const E1c8& o);              // 0x437d60
};
// @ 0x46b4c0
E1c8* __cdecl copy1c8(E1c8* f, E1c8* l, E1c8* d)
{
    for (; f != l; ++f, ++d) *d = *f;
    return d;
}

// ---- std::_Uninit_fill_n with an opaque allocator::construct (catch(...) { throw; } funclets follow) -----------------
struct rec36 { int a[9]; };         // 0x24
struct elem24 { int a[6]; };
struct particle { int a[25]; };     // 100 bytes
struct elem52 { int a[13]; };
struct alloc36 { void construct(rec36* p, const rec36& v); };          // 0x436cb0
struct alloc24 { void construct(elem24* p, const elem24& v); };        // 0x468130
struct alloc100 { void construct(particle* p, const particle& v); };   // 0x468170
struct alloc52 { void construct(elem52* p, const elem52& v); };        // 0x468190

// @ 0x46b500 (+ funclet 0x46b564)
void __cdecl uninit_fill_n36(rec36* first, unsigned count, const rec36& val, alloc36& al)
{
    rec36* next = first;
    try {
        for (; 0 < count; --count, ++first) al.construct(first, val);
    } catch (...) {
        for (; next != first; ++next) ;
        throw;
    }
}
// @ 0x46b570 (+ funclet 0x46b5d4)
void __cdecl uninit_fill_n24(elem24* first, unsigned count, const elem24& val, alloc24& al)
{
    elem24* next = first;
    try {
        for (; 0 < count; --count, ++first) al.construct(first, val);
    } catch (...) {
        for (; next != first; ++next) ;
        throw;
    }
}
// @ 0x46b5e0 (+ funclet 0x46b644)
void __cdecl uninit_fill_n100(particle* first, unsigned count, const particle& val, alloc100& al)
{
    particle* next = first;
    try {
        for (; 0 < count; --count, ++first) al.construct(first, val);
    } catch (...) {
        for (; next != first; ++next) ;
        throw;
    }
}
// @ 0x46b650 (+ funclet 0x46b6b4)
void __cdecl uninit_fill_n52(elem52* first, unsigned count, const elem52& val, alloc52& al)
{
    elem52* next = first;
    try {
        for (; 0 < count; --count, ++first) al.construct(first, val);
    } catch (...) {
        for (; next != first; ++next) ;
        throw;
    }
}

// ---- 0x46b6c0: construct a 0x38-byte ArrSet (name + five count/array pairs) from another ----------------------------
struct ArrSet {
    tstring name;
    int cnt[5];
    int* arr[5];
    ArrSet(const ArrSet& o);                               // 0x4636d0
};
inline void* operator new(size_t, void* p) { return p; }
// @ 0x46b6c0
void __stdcall construct_arrset(ArrSet* p, const ArrSet& v) { if (p) new (p) ArrSet(v); }

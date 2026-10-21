// Slice s00423540: key-frame quaternion interpolation of a scene effect definition (the "Curve" of
// s00422db0: slerp/nlerp of cached key pairs, parent chaining) and the object-to-cell relocation of the world
// grid (the cell rectangle of an object, grid cells with a sorted vector and a list). Names provisional.
// Flags: /O2 /MT /Gy /EHa (the world-grid part uses std::list/vector pieces, see s00423ed0).
#include <math.h>
#include <stdlib.h>
#include <new>
#include "rgl.h"

using rgl::vector4;

// ---- quaternions (copy of the layout in s00422db0: memory order w, x, y, z) --------------------------------
struct quaternion {
    float w, x, y, z;
    quaternion* mul(quaternion* r, const quaternion* o) const;  // 0x422eb0 (by-value return written as its out-parameter)
    bool dot_negative(const quaternion& o) const { return o.z * z + o.y * y + o.x * x + o.w * w < 0.0f; }
    void negate() { x = -x; y = -y; z = -z; w = -w; }
};

// ---- key tables ----------------------------------------------------------------------------------------
struct KeyTable {
    quaternion* vals; int* keys; int count;                     // values, key array (ints), key count
    void find(int* cur, float* frac, float* key);               // 0x423a50
};
struct KeyObj {
    char pad0[0x1c];
    KeyTable* tables;                                           // +0x1c: one table per channel k
    char pad20[8];
    KeyTable tbl;                                               // +0x28
};
struct CurveRec {                       // 0xd8 bytes
    char pad0[0x18];
    float f18;
    char pad1[0x4c - 0x1c];
    int key_begin, key_end;             // +0x4c, +0x50
    char pad2[0x5c - 0x54];
    KeyObj* table;                      // +0x5c
    char pad3[0xc8 - 0x60];
    int off[3];                         // +0xc8
    char pad4[0xd8 - 0xd4];
};
// per-channel cache block (0x1dc bytes), indexed by k
struct QBlock {
    char pad0[0xc4];
    quaternion ref;                     // +0xc4
    char pad1[0x104 - 0xd4];
    int cur[4];                         // +0x104  cached key index per (2*i + j)
    KeyObj* tab[2];                     // +0x114  cached table per i
    quaternion a[4];                    // +0x11c  key pair per (2*i + j), sign-corrected to ref
    quaternion b[4];                    // +0x15c
    char pad2[0x1c0 - 0x19c];
    int parent;                         // +0x1c0
    char pad3[0x1dc - 0x1c4];
};
extern int g_use_slerp;                 // 0xa171dc
void slerp(quaternion* out, const quaternion* a, const quaternion* b, const float* t);      // 0x4238c0
void nlerp(quaternion* out, const quaternion* a, const quaternion* b, const float* t);      // 0x4239c0

struct Curve {
    char pad0[4];
    QBlock* cache;                      // +4
    char pad1[0x40 - 8];
    CurveRec* recs;                     // +0x40
    quaternion* value(quaternion* out, int i, int j, int k);                                 // 0x423540
    quaternion* chain(quaternion* out, int i, int k);                                        // 0x423b60
};

// @ 0x4238c0
void slerp(quaternion* out, const quaternion* a, const quaternion* b, const float* t)
{
    float c = a->x * b->x + a->z * b->z + a->y * b->y + b->w * a->w;
    float sgn;
    if (c < 0.0) {
        sgn = -1.0f;
        c = -c;
    } else {
        sgn = 1.0f;
    }
    float s0, s1;
    if (1.0 - c > 0.1f) {
        float om = (float)acos(c);
        float so = (float)sin(om);
        s0 = (float)sin((1.0f - *t) * om) * (1.0f / so);
        s1 = (float)sin(om * *t) * (1.0f / so);
    } else {
        s0 = 1.0f - *t;
        s1 = *t;
    }
    s1 *= sgn;
    out->x = s1 * b->x + s0 * a->x;
    out->y = s1 * b->y + s0 * a->y;
    out->z = s1 * b->z + s0 * a->z;
    out->w = s1 * b->w + s0 * a->w;
}

// @ 0x4239c0
void nlerp(quaternion* out, const quaternion* a, const quaternion* b, const float* t)
{
    float sgn;
    if (a->x * b->x + a->z * b->z + a->y * b->y + b->w * a->w < 0.0)
        sgn = -1.0f;
    else
        sgn = 1.0f;
    float s0 = 1.0f - *t;
    sgn = sgn * *t;
    out->x = sgn * b->x + s0 * a->x;
    out->y = sgn * b->y + s0 * a->y;
    out->z = sgn * b->z + s0 * a->z;
    out->w = sgn * b->w + s0 * a->w;
}

// @ 0x423a50
void KeyTable::find(int* cur, float* frac, float* key)
{
    int n = count;
    float last = (float)keys[n - 1];
    float k = (float)keys[0];
    if (!(*key < k)) {
        k = last;
        if (!(last <= *key))
            k = *key;
    }
    int c = *cur;
    if (c > -1 && c < n - 1) {
        if ((float)keys[c] <= k) {
            if (k < (float)keys[c + 1])
                goto done;
            if (c < n - 2 && k < (float)keys[c + 2]) {
                *cur = c + 1;
                goto done;
            }
        } else if (0 < c && (float)keys[c - 1] <= k) {
            *cur = c - 1;
            goto done;
        }
    }
    {
        int hi = n - 1;
        int lo = 0;
        if (n != 2 && -1 < n - 2) {
            int m = lo;
            do {
                m = (hi + lo) >> 1;
                if (k < (float)keys[m]) {
                    hi = m;
                    m = lo;
                }
                lo = m;
            } while (m < hi - 1);
        }
        *cur = lo;
    }
done:
    int base = keys[*cur];
    *frac = (k - (float)base) / (float)(keys[*cur + 1] - base);
}

// @ 0x423540
quaternion* Curve::value(quaternion* out, int i, int j, int k)
{
    CurveRec* r = &recs[i];
    int ofs = 0;
    if (r->table == 0) {
        out->x = 0.0f;
        out->y = 1.0f;
        out->z = 0.0f;
        out->w = 0.0f;
        return out;
    }
    if (j != 0)
        ofs = r->off[j];
    int span = r->key_end - r->key_begin;
    float key = (float)span * r->f18 + (float)r->key_begin + (float)ofs;
    int slot = (k * 0x77 + 2 * i + j) * 4 + 0x104;
    int cur = *(int*)((char*)cache + slot);
    float frac;
    r->table->tables[k].find(&cur, &frac, &key);
    quaternion q;
    if (cur != *(int*)((char*)cache + slot) || r->table != cache[k].tab[i]) {
        *(int*)((char*)cache + slot) = cur;
        cache[k].tab[i] = r->table;
        quaternion* va = r->table->tables[k].vals;
        cache[k].a[2 * i + j] = va[cur];
        cache[k].b[2 * i + j] = va[cur + 1];
        if (cache[k].ref.dot_negative(cache[k].a[2 * i + j])) {
            cache[k].a[2 * i + j].x = -cache[k].a[2 * i + j].x;
            cache[k].a[2 * i + j].y = -cache[k].a[2 * i + j].y;
            cache[k].a[2 * i + j].z = -cache[k].a[2 * i + j].z;
            cache[k].a[2 * i + j].w = -cache[k].a[2 * i + j].w;
        }
        if (cache[k].ref.dot_negative(cache[k].b[2 * i + j])) {
            cache[k].b[2 * i + j].x = -cache[k].b[2 * i + j].x;
            cache[k].b[2 * i + j].y = -cache[k].b[2 * i + j].y;
            cache[k].b[2 * i + j].z = -cache[k].b[2 * i + j].z;
            cache[k].b[2 * i + j].w = -cache[k].b[2 * i + j].w;
        }
    }
    if (g_use_slerp == 0) {
        nlerp(&q, &cache[k].a[2 * i + j], &cache[k].b[2 * i + j], &frac);
    } else {
        quaternion* va = r->table->tables[k].vals;
        slerp(&q, &va[cur], &va[cur + 1], &frac);
    }
    *out = q;
    return out;
}

// @ 0x423b60
quaternion* Curve::chain(quaternion* out, int i, int k)
{
    quaternion t, t3;
    quaternion* p = value(&t, i, 0, k);
    quaternion q = *p;
    int parent = cache[k].parent;
    if (parent != -1) {
        chain(&t, i, parent);
        p = t.mul(&t3, &q);
    }
    *out = *p;
    return out;
}

// ---- world grid: relocating an object whose cell rectangle changed ------------------------------------------
// (Obj, ObjVec, ObjList and Cell as in s00423ed0; only the out-of-line members are declared here.)
struct Obj;
struct int2 { int x, y; };
struct Obj {
    virtual void v0();
    virtual int kind();                                         // slot 1: 2 = polygon, 3 = group, 4 = other
    virtual void v2(); virtual void v3(); virtual void v4(); virtual void v5(); virtual void v6();
    virtual void v7(); virtual void v8(); virtual void v9();
    virtual const float* bbox_min(float* tmp);                  // slot 10 (+0x28)
    virtual const float* bbox_max(float* tmp);                  // slot 11 (+0x2c)
    int f4;
    char pad8[8];
    int2 lo, hi;                                                // +0x10: cell rectangle
    Obj** kids;                                                 // +0x20 (kind 3)
    int nkids;                                                  // +0x24
};
struct ListNode { ListNode* next; ListNode* prev; Obj* obj; };
struct Cell {                                                   // 0x34 bytes
    char pad0[4];
    Obj** first; Obj** last; Obj** end;                         // sorted objects (vector<Obj*>)
    char pad10[4];
    ListNode* head;                                             // +0x14 list sentinel
    int size;                                                   // +0x18 list size
    char pad1c[0x34 - 0x1c];
    void remove(Obj* o);                                        // 0x424060 (vector part)
    void add(Obj* o);                                           // 0x423f10
};
int2* min2(int2* out, const int2* a, const int2* b);                        // 0x423e70
int2* max2(int2* out, const int2* a, const int2* b);                        // 0x423ea0

struct World {
    char pad0[0x28];
    Cell** cols;                                                // +0x28
    void to_cell(const float* p, int* cell);                    // 0x41ed00
    bool in_rect(const int2* p, const int2* lo, const int2* hi);   // 0x423ed0 (callee: s00423ed0, there declared as a free stdcall function returning int)
    void relocate(Obj* o);                                      // 0x423c80
    void update(Obj* o);                                        // 0x423c00
};

// @ 0x423c80
void World::relocate(Obj* o)
{
    float tmp[3];
    int2 nlo, nhi;
    to_cell(o->bbox_min(tmp), &nlo.x);
    to_cell(o->bbox_max(tmp), &nhi.x);
    if (nlo.x == o->lo.x && nlo.y == o->lo.y && nhi.x == o->hi.x && nhi.y == o->hi.y)
        return;
    int2 t1, t2;
    int2 mn = *min2(&t1, &nlo, &o->lo);
    int2 mx = *max2(&t2, &nhi, &o->hi);
    int2 p;
    for (p.x = mn.x; p.x <= mx.x; p.x++) {
        for (p.y = mn.y; p.y <= mx.y; p.y++) {
            if (in_rect(&p, &o->lo, &o->hi)) {
                if (!in_rect(&p, &nlo, &nhi)) {
                    Cell* c = &cols[p.x][p.y];
                    c->remove(o);
                    ListNode* h = c->head;
                    ListNode* n = h->next;
                    while (n != h) {
                        if (n->obj == o)
                            break;
                        n = n->next;
                    }
                    if (n != h) {
                        n->prev->next = n->next;
                        n->next->prev = n->prev;
                        operator delete(n);
                        c->size--;
                    }
                }
            } else if (in_rect(&p, &nlo, &nhi)) {
                cols[p.x][p.y].add(o);
            }
        }
    }
    o->lo = nlo;
    o->hi = nhi;
}

// @ 0x423c00
void World::update(Obj* o)
{
    if (o->kind() == 3) {
        for (int i = 0; i < o->nkids; i++) {
            if (o->kids[i]->kind() == 3)
                update(o->kids[i]);
            else
                relocate(o->kids[i]);
        }
    } else if (o->kind() != 4) {
        relocate(o);
    }
}

// the callers above see only the declarations; the bodies live in a separate namespace so cl keeps the calls out of line
namespace def {
// @ 0x423e70
int2* min2(int2* out, const int2* a, const int2* b)
{
    int x = a->x < b->x ? a->x : b->x;
    int y = a->y < b->y ? a->y : b->y;
    out->x = x;
    out->y = y;
    return out;
}
// @ 0x423ea0
int2* max2(int2* out, const int2* a, const int2* b)
{
    int x = a->x > b->x ? a->x : b->x;
    int y = a->y > b->y ? a->y : b->y;
    out->x = x;
    out->y = y;
    return out;
}
}

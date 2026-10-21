// Slice s00436c90: std::allocator<T>::construct / std::_Uninit_copy instantiations for small records, the
// height-field line-of-sight test (0x436da0), a 2-D grid ray walker (0x437290) and ItemRef::reduce (0x437510).
#include <new>
#include <math.h>
#include "rgl.h"

using rgl::vector4;

// rgl string with a read cursor (16 bytes), layout from s00429890/tstring.h
struct tstring {
    int pos; char* buf; int cap; int len;
    tstring(const tstring& o);                  // 0x42c110
};

// ---- std::allocator<T>::construct(T* p, const T& v): thiscall on an empty allocator, `new (p) T(v)` ----
struct rec36 { int a[9]; };        // 0x24 bytes
struct rec136 { int a[34]; };      // 0x88 bytes
struct int3 { int a, b, c; };      // 12 bytes (provisional)
struct int1 { int a; };
struct alloc_int3;

#define ALLOC(N, T) struct N { __declspec(noinline) void construct(T* p, const T& v); };
ALLOC(alloc_tstring, tstring)
ALLOC(alloc_rec36, rec36)
ALLOC(alloc_rec136, rec136)
ALLOC(alloc_int3, int3)
ALLOC(alloc_int1, int1)

// @ 0x436c90
void alloc_tstring::construct(tstring* p, const tstring& v) { new ((void*)p) tstring(v); }
// @ 0x436cb0
void alloc_rec36::construct(rec36* p, const rec36& v) { new ((void*)p) rec36(v); }
// @ 0x436cd0
void alloc_rec136::construct(rec136* p, const rec136& v) { new ((void*)p) rec136(v); }
// @ 0x436cf0
void alloc_int3::construct(int3* p, const int3& v) { new ((void*)p) int3(v); }

// @ 0x436d10 (+ catch-all funclet 0x436d76: `throw;`)  std::_Uninit_copy (non-scalar tag)
int3* uninit_copy_int3(const int3* first, const int3* last, int3* dest, alloc_int3& al)
{
    int3* next = dest;
    try {
        for (; first != last; ++dest, ++first)
            al.construct(dest, *first);
    } catch (...) {
        for (; next != dest; ++next)
            ;   // _Al.destroy(next): trivial, but the empty loop is what keeps dest/first in memory
        throw;
    }
    return dest;
}
// @ 0x436d80
void alloc_int1::construct(int1* p, const int1& v) { new ((void*)p) int1(v); }

// ---------------------------------------------------------------------------------------------------------
// Height field (terrain tiles; layout from s004333c0 HeightField, plus the origin at +4/+8) and a 2-D grid.
// ---------------------------------------------------------------------------------------------------------
struct vec2 {
    float x, y;
    float length() const { return (float)sqrt(y * y + x * x); }
    float normalize()                       // 0x4367c0 (out of line elsewhere; inlined here)
    {
        float len = (float)sqrt(y * y + x * x);
        if (len > 0.0f) {
            float inv = 1.0f / len;
            x = inv * x;
            y = inv * y;
        } else {
            x = 0.0f;
            y = 1.0f;
        }
        return len;
    }
};

struct vec2e {                              // the same type seen with the out-of-line normalize
    float x, y;
    float normalize();                      // 0x4367c0 (returns the old length)
};

struct HCell { float h; unsigned flags; int id; int id2; };
struct HeightField {
    HCell** cols;                           // +0
    float x0, y0;                           // +4, +8 (world origin of the tile grid)
    char padc[0x14 - 0xc];
    int w, h;                               // +0x14, +0x18
    float cell;                             // +0x1c
    bool line_clear(const vector4* a, const vector4* b);        // 0x436da0
};

// @ 0x436da0
bool HeightField::line_clear(const vector4* a, const vector4* b)
{
    const float* org = &x0;
    vec2e lo = { org[0], org[1] };
    vec2e hi = lo;
    hi.x = (float)w * cell + hi.x;
    hi.y = (float)h * cell + hi.y;
    float t0 = 0.0f;
    vector4 d;
    d.x = b->x - a->x;
    d.y = b->y - a->y;
    d.z = b->z - a->z;
    float t1 = d.normalize();
    vector4 p0;
    p0.x = d.x * t0 + a->x;
    if (p0.x < lo.x) {
        if (d.x > 0.0f)
            t0 = (lo.x - p0.x) / d.x;
    } else if (p0.x > hi.x && d.x < 0.0f)
        t0 = (hi.x - p0.x) / d.x;
    p0.y = d.y * t0 + a->y;
    if (p0.y < lo.y) {
        if (d.y > 0.0f)
            t0 += (lo.y - p0.y) / d.y;
    } else if (p0.y > hi.y && d.y < 0.0f)
        t0 += (hi.y - p0.y) / d.y;
    p0.x = d.x * t1 + a->x;
    if (p0.x < lo.x) {
        if (d.x < 0.0f)
            t1 += (lo.x - p0.x) / d.x;
    } else if (p0.x > hi.x && d.x > 0.0f)
        t1 += (hi.x - p0.x) / d.x;
    p0.y = d.y * t1 + a->y;
    if (p0.y < lo.y) {
        if (d.y < 0.0f)
            t1 += (lo.y - p0.y) / d.y;
    } else if (p0.y > hi.y && d.y > 0.0f)
        t1 += (hi.y - p0.y) / d.y;
    if (!(t0 < t1))
        return true;
    vector4 p1;
    p0.x = d.x * t0 + a->x;
    p0.y = d.y * t0 + a->y;
    p0.z = d.z * t0 + a->z;
    p1.x = d.x * t1 + a->x;
    p1.y = d.y * t1 + a->y;
    p1.z = d.z * t1 + a->z;
    vec2e seg;
    seg.x = p1.x - p0.x;
    seg.y = p1.y - p0.y;
    if (sqrt(seg.y * seg.y + seg.x * seg.x) > 1e10f)
        return false;
    vec2e dir = seg;
    float len = dir.normalize();
    float acc = 0.0f;
    float tmax[2], tdelta[2], fpos[2];
    int step[2], cellidx[2];
    const float* P0 = &p0.x;
    float* dd = &dir.x;
    for (int i = 0; i < 2; i++) {
        if (dd[i] > 0.0f)
            step[i] = 1;
        else
            step[i] = -1;
        float inv = 1.0f / dd[i];
        tdelta[i] = (float)fabs(inv * cell);
        fpos[i] = (P0[i] - org[i]) / cell;
        cellidx[i] = (int)floor(fpos[i]);
        float fr = (float)fmod(P0[i] - org[i], cell);
        if (dd[i] == 0.0f)
            tmax[i] = 1e9f;
        else {
            if (step[i] > 0)
                fr = cell - fr;
            tmax[i] = (float)fabs(fr * inv);
        }
    }
    bool hit = false;
    while (!hit && (tmax[0] + acc < len || tmax[1] + acc < len)) {
        int idx = !(tmax[0] < tmax[1]);
        acc += tmax[idx];
        tmax[1 - idx] -= tmax[idx];
        float zc = (p1.z - p0.z) * (acc / len) + p0.z;
        int cx = cellidx[0], cy = cellidx[1];
        if (cx >= 0 && cx < w && cy >= 0 && cy < h) {
            int lx = cx, ly = cy;
            if (step[idx] == 1) { if (idx == 0) lx++; else ly++; }
            int hx = lx, hy = ly;
            if (idx == 0) hy++; else hx++;
            float fr = (float)fabs((dd[1 - idx] / cell) * tmax[1 - idx]);
            if (step[1 - idx] == 1)
                fr = 1.0f - fr;
            if (zc < (1.0f - fr) * cols[lx][ly].h + fr * cols[hx][hy].h)
                hit = true;
        }
        cellidx[idx] += step[idx];
        tmax[idx] = tdelta[idx];
    }
    return !hit;
}

// ---------------------------------------------------------------------------------------------------------
// 2-D grid ray walker: collects the cells a segment crosses (Amanatides-Woo traversal).
// ---------------------------------------------------------------------------------------------------------
struct Grid2 {
    char pad0[8];
    int dim[2];                             // +8, +0xc (cells per axis)
    float mn[2];                            // +0x10
    float mx[2];                            // +0x18
    float cell[2];                          // +0x20 (cell size per axis)
    void walk(const float* a, const float* b, int (*cells)[2], int* count);   // 0x437290
};

// @ 0x437290
void Grid2::walk(const float* a, const float* b, int (*cells)[2], int* count)
{
    vec2 d;
    d.x = b[0] - a[0];
    d.y = b[1] - a[1];
    vec2 dir = d;
    float l = (float)sqrt(dir.x * dir.x + dir.y * dir.y);
    if (l > 0.0f) {
        float inv = 1.0f / l;
        dir.x = inv * dir.x;
        dir.y = inv * dir.y;
    } else {
        dir.x = 0.0f;
        dir.y = 1.0f;
    }
    int step[2], cur[2];
    float tdelta[2], tmax[2], fpos[2];
    float* dd = &dir.x;
    for (int i = 0; i < 2; i++) {
        if (dd[i] > 0.0f)
            step[i] = 1;
        else
            step[i] = -1;
        float inv = 1.0f / dd[i];
        tdelta[i] = (float)fabs(inv * cell[i]);
        fpos[i] = (a[i] - mn[i]) / cell[i];
        cur[i] = (int)floor(fpos[i]) + 1;
        if (cur[i] < 0)
            cur[i] = 0;
        else if (cur[i] > dim[i] - 1)
            cur[i] = dim[i] - 1;
        if (a[i] < mn[i]) {
            if (step[i] < 0)
                tmax[i] = 1e10f;
            else
                tmax[i] = (float)fabs((mn[i] - a[i]) * inv);
        } else if (a[i] > mx[i]) {
            if (step[i] > 0)
                tmax[i] = 1e10f;
            else
                tmax[i] = (float)fabs((a[i] - mx[i]) * inv);
        } else {
            float fr = (float)fmod(a[i] - mn[i], cell[i]);
            if (step[i] > 0)
                fr = cell[i] - fr;
            tmax[i] = (float)fabs((double)(fr * inv));
        }
    }
    int* pc = cur;
    *count = 0;
    cells[0][0] = cur[0];
    cells[0][1] = cur[1];
    int n = *count + 1;
    *count = n;
    float acc = 0.0f;
    float total = (float)sqrt(d.y * d.y + d.x * d.x);
    while (total > tmax[0] + acc || total > tmax[1] + acc) {
        int idx;
        if (tmax[0] < tmax[1])
            idx = 0;
        else
            idx = 1;
        cur[idx] += step[idx];
        cells[n][0] = cur[0];
        cells[n][1] = cur[1];
        n = *count + 1;
        *count = n;
        float* tp = &tmax[idx];
        float* to = &tmax[1 - idx];
        *to -= *tp;
        acc += *tp;
        if (pc[idx] == 0)
            *tp = 1e10f;
        else if (pc[idx] == dim[idx] - 1)
            *tp = 1e10f;
        else
            *tp = tdelta[idx];
    }
}

// ---------------------------------------------------------------------------------------------------------
// ItemRef (item index + packed amount word, see s00434ad0): consume `n` from the stack, or restore a damage word.
// ---------------------------------------------------------------------------------------------------------
struct ItemKind {
    char pad0[0x1c8];
    unsigned type_flags;                    // +0x1c8 (low byte = itp_type_*)
    char pad1cc[0x238 - 0x1cc];
};
extern ItemKind* g_items;                   // 0x5e29d0

enum {
    itp_type_horse = 1, itp_type_arrows = 5, itp_type_bolts, itp_type_shield, itp_type_bow, itp_type_crossbow,
    itp_type_thrown, itp_type_pistol = 16, itp_type_musket, itp_type_bullets, itp_type_animal
};

struct ItemRef {
    int item;
    union {
        unsigned m4;
        struct { unsigned char amount; unsigned char pad[2]; signed char mod; } b;
    };
    int stat_1e8();                         // 0x434df0
    void reduce(int n);                     // 0x437510
};

// @ 0x437510
void ItemRef::reduce(int n)
{
    if (item > 0) {
        unsigned t = g_items[item].type_flags & 0xff;
        if (t == itp_type_thrown || t == itp_type_arrows || t == itp_type_bolts || t == itp_type_pistol ||
            t == itp_type_musket || t == itp_type_crossbow || t == itp_type_bullets) {
            if (n <= 0) {
                b.amount = 0;
                return;
            }
            unsigned v = (m4 & 0xff) - n;
            b.amount = 0;
            m4 = m4 | (((int)v < 0) - 1 & v);
        } else if (t == itp_type_shield || t == itp_type_horse || t == itp_type_animal) {
            unsigned v = stat_1e8();
            *(short*)&m4 = 0;
            m4 |= v;
        }
    }
}

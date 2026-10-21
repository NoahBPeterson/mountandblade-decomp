// Slice s00467250: terrain layer loading, terrain quad statistic, and mission helpers (damage scaling, flag
// clearing, slot reset, valid-target test) followed by five std::vector size()/fill/copy_backward instantiations.
// Names provisional.
#include <windows.h>
#include <stdlib.h>
#include <math.h>
#include "s00467250.h"

// ---- mission (object at 0x5e292c; see s00435490) -----------------------------------------------------
struct m64 { int d[16]; };                      // 64-byte block (a matrix)
struct slot130 {                                // 0x130 bytes, array at mission+0x6b0
    m64 a;                                      // 0x00
    m64 b;                                      // 0x40
    m64 c;                                      // 0x80
    int f0c0, f0c4;                             // 0xc0 / 0xc4
    char padc8[0xe8 - 0xc8];
    int f0e8;                                   // 0xe8
    char padec[0x130 - 0xec];
};
struct ModeRec2 { char pad[0x34]; char* recs; char pad2[0x58 - 0x38]; };    // 0x58, array at 0x5e2a00; +0x34 -> 0x4c-byte records
extern ModeRec2* g_modes2;                      // 0x5e2a00
struct Troop3 { char pad0[0x30]; __int64 flags; char pad38[0x508 - 0x38]; };     // 0x508
struct World3 { char pad0[0x13a00]; Troop3* troops; char pad1[0x13a50 - 0x13a04]; int* tbl; };
extern World3* g_world3;                        // 0x5e2930
extern float g_speed_base;                      // 0x5aa07c
int __cdecl round_float(float* p);              // 0x432540

struct Mission3 {
    char pad0[0x6b0];
    slot130 slots[18];                          // 0x6b0
    char pad1c70[0x5df0 - 0x6b0 - sizeof(slot130) * 18];
    int mode;                                   // 0x5df0
    int f5df4;                                  // 0x5df4
    char pad5df8[0x6214 - 0x5df8];
    int f6214, f6218;                           // 0x6214 / 0x6218
    char pad621c[0x6520 - 0x621c];
    int f6520;                                  // 0x6520
    char pad6524[0x6824 - 0x6524];
    char done[64];                              // 0x6824
    void clear_done();                          // 0x4676c0
    void copy_slot(int i);                      // 0x4676e0
    float speed_factor(float x);               // 0x467740 (thiscall, ignores this)
    void add_amount(int idx, unsigned flags);   // 0x4677a0
};


struct RuleRec { int f0; unsigned flags; unsigned f8; };            // flags: 0x100/0x200/0x400 and 0x10000/0x20000/0x40000
struct CandRec { int f0; int troop; unsigned char f8; };


// ---- std::vector<T> (VC7.1 layout: allocator at +0, _First +4, _Last +8, _End +0xc) -------------------
struct S24 { int a, b, c, d, e, f; };              // 0x18 element
struct S36 { int a[9]; };                          // 0x24 element
struct S52 { int a[13]; };                         // 0x34 element
struct S516 { int a[129]; };                       // 0x204 element
#define VEC_STRUCT(NAME, T) \
    struct NAME { int alloc; T* first; T* last; T* end; int size() const; }
VEC_STRUCT(vec36, S36);     // size() 0x467ac0
VEC_STRUCT(vec24, S24);     // size() 0x467ae0
VEC_STRUCT(vec52, S52);     // size() 0x467b00
VEC_STRUCT(vec516, S516);   // size() 0x467b20

extern ttype g_ttypes[];                        // 0x8ed4df8

// ---- terrain ---------------------------------------------------------------------------------------
// @ 0x467250
void terrain_obj::load_layer(int* set, int idx)
{
    // set[0] = count; entry idx has its type at set[idx * 6 + 7] and its data pointer at set[idx * 6 + 8]
    if (set != 0 && idx < *set) {
        if (set[idx * 6 + 7] == -0x1e71) {
            // height offset layer: f88 is the offset, pz = h2 + offset
            if (set[idx * 6 + 8] == 0) {
                for (int y = 0; y <= h; y++)
                    for (int x = 0; x <= w; x++) {
                        at(x, y).f88 = 0.0f;
                        at(x, y).pz = at(x, y).f88 + at(x, y).h2;
                    }
            } else {
                for (int y = 0; y <= h; y++)
                    for (int x = 0; x <= w; x++) {
                        at(x, y).f88 = *(float*)(y * 4 + x * 0x690 + set[idx * 6 + 8]);
                        at(x, y).pz = at(x, y).f88 + at(x, y).h2;
                    }
            }
        } else if (set[idx * 6 + 7] == -0x3115) {
            if (set[idx * 6 + 8] == 0) {
                for (int y = 0; y <= h; y++)
                    for (int x = 0; x <= w; x++)
                        at(x, y).f8c = -1;
            } else {
                for (int y = 0; y <= h; y++)
                    for (int x = 0; x <= w; x++)
                        at(x, y).f8c = *(int*)(y * 4 + x * 0x690 + set[idx * 6 + 8]);
            }
        } else {
            if (set[idx * 6 + 8] == 0) {
                for (int y = 0; y <= h; y++)
                    for (int x = 0; x <= w; x++)
                        at(x, y).layer[idx] = 0;
            } else {
                for (int y = 0; y <= h; y++)
                    for (int x = 0; x <= w; x++)
                        at(x, y).layer[idx] = *(int*)(y * 4 + x * 0x690 + set[idx * 6 + 8]);
            }
            tlayer* l = &layers[idx];
            int t = set[idx * 6 + 7];
            l->type = t;
            if (t >= 0) {
                ttype* tt = &g_ttypes[t];
                l->f1c = tt->a;
                l->f24 = tt->b;
                l->f14 = tt->c;
                l->f18 = -1;
            }
        }
    }
}

// @ 0x467530
int terrain_obj::count_low_quads()
{
    int count = 0;
    for (int y = 20; y < h - 20; y++)
        for (int x = 20; x < w - 20; x++) {
            terrain_quad& q = quad(x, y);
            if (q.f14 < 0.6f || q.f24 < 0.45f)
                count++;
        }
    return count;
}

// @ 0x4676c0
void Mission3::clear_done()
{
    for (int i = 0; i < 64; i++) done[i] = 0;
}

// @ 0x4676e0
void Mission3::copy_slot(int i)
{
    char* base = (char*)this + i * 0x130;       // slot i of the 0x130-byte records at +0x6b0 (cl keeps the +0x6b0 separate)
    *(m64*)(base + 0x6b0) = *(m64*)(base + 0x6f0);
    *(int*)(base + 0x774) = 0;
    *(int*)(base + 0x770) = 0;
    *(m64*)(base + 0x730) = *(m64*)(base + 0x6f0);
    *(int*)(base + 0x798) = 0;
}

// @ 0x467740
float Mission3::speed_factor(float x)
{
    float base = g_speed_base * 2.5f + 0.833333f;
    float t = (x + 15.0f) * 0.06666667f;
    return base * (t < 0.2f ? 0.2f : (t >= 2.5f ? 2.5f : t));
}

// @ 0x4677a0
void Mission3::add_amount(int idx, unsigned flags)
{
    unsigned mask = flags & 0xfffffff;
    char* rec = g_modes2[mode].recs + idx * 0x4c;
    if (mask == 0) return;
    unsigned rf = *(unsigned*)(rec + 4);
    unsigned amount;
    if ((rf & 4) || (rf & 0x2000000)) {
        amount = mask;
    } else if (rf & 8) {
        amount = 1;
    } else if (rf & 0x10) {
        amount = g_world3->tbl[0xc5 + f5df4 * 0x125 + idx];
    } else {
        int v;
        if ((rf & 1) || ((rf & 0x40) && f6214 == 1) || ((rf & 0x80) && f6214 == 0)
            || (f6214 == 2 && (rf & 0x40) && f6218 == 0) || (f6214 == 2 && (rf & 0x80) && f6218 == 1))
            v = -f6520;
        else
            v = f6520;
        float f = speed_factor((float)v) * (float)(int)mask;
        amount = round_float(&f);
        if ((int)amount <= 0) amount = 1;
    }
    if (flags & 0x20000000) {
        float f = ((float)(rand() % 0x3dc9) * 5.0578492e-05f + 0.6f) * (float)(int)amount;
        float fl = (float)floor((double)f);     // 0x432540 inlined
        int n = (int)fl;
        amount = (f - fl <= 0.5f) ? n : n + 1;
    }
    *(int*)(rec + 0x18) += amount;
    done[idx] = 0;
}

// ---- test whether a candidate (b) passes the filter flags of a rule (a) -------------------------------

// @ 0x467940
bool __stdcall rule_accepts(RuleRec* a, CandRec* b, int p3, char p4, char p5, char p6)
{
    bool ok = true;
    if (b->f8 & 1) ok = false;
    unsigned fl = a->flags;
    if ((fl & 0x400) && (g_world3->troops[b->troop].flags & 0x10) == 0)
        ok = false;
    if ((fl & 0x200) && p3 != 0 && (g_world3->troops[b->troop].flags & 0x10) != 0)
        ok = false;
    if ((fl & 0x100) && p3 == 0 && (g_world3->troops[b->troop].flags & 0x10) != 0)
        return false;
    if (ok && (fl & 0x70000)) {
        Troop3* t = &g_world3->troops[b->troop];
        __int64 tf = t->flags;
        bool x3 = false, x5 = false, x4 = false;
        if ((tf & 0x1000000) && (a->f8 & 0x100) == 0) {
            x5 = true;
        } else if ((tf & 0x4000000)) {
            x4 = true;
        } else {
            x3 = true;
        }
        ok = false;
        if ((fl & 0x10000) && (p4 == 0 || x3)) ok = true;
        if ((fl & 0x20000) && (p5 == 0 || x4)) ok = true;
        if ((fl & 0x40000) && (p6 == 0 || x5)) ok = true;
    }
    return ok;
}

// ---- vector instantiations (names provisional; each is a distinct element type) ---------------------
// @ 0x467ac0
int vec36::size() const { return first == 0 ? 0 : (int)(last - first); }
// @ 0x467ae0
int vec24::size() const { return first == 0 ? 0 : (int)(last - first); }
// @ 0x467b00
int vec52::size() const { return first == 0 ? 0 : (int)(last - first); }
// @ 0x467b20
int vec516::size() const { return first == 0 ? 0 : (int)(last - first); }

// @ 0x467b40
void fill36(S36* first, S36* last, const S36* val)
{
    for (; first != last; ++first)
        *first = *val;
}

// @ 0x467b70
S36* copy_backward36(S36* first, S36* last, S36* dest)
{
    while (first != last) {
        --last;
        --dest;
        *dest = *last;
    }
    return dest;
}

// @ 0x467ba0
void fill24(S24* first, S24* last, const S24* val)
{
    for (; first != last; ++first)
        *first = *val;
}

// Slice s0046b6e0: VC7.1 <algorithm>/<memory> instantiations (std::stable_sort over a vector of 8-byte records:
// _Copy_opt into _Temp_iterator, _Rotate, merge, copy, copy_backward, _Lower_bound, _Upper_bound), the _Uninit_copy of
// 0x24- and 0x88-byte records, a weather/wind-style parameter block ctor, an array allocator and rain/particle seeding.
// Names provisional.
#include <vector>
#include <algorithm>
#include <new>

struct E8 { int a, b; };            // 8-byte sortable record
typedef bool (*Pr8)(const E8&, const E8&);

// 0x46b7d0 _Copy_opt<vector<E8>::iterator, _Temp_iterator<E8>>   0x46b860 _Rotate<vector<E8>::iterator, int, E8>
// 0x46c260 copy<..., _Temp_iterator<E8>>  0x46c2c0 merge<..., _Temp_iterator<E8>, Pr8>  0x46c3f0 _Copy_opt/copy
// 0x46c420 copy_backward  0x46c450 _Lower_bound  0x46c4a0 _Upper_bound: all come out of one stable_sort.
void sort_e8(std::vector<E8>& v, Pr8 pred) { std::stable_sort(v.begin(), v.end(), pred); }

// ---- _Uninit_copy with an opaque allocator::construct ------------------------------------------------------------
struct rec36 { int a[9]; };          // 0x24 bytes
struct rec136 { int a[34]; };        // 0x88 bytes
struct alloc36 { void construct(rec36* p, const rec36& v); };           // 0x436cb0
struct alloc136 { void construct(rec136* p, const rec136& v); };        // 0x436cd0

// @ 0x46b6e0 (+ catch(...) funclet 0x46b746)
rec36* __cdecl uninit_copy36(rec36* first, rec36* last, rec36* dest, alloc36& al)
{
    rec36* next = dest;
    try {
        for (; first != last; ++dest, ++first) al.construct(dest, *first);
    } catch (...) {
        for (; next != dest; ++next) ;
        throw;
    }
    return dest;
}
// @ 0x46b750 (+ catch(...) funclet 0x46b7bc)
rec136* __cdecl uninit_copy136(rec136* first, rec136* last, rec136* dest, alloc136& al)
{
    rec136* next = dest;
    try {
        for (; first != last; ++dest, ++first) al.construct(dest, *first);
    } catch (...) {
        for (; next != dest; ++next) ;
        throw;
    }
    return dest;
}

// ---- 0x46b940: constructor of a viewport / light parameter block (frustum + two identity frames) -------------------
#include <math.h>
#include <stdlib.h>
#include "rgl.h"
using rgl::vector4;

struct frustum { char b[0x120]; frustum(); };          // 0x41ef60
struct mat4 { vector4 s, f, u, o; };
mat4* identity_pos(mat4* out);                         // 0x40cd00 (cdecl; returns out)

struct ViewParams {
    float f0, f4, f8;               // +0x00 1000, -1, -100
    int fc;                         // +0x0c 0x384878
    char b10;                       // +0x10
    float f14, f18, f1c;            // +0x14 0.1, -1, -100
    int f20;                        // +0x20 0x384878
    float f24;                      // +0x24 -1e7
    float f28;                      // +0x28 0.1
    char b2c;                       // +0x2c
    int f30;                        // +0x30
    frustum fr;                     // +0x34
    char pad154[0x1a4 - 0x154];
    mat4 m1a4;                      // +0x1a4 (copy of identity)
    mat4 m1e4;                      // +0x1e4
    ViewParams();                   // 0x46b940
};
// @ 0x46b940
ViewParams::ViewParams()
{
    f14 = 0.1f; f28 = 0.1f;
    fc = 0x384878; f20 = 0x384878;
    f0 = 1000.0f; f8 = -100.0f; f4 = -1.0f;
    f18 = -1.0f; f1c = -100.0f; f24 = -1.0e7f;
    b10 = 0; b2c = 0;
    mat4 tmp;
    mat4* id = identity_pos(&tmp);
    m1a4 = *id;
    m1e4 = *id;
    f30 = 0;
}

// ---- 0x46b9d0: (re)allocate the per-slot EnvRec records of an effect definition (FxDef of s00428b10) -----------------
extern __int64 g_ticks[];       // 0x5e2948: 64-bit clocks, 100 kHz units

struct EnvRec {                 // 0xd8 bytes; inline constructor, reset() at 0x4686a0, vector deleting dtor at 0x41ed70
    int f0;                     // +0
    char pad4[4];
    int f8, fc;
    char pad10[0x10];
    int clock;                  // +0x20
    char pad24[4];
    __int64 t0;                 // +0x28
    char pad30[0x60 - 0x30];
    vector4* k0; vector4* k1; vector4* k2;     // +0x60..0x68
    char pad6c[0x7c - 0x6c];
    vector4 v7c[2];             // +0x7c
    char pad9c[0xd0 - 0x9c];
    int fd0;                    // +0xd0 (clock index copy)
    char padd4[4];
    EnvRec() : clock(0), t0(g_ticks[0]) {
        fd0 = 0;
        f8 = -1;
        fc = -1;
        f0 = 0;
        k0 = 0; k1 = 0; k2 = 0;
        reset();
    }
    ~EnvRec() { delete[] k0; delete[] k1; delete[] k2; }    // inline: delete[] env calls the vector deleting dtor 0x41ed70
    void reset();               // 0x4686a0
    void alloc(int n);          // 0x428c60
};
struct FxDef2 {
    int count;                  // +0
    char pad4[0x40 - 4];
    EnvRec* env;                // +0x40
    int nenv;                   // +0x44
    int* envidx;                // +0x48
    char pad4c[0x60 - 0x4c];
    int clock;                  // +0x60
    void set_env_count(int n);  // 0x46b9d0
};
// @ 0x46b9d0
void FxDef2::set_env_count(int n)
{
    if (env != 0) {
        delete[] env;
        delete[] envidx;
    }
    nenv = n;
    env = new EnvRec[n];
    envidx = new int[nenv];
    for (int i = 0; i < nenv; i++) {
        EnvRec* b = env;
        EnvRec* e = b + i;
        int c = clock;
        env[i].fd0 = c;
        e->clock = c;
        e->t0 = g_ticks[c];
        env[i].alloc(count);
    }
}

// ---- 0x46baf0: scatter random brush strokes over the terrain ------------------------------------------------------
struct Terr {
    char pad0[0x270];
    float wx, wy;                       // +0x270 / +0x274 world extent
    float rand_avg4();                  // 0x4659e0 (s004650f0)
    void apply_brush(const float* center, float radius, float strength);    // 0x468da0 (s00468da0)
    void seed_brushes(float size, float density, float scale);              // 0x46baf0
};
// @ 0x46baf0
void Terr::seed_brushes(float size, float density, float scale)
{
    float r = (float)(rand() % 15817) * 1.2644623e-5f + 0.4f;
    density = density * wx * wy / (size * size * 0.25f);
    float fl = (float)floor(density);
    int k = (int)fl;
    if (!(density - fl <= 0.5f)) k++;
    int n = (int)((float)k * r * 30.0f);
    for (; n > 0; n--) {
        density = size + size;      // (the original reuses the argument slot)
        float f = (rand_avg4() * 0.5f + 0.05f) * size;
        float pos[2];
        pos[1] = (density + wy) * (float)(rand() % 15817) * 6.322312e-05f - size;
        pos[0] = (density + wx) * (float)(rand() % 15817) * 6.322312e-05f - size;
        apply_brush(pos, f, ((float)(rand() % 15817) * 6.3223115e-06f + 0.2f) * f * scale);
    }
}

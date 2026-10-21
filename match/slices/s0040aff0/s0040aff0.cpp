// Slice s0040aff0: polygon shape ("face", vtable 0x54a588) get_max and the component-wise vector max helper.
// Mirror of get_min / vmin (0x40aef0 / 0x40af80, slice s0040a640).
#include "rgl.h"
using namespace rgl;

struct shape_base {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    shape_base() : m4(-1), m8(8), mc(-1), min_x(100000000), min_y(100000000), max_x(-100000000), max_y(-100000000) {}
    virtual ~shape_base();
};

struct face : shape_base {           // vtable 0x54a588, 0xb4 bytes
    vector4 w[4];                    // 0x20 world-space vertices
    vector4 v[4];                    // 0x60 local vertices
    vector4 normal;                  // 0xa0
    int count;                       // 0xb0
    virtual ~face();
    virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4(); virtual void s5();
    virtual void s6(); virtual void s7(); virtual void s8(); virtual void s9();
    virtual void s10();
    virtual vector4 get_max();       // slot 11  0x40aff0 (by value: hidden result pointer)
};

void __cdecl vmax_ext(vector4* out, const vector4* a, const vector4* b);   // 0x40b080 (declared apart so the compiler sees it as external)

static inline float fmax_(float a, float b) { return a > b ? a : b; }

// @ 0x40aff0
vector4 face::get_max()
{
    vector4 t = w[0];
    for (int i = 1; i < count; i++) vmax_ext(&t, &t, &w[i]);
    return t;
}
// @ 0x40b080
void __cdecl vmax(vector4* out, const vector4* a, const vector4* b)
{
    out->x = fmax_(a->x, b->x);
    out->y = fmax_(a->y, b->y);
    out->z = fmax_(a->z, b->z);
}

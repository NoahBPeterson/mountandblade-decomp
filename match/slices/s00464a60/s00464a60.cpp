// Slice s00464a60: Terrain (scene ground generator) helpers: packed flag-field setters, cell lookup, ground-spec
// weights, noise-based ground colour and default layer set-up. Names are provisional. Flags: default.
#include <stdlib.h>
#include <math.h>
#include "terrain.h"

extern int g_field_mask;                 // 0x5a8ea0 (field values are clamped to g_field_mask - 1)

static inline float fmin_(float a, float b) { return b < a ? b : a; }
static inline float fmax_(float a, float b) { return b > a ? b : a; }
#define IFLOOR(e) ((int)((e) > 0.0f ? (e) : (e) - 0.99999988f))

// @ 0x464a60
void Terrain::set_field_a(const float* p)
{
    int i = IFLOOR(*p);
    unsigned v = i < 0 ? 0 : (i >= 128 ? 127 : i);
    unsigned m = g_field_mask - 1;
    if (v > m) v = m;
    flags = (flags & ~(m << 7)) | (v << 7);
}

// @ 0x464ac0
void Terrain::set_field_b(const float* p)
{
    float x = *p * 100.0f;
    float fl = (float)floor((double)x);
    int i = (int)fl;
    if (!(x - fl <= 0.5f)) i++;
    unsigned v = i < 0 ? 0 : (i >= 100 ? 99 : i);
    unsigned m = g_field_mask - 1;
    if (v > m) v = m;
    flags = (flags & ~(m << 21)) | (v << 21);
}

// @ 0x464b40
int* Terrain::cell_of(int* out, const float* p)
{
    float s = 1.0f / scale;
    int x = IFLOOR(s * p[0]);
    int ix;
    if (x < 0) ix = 0; else { ix = w - 1; if (x < w) ix = x; }
    int y = IFLOOR(s * p[1]);
    int iy;
    if (y < 0) iy = 0; else if (y >= h) iy = h - 1; else iy = y;
    out[0] = ix; out[1] = iy;
    return out;
}

// @ 0x464bf0
float Terrain::spec_weight(int* spec, int* type, int* mode)
{
    GroundSpec* g = &g_ground_specs[*spec];
    int m = *mode;
    int t;
    if (m == 0 && (g->flags & 0x400000) == 0)
        return 0.0f;
    if (m == 1) {
        __int64 f = g->flags;
        if ((f & 0x400000) != 0 || (f & 0x80000) != 0)
            return 0.0f;
    }
    if ((m != 2 || (g->flags & 0x80000) != 0) &&
        (t = *type, t != 2 || (g->flags & 0x8) != 0) &&
        (t != 10 || (g->flags & 0x800) != 0) &&
        (t != 4 || (g->flags & 0x10) != 0) &&
        (t != 12 || (g->flags & 0x1000) != 0) &&
        (t != 5 || (g->flags & 0x20) != 0) &&
        (t != 13 || (g->flags & 0x2000) != 0) &&
        (t != 3 || (g->flags & 0x4) != 0) &&
        (t != 11 || (g->flags & 0x400) != 0) &&
        ((t & 7) != 7 || (g->flags & 0x4) != 0))
        return 1.0f;
    return 0.0f;
}

// @ 0x464d50
vector4* Terrain::ground_color(vector4* out, const float* pos)
{
    unsigned t = flags >> 28 & g_type_mask - 1U;
    if (t == 4 || t == 12) {
        out->x = 1.0f; out->y = 1.0f; out->z = 1.0f;
        return out;
    }
    vector4 c;
    float n[4];
    c.x = off_b[0] + pos[0];
    c.y = off_b[1] + pos[1];
    c.z = off_b[2] + pos[2];
    noise_fractal(15.0f, 0.6f, 3, 0, &c.x, n);
    c.x = fmin_(n[0] * 1.2f + 0.97f, 1.0f);
    c.y = fmin_(n[1] + 0.975f, 1.0f);
    c.y = fmax_(c.x - 0.022f, c.y);
    c.x = fmax_(c.y - 0.2f, c.x);
    c.z = fmin_(n[2] + 0.97f, 1.0f);
    c.z = fmin_(c.z, c.x);
    c.z = fmin_(c.z, c.y);
    if (g_use_shaders) {
        c.x = c.x * c.x;
        c.y = c.y * c.y;
        c.z = c.z * c.z;
    }
    *out = c;
    return out;
}

// @ 0x464f10
void Terrain::init_layers()
{
    int r = rand() % 2;
    int* q = (int*)((char*)this + 0x50);
    for (int g = 0; g < 3; g++) {
        for (int k = 0; k < 5; k++)
            q[k * 10 - 10] = -1;
        q += 50;
    }
    switch (r) {
    case 0:
        layer[0].type = 0;
        layer[0].fc = g_ground_types[0].f10;
        layer[0].f14 = g_ground_types[0].f18;
        layer[0].f4 = g_ground_types[0].f1c;
        layer[0].f8 = -1;
        break;
    case 1:
        layer[0].type = 1;
        layer[0].fc = g_ground_types[1].f10;
        layer[0].f14 = g_ground_types[1].f18;
        layer[0].f4 = g_ground_types[1].f1c;
        layer[0].f8 = -1;
        break;
    }
    rand();
    int a = -1, b = 2, c = 5;
    switch (flags >> 28 & g_type_mask - 1U) {
    case 3:
        a = b = 2;
        break;
    case 7:
        b = 2;
        break;
    case 2:
        b = 3;
        rand();
        break;
    case 4:
    case 12:
        c = 4;
        b = -1;
        break;
    case 5:
    case 13:
        b = 6;
        break;
    case 11:
        b = 2;
        c = 7;
        break;
    case 10:
        b = 3;
        c = 7;
        break;
    }
    layer[1].type = c;
    if (c >= 0) {
        GroundType* gt = &g_ground_types[c];
        layer[1].fc = gt->f10;
        layer[1].f14 = gt->f18;
        layer[1].f4 = gt->f1c;
        layer[1].f8 = -1;
    }
    layer[2].type = b;
    if (b >= 0) {
        GroundType* gt = &g_ground_types[b];
        layer[2].fc = gt->f10;
        layer[2].f14 = gt->f18;
        layer[2].f4 = gt->f1c;
        layer[2].f8 = -1;
    }
    layer[3].type = 8;
    layer[3].fc = g_ground_types[8].f10;
    layer[3].f14 = g_ground_types[8].f18;
    layer[3].f4 = g_ground_types[8].f1c;
    layer[3].f8 = -1;
    layer[1].f10 = 0;
    if (a >= 0)
        layer[1].f10 = g_ground_types[a].f14;
}

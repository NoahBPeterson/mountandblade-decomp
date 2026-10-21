// Slice s00468fc0: world-map terrain object, third part: a weighted random walk across the cell grid
// (0x468fc0), averaging of neighbouring flow vectors (0x469a70), and ambient particle spawning from the
// ground-type scatter records (0x46a270). Names provisional.
#include <windows.h>
#include <stdlib.h>
#include <math.h>
#include "s00468fc0.h"

struct ivec2 { int dx, dy; };
extern ivec2 g_nbr[];                                   // 0x591340 eight neighbour offsets
extern unsigned g_mask_a;                               // 0x5a8ea0
extern unsigned g_mask_b;                               // 0x5a8e78
extern int g_terrain_gate;                              // 0x5aa03c
ptype* g_ptypes;                                        // 0x5e28f8 (defined elsewhere; declared here)

void __cdecl noise_fractal(float freq, float gain, int octaves, int sharpen, const float* p, float* out);   // 0x414fc0
struct mat3 { rgl::vector4 s, f, u; };
__declspec(noinline) mat3* identity3(mat3* out);                             // 0x40cd60
struct pos2 : rgl::matrix {
    void rotate_sf(const float* angle);     // 0x40e770
};

static const int kStride = 251;

// inline vec2 helpers (scalar by const reference)
static __forceinline vec2 v2sub(const vec2& a, const vec2& b) { vec2 r; r.x = a.x - b.x; r.y = a.y - b.y; return r; }
static __forceinline vec2 v2add(const vec2& a, const vec2& b) { vec2 r; r.x = a.x + b.x; r.y = a.y + b.y; return r; }
static __forceinline vec2 v2mul(const vec2& a, const float& k) { vec2 r; r.x = a.x * k; r.y = a.y * k; return r; }
// rotate e by the unit vector d
static __forceinline vec2 v2rot(const vec2& e, const vec2& d) { vec2 r; r.x = e.x * d.y - e.y * d.x; r.y = e.y * d.y + e.x * d.x; return r; }

static __forceinline void vnorm(vec2& v)
{
    float l = (float)sqrt(v.y * v.y + v.x * v.x);
    if (l > 0.0f) {
        l = 1.0f / l;
        v.x = l * v.x;
        v.y = l * v.y;
    } else {
        v.x = 0.0f;
        v.y = 1.0f;
    }
}

// @ 0x468fc0
void terrain_obj::trace_path(const int* from, const int* to, int bit, int* steps, float* minz, float w1, float w2, int unused, char record)
{
    *minz = 10000.0f;
    ivec2 pos = *(const ivec2*)from;
    *steps = 0;
    vec2 g;
    float gx = (float)(to[0] - pos.dx);
    float gy = (float)(to[1] - pos.dy);
    float gl = (float)sqrt(gx * gx + gy * gy);
    if (gl > 0.0f) {
        gl = 1.0f / gl;
        g.x = gl * gx;
        g.y = gl * gy;
    } else {
        g.x = 0.0f;
        g.y = 1.0f;
    }
    vec2 nd[8];
    for (int k = 0; k < 8; k++) {
        nd[k].x = (float)g_nbr[k].dx;
        nd[k].y = (float)g_nbr[k].dy;
        vnorm(nd[k]);
    }
    unsigned bitmask = 1 << bit;
    terrain_cell* cur = &at(pos.dx, pos.dy);
    cur->flags |= bitmask;
    if (record) {
        cur->dir = g;
        vec2 t;
        t.x = 0.0f;
        t.y = (float)*steps;
        cur->disp = t;
    }
    bool blocked = false;
    for (;;) {
        unsigned fl = flags10;
        float tx = (float)(to[0] - pos.dx);
        int w = this->w;
        int h = this->h;
        float ty = (float)(to[1] - pos.dy);
        float sc[8];
        for (int k = 0; k < 8; k++) {
            int nx = g_nbr[k].dx + pos.dx;
            int ny = g_nbr[k].dy + pos.dy;
            sc[k] = 0.0f;
            float tl = (float)sqrt(tx * tx + ty * ty);
            float ux, uy;
            if (tl > 0.0f) {
                tl = 1.0f / tl;
                ux = tl * tx;
                uy = tl * ty;
            } else {
                ux = 0.0f;
                uy = 1.0f;
            }
            int a = ny - h + 20;
            int b = nx - w + 20;
            int pen = (a < 0 ? 0 : a) + (20 - ny < 0 ? 0 : 20 - ny) + (b < 0 ? 0 : b) + (20 - nx < 0 ? 0 : 20 - nx);
            float edge = 0.0f;
            if (pen > 0 &&
                ((nx < 21 && nd[k].x <= 0.0f) || (ny < 21 && nd[k].y <= 0.0f) ||
                 (w - 20 <= nx && 0.0f <= nd[k].x) || (h - 20 <= ny && 0.0f <= nd[k].y))) {
                edge = (float)pen;
                if (edge < 0.0f) edge = 0.0f;
                else if (24.0f <= edge) edge = 24.0f;
                edge = edge * -0.04f;
            }
            if (nx < 0 || w < nx || ny < 0 || h < ny) {
                sc[k] = ((g.x * nd[k].x + g.y * nd[k].y) + 1.0f) * 0.5f;
            } else {
                terrain_cell* n = &at(nx, ny);
                if ((bitmask & n->flags) == 0) {
                    float dz = ((cur->pz - n->pz) * w2) * 3.0f;
                    if (dz < 0.0f)
                        dz = (0.25f - (float)((fl >> 7) & (g_mask_a - 1)) * 0.0022f) * dz;
                    sc[k] = (((((uy * nd[k].y + ux * nd[k].x) + 0.7f) * 0.5f) * 0.65f + dz) +
                             (((g.y * nd[k].y + g.x * nd[k].x) + 0.7f) * 0.5f) * w1) + edge;
                } else {
                    if (n->order < *steps - 30)
                        blocked = true;
                    sc[k] = -1.0f;
                }
            }
        }
#define CUBE(i) if (0.0f < sc[i]) sc[i] = (sc[i] * sc[i]) * sc[i];
        CUBE(0) CUBE(1) CUBE(2) CUBE(3) CUBE(4) CUBE(5) CUBE(6) CUBE(7)
#undef CUBE
        float total = 0.0f;
#define ADD0(i) if (0.0f < sc[i]) total = sc[i];
#define ADD(i) if (0.0f < sc[i]) total = sc[i] + total;
        ADD0(0) ADD(1) ADD(2) ADD(3) ADD(4) ADD(5) ADD(6) ADD(7)
#undef ADD
#undef ADD0
        int r = rand();
        int pick = -1;
        float roll = ((float)(r % 0x3dc9) * total) * 6.322312e-05f;
        float acc = 0.0f;
        if (0.0f < sc[0]) {
            acc = sc[0];
            if (0.0f <= roll && roll < sc[0]) pick = 0;
        }
#define SEL(i) if (0.0f < sc[i]) { if (acc <= roll && roll < sc[i] + acc) pick = i; acc = acc + sc[i]; }
        SEL(1) SEL(2) SEL(3) SEL(4) SEL(5) SEL(6)
#undef SEL
        if (0.0f < sc[7] && acc <= roll && roll < sc[7] + acc)
            pick = 7;
        else if (pick == -1)
            return;
        pos.dx = g_nbr[pick].dx + pos.dx;
        if (pos.dx < 3) return;
        if (this->w - 3 < pos.dx) return;
        pos.dy = g_nbr[pick].dy + pos.dy;
        if (pos.dy < 3) return;
        if (this->h - 3 < pos.dy) return;
        if (sc[pick] < 0.0f) return;
        float mx = g.x * 0.9f + nd[pick].x * 0.1f;
        float my = g.y * 0.9f + nd[pick].y * 0.1f;
        g.y = sqrtf(mx * mx + my * my);
        if (g.y > 0.0f) {
            g.y = 1.0f / g.y;
            g.x = mx * g.y;
            g.y = my * g.y;
        } else {
            g.x = 0.0f;
            g.y = 1.0f;
        }
        (*steps)++;
        cur = &at(pos.dx, pos.dy);
        cur->flags |= bitmask;
        cur->order = *steps;
        if (record) {
            cur->dir = g;
            vec2 t;
            t.x = 0.0f;
            t.y = (float)*steps * 0.7f;
            cur->disp = t;
        }
        if (cur->pz < *minz)
            *minz = cur->pz;
        if (*steps == 100000)
            return;
        if (blocked)
            return;
    }
}


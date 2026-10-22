// Slice s004713d0: terrain generation at the end of the terrain block: the procedural height pass that fills
// the 251x251 cell grid (noise relief, border ridges, brush scatter, normalisation to ground level) and a
// sun-visibility probe. ALL NAMES PROVISIONAL (Terr = the Terrain object of s00464a60 / s0046b6e0).
// Flags: default /O2 /MT /Gy /TP.
#include <stdlib.h>
#include <math.h>
#include "rgl.h"

using rgl::vector4;

void __cdecl noise_fractal(float freq, float gain, int octaves, int sharpen, const float* p, float* out);   // 0x414fc0
extern unsigned g_field_mask;              // 0x5a8ea0 (field values are clamped to g_field_mask - 1)
extern unsigned __int64 g_bit[];           // 0x5a8e68 (g_bit[i] == 1 << i)

struct TCell {                             // 0x94 bytes (TerrainCell of s00464a60)
    float x, y, z;                         // 0x00
    char pad0c[0x18 - 0x0c];
    float f18;                             // 0x18 (relief weight written by the noise pass)
    char pad1c[0x84 - 0x1c];
    float f84;                             // 0x84 copy of z
    char pad88[0x90 - 0x88];
    int f90;                               // 0x90
};
struct Terr {
    char pad0[0x10];
    unsigned flags;                        // 0x10 (bits 14..: terrain level field)
    char pad14[0x270 - 0x14];
    float wx, wy;                          // 0x270 / 0x274 world extent
    int w, h;                              // 0x278 / 0x27c last cell index in x / y
    char pad280[8];
    TCell cell[251][251];                  // 0x288 [x][y]
    char padc3b0e0[0x843bd4c - 0x288 - 251 * 251 * sizeof(TCell)];
    float noise_seed;                      // 0x843bd4c
    void seed_brushes(float size, float density, float scale);      // 0x46baf0
    void generate_heights();                                        // 0x4713d0
};

// @ 0x4713d0
void Terr::generate_heights()
{
    float r1 = (float)(rand() % 15817) * 6.322312e-05f;
    float scale = (r1 * 0.5f + 1.0f) * (wy + wx) * 0.5f;
    float r2 = (float)(rand() % 15817) * 6.322312e-05f;
    noise_seed = (r2 * 0.5f + 1.0f) * (wy + wx) * 0.2f;
    float fl = (float)((g_field_mask - 1) & flags) * 0.01f;
    float off[3];
    off[0] = (float)(rand() % 15817) * 0.6322311f;
    off[1] = (float)(rand() % 15817) * 0.6322311f;
    off[2] = (float)(rand() % 15817) * 0.6322311f;
    // relief weight from low-frequency noise
    for (int y = 0; y <= h; y++) {
        for (int x = 0; x <= w; x++) {
            float p[3], n[3];
            p[0] = off[0] + cell[x][y].x;
            p[1] = off[1] + cell[x][y].y;
            p[2] = off[2] + cell[x][y].z;
            noise_fractal(noise_seed, 0.6f, 1, 0, p, n);
            float v = (n[2] + n[1] + n[0]) * fl * 9.0f;
            if (v < 0.0f) v = 0.0f;
            else if (v >= 1.0f) v = 1.0f;
            cell[x][y].f18 = ((float)cos(v * 3.1415927f) + 1.0f) * 0.45f + 0.1f;
        }
    }
    // ridges along the four borders
    float amp = (float)(rand() % 15817) * 0.00018966934f + 7.0f;
    int* lim = &h;
    for (int pass = 0; pass < 2; pass++) {
        for (int b = 0; b < 13; b++) {
            if (*lim >= 0) {
                float k = (float)sin((1.0f - (float)b * 0.07692308f) * 3.2f) * amp;
                for (int a = 0; a <= *lim; a++) {
                    int x1 = b, y1 = a, x2 = w - b, y2 = a;
                    if (pass != 0) {
                        x1 = a; y1 = b; x2 = a; y2 = h - b;
                    }
                    TCell* c1 = &cell[x1][y1];
                    float p1[3], n1[3];
                    p1[0] = off[0] + c1->x;
                    p1[1] = off[1] + c1->y;
                    p1[2] = off[2] + c1->z;
                    noise_fractal(40.0f, 0.6f, 4, 0, p1, n1);
                    float v1 = (n1[2] + n1[2] + 0.4f) * k;
                    if (c1->z < v1) { c1->z = v1; c1->f84 = v1; }
                    TCell* c2 = &cell[x2][y2];
                    float p2[3], n2[3];
                    p2[0] = off[0] + c2->x;
                    p2[1] = off[1] + c2->y;
                    p2[2] = off[2] + c2->z;
                    noise_fractal(40.0f, 0.6f, 4, 0, p2, n2);
                    float v2 = (n2[2] + n2[2] + 0.4f) * k;
                    if (c2->z < v2) { c2->z = v2; c2->f84 = v2; }
                    if (b == 0) { c1->f90 = 0; c2->f90 = 0; }
                }
            }
        }
        lim = &w;
    }
    // random brush strokes, coarse to fine
    int lv = (int)(flags >> 14 & (g_field_mask - 1));
    if (lv < 0) lv = 0;
    else if (lv > 99) lv = 99;
    float ratio = scale / ((float)pow(2.0, (double)((100 - lv) * 0.045f)) * 4.0f);
    if (ratio < 2.0f) ratio = 2.0f;
    int steps = (int)(log(ratio) * 1.442695f);
    float strength = 1.0f;
    for (int k = 0; k <= steps; k++) {
        float density;
        if (k == steps)
            density = (ratio - (float)g_bit[steps]) / (float)g_bit[steps];
        else
            density = 1.0f;
        seed_brushes(scale / (float)g_bit[k], density, strength);
        strength = strength - 0.05f;
    }
    // shift so the lowest cell sits at zero
    float lo = 10000.0f;
    for (int y = 0; y <= h; y++)
        for (int x = 0; x <= w; x++)
            if (cell[x][y].z < lo) lo = cell[x][y].z;
    for (int y = 0; y <= h; y++)
        for (int x = 0; x <= w; x++) {
            float v = -lo + cell[x][y].z;
            cell[x][y].z = v;
            cell[x][y].f84 = v;
        }
}

// ---- 0x471a30: is the sun visible from p? (segment from p towards the sun against the height field) ---------------
struct HeightField {
    bool line_clear(const vector4* a, const vector4* b);        // 0x436da0 (s00439100)
};
struct SunCtx {                            // the scene object (Ctx of s0042aaf0); only what is used here
    char pad0[0x64];
    HeightField* hf;                       // 0x64
    char pad68[0x300];
    void sample_lights(const vector4* p, vector4* dir, vector4* amb);   // 0x42ad60
};
extern int g_use_shaders;                  // 0x5e28fc (nonzero: no software visibility test)
struct TerrSun {
    char pad0[0xc3b0dc];
    SunCtx* world;                         // 0xc3b0dc
    float sun_visibility(const float* p);  // 0x471a30
};

// @ 0x471a30
float TerrSun::sun_visibility(const float* p)
{
    vector4 a(0.0f, 0.0f, 0.0f);
    vector4 d;
    {
        vector4 amb;                       // light colour: unused; its slot is shared with the scratch below
        world->sample_lights(&a, &d, &amb);
    }
    vector4 s;
    s.x = d.x * 1000.0f; s.y = d.y * 1000.0f; s.z = d.z * 1000.0f;
    a.x = p[0] - s.x; a.y = p[1] - s.y; a.z = p[2] - s.z;
    s.x = d.x * 0.05f; s.y = d.y * 0.05f; s.z = d.z * 0.05f;
    d.x = p[0] - s.x; d.y = p[1] - s.y; d.z = p[2] - s.z;
    bool vis = g_use_shaders != 0 || world->hf->line_clear(&d, &a);
    if (vis)
        return 1.0f;
    return 0.0f;
}

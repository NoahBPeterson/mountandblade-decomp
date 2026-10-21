// Slice s0046bc70 shared declarations (terrain object subset, see s00468fc0.h / s00464a60/terrain.h). NAMES PROVISIONAL.
#ifndef S0046BC70_H
#define S0046BC70_H
#include "rgl.h"

struct terrain_obj {
    char pad000[4];
    int f4;                     // 0x04 (sign bit tested)
    char pad008[0x278 - 8];
    int w;                      // 0x278
    int h;                      // 0x27c
    char pad280[4];
    int n284;                   // 0x284 number of extra flow passes
    char pad288[0xc3b0e0 - 0x288];
    float spawn_scale;          // 0xc3b0e0
    char pad_c3b0e4[0x843bd75 - 0xc3b0e4];
    char flag843bd75;           // 0x843bd75
    float noise_at(int x, int y);                                           // 0x465a70
    void set_cells(int bit, float v);                                       // 0x465e70
    void smooth_cells(int bit, float k);                                    // 0x465ee0
    void clear_flags(unsigned mask);                                        // 0x466480
    void trace_path(const int* from, const int* to, unsigned char bit, int* steps, float* minz, float w1, float w2, int unused, char record);   // 0x468fc0
    void spread_flow(unsigned mask, int mode, unsigned mark);              // 0x469a70
    void make_river();                                                      // 0x46bc70
};
extern unsigned g_mask_a;       // 0x5e2580
extern unsigned g_mask_b;       // 0x5e2584
extern int g_pow2_low;          // 0x5a8e70 (low dword of g_pow2[0] == 2)
#endif

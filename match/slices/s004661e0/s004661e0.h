// Layout of the world-map terrain object (provisional name `terrain`), recovered from s004661e0.
// Cells: cells[x][y] at +0x288, 251 y entries per x (stride 0x911c per x, 0x94 per y); extent w x h in
// +0x278 / +0x27c (inclusive indices 0..w, 0..h). Quads: quads[x][y] at +0x8e48fc, 250 per x, 0x38 each.
#ifndef TERRAIN_H
#define TERRAIN_H
#include "rgl.h"

struct terrain_cell {           // 0x94
    float px, py, pz;           // 0x00 position; pz is the height
    float pw;                   // 0x0c
    char f10[0x1c - 0x10];
    unsigned flags;             // 0x1c
    char f20[0x28 - 0x20];
    float f28;                  // 0x28
    char f2c[0x4c - 0x2c];
    float v4c, v50, v54;        // 0x4c/0x50/0x54
    char f58[0x84 - 0x58];
    float h2;                   // 0x84 copy of pz
    char f88[0x94 - 0x88];
    const rgl::vector4& pos() const { return *(const rgl::vector4*)this; }
    int kind();                 // 0x4646b0 (thiscall)
};

struct terrain_quad {           // 0x38
    int diag;                   // 0x00
    int tri_a;                  // 0x04
    int tri_b;                  // 0x08
    char f0c[0x38 - 0x0c];
};

struct terrain_obj {
    char pad000[0x58];
    int bit58;                  // 0x58 flag bit index or -1
    char pad5c[0x78 - 0x5c];
    int f78;                    // 0x78
    char pad7c[0x278 - 0x7c];
    int w;                      // 0x278
    int h;                      // 0x27c
    char pad280[8];
    terrain_cell cells[251 * 251];      // 0x288 .. index x * 251 + y
    terrain_cell& at(int x, int y) { return cells[x * 251 + y]; }
    terrain_quad quads[250 * 250];     // follows, index x * 250 + y
    terrain_quad& quad(int x, int y) { return quads[x * 250 + y]; }
    char pad_tail[0x843bd75 - 0xc3b0dc];   // quads end at 0xc3b0dc
    char flag843bd75;
    void smooth_heights();                  // 0x4661e0
    void clear_flags(unsigned mask);        // 0x466480
    void compute_weights();                 // 0x4664e0
    void pick_diagonals();                  // 0x4666b0
    void pick_triangle_types();             // 0x466930
};
#endif

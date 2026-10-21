// Slice s0046ad50 shared declarations. ALL NAMES PROVISIONAL.
#ifndef S0046AD50_H
#define S0046AD50_H
#include "rgl.h"
#include "tstring.h"

using rgl::vector4;

// ---- world-map terrain object (subset of s00468fc0.h / terrain.h) ------------------------------------------
struct terrain_cell {           // 0x94
    vector4 pos;                // 0x00 position (pz = height); w = ntype
    char f10[0x1c - 0x10];
    unsigned flags;             // 0x1c
    vector4 nrm;                // 0x20 accumulated vertex normal (x, y, z)
    char f30[0x94 - 0x30];
};
struct terrain_quad {           // 0x38
    int diag;                   // 0x00
    int tri[2];                 // 0x04 / 0x08
    vector4 nrm[2];             // 0x0c / 0x1c triangle normals
    int slot[2];                // 0x2c / 0x30
    float f34;                  // 0x34
};
struct terrain_obj {
    char pad000[0x278];
    int w;                      // 0x278
    int h;                      // 0x27c
    char pad280[8];
    terrain_cell cells[251 * 251];      // 0x288 .. index x * 251 + y
    terrain_cell& at(int x, int y) { return cells[x * 251 + y]; }
    terrain_quad quads[250 * 250];      // follows, index x * 250 + y
    terrain_quad& quad(int x, int y) { return quads[x * 250 + y]; }
    void triangle_corners(int x, int y, int flip, int* a, int* b, int* c);   // 0x466e20
    void compute_normals();     // 0x46ad50
};
vector4* tri_normal(vector4* out, const vector4* p);     // 0x4612d0 (cdecl, returns out)

// ---- items -----------------------------------------------------------------------------------------------------
struct ItemKind {               // 0x238 bytes (array at 0x5e29d0)
    char pad0[0x1c8];
    int type_flags;             // +0x1c8 (low byte = itp_type_*; bit 31 tested as a sign)
    char pad1cc[0x20c - 0x1cc];
    int f20c;                   // +0x20c
    char pad210[0x238 - 0x210];
};
extern ItemKind* g_items;       // 0x5e29d0
extern __int64 g_bit[];         // 0x5a8e68 (g_bit[i] == 1 << i)

struct ItemRef {                // 8 bytes: item index + packed amount/modifier word
    int item;
    unsigned m4;
    void reduce(int n);         // 0x437510
};
struct Troop {                  // 0x508 bytes
    char pad0[0x4a4];
    ItemRef equip[12];          // +0x4a4
    char pad504[0x508 - 0x504];
    bool fits_slot(ItemRef* ref, int slot);     // 0x462c80
};
struct World {
    char pad0[0x13a00];
    Troop* troops;              // +0x13a00 (stride 0x508)
};
extern World* g_world;          // 0x5e2930
#endif

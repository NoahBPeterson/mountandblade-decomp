// Terrain (scene ground) generator object, shared by slices s00464a60 / s004650f0 / s00465a70.
// Layout recovered from field offsets used by these functions; names are provisional.
// The object is huge (>0x843bd70 bytes): a 251x251 grid of 0x94-byte cells at +0x288, three scatter sets of
// 0x2800408 bytes at +0xc3b0e4 / +0x343b4ec / +0x5c3b8f4, then noise parameters.
#ifndef MB_TERRAIN_H
#define MB_TERRAIN_H
#include "rgl.h"
#include "../s0042b790/tstring.h"

using rgl::vector4;

void __cdecl noise_fractal(float freq, float gain, int octaves, int sharpen, const float* p, float* out);   // 0x414fc0
void mb_error(const char* fmt, ...);                                // 0x42cc80
int __cdecl rand_below(const int* n);                               // 0x432580

// ground spec record (table at 0x5e28f8, count 0x5e29e8, 0x20 bytes each; name at +0)
struct GroundSpec {
    tstring name;                    // +0
    __int64 flags;                   // +0x10 (bit 0x400000 / 0x80000 / 0x1000000 / ... and a 16-bit field at +0x14)
    int f18, f1c;
};
extern GroundSpec* g_ground_specs;   // 0x5e28f8
extern int g_ground_spec_count;      // 0x5e29e8

// ground type record (table of 11 at 0x8ed4df8, 0x20 bytes each)
struct GroundType {
    tstring name;                    // +0
    int f10, f14, f18, f1c;
};
extern GroundType g_ground_types[11];   // 0x8ed4df8

extern int g_type_mask;              // 0x5a8e88 (terrain type field is 4 bits wide: value & (mask - 1))
extern int g_use_shaders;            // 0x5e28fc

struct TerrainCell {                 // 0x94 bytes
    float f[4];                      // +0   (x, y, z, w)
    char pad10[0xc];
    unsigned mask;                   // +0x1c
    char pad20[0x84 - 0x20];
    float f84;                       // +0x84
    char pad88[0x94 - 0x88];
};

struct ScatterItem {                 // 0x20 bytes
    int kind;                        // +0   index into g_ground_specs
    float f4, f8;                    // +4, +8
    vector4 v;                       // +0xc
    float f1c;                       // +0x1c
};
struct ScatterSet {                  // 0x2800408 bytes
    char pad[0x2800004];
    int count;                       // +0x2800004
    ScatterItem items[32];           // +0x2800008
};

// terrain descriptor filled by Terrain::fill_desc_named / fill_desc_none
struct TerrainDescEntry { tstring name; int idx; int f14; };      // 0x18
struct TerrainDesc {
    int mode;                               // +0
    int elev_idx;                           // +4
    int level_idx;                          // +8
    TerrainDescEntry e[13];                 // +0xc
    int w, h;                               // +0x144, +0x148
    void init();                            // 0x464400
};
extern const char* g_name_elevation;        // 0x5a9bf4 "ground_elevation"
extern const char* g_name_leveling;         // 0x5a9bf8 "ground_leveling"
// six hex words parsed from a "0x..." string, 8 digits at a time from the right (ctor 0x4658c0)
struct HexWords {
    int v[6];
    void parse(const tstring& s);           // 0x4658c0 (zeroes v[], then fills it)
};
extern int g_terrain_gate;                  // 0x5aa03c

struct TerrainLayer {                // 0x28 bytes, from +0x28
    int type;                        // +0   index into g_ground_types (-1 = none)
    int f4;                          // +4
    int f8;                          // +8
    int fc;                          // +0xc
    int f10;                         // +0x10
    int f14;                         // +0x14
    char pad18[0x28 - 0x18];
};

struct Terrain {
    char pad0[0x10];
    unsigned flags;                  // +0x10  bits 7..13, 21..27, 28..31 are fields
    char pad14[0x28 - 0x14];
    TerrainLayer layer[14];          // +0x28 (a 15th slot index at +0x258 is written by init_layers)
    char pad_a[0x278 - 0x28 - 14 * 0x28];
    int w;                           // +0x278
    int h;                           // +0x27c
    char pad280[8];
    TerrainCell cell[251][251];      // +0x288  [x][y]
    char pad_b[0xc3b0e0 - 0x288 - 251 * 251 * 0x94];
    float scale;                     // +0xc3b0e0
    ScatterSet set1;                 // +0xc3b0e4
    ScatterSet set0;                 // +0x343b4ec
    ScatterSet set2;                 // +0x5c3b8f4
    char pad_c[0x843bd4c - 0x843bcfc];
    float noise_seed;                // +0x843bd4c
    float off_a[3];                  // +0x843bd50
    float pad_d;
    float off_b[3];                  // +0x843bd60

    void set_field_a(const float* p);                     // 0x464a60
    void set_field_b(const float* p);                     // 0x464ac0
    int* cell_of(int* out, const float* p);               // 0x464b40
    float spec_weight(int* spec, int* type, int* mode);   // 0x464bf0
    vector4* ground_color(vector4* out, const float* pos);    // 0x464d50
    void init_layers();                                   // 0x464f10
    void scatter(ScatterSet* set, int mode, int n);       // 0x4650f0
    void scatter_all();                                   // 0x465540
    float noise_at(int x, int y);                         // 0x465a70
    vector4 sample(const float* p);                       // 0x465af0
    void fill_desc_named(TerrainDesc* d);                 // 0x4656c0
    void fill_desc_none(TerrainDesc* d);                  // 0x4657c0
    void set_cells(int bit, float v);                     // 0x465e70
    void smooth_cells(int bit, float k);                  // 0x465ee0
};
#endif

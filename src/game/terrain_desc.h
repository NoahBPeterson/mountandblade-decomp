// terrain_desc.h: the sco terrain description (0x1c8 bytes; Mission +0x63d0 in terrain_b, passed to
// Terrain::fill_desc_* in terrain_a). Layers and dimensions: terrain_a.cpp 0x464200..0x464440 (read, init) and
// Terrain::fill_desc_named 0x4656c0 / fill_desc_none 0x4657c0; the play-area rectangle and ground height are read by
// terrain_b (0x479e20, 0x47a0c0, 0x473a60). One definition for both modules.
#ifndef GAME_TERRAIN_DESC_H
#define GAME_TERRAIN_DESC_H
#include "./tstring.h"

struct stream_base;

struct Dims { int w, h; };
struct TerrainDescEntry {                       // 0x18 bytes
    tstring name;                               // +0
    int type;                                   // +0x10 (-1: none; -0x1e71 elevation, -0x3115 leveling)
    float* data;                                // +0x14 (420 x 420 floats, index x * 420 + y)
    void read(stream_base* s, Dims* d);         // 0x464200
};
struct TerrainDesc {
    int n;                                      // +0 (layers in use, max 13)
    int n_minus2;                               // +4 index of the elevation entry after fill_desc
    int n_minus1;                               // +8 index of the leveling entry
    TerrainDescEntry layers[13];                // +0xc
    Dims dims;                                  // +0x144 / +0x148
    char pad14c[0x158 - 0x14c];
    unsigned char f158;                         // +0x158 mission flag byte (Mission +0x6528; 0x448eb0 tests bit 0, 0x4556d0)
    char pad159[0x16c - 0x159];
    float min_x, min_y, max_x, max_y;           // +0x16c play-area rectangle (0x479e20)
    float base_z;                               // +0x17c ground height (0x47a0c0)
    int f180;                                   // +0x180 (0x473a60)
    char f184;                                  // +0x184 (Mission +0x6554; 0x4465e0, 0x448eb0)
    bool muted;                                 // +0x185 (0x4435c0)
    char pad186[0x1c8 - 0x186];
    void init();                                // 0x464400
    void read(stream_base* s);                  // 0x464440
};
typedef char TerrainDesc_size_check[sizeof(TerrainDesc) == 0x1c8 ? 1 : -1];

#endif // GAME_TERRAIN_DESC_H

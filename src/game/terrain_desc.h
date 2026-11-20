// terrain_desc.h: the sco terrain description (0x14c bytes; Mission +0x63d0 in terrain_b, passed to
// Terrain::fill_desc_* in terrain_a). Layers and dimensions: terrain_a.cpp 0x464200..0x464440 (read, init) and
// Terrain::fill_desc_named 0x4656c0 / fill_desc_none 0x4657c0; constructor 0x4be6b0 / destructor 0x4be6f0 (config/config.cpp).
// One definition for both modules. (The mission fields after it, +0x6520.., are Mission's own: mission.h.)
#ifndef GAME_TERRAIN_DESC_H
#define GAME_TERRAIN_DESC_H
#include "./tstring.h"
#include "../include/layout32.h"

struct stream_base;

struct Dims { int w, h; };
struct TerrainDescEntry {                       // 0x18 bytes
    tstring name;                               // +0
    int type;                                   // +0x10 (-1: none; -0x1e71 elevation, -0x3115 leveling)
    float* data;                                // +0x14 (420 x 420 floats, index x * 420 + y)
    TerrainDescEntry() : data(0) {}             // inline in TerrainDesc's constructor (0x4be6b0)
    ~TerrainDescEntry() { if (data) delete[] data; }   // inline in TerrainDesc's destructor (0x4be6f0)
    void read(stream_base* s, Dims* d);         // 0x464200
    void save(stream_base* s, Dims* d);         // 0x528060 (mission5/mission_5.cpp: run-length encodes the layer)
    // the editor brush's value block (mission5/mission_5.cpp; data is 0x1a4 x 0x1a4 values, index x * 0x1a4 + y)
    __declspec(noinline) void clear();                              // 0x52ce00 zero the block
    __declspec(noinline) void put(int* ix, int* iy, int* value);    // 0x52ce70 allocate (filled with -1) on first use, store one cell
};
struct TerrainDesc {
    int n;                                      // +0 (layers in use, max 13)
    int n_minus2;                               // +4 index of the elevation entry after fill_desc
    int n_minus1;                               // +8 index of the leveling entry
    TerrainDescEntry layers[13];                // +0xc
    Dims dims;                                  // +0x144 / +0x148
    TerrainDesc();                              // 0x4be6b0 (config/config.cpp)
    ~TerrainDesc();                             // 0x4be6f0
    void init();                                // 0x464400
    void read(stream_base* s);                  // 0x464440
    void save(stream_base* s);                  // 0x528000 (mission5/mission_5.cpp: the layer count, the dimensions, every layer)
};
MB_LAYOUT32_ASSERT(TerrainDesc_size_check, sizeof(TerrainDesc) == 0x14c);

#endif // GAME_TERRAIN_DESC_H

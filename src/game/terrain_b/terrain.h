// terrain.h: the terrain object (a ~140 MB global object): world extent, the 251 x 251 grid of 0x94-byte cells
// (height, relief weight, sun term), the pointer to the scene container and the sun light; plus the world-map
// particle lists it lights. Offsets are read or written by terrain_b.cpp (cited); the rest belongs to
// terrain_a's functions. Names provisional (the slices called the object Terr, TerrSun, Terrain, terrain_obj2).
#ifndef TERRAIN_B_TERRAIN_H
#define TERRAIN_B_TERRAIN_H
#include "../../rgl/rgl_math.h"
#include "../../rgl/mesh.h"
#include "./cell_grid.h"
#include "./entity_rec.h"
#include "../terrain_a/terrain.h"          // Terrain, particle, particle_list, terrain_cell (shared with terrain_a)

// ---- world-map particles lit by Terrain::light_particle -----------------------------------------------------------

struct DustEntry {                              // 0xa0 bytes (0x47a460)
    float x, y, z;
    char pad0c[0xa0 - 0x0c];
};
struct Entity;
struct res_shape;
struct SpawnEntry {                             // 0xa0 bytes: one prop spawn (0x479da0)
    int type;                                   // +0x00 index into g_ptypes
    char pad4[0x10];
    rgl::matrix m;                              // +0x14
    char pad54[0x10];
    int counter;                                // +0x64
    char pad68[0xa0 - 0x68];
};
struct SpawnSet { int count; SpawnEntry entries[1]; };      // +0x00 count, +0x04 entries


// refresh_cells reaches sun_visibility through the same address without seeing its body (the slices compiled them
// apart): cl must not assume its register use.
struct TerrainExt {
    float sun_visibility(const float* p);                           // 0x471a30
};

#endif // TERRAIN_B_TERRAIN_H

// terrain.h: the terrain object (a ~140 MB global object): world extent, the 251 x 251 grid of 0x94-byte cells
// (height, relief weight, sun term), the pointer to the scene container and the sun light; plus the world-map
// particle lists it lights. Offsets are read or written by terrain_b.cpp (cited); the rest belongs to
// terrain_a's functions. Names provisional (the slices called the object Terr, TerrSun, Terrain, terrain_obj2).
#ifndef TERRAIN_B_TERRAIN_H
#define TERRAIN_B_TERRAIN_H
#include "../../rgl/rgl_math.h"
#include "../../rgl/mesh.h"
#include "./cell_grid.h"
#include "../terrain_a/terrain.h"          // Terrain, particle, particle_list, terrain_cell (shared with terrain_a)

#endif // TERRAIN_B_TERRAIN_H

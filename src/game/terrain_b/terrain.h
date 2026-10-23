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

struct TCell {                                  // 0x94 bytes
    float x, y, z;                              // 0x00 (0x4713d0)
    char pad0c[0x18 - 0x0c];
    float f18;                                  // 0x18 relief weight written by the noise pass (0x4713d0)
    char pad1c[0x40 - 0x1c];
    float f40;                                  // 0x40 sun visibility (0x473980)
    char pad44[0x84 - 0x44];
    float f84;                                  // 0x84 copy of z (0x4713d0)
    char pad88[0x90 - 0x88];
    int f90;                                    // 0x90 (0x4713d0)
};

// ---- world-map particles lit by Terrain::light_particle -----------------------------------------------------------
struct res_shape;
struct AnimBox {                                // 0x48 (one frame / element of a type; mission.cpp: spawn_bodies reads res)
    char pad0[0x20];
    res_shape* res;                             // +0x20 (0x479da0)
    char pad24[4];
    float mn[3];                                // +0x28
    float pad34;
    float mx[3];                                // +0x38
    char pad44[0x48 - 0x44];
};
struct ptype {                                  // 0x20, table at *0x5e28f8
    char pad0[0x10];
    __int64 flags;                              // +0x10
    int nframes;                                // +0x18 (0x479da0: count)
    AnimBox* boxes;                             // +0x1c: nframes boxes of 0x48 bytes
};
struct particle {                               // 0xa0
    int type;                                   // 0x00
    int cx, cy;                                 // 0x04 / 0x08
    int f0c;                                    // 0x0c
    char pad10[0x14 - 0x10];
    rgl::matrix frame;                          // 0x14
    rgl::vector4 dir;                           // 0x54
    int anim;                                   // 0x64
    char pad68[0x6c - 0x68];
    rgl::mesh* meshes[4];                       // 0x6c
    int first[4];                               // 0x7c: vertex range [first, last) of each mesh
    int last[4];                                // 0x8c
    int nmesh;                                  // 0x9c
};
struct particle_list {
    int count;
    particle items[1];                          // really 0x40000
};
struct Probe {
    bool test(unsigned p, float* out);          // 0x468c80
};

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

struct Terrain {
    char pad0[0x10];
    unsigned flags;                             // 0x10 (bits 14..: terrain level field) (0x4713d0)
    char pad14[0x270 - 0x14];
    float wx, wy;                               // 0x270 / 0x274 world extent
    int w, h;                                   // 0x278 / 0x27c last cell index in x / y
    char pad280[8];
    TCell cell[251][251];                       // 0x288 [x][y]
    char padc3b0e0[0xc3b0dc - 0x288 - 251 * 251 * sizeof(TCell)];
    CellGrid* world;                            // 0xc3b0dc (0x471a30, 0x471b30)
    char pad_c3b0e0[0x5c3b8f4 - 0xc3b0e0];
    int dust_count;                             // 0x5c3b8f4 (0x47a460)
    char pad_5c3b8f8[0x5c3b93c - 0x5c3b8f8];
    DustEntry dust[1];                          // 0x5c3b93c dust particles (more than one entry)
    char pad_dust_end[0x843bd0c - 0x5c3b93c - sizeof(DustEntry)];
    rgl::vector4 sun;                           // 0x843bd0c direction of the light (0x471b30)
    char pad_843bd1c[0x843bd2c - 0x843bd1c];
    rgl::vector4 sun_amb;                       // 0x843bd2c
    rgl::vector4 sun_dif;                       // 0x843bd3c
    float noise_seed;                           // 0x843bd4c (0x4713d0)
    char pad_843bd50[0x843bd78 - 0x843bd50];
    Entity* gridA[0x30][0x30];                  // 0x843bd78 scene object of each dust cell, [inner][outer] (0x47a460)
    Entity* gridB[0x30][0x30];                  // 0x843e178
    rgl::aabb boxes[0x30][0x30];                     // 0x8440578 per-cell dust bounds
    char pad_845d978[4];
    int n_inner;                                // 0x845d97c cells along the first axis
    int n_outer;                                // 0x845d980 cells along the second axis
    void seed_brushes(float size, float density, float scale);      // 0x46baf0
    void generate_heights();                                        // 0x4713d0
    float sun_visibility(const float* p);                           // 0x471a30
    void light_particle(particle_list* l, int idx, bool trace);     // 0x471b30
    void light_all(particle_list* l, bool trace);                   // 0x472020
    void refresh_cells();                                           // 0x473980
    void spawn_bodies(SpawnSet* r);                                 // 0x479da0
    void make_grid();                                               // 0x47a460
};

// refresh_cells reaches sun_visibility through the same address without seeing its body (the slices compiled them
// apart): cl must not assume its register use.
struct TerrainExt {
    float sun_visibility(const float* p);                           // 0x471a30
};

#endif // TERRAIN_B_TERRAIN_H

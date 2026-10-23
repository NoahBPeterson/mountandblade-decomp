// terrain.h: the world-map terrain object (a ~140 MB global object: 251 x 251 cells of 0x94 bytes at +0x288, the
// 250 x 250 quads of 0x38 bytes that follow them, three particle lists, the sun and the chunk entities) and the
// records around it that terrain_a.cpp (0x464200..0x46e590) reads: the sco terrain description, the ground-type and
// particle-type tables, the packed bit-field helpers. One layout per class; every offset cites the function that
// proves it. The slices called the object Terrain (s00464a60, s004650f0, s00465a70, s00468da0, s0046b6e0) and
// terrain_obj (s004661e0 .. s0046bc70): both are kept as the two class names of the symbols (terrain_obj derives from
// Terrain, no vtable, no new members), so the mangled names of the byte-exact rows are unchanged.
// src/game/terrain_b declares a smaller Terrain/ShadowMap/particle of its own for 0x46e590+ (see MIGRATION.md).
#ifndef TERRAIN_A_TERRAIN_H
#define TERRAIN_A_TERRAIN_H
#include <stdlib.h>
#include <math.h>
#include <new>
#include "./common.h"
#include "../tstring.h"
#include "../../rgl/stream.h"

#include "../../include/vec.h"
#include "../../include/mesh_prims.h"       // rgl::aabb
namespace rgl { struct mesh; }
struct CellGrid;                                // terrain_b/cell_grid.h: the scene container (Terrain::world seen by terrain_b)
struct Entity;                                  // terrain_b/entity_rec.h: scene entity (dust cell objects)
struct SpawnSet;                                // terrain_b/terrain.h
struct ivec2 { int dx, dy; };
extern ivec2 g_nbr[];                           // 0x591340 the eight neighbour offsets (0x4666b0, 0x468fc0)

// ---- tables ---------------------------------------------------------------------------------------------------
// ground type record (table of 11 at 0x8ed4df8, 0x20 bytes each): name, then the values copied into a layer
struct GroundType {
    tstring name;                               // +0 (0x464200, 0x4656c0)
    int f10, f14, f18, f1c;                     // +0x10.. (0x464f10: fc <- f10, f14 <- f18, f4 <- f1c; 0x467250)
};
extern GroundType g_ground_types[11];           // 0x8ed4df8 (beyond the annotation range: bound by symbols/slices/terrain_a.txt)

// particle / ground-spec type record (table at *0x5e28f8, count 0x5e29e8, 0x20 bytes each)
struct res_shape;
struct AnimBox {                                // 0x48 (one frame / element of a type; terrain_b mission.cpp: spawn_bodies reads res)
    char pad0[0x20];
    res_shape* res;                             // +0x20 (0x479da0)
    char pad24[4];
    float mn[3];                                // +0x28
    float pad34;
    float mx[3];                                // +0x38
    char pad44[0x48 - 0x44];
};
struct ptype {
    tstring name;                               // +0 (0x4650f0)
    __int64 flags;                              // +0x10 (0x400000 / 0x80000 / 0x1000000 ...; the high dword's low 16 bits are a density)
    int nframes;                                // +0x18 (0x466cd0, 0x46a270; terrain_b 0x479da0: count of boxes)
    union {
        int f1c;                                // +0x1c
        AnimBox* boxes;                         // +0x1c: nframes boxes of 0x48 bytes (terrain_b)
    };
};
extern ptype* g_ptypes;                         // 0x5e28f8
extern int g_ptype_count;                       // 0x5e29e8

extern unsigned g_field_mask;                   // 0x5a8ea0 (field values are clamped to g_field_mask - 1)
extern unsigned g_sel_mask;                     // 0x5a8e78 (0x46a270: second flag field)
extern int g_type_mask;                         // 0x5a8e88 (terrain type field is 4 bits wide: value & (mask - 1))
extern int g_use_shaders;                       // 0x5e28fc
extern int g_terrain_gate;                      // 0x5aa03c
extern unsigned g_river_mask_a;                 // 0x5e2580 (0x46bc70: flag bit of the river path)
extern unsigned g_river_mask_b;                 // 0x5e2584
extern int g_pow2_low;                          // 0x5a8e70 (low dword of g_pow2[0] == 2)
extern unsigned __int64 g_pow2[];               // 0x5a8e70 (g_pow2[n] = 2 << n)
extern const char* g_name_elevation;            // 0x5a9bf4 "ground_elevation"
extern const char* g_name_leveling;             // 0x5a9bf8 "ground_leveling"

// ---- one terrain cell (0x94 bytes) ----------------------------------------------------------------------------
// w[1..14] are the per-layer weights (0x4646b0 `kind` loops over +0x4c..+0x80); w[0] is the "none" weight (1.0 after
// init); the sco layers 0..10 land in w[4..14] (0x467250 stores them at +0x58 + 4 * layer).
struct v4 { float x, y, z, w; };               // POD stand-in of rgl::vector4 (unions cannot hold a class with a constructor)
struct terrain_cell {
    float px, py, pz;                           // +0 position (0x4644e0 copies a vector4 here); pz is the height (0x4661e0)
    int ntype;                                  // +0x0c (the vector4's 4th lane)
    int f10, f14;                               // +0x10 / +0x14 (-1 after init, 0x4644e0)
    float weight;                               // +0x18 (1.0 after init; scales the brush, 0x468da0)
    unsigned flags;                             // +0x1c (layer bits; 0.0f bits after init, 0x4644e0)
    v4 nrm;                                     // +0x20 accumulated vertex normal; nrm.z is the slope term of 0x4664e0
    union {
        struct { vec2 disp; vec2 dir; };        // +0x30 / +0x38 flow displacement and unit direction (0x468fc0, 0x469a70)
        v4 v30;                                 // +0x30 (0x4644e0: (x * 0.7, y * 0.7, 0, 1))
    };
    float f40;                                  // +0x40 sun visibility (terrain_b 0x473980)
    int order;                                  // +0x44 step number when the path walk visited it (-1 after init)
    union {
        float w[15];                            // +0x48 layer weights
        struct { float w0, v4c, v50, v54; int layer[11]; };     // +0x48; v4c / v50 / v54 are w[1..3]; layer[i] = +0x58 + 4 i
    };
    float h2;                                   // +0x84 copy of the height (0x4661e0)
    float f88;                                  // +0x88 height offset of the elevation layer (0x467250)
    int f8c;                                    // +0x8c leveling layer value (0x467250)
    float f90;                                  // +0x90 (1.0 after init)

    rgl::vector4& P() { return *(rgl::vector4*)this; }          // position as a vector4
    rgl::vector4& N() { return *(rgl::vector4*)&nrm; }          // vertex normal as a vector4
    rgl::vector4& V30() { return *(rgl::vector4*)&v30; }
    int kind();                                 // 0x4646b0 (thiscall): the heaviest layer (> 0.4) or 0
    void init(const float* p4);                 // 0x4644e0
    bool visible(int i);                        // 0x4645b0
};

// ---- one terrain quad (0x38 bytes) -----------------------------------------------------------------------------
struct terrain_quad {
    int diag;                                   // +0 (0x4666b0)
    int tri[2];                                 // +4 / +8 triangle kinds (0x466930)
    rgl::vector4 nrm[2];                        // +0xc / +0x1c triangle normals (0x46ad50); nrm[0].z / nrm[1].z are read as f14 / f24 by 0x467530
    int slot[2];                                // +0x2c / +0x30 (-1 after init and in 0x46a270)
    float f34;                                  // +0x34 (0x466aa0)
    void init();                                // 0x464760
};

// ---- scene objects the terrain creates / fills ------------------------------------------------------------------
struct TEntity {                                // stub of the scene entity (vtable 0x54b460 family); slot 6 takes a pointer to an int
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4(); virtual void s5();
    virtual void s6(int* p);
    char pad[0xc4 - 4];
    rgl::vector4 pos;                           // +0xc4 (0x466fa0)
};
struct SceneWorld { char pad0[0x9c]; TEntity* create_entity(int id); };     // 0x428440

struct TerrainParticle {                               // 0xa0
    int type;                                   // +0x00 index into g_ptypes
    int cx, cy;                                 // +0x04 / +0x08 source cell
    int f0c;                                    // +0x0c
    char f10[0x14 - 0x10];
    rgl::matrix frame;                          // +0x14 copy of the source frame
    rgl::vector4 dir;                           // +0x54
    int anim;                                   // +0x64
    char f68[0x6c - 0x68];
    rgl::mesh* meshes[4];                       // +0x6c particle meshes (terrain_b: Terrain::light_particle 0x471b30, spawn 0x47a460)
    int first[4];                               // +0x7c vertex range [first, last) of each mesh
    int last[4];                                // +0x8c
    int nmesh;                                  // +0x9c (terrain_a: f9c)
};
struct scatter_src {                            // 0x20, source records after the particles (0x4650f0 writes, 0x46a270 reads)
    int kind;                                   // +0x00 index into g_ptypes
    float freq, thresh;                         // +0x04 / +0x08
    rgl::vector4 v;                             // +0x0c
    float rate;                                 // +0x1c
};
struct particle_list {                          // 0x2800408
    int count;
    TerrainParticle items[0x40000];
    int nsrc;                                   // +0x2800004
    scatter_src src[32];                        // +0x2800008
};

// ---- the sco terrain description (0x464200..0x4644e0) --------------------------------------------------------
#include "../terrain_desc.h"                 // Dims, TerrainDescEntry, TerrainDesc (shared with terrain_b)

// six hex words parsed from a "0x..." string, 8 digits at a time from the right (0x4658c0)
struct HexWords {
    int v[6];
    void parse(const tstring& s);               // 0x4658c0
};

// bit-field accessors: word array at this, widths from the power-of-two table at 0x5a8e70
struct BitWords {
    unsigned w[8];
    void set(const unsigned& lo, const int& hi, const unsigned& v);   // 0x464960
    unsigned get(const unsigned& lo, const int& hi);                  // 0x4649b0
};
struct Pct {
    char pad0[0x10];
    unsigned bits;                              // +0x10
    void set_pct(const float& f);               // 0x4649f0
};

// ---- the terrain layers (0x28 bytes each, from +0x28) ------------------------------------------------------------
struct TerrainLayer {
    int type;                                   // +0 index into g_ground_types (-1 = none)
    int f4;                                     // +4
    int f8;                                     // +8
    int fc;                                     // +0xc
    int f10;                                    // +0x10
    int f14;                                    // +0x14
    char pad18[0x28 - 0x18];
};
struct tlayer {                                 // the same records as 0x467250 sees them: the layer of sco index i starts 0x10 bytes early
    char pad0[0x10];
    int type;                                   // +0x10
    int f14;                                    // +0x14
    int f18;                                    // +0x18
    int f1c;                                    // +0x1c
    char pad20[4];
    int f24;                                    // +0x24
};
struct layer_set {                              // set of sco layers handed to load_layer (0x467250); entry i: type at set[i * 6 + 7], data at set[i * 6 + 8]
    int count;
    int hdr[6];
    struct { int type; int* data; int pad[4]; } items[1];
};

// ---- the object -----------------------------------------------------------------------------------------------------
struct Terrain {
    char pad0[4];
    int f4;                                     // +4 (sign bit tested by 0x46bc70)
    char pad8[0x10 - 8];
    unsigned flags;                             // +0x10 bits 7..13 / 21..27 / 28..31 are fields (0x464a60, 0x464ac0, 0x464d50)
    unsigned flags14;                           // +0x14 bits 2.. are a field (0x46a270)
    char pad18[0x28 - 0x18];
    union {
        TerrainLayer layer[14];                 // +0x28 (0x464f10); layer[1].f8 (+0x58) is the path flag bit or -1, layer[2].type (+0x78) the second type (0x4664e0)
        struct { char pad_l[0xb8 - 0x28]; tlayer layers[11]; };     // +0xb8 (0x467250)
    };
    float wx, wy;                               // +0x270 / +0x274 world extent (0x46baf0)
    int w;                                      // +0x278 last cell index in x (0x464b40)
    int h;                                      // +0x27c last cell index in y
    char pad280[4];
    int n284;                                   // +0x284 number of extra flow passes (0x46bc70)
    union {
        terrain_cell cell[251][251];            // +0x288 [x][y]
        terrain_cell cells[251 * 251];          // index x * 251 + y
    };
    terrain_quad quads[250 * 250];              // +0x8e48fc index x * 250 + y
    union {
        SceneWorld* world;                      // +0xc3b0dc (0x466fa0)
        CellGrid* grid;                         // the same pointer as terrain_b types it (0x471a30, 0x471b30, 0x47a460)
    };
    float scale;                                // +0xc3b0e0 world size of a cell (0x464b40, 0x468da0, 0x466c10, 0x46a270)
    particle_list list_a;                       // +0xc3b0e4 mode 1 (0x4650f0: scatter_all passes &set1)
    particle_list list_b;                       // +0x343b4ec mode 0, type flag 0x400000
    particle_list list_c;                       // +0x5c3b8f4 mode 2, type flag 0x80000
    char pad_843bcfc[0x843bd0c - 0x843bcfc];
    float sun_dir[3];                           // +0x843bd0c (0x466ed0)
    char pad_843bd18[0x843bd2c - 0x843bd18];
    rgl::vector4 sun_ambient;                   // +0x843bd2c
    float sun_diffuse[3];                       // +0x843bd3c
    char pad_843bd48[4];
    float noise_seed;                           // +0x843bd4c (0x465a70)
    float off_a[3];                             // +0x843bd50 (0x465a70)
    float pad_d;
    float off_b[3];                             // +0x843bd60 (0x464d50)
    char pad_843bd6c[4];
    int block;                                  // +0x843bd70 cells per chunk edge (0x466fa0)
    char pad_843bd74;
    char flag843bd75;                           // +0x843bd75 (0x4664e0, 0x46bc70)
    char pad_843bd76[0x843bd78 - 0x843bd76];
    Entity* gridA[0x30][0x30];                  // +0x843bd78 scene object of each dust cell, [inner][outer] (0x47a460)
    Entity* gridB[0x30][0x30];                  // +0x843e178
    rgl::aabb boxes[0x30][0x30];                // +0x8440578 per-cell dust bounds (rgl::aabb is 0x34 bytes: ends at 0x845d978)
    char pad_845d978[4];
    int n_inner;                                // +0x845d97c cells along the first axis (0x47a460)
    int n_outer;                                // +0x845d980 cells along the second axis
    int nx, ny;                                 // +0x845d984 / +0x845d988 (0x466fa0)
    TEntity* ents[25];                          // +0x845d98c chunk entities, index i * 5 + j
    int arr2[25];                               // +0x845d9f0 (cleared with ents)
    int arr3[15][25];                           // +0x845da54

    // ---- methods of the first Terrain slices
    void set_field_a(const float* p);                       // 0x464a60
    void set_field_b(const float* p);                       // 0x464ac0
    int* cell_of(int* out, const float* p);                 // 0x464b40
    float spec_weight(int* spec, int* type, int* mode);     // 0x464bf0
    rgl::vector4* ground_color(rgl::vector4* out, const float* pos);    // 0x464d50
    void init_layers();                                     // 0x464f10
    void scatter(particle_list* set, int mode, int n);      // 0x4650f0
    void scatter_all();                                     // 0x465540
    void fill_desc_named(TerrainDesc* d);                   // 0x4656c0
    void fill_desc_none(TerrainDesc* d);                    // 0x4657c0
    float noise_at(int x, int y);                           // 0x465a70
    rgl::vector4 sample(const float* p);                    // 0x465af0
    void set_cells(int bit, float v);                       // 0x465e70
    void smooth_cells(int bit, float k);                    // 0x465ee0
    void apply_brush(const float* center, float radius, float strength);    // 0x468da0
    void seed_brushes(float size, float density, float scale);              // 0x46baf0
    void reset();                                           // 0x4647e0
    void generate_heights();                                // 0x4713d0 (terrain_b.cpp)
    float sun_visibility(const float* p);                   // 0x471a30
    void light_particle(particle_list* l, int idx, bool trace);     // 0x471b30
    void light_all(particle_list* l, bool trace);           // 0x472020
    void refresh_cells();                                   // 0x473980
    void spawn_bodies(SpawnSet* r);                         // 0x479da0 (mission.cpp)
    void make_grid();                                       // 0x47a460 (mission.cpp)
    float rand_avg4();                                      // 0x4659e0 (does not use this; seed_brushes calls it with ecx = this)
    terrain_cell& at(int x, int y) { return cells[x * 251 + y]; }
    terrain_quad& quad(int x, int y) { return quads[x * 250 + y]; }
};

#include <stddef.h>
#define TERRAIN_AT(m, off) typedef char Terrain_##m##_at[(offsetof(Terrain, m) == (off)) ? 1 : -1]
TERRAIN_AT(wx, 0x270); TERRAIN_AT(cell, 0x288); TERRAIN_AT(world, 0xc3b0dc); TERRAIN_AT(list_a, 0xc3b0e4);
TERRAIN_AT(list_c, 0x5c3b8f4); TERRAIN_AT(sun_dir, 0x843bd0c); TERRAIN_AT(block, 0x843bd70); TERRAIN_AT(gridA, 0x843bd78);
TERRAIN_AT(boxes, 0x8440578); TERRAIN_AT(n_inner, 0x845d97c); TERRAIN_AT(nx, 0x845d984); TERRAIN_AT(ents, 0x845d98c);
TERRAIN_AT(arr3, 0x845da54);

struct terrain_obj : Terrain {
    void smooth_heights();                  // 0x4661e0
    void clear_flags(unsigned mask);        // 0x466480
    void compute_weights();                 // 0x4664e0
    void pick_diagonals();                  // 0x4666b0
    void pick_triangle_types();             // 0x466930
    void accumulate_kinds();                // 0x466aa0
    void erode();                           // 0x466c10
    void emit_particle(particle_list* l, int type, const rgl::matrix* frame, const rgl::vector4* dir, int anim);   // 0x466cd0
    void emit_by_type(int type, const rgl::matrix* frame, const rgl::vector4* dir, int anim);                     // 0x466d90
    void triangle_corners(int x, int y, int flip, int* a, int* b, int* c);   // 0x466e20
    void sun_light(rgl::vector4* out, const float* dir, const float* scale); // 0x466ed0
    void create_chunk_entities();           // 0x466fa0
    void cell_of(int* out, const float* p); // 0x464b40 (the same function as Terrain::cell_of, seen with a void result by 0x466cd0)
    void load_layer(int* set, int idx);     // 0x467250
    int count_low_quads();                  // 0x467530
    void trace_path(const int* from, const int* to, unsigned char bit, int* steps, float* minz, float w1, float w2, int unused, char record);   // 0x468fc0
    void spread_flow(unsigned mask, int mode, unsigned mark);       // 0x469a70
    void spawn_particles(particle_list* l, int unused);             // 0x46a270
    void compute_normals();                 // 0x46ad50
    void make_river();                      // 0x46bc70
};

#endif // TERRAIN_A_TERRAIN_H

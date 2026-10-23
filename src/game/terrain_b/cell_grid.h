// cell_grid.h: the scene container the mission points at (Mission::scene, +0) and the terrain object calls into
// (Terrain::world): a 2D grid of per-cell id lists, two free lists, the object list, the height field, the ground
// nav data, the wind objects, the view / shadow-camera block and the light sampling entry points. The slices
// called this one 0x344-byte object CellGrid, NavStore, Scene, World, Ctx, SceneW, SunCtx, LightCtx and Level;
// src/rgl/scene/world.h declares the same class as rgl::world (constructor 0x478ad0 there is world::world).
// Offsets are the ones read or written by a function of terrain_b.cpp / mission.cpp (cited at each field).
#ifndef TERRAIN_B_CELL_GRID_H
#define TERRAIN_B_CELL_GRID_H
#include <stddef.h>
#include <vector>
#include <list>
#include "../../rgl/rgl_math.h"
#include "./frustum.h"
#include "./poly_mesh.h"

struct face;
struct Entity;

// ---- height field (CellGrid::hf, +0x64): 16-byte cells in columns -------------------------------------------------
#include "../../include/height_field.h"      // HCell, HeightField (cols 0x471b30, 0x4785e0; line_clear 0x436da0; sample 0x433510)

// ---- ground / nav data (CellGrid::core, +0xb0): the four-vector container with other element types ---------------
template<class T> struct vec {
    char pad[4];
    T* first, *last, *end;
    unsigned size() const { return first == 0 ? 0 : (unsigned)(last - first); }
    unsigned capacity() const { return first == 0 ? 0 : (unsigned)(end - first); }
    T& operator[](unsigned i) const { return first[i]; }
};
struct NavVert {                // 0x24 bytes (0x479980, 0x472ed0: the scene's light probes are these vertices)
    rgl::vector4 p;             // +0x00
    rgl::vector4 n;             // +0x10 accumulated normal (0x479980)
    float level;                // +0x20 accumulated brightness (0x472ed0; the f20 of 0x479980)
};
struct NavCell {                // 0x88 bytes (0x479980, 0x472650: the cell's ground plane is trans)
    int npts;                   // +0x00
    int ids[0x14];              // +0x04 vertex indices
    rgl::vector4 trans;         // +0x54 plane (normal, distance)
    float scale;                // +0x64
    int f68;                    // +0x68
    char pad6c[0x84 - 0x6c];
    face* obj;            // +0x84
};
struct NavData {                // 0x68 bytes
    vec<NavVert> verts;         // +0x00
    vec<NavCell> cells;         // +0x10
    vec<int> v20;               // +0x20
    vec<int> v30;               // +0x30
    int m40;                    // +0x40
    rgl::vector4 e44;           // +0x44
    rgl::vector4 f54;           // +0x54
    int m64;                    // +0x64
    NavData();                  // 0x478150 (same constructor as NavCore's, ICF-folded)
    ~NavData();                 // 0x41f5d0 (four_vecs::~four_vecs)
};

// ---- smaller records ----------------------------------------------------------------------------------------------
// One cell of the grid (0x34 bytes): ground-collision ids, a list, the ids of the objects overlapping it.
struct Cell {
    std::vector<int> a;                         // +0x00
    std::list<int> l;                           // +0x10
    int pad1c;
    std::vector<int> b;                         // +0x20
    int tail;                                   // +0x30
    ~Cell();                                    // 0x431ab0 (the slices' RecP: array delete in ~CellGridBase)
    void collect_point(const float* p, int* use3d, unsigned mask, face** out, int* count);   // 0x439780
};
// A list of owned int arrays (pages of ids) with a capacity and a count (0x14 bytes): the grid's two free lists. The
// destructor of the grid frees the arrays first (free_values 0x472460), then the usual node sweep.
struct PList : std::list<int*> {
    int cap;                    // +0x0c (0x400 for the grid's lists)
    int n;                      // +0x10
    PList() : cap(0x400), n(0) {}
    void free_values();         // 0x472460
    ~PList() { free_values(); }
};
// The object list: a std::vector<void*> whose growth path (_Insert_n of the pointer vectors) is the shared instance 0x41f720.
struct ObjVec : std::vector<void*> {
    void insert_n(iterator where, size_t count, void* const& val);   // 0x41f720
};
struct Wind {                   // 0x7c-byte effect / wind object (0x478f30, 0x4792d0)
    int f0;                     // +0x00
    rgl::vector4 pos;           // +0x04
    rgl::vector4 f14;           // +0x14
    rgl::vector4 f24;           // +0x24
    char pad34[0x44 - 0x34];
    rgl::vector4 f44;           // +0x44
    float f54;                  // +0x54
    char pad58[0x6c - 0x58];
    int f6c;                    // +0x6c flags (bit 1: too weak)
    char pad70[0x7c - 0x70];
    Wind();                     // 0x461480
};
// Viewport / light parameter block; layout and constructor (0x46b940) from the terrain object's module. The five
// vectors after the frustum are the shadow camera's bounds (ShadowMap::attach_camera 0x4711e0).
struct ViewParams {
    float f0, f4, f8;
    int fc;
    char b10;
    float f14, f18, f1c;
    int f20;
    float f24, f28;
    char b2c;
    int f30;
    frustum fr;                 // +0x34
    rgl::vector4 vmin;          // +0x154 (CellGrid +0x22c)
    rgl::vector4 vmax;          // +0x164
    rgl::vector4 size;          // +0x174
    rgl::vector4 center;        // +0x184
    rgl::vector4 wcenter;       // +0x194
    rgl::matrix world;          // +0x1a4 (CellGrid +0x27c)
    rgl::matrix light;          // +0x1e4 (CellGrid +0x2bc)
    ViewParams();               // 0x46b940
};

// ---- the container ---------------------------------------------------------------------------------------------
// The grid part (destructor 0x475c60): dimensions, the cell array and the two page lists, the object list.
struct CellGridBase {
    int dim[2];                                 // +0x00 log2 of the cell counts (0x478830)
    int n[2];                                   // +0x08 (n[0]: number of cell columns, 0x475c60)
    float lo[2];                                // +0x10
    float hi[2];                                // +0x18
    float step[2];                              // +0x20
    Cell** cells;                               // +0x28 cells[x] -> array of n[1] cells
    PList pool0;                                // +0x2c
    PList pool1;                                // +0x40
    ObjVec items;                               // +0x54 owned objects (0x479980)
    ~CellGridBase();                            // 0x475c60
};
struct CellGrid : CellGridBase {
    HeightField* hf;                            // +0x64 (0x471a30, 0x471b30, 0x472650, 0x4785e0)
    rgl::vector4 amb;                           // +0x68 colour the cell hands to the emitters, w (+0x74) = strength
    int f78;                                    // +0x78
    std::vector<int> v7c;                       // +0x7c
    std::vector<int> v8c;                       // +0x8c
    int f9c;                                    // +0x9c
    std::vector<int> va0;                       // +0xa0
    NavData* core;                              // +0xb0 ground / nav data (0x472ed0, 0x472650, 0x479980)
    Mesh* navmesh;                              // +0xb4 the nav mesh the grid is built from (0x478c50)
    std::vector<Wind*> winds;                   // +0xb8
    std::vector<int> vc8;                       // +0xc8
    ViewParams view;                            // +0xd8 (shadow camera: +0x22c .. +0x2fc)
    rgl::vector4 shadow_amb;                    // +0x2fc light colour of the shadow camera (0x4711e0)
    rgl::vector4 m30c;                          // +0x30c
    rgl::vector4 m31c;                          // +0x31c
    std::vector<int> v32c;                      // +0x32c
    int f33c;                                   // +0x33c (not touched by the constructor)
    int f340;                                   // +0x340

    void init(const float* lo_, const float* hi_, int dimx, int dimy);      // 0x478830
    void locate(const float* p, int* out) const;                            // 0x41ed00
    void insert(int id, const float* lo_, const float* hi_);                // 0x478a10
    CellGrid(const float* lo_, const float* hi_, const int* dimx, const int* dimy, const int* v);  // 0x478ad0
    ~CellGrid();                                                            // 0x475d50
    void cleanup();                                                         // 0x421390 (rgl::world::reset)
    void build(Mesh* m);                                                    // 0x478c50
    Wind* spawn_wind();                                                     // 0x478f30
    void sample_lights(const rgl::vector4* p, rgl::vector4* dir, rgl::vector4* amb);   // 0x42ad60
    void build_light_frame(rgl::matrix* m, rgl::vector4* amb);              // 0x470c00
    int trace(const rgl::vector4* from, const rgl::vector4* to, float maxlen, int a, void** hit);   // 0x439d60 (ret 0x14)
    void floor_at(const float* p, float* out_z, face** out_face);           // 0x439be0 (ret 0xc)
    void light_at(rgl::vector4 p, rgl::vector4 n, rgl::vector4* out_dir, rgl::vector4* out_col, unsigned flags, float scale);   // 0x43ac40 (ret 0x30)
    void update_probes(unsigned flags);                                     // 0x472ed0
    void reg(void* o);                                                      // 0x424170
    void post(void* o);                                                     // 0x425240
    Entity* create_entity(int type_id);                                     // 0x428440
    void build_faces(NavData* map);                                         // 0x479980
};

#endif // TERRAIN_B_CELL_GRID_H

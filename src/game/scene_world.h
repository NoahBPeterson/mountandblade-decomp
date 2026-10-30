// scene_world.h: the 0x344-byte scene container (ctor 0x478ad0) in the match/include/rgl.h universe: one global `World` for
// char_sheet (face-gen portrait, camera view, terrain page, map camera), inventory_panel (view_open, game_tick) and cs_update.
// It is src/rgl/scene/world.h's rgl::world (the rgl_math.h universe cannot be included next to rgl.h) and terrain_b/cell_grid.h's
// CellGrid, proven by the shared addresses: ctor 0x478ad0 (the face-gen portraits `new World(lo, hi, ...)`), step 0x427f40
// (world::step), reset 0x421390 (world::reset), create_entity 0x428440 (world::create_entity), destroy 0x475d50 (the CellGrid dtor),
// the colour at +0x68 (world::ambient), the cell table at +0x28 (world::rows, read by ground_height 0x470f40), the light list at +0xb8
// (world::lights; the portraits push their wind tracks onto it through _Insert_n 0x410610) and the view block at +0x1ac.
// Only the members the modules reach are named; the global map world is g_map_scene (0xa732b4).
#ifndef GAME_SCENE_WORLD_H
#define GAME_SCENE_WORLD_H
#include "../include/rgl_base.h"

struct Entity;
struct CellGrid;
bool __stdcall ground_height(CellGrid* lv, const float* p, float* out);   // 0x470f40 (terrain_b.cpp: a free stdcall, ret 0xc, returns bool; callers load no ecx)

struct World {
    char pad0[0x28];
    char pad28[0x68 - 0x28];                                // +0x28 cell table (rows)
    rgl::vector4 color;                                     // +0x68 ambient colour (w = +0x74: the scene alpha)
    Entity* sky;                                            // +0x78 the optional backdrop entity (world::sky; mesh_start.cpp stores the sun here)
    char pad7c[0xb8 - 0x7c];
    struct TrackVec {                                       // std::vector<wind*> (lead dword, then first / last / end)
        int lead;                                           // +0x00
        int* first;                                         // +0x04
        int* last;                                          // +0x08
        int* end;                                           // +0x0c
        void _Insert_n(int* where, unsigned n, const int& val);   // 0x410610
        void push_back(const int& v)
        {
            if (first == 0 || (unsigned)(last - first) >= (unsigned)(end - first))
                _Insert_n(last, 1, v);
            else
                *last++ = v;
        }
    } tracks;                                               // +0xb8 (first at +0xbc)
    char padc8[0x1ac - 0xc8];
    int block[0x48];                                        // +0x1ac the 0x120-byte view block (copy of the terrain page's +0x70)
    char pad2cc[0x344 - 0x2cc];
    World(const float* lo, const float* hi, const int* a, const int* b, const int* c);   // 0x478ad0  // old name of CellGrid::CellGrid, removed after the names merge
    Entity* create_entity(int type);                        // 0x428440
    void set_fog(const void* a, const void* b, const void* c);   // 0x488cd0
    void destroy();                                         // 0x475d50  // old name of CellGrid::~CellGrid, removed after the names merge
    void reset();                                           // 0x421390
    void step(float dt);                                    // 0x427f40
    void set_fade(float* fog, void* spec);                  // 0x4624c0 (the fog / fade envelope: members at +0xdc..+0xf4; mesh_slot/mesh_start.cpp)
    void set_params(float a, float b, unsigned* color, float* z);   // 0x462540
    void floor_at(float* pos, float* y, void** face);       // 0x439be0 (the ground face under a position)
    void build(void* nav);                                // 0x478c50 (hands the nav mesh to the cell grid, terrain_b/cell_grid.h)
};
#endif // GAME_SCENE_WORLD_H

// world.h: the scene (rgl world), the game's view of rgl::world (src/rgl/scene/world.h has the full class in
// namespace rgl; this one is the global-namespace view the mission code and entity.cpp use). First field of the
// mission object. ONE definition for src/game: the members of other objects are only declared.
#ifndef GAME_WORLD_H
#define GAME_WORLD_H
#include <vector>
#include "rgl.h"
#include "./cell.h"

namespace rgl { struct entity; }
struct Emit;                                            // particle emitter (agent3/agent_fx_sound.h)
struct shape_base;

struct world {
    char pad0[0x28];
    cell** cells;                                       // +0x28 grid of cells (0x442180)
    char pad2c[0x7c - 0x2c];
    std::vector<rgl::entity*> ents;                     // +0x7c (first at +0x80)              0x41f450
    char pad8c[0xb4 - 0x8c];
    void* nav;                                          // +0xb4 navigation mesh
    void locate(const rgl::vector4* p, int* out) const;      // 0x41ed00 (grid cell of a point)
    Emit* alloc_emitter();                              // 0x427ea0 (0x442ca0)
    int trace_ray(const float* a, const float* b, float maxlen, shape_base** hit, int unused);   // 0x439d60 (0x440e30)
    void collect_boxed(const rgl::vector4* bmin, const rgl::vector4* bmax, std::vector<unsigned>* out);   // 0x442180 (agent_fx_sound.cpp)
    void erase_entity(rgl::entity* e);                  // 0x41f450 (entity.cpp)
};

#endif // GAME_WORLD_H

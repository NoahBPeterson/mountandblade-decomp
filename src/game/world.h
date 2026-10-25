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
struct face;                                            // src/include/shape.h
struct SceneEnt;                                        // agent4/agent_think.h
struct HeightField;                                     // src/include/height_field.h
struct LevelNav;                                        // inventory_panel/map_update.cpp: the strategy-map cell table (+0xb0)

struct world {
    char pad0[0x28];
    cell** cells;                                       // +0x28 grid of cells (0x442180)
    char pad2c[0x58 - 0x2c];
    face** faces;                                       // +0x58 -> the first face of the level (agent4/agent_update.cpp 0x45d820: the fallback floor)
    char pad5c[0x64 - 0x5c];
    HeightField* hf;                                    // +0x64 (agent4/agent_think.cpp 0x4465e0)
    char pad68[0x7c - 0x68];
    std::vector<rgl::entity*> ents;                     // +0x7c (first at +0x80)              0x41f450
    char pad8c[0xb0 - 0x8c];
    LevelNav* wmap;                                     // +0xb0 the strategy-map cell table (inventory_panel/map_update.cpp 0x5191e0: zmin / zmax / recs / linked)
    void* nav;                                          // +0xb4 navigation mesh
    void to_cell(const rgl::vector4* p, int* out) const;      // 0x41ed00 (grid cell of a point)
    void query_box(const float* bmin, const float* bmax, unsigned mask, shape_base** out, int* count);   // 0x42aa50 (0x448eb0)
    SceneEnt* find_entity(int id);                      // 0x42ba50 (agent4: the entity of a prop hit)
    SceneEnt* create_entity(int type_id);               // 0x428440 (0x44be50: 9 = missile, 0x44c210)
    void reg_shape(shape_base* s);                            // 0x424170 (collision registry; 0x44c210)
    void light_at(rgl::vector4 p, rgl::vector4 n, rgl::vector4* out_dir, rgl::vector4* out_col, unsigned flags, float scale);   // 0x43ac40 (0x44c210)
    void floor_at(const float* p, float* out_z, face** out_face);   // 0x439be0 (agent4: the floor face under a point)
    float floor_z(const float* p);                      // 0x439d40 (agent4: the floor height under a point)
    Emit* alloc_emitter();                              // 0x427ea0 (0x442ca0)
    int trace_ray(const float* a, const float* b, float maxlen, shape_base** hit, int unused);   // 0x439d60 (0x440e30)
    // ---- the other views of the scene object (agent.h / script_interpreter.h `Level`, inventory_panel/map_update.cpp `Level` = g_level 0xa732b4,
    //      the same pointer as mission +0) ----
    void query_segment(const float* a, const float* b, unsigned mask, shape_base** out, int* count);   // 0x439870 (map_update.cpp)
    void collect(unsigned key, shape_base** out, int* count);   // 0x51a3a0 (map_update.cpp)
    float ground_height(const rgl::vector4* p);         // 0x439d40 (script: floor_z taking the point as a vector)  // old name of level::floor_z, removed after the names merge
    int nearest_face(const rgl::vector4* p);                 // 0x439a50 (thiscall, ret 4; script)
    void set_fog(const float* a1, const float* distance, const unsigned* color);   // 0x488cd0 (ret 0xc; script)
    void collect_boxed(const rgl::vector4* bmin, const rgl::vector4* bmax, std::vector<unsigned>* out);   // 0x442180 (agent_fx_sound.cpp)
    void erase_entity(rgl::entity* e);                  // 0x41f450 (entity.cpp)
};

typedef world Level;                                    // agent/agent.h, script and map_update named the scene `Level`

#endif // GAME_WORLD_H

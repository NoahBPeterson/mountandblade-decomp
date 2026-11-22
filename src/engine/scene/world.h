// world.h: the scene container (rgl::world): a grid of cells with two free-list pools, the entity lists, the
// emitter pool, the ground geometry, lights and meshes. Objects of this class are the owner (+4) of every entity and
// the owner (+0x1c4) of every render scene. The slices called this one 0x344-byte object CellGrid, NavStore, Scene,
// World, Ctx, SceneW, SunCtx, LightCtx, Level, objgrp, mesh_pool and grid; the grid part (0x00..0x64, destructor
// 0x475c60) is its base class cell_grid. Offsets are the ones read or written by a function of src/rgl/scene,
// src/game/terrain_b (scene_container_grid.cpp, terrain_scene_renderer.cpp) or a caller (cited at each field). Member functions of this class in
// other objects are declared with their addresses so the equivalence resolver can map them.
#ifndef SCENE_WORLD_H
#define SCENE_WORLD_H
#include <vector>
#include "engine/core/layout32.h"
#include <list>
#include "engine/math/rgl_math.h"
#include "engine/math/scene_math.h"
#include "engine/scene/scene.h"
#include "engine/scene/emitter.h"
#include "engine/scene/scene_entity.h"
#include "engine/physics/height_field.h"
#include "engine/scene/view_params.h"
#include "engine/render/light.h"
#include "game/terrain/nav_data.h"

struct PolyMesh;                // game/terrain/poly_mesh.h: the nav mesh the grid is built from
struct shape_base;              // engine/physics/shape.h: the collision shapes the cells hold
struct face;                    // engine/physics/shape.h: the ground polygons
#include "engine/math/vec.h"   // int2: a cell coordinate (engine/physics/world_grid.h)

namespace rgl {

// ---- spatial hash: cells hold a doubly linked list of nodes, entities remember their cell references ----------
struct cellnode { cellnode* next; cellnode* prev; entity* obj; };           // 12 bytes     World::insert 0x421560
struct cellref { cellref* next; cellnode* node; int x, y; };                // 16 bytes
// one cell of the table (0x34 bytes): ground-collision vectors, the ids of the objects overlapping it and the head of the node list
struct cell {
    std::vector<shape_base*> a; // 0x00 the collision shapes in the cell, sorted by address
    std::list<shape_base*> objs;   // 0x10 the same shapes in the order they entered the cell
    int f1c;                    // 0x1c
    std::vector<int> ids;       // 0x20  ids of the objects overlapping the cell (world::insert 0x478a10)
    cellnode* head;             // 0x30  World::insert 0x421560, world::release_links 0x41ee20
    ~cell();                    // 0x431ab0 (defined with the grid's destructor: the array delete is emitted as in the original)
    void add(shape_base* x);                    // 0x423f10 (engine/physics/world_cells.cpp: into the sorted vector and the list)
    void remove(shape_base* o);                 // 0x424060 (from the sorted vector)
    int find(shape_base* o) const;              // 0x4240d0 (its index in the sorted vector, or -1)
    void collect_masked(unsigned mask, shape_base** out, int* count);                                     // 0x4396b0 (game/agent/agent_stats.cpp)
    void collect_point(const vector4* p, int* use3d, unsigned mask, shape_base** out, int* count);          // 0x439780 (game/agent/agent_stats.cpp)
    void collect(const vector4* bmin, const vector4* bmax, int* flat, unsigned mask, shape_base** out, int* count);  // 0x426500 (engine/scene/strategic_entity.cpp)
};

// chunked free-list pool: a list of chunk pointers, `count` cells per chunk (grow: 0x4206a0 / 0x420720); the grid's two
// pools hold 0x400 cells per chunk and free their chunks (free_values 0x472460) before the usual node sweep
template<class T> struct pool {
    std::list<T*> chunks;       // +0
    int count;                  // +0xc
    T* freelist;                // +0x10
    pool() : count(0x400), freelist(0) {}
    ~pool() { free_values(); }
    void grow();
    T* alloc() { if (!freelist) grow(); T* n = freelist; freelist = n->next; return n; }
    void link_chunks();         // 0x41f350 (cellnode), 0x41f3d0 (cellref): entity.cpp
    void free_values();         // 0x472460 (terrain_scene_renderer.cpp)
};

// The grid part of the scene (destructor 0x475c60): dimensions, the cell array and the two free-list pools, the object list.
struct cell_grid {
    int dim[2];                 // 0x00 log2 of the cell counts (0x478830)
    int n[2];                   // 0x08 cell counts: n[0] columns (cells[i], 0x475c60), n[1] cells per column (clear 0x420a70)
    float lo[2];                // 0x10 (to_cell 0x41ed00)
    float hi[2];                // 0x18
    float cell_size[2];         // 0x20 (world::step is the method of 0x427f40)
    cell** cells;               // 0x28 cell table (cells[i][j])
    pool<cellnode> nodes;       // 0x2c
    pool<cellref> refs;         // 0x40 (free list at +0x50)
    std::vector<Obj*> objs;     // 0x54 owned objects (0x420a70 deletes them; 0x479980 fills it)
    ~cell_grid();               // 0x475c60
    void walk(const float* a, const float* b, int (*cells)[2], int* count);   // 0x437290 (game/agent/agent_queries.cpp: the cells a segment crosses)
};

struct world : cell_grid {
    HeightField* hf;            // 0x64 the cell table of the height field (0x471a30, 0x471b30, 0x472650, 0x4785e0; dtor 0x41ecc0)
    vector4 ambient;            // 0x68 colour the cell hands to the emitters, w (+0x74) = strength
    entity* sky;                // 0x78 optional backdrop entity (rendered alone in the reflection pass; clear_ents 0x4207e0)
    std::vector<entity*> ents;  // 0x7c entities (first at 0x80)    remove_entity 0x41f0d0
    std::vector<entity*> ents2; // 0x8c pending entities (first at 0x90)
    int next_id;                // 0x9c World::create_entity 0x428440
    std::vector<int> removed;   // 0xa0 ids queued for removal (world::step 0x427f40)
    NavData* ground;            // 0xb0 ground / nav data (0x472ed0, 0x472650, 0x479980)
    PolyMesh* nav;              // 0xb4 the nav mesh the grid is built from (0x478c50)
    std::vector<Light*> lights; // 0xb8 light records (freed as raw blocks; spawn_wind 0x478f30)
    std::vector<mesh*> meshes;  // 0xc8 scene meshes (deleted through their destructor)
    ViewParams view;            // 0xd8 fog / viewport block (its address is stored in scene::fog by render_scene 0x429890; view.sun_shadows is the loaded flag); shadow camera: +0x22c .. +0x2fc
    vector4 shadow_amb;         // 0x2fc light colour of the shadow camera (0x4711e0)
    vector4 cam_dir;            // 0x30c a camera direction: (1, 0, 0) from the ctor 0x478ad0, the negated up row of the HUD eye (game/ui/hud/agent_hud.cpp)
    vector4 cam_pos;            // 0x31c that camera's origin (0 from the ctor)
    std::vector<psys*> emitters;    // 0x32c emitter pool (first at 0x330)
    int live;                   // 0x33c emitters in use
    int noise_counter;          // 0x340

    // ---- construction (game/mission/scene_container_grid.cpp) ----
    world(const float* lo_, const float* hi_, const int* dimx, const int* dimy, const int* v);   // 0x478ad0
    world();                    // 0x493ab0 (game/campaign/game_data_party_quest.cpp: a tableau's scene, 2000 x 2000 around the origin)
    ~world();                                                   // 0x475d50
    void init(const float* lo_, const float* hi_, int dimx, int dimy);   // 0x478830
    void insert(int id, const float* lo_, const float* hi_);    // 0x478a10 (the box of a nav face into the cells)
    void build(PolyMesh* m);                                    // 0x478c50
    Light* spawn_wind();                                        // 0x478f30
    void build_faces(NavData* map);                             // 0x479980
    void build_light_frame(matrix* m, vector4* amb);            // 0x470c00
    void update_probes(unsigned flags);                         // 0x472ed0
    void sample_lights(const vector4* p, vector4* dir, vector4* amb);   // 0x42ad60 (engine/scene/strategic_entity.cpp)
    // ---- queries (each defined in the module whose address range holds it) ----
    void query_segment(const vector4* a, const vector4* b, unsigned mask, shape_base** out, int* count);   // 0x439870 (game/agent/agent_stats.cpp)
    void nav_search(const float* p, const int* face, const int* max_depth, int depth, int* best, float* best_d);   // 0x439900
    int nearest_face(const float* p);                           // 0x439a50
    void floor_at(const vector4* p, float* out_z, ::face** out_face);   // 0x439be0 (ret 0xc)
    float floor_z(const vector4* p);                              // 0x439d40
    int trace_ray(const vector4* a, const vector4* b, float maxlen, shape_base** hit, int unused);   // 0x439d60 (ret 0x14)
    int locate_near(const vector4* pos, int* hint, int* mode);  // 0x43abf0
    void light_at(vector4 p, vector4 n, vector4* out_dir, vector4* out_col, unsigned flags, float scale);   // 0x43ac40 (ret 0x30)
    Light* first_dir_light();                                   // 0x42a1a0 (engine/scene/strategic_entity.cpp)
    void query_box(const vector4* bmin, const vector4* bmax, unsigned mask, shape_base** out, int* count);   // 0x42aa50
    vector4 get_ambient();                                      // 0x42b6d0 (engine/scene/strategic_shadow.cpp, by-value return)
    entity* find_entity(int id);                                // 0x42ba50
    int in_rect(const int2* p, const int2* lo, const int2* hi);   // 0x423ed0 (engine/physics/world_grid.cpp: a member that never reads this)
    void relocate(shape_base* o);                               // 0x423c80
    void reg_shape(shape_base* o);                              // 0x424170
    void add_tree(shape_base* s);                               // 0x425240
    void add_shape(shape_base* s);                              // 0x4252c0
    void collect_boxed(const vector4* bmin, const vector4* bmax, std::vector<entity*>* out);   // 0x442180 (game/agent3/agent_fx_sound.cpp: the entities whose box meets the query box)
    void erase_entity(entity* e);                               // 0x41f450 (engine/scene/fx_element_pool.cpp)
    void collect(unsigned key, entity** out, int* count);       // 0x51a3a0 (game/campaign/map_panel.cpp: the entities whose type_id() bit-mask contains the key)
    void light_mesh(mesh* m, const matrix* frame, unsigned flags);   // 0x47b7b0 (game/mission/mesh_vertex_ops.cpp: bakes the scene lights into a mesh's vertex colours)
    void query_shapes(entity* e, shape_base** out, int* count, int* use3d, unsigned mask);   // 0x426490 (engine/physics/rigid_body.cpp)
    void query_entities(entity* e, unsigned mask, bool ordered, std::vector<entity*>* out);  // 0x426750
    bool less(entity* a, entity* b);                            // 0x426960

    // ---- the fog / fade block at +0xdc (view.fog_start ..) ----
    void set_fade(const float* t, const int* v);                // 0x4624c0 (engine/render/renderer.cpp)
    void set_params(float a, float t, const int* b, const float* c);   // 0x462540 (engine/render/renderer.cpp)
    void set_fog(const float* ref, const float* rate, const int* color);   // 0x488cd0 (game/ui/widgets/widget_base.cpp)

    void render_part(scene* sc, int rem, int div);              // 0x421110
    void reset();                                               // 0x421390
    void clear_ents();                                          // 0x4207e0
    void clear_cells();                                         // 0x420a70
    void insert(entity* o, int x, int y);                       // 0x421560
    void insert_obj(entity* o);                                 // 0x421600
    void to_cell(const float* p, int* out);                     // 0x41ed00
    void release_links(entity* e);                              // 0x41ee20
    void partition_items();                                     // 0x41efd0
    void remove_entity(int id);                                 // 0x41f0d0
    psys* alloc_emitter();                                      // 0x427ea0
    void step(float dt);                                        // 0x427f40
    entity* instantiate(int type_id);                           // 0x428240
    entity* create_entity(int type_id);                         // 0x428440
    void add_pending(entity* o);                                // 0x4207a0
    void add_item(shape_base* e);                                 // 0x423c00
    void render_scene(scene* sc, bool reflect);                 // 0x429890
    mesh* get_mesh(skin* r);                                    // 0x413d30 (engine/render/particle_system.cpp: the particle mesh drawn with r, from meshes)
};

MB_LAYOUT32_ASSERT(world_size_check, sizeof(world) == 0x344);     // the scene container is allocated as 0x344 bytes (0x478ad0)

} // namespace rgl

#endif

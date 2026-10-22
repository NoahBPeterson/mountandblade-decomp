// world.h: the scene container (rgl::world): a grid of cells with two free-list pools, the entity lists, the
// emitter pool, the ground geometry, lights and meshes. Objects of this class are the owner (+4) of every entity and
// the owner (+0x1c4) of every render scene. Other names for it in the slices: world, World, objgrp, Ctx, mesh_pool
// (../psys.h), grid. Fields are listed with the function that reads them; the rest of the object (0x1c .. 0x344)
// belongs to other objects and is padding here. Member functions of this class in other objects are declared
// with their addresses so the equivalence resolver can map them.
#ifndef SCENE_WORLD_H
#define SCENE_WORLD_H
#include <vector>
#include <list>
#include "../rgl_math.h"
#include "./scene_math.h"
#include "./scene.h"
#include "./emitter.h"
#include "./scene_entity.h"

namespace rgl {

// ---- spatial hash: cells hold a doubly linked list of nodes, entities remember their cell references ----------
struct light;                   // 0x70-byte light record (not needed here)
struct cellnode { cellnode* next; cellnode* prev; entity* obj; };           // 12 bytes     World::insert 0x421560
struct cellref { cellref* next; cellnode* node; int x, y; };                // 16 bytes
// one cell of the table (0x34 bytes): ground-collision vectors and the head of the node list
struct cell {
    std::vector<Obj*> a;        // 0x00
    std::list<int> b;           // 0x10
    int f1c;                    // 0x1c
    std::vector<int> c;         // 0x20
    cellnode* head;             // 0x30  World::insert 0x421560, world::release_links 0x41ee20
};

// chunked free-list pool: a list of chunk pointers, `count` cells per chunk (grow: 0x4206a0 / 0x420720)
template<class T> struct pool {
    std::list<T*> chunks;       // +0
    int count;                  // +0xc
    T* freelist;                // +0x10
    void grow();
    T* alloc() { if (!freelist) grow(); T* n = freelist; freelist = n->next; return n; }
    void link_chunks();         // 0x41f350 (cellnode), 0x41f3d0 (cellref): entity.cpp
};

struct ground_geom { ~ground_geom(); };                                    // 0x41f5d0 (four arrays)
struct world_aux { ~world_aux(); };                                        // 0x41ecc0

struct world {
    char pad0[8];
    int nrows;                  // 0x08 cell rows     (grid clear 0x420a70)
    int ncols;                  // 0x0c
    char pad10[0x28 - 0x10];
    cell** rows;                // 0x28 cell table (rows[i][j])
    pool<cellnode> nodes;       // 0x2c
    pool<cellref> refs;         // 0x40 (free list at +0x50)
    std::vector<Obj*> objs;     // 0x54 owned objects (0x420a70 deletes them)
    world_aux* p64;             // 0x64
    vector4 ambient;            // 0x68
    entity* sky;                // 0x78 optional backdrop entity (rendered alone in the reflection pass; clear_ents 0x4207e0)
    std::vector<entity*> ents;  // 0x7c entities (first at 0x80)    remove_entity 0x41f0d0
    std::vector<entity*> ents2; // 0x8c pending entities (first at 0x90)
    int next_id;                // 0x9c World::create_entity 0x428440
    vec<int> removed;           // 0xa0 ids queued for removal (world::step 0x427f40)
    ground_geom* ground;        // 0xb0
    int fb4;                    // 0xb4
    std::vector<light*> lights; // 0xb8 light records (freed as raw blocks)
    std::vector<mesh*> meshes;  // 0xc8 scene meshes (deleted through their destructor)
    char env[0x104 - 0xd8];     // 0xd8 (its address is stored in scene::f1b0 by render_scene 0x429890)
    bool loaded;                // 0x104
    char pad105[0x32c - 0x105];
    std::vector<psys*> emitters;    // 0x32c emitter pool (first at 0x330)
    int live;                   // 0x33c emitters in use
    int noise_counter;          // 0x340

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
    void add_item(item_obj* e);                                 // 0x423c00
    void render_scene(scene* sc, bool reflect);                 // 0x429890
};

} // namespace rgl
#endif

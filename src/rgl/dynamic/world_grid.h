// world_grid.h: the world's collision grid (src/rgl/dynamic/world_grid.cpp, 0x423c00..0x425370): placing collision shapes
// into the cells of the grid, the cell's sorted shape vector, the ground geometry (vertex and face-group records) that the
// shapes of an entity are registered with, and the std::vector instantiations those use. One header for the classes
// of this object.
//
// The classes are views of the shared ones; this header adds only members, never fields:
//   world_grid : rgl::world      the members of rgl::world that world.h does not declare (relocate, add_shape,
//                                add_tree, reg_shape); rgl::world::add_item (0x423c00) is declared in world.h
//   entity_grid : rgl::entity    entity::add_shape (0x424100)
//   grid_cell                    rgl::cell (world.h) with its sorted shape vector and the shape list typed
//   ground_store : rgl::ground_geom   the ground geometry's two vectors (world.h leaves its fields out)
// The shapes are src/rgl/shapes' grid_shape / face / composite (the slices called them Obj, kind() = shape_type()); that
// header cannot be included next to resource.h (scene.h pulls it in, both define grid_shape), so the grid sees them as
// views of resource.h's grid_shape that add the vtable slots 5..13 and the two fields it reads.
// The slices' names: World (world_grid), Registry (world_grid::reg_shape), Scene (entity_grid), Cell (grid_cell),
// Store (ground_store), ObjVec / ObjList (shape_vec / shape_list), Obj (grid_shape).
#ifndef RGL_DYNAMIC_WORLD_GRID_H
#define RGL_DYNAMIC_WORLD_GRID_H
#include <vector>
#include <list>
#include <new>
#include <memory>
#include "../rgl_math.h"
#include "../resource.h"
#include "./grid_shape.h"
#include "../scene/scene_math.h"
#include "../scene/frustum.h"
#include "../scene/scene.h"
#include "../scene/emitter.h"
#include "../scene/scene_entity.h"
#include "../scene/world.h"

struct int2 { int x, y; };                          // a cell coordinate
struct ground_v36  { int v[9]; };                          // 0x24-byte vertex record of the ground geometry
struct ground_g136 { int v[34]; };                         // 0x88-byte face-group record

// ---- the cell ---------------------------------------------------------------------------------------------------
// std::vector<grid_shape*> written out (VC7.1 layout: allocator slot, first, last, end) so that its members stay out of
// line and under the slices' signatures. push_back is the instantiation at 0x423fc0 (fast path inline, else insert).
struct shape_vec {
    char pad0[4];
    grid_shape **first, **last, **end;
    unsigned size() const { return first == 0 ? 0 : last - first; }
    unsigned capacity() const { return first == 0 ? 0 : end - first; }
    struct iter { grid_shape** p; };
    void push_back(grid_shape* const& v);                   // 0x423fc0
    iter insert(iter where, grid_shape* const& v);          // 0x424010 (std::vector<grid_shape*>::insert)
    void resize(unsigned n, grid_shape* v);                 // 0x41fa00 (entity.cpp)
    int find(grid_shape* o) const;                          // 0x4240d0 (called by remove; defined on shape_vec_f)
    void remove(grid_shape* o);                             // 0x424060
};
// std::list<grid_shape*>: allocator slot, head sentinel, size
struct shape_list {
    char pad0[4];
    struct node { node* next; node* prev; grid_shape* obj; } *head;
    int size;
    void _Insert(node* where, grid_shape* const& v);        // 0x41f640 (entity.cpp)
};
// one cell of the grid (0x34 bytes, rgl::cell in world.h): the shapes in it, sorted by address, and the list of all of them
struct grid_cell : shape_vec {
    shape_list objs;                                        // +0x10
    char pad1c[0x34 - 0x1c];
    void add(grid_shape* x);                                // 0x423f10
};

// ---- the ground geometry ------------------------------------------------------------------------------------------
struct Vert { rgl::vector4 p; rgl::vector4 n; int f20; };   // ground_v36: position, face normal, a flag
// ground_g136: up to four corners
struct Group {
    int n;                      // +0x00
    int vidx[4];                // +0x04 first vertex index per corner
    char pad14[0x10];
    int f24;                    // +0x24
    int link[4];                // +0x28 (-1)
    int f38[4];                 // +0x38 (0)
    int f48, f4c, f50;          // +0x48 (-1)
    char pad54[0x10];
    float f64;                  // +0x64 (1.0)
    int f68, f6c, f70;          // +0x68 (0)
    rgl::vector4 c;             // +0x74 sum of the corner positions
    int f84;                    // +0x84 (0)
    Group();                    // 0x4246f0
};
// vector<ground_v36> / vector<ground_g136> stand-ins (resize stays out of line)
struct V36 {
    char pad0[4];
    Vert *first, *last, *end;
    unsigned size() const { return first == 0 ? 0 : last - first; }
    void resize(unsigned n, Vert v);                        // 0x424740
};
struct V136 {
    char pad0[4];
    Group *first, *last, *end;
    unsigned size() const { return first == 0 ? 0 : last - first; }
    void resize(unsigned n, Group v);                       // 0x424cc0
    struct iter { Group* p; iter operator+(int off) const; };   // 0x424720 (out of line)
    iter begin() const { iter r; r.p = first; return r; }
};
rgl::vector4 face_normal(const rgl::vector4* a, const rgl::vector4* b, const rgl::vector4* c);   // 0x424650

struct ground_store : rgl::ground_geom {                    // the object at world+0xb0
    V36 verts;                  // +0
    V136 groups;                // +0x10
    int add(int n, const rgl::vector4* pts);                // 0x424220: one polygon -> n vertices + a group; returns the group index
};

// ---- the world's members ------------------------------------------------------------------------------------------
struct world_grid : rgl::world {
    int in_rect(const int* p, const int* lo, const int* hi);   // 0x423ed0 (a member that never reads this)
    void relocate(grid_shape* o);                           // 0x423c80
    void reg_shape(grid_shape* o);                          // 0x424170
    void add_tree(grid_shape* s);                           // 0x425240
    void add_shape(grid_shape* s);                          // 0x4252c0
};
int2* min2(int2* out, const int2* a, const int2* b);        // 0x423e70
int2* max2(int2* out, const int2* a, const int2* b);        // 0x423ea0

struct entity_grid : rgl::entity {
    void add_shape(rgl::item_obj* o);                        // 0x424100: keep the shape, tag it with the entity id, register it in the world
};

#endif

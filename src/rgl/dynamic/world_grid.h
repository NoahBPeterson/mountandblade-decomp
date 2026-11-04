// world_grid.h: the world's collision grid (src/rgl/dynamic/world_grid.cpp, 0x423c00..0x425370): placing collision shapes
// into the cells of the grid, the cell's sorted shape vector, the ground geometry (vertex and face-group records) that the
// shapes of an entity are registered with, and the std::vector instantiations those use. One header for the classes
// of this object.
//
// The classes are views of the shared ones; this header adds only members, never fields:
//   world_grid : rgl::world      the members of rgl::world that world.h does not declare (relocate, add_shape,
//                                add_tree, reg_shape); rgl::world::add_item (0x423c00) is declared in world.h
//   (strategic_entity::add_shape 0x424100, slot 28 of the strategic entity's table, is declared in scene_entity.h)
//   grid_cell                    rgl::cell (world.h) with its sorted shape vector and the shape list typed
//   ground_store : rgl::ground_geom   the ground geometry's two vectors (world.h leaves its fields out)
// The shapes are src/rgl/shapes' shape_base / face / composite (the slices called them Obj, kind() = shape_type()); that
// header cannot be included next to resource.h (scene.h pulls it in, both define shape_base), so the grid sees them as
// views of resource.h's shape_base that add the vtable slots 5..13 and the two fields it reads.
// The slices' names: World (world_grid), Registry (world_grid::reg_shape), Scene (the strategic entity's add_shape), Cell (grid_cell),
// Store (ground_store), ObjVec / ObjList (shape_vec / shape_list), Obj (shape_base).
#ifndef RGL_DYNAMIC_WORLD_GRID_H
#define RGL_DYNAMIC_WORLD_GRID_H
#include <vector>
#include <list>
#include <new>
#include <memory>
#include "../rgl_math.h"
#include "../resource.h"
#include "../scene/scene_math.h"
#include "../scene/frustum.h"
#include "../scene/scene.h"
#include "../scene/emitter.h"
#include "../scene/scene_entity.h"
#include "../scene/world.h"
#include "../../include/vec_xlen.h"

struct ground_v36 { rgl::vector4 p; rgl::vector4 n; int f20; };   // 0x24-byte vertex record of the ground geometry: position, face normal, a flag

// ---- the cell ---------------------------------------------------------------------------------------------------
// std::vector<shape_base*> written out (VC7.1 layout: allocator slot, first, last, end) so that its members stay out of
// line and under the slices' signatures. push_back is the instantiation at 0x423fc0 (fast path inline, else insert).
struct shape_vec {
    char pad0[4];
    shape_base **first, **last, **end;
    unsigned size() const { return first == 0 ? 0 : last - first; }
    unsigned capacity() const { return first == 0 ? 0 : end - first; }
    void resize(unsigned n, shape_base* v);                 // 0x41fa00 (entity.cpp)
    int find(shape_base* o) const;                          // 0x4240d0 (called by remove; defined on shape_vec_f)
    void remove(shape_base* o);                             // 0x424060
};
// std::list<shape_base*>: allocator slot, head sentinel, size
struct shape_list {
    char pad0[4];
    struct node { node* next; node* prev; shape_base* obj; } *head;
    int size;
    void _Insert(node* where, shape_base* const& v);        // 0x41f640 (entity.cpp)
};
// one cell of the grid (0x34 bytes, rgl::cell in world.h): the shapes in it, sorted by address, and the list of all of them
struct grid_cell : shape_vec {
    shape_list objs;                                        // +0x10
    char pad1c[0x34 - 0x1c];
    void add(shape_base* x);                                // 0x423f10
};

// ---- the ground geometry ------------------------------------------------------------------------------------------
// ground_g136: the 0x88-byte face-group record, up to four corners
struct ground_g136 {
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
    ground_g136();              // 0x4246f0
};
// std::vector<ground_g136>, written out (VC7.1 layout: allocator slot, first, last, end). These bodies do not come out of the stock
// <vector> with cl 13.10.3052 (it leaves allocator::construct and the _Ucopy / _Ufill wrappers out of line, and stops inlining
// std::copy for elements of >= 128 bytes), so the members are written out with the same shape as the VC7.1 <vector>.
#define VEC_COMMON(T) \
    std::allocator<T> _Alval; \
    T *_Myfirst, *_Mylast, *_Myend; \
    typedef unsigned size_type; \
    typedef T* iterator; \
    size_type capacity() const { return _Myfirst == 0 ? 0 : _Myend - _Myfirst; } \
    size_type size() const { return _Myfirst == 0 ? 0 : _Mylast - _Myfirst; } \
    size_type max_size() const { return _Alval.max_size(); } \
    __forceinline T* _Ucopy(T* f, T* l, T* p) { return std::_Uninitialized_copy(f, l, p, _Alval); } \
    __forceinline T* _Ufill(T* p, size_type n, const T& v) { std::_Uninitialized_fill_n(p, n, v, _Alval); return p + n; } \
    void _Destroy(T* f, T* l) { for (; f != l; ++f) std::_Destroy(f); } \
    __forceinline void _Xlen() const { VEC_XLEN_CALL(this); }   // 0x43e2a0

struct vec136 {
    VEC_COMMON(ground_g136)
    struct iterator_ {
        ground_g136* _Ptr;
        static iterator_ mk(ground_g136* p) { iterator_ r; r._Ptr = p; return r; }
        iterator_ operator+(int off) const;                 // 0x424720 (out of line)
    };
    iterator_ begin() const { return iterator_::mk(_Myfirst); }
    void insert(iterator w, size_type c, const ground_g136& v) { _Insert_n(w, c, v); }
    void _Insert_n(iterator _Where, size_type _Count, const ground_g136& _Val);   // 0x424db0
    void resize(size_type _Newsize, ground_g136 _Val);                            // 0x424cc0
    iterator_ erase(iterator_ _First, iterator_ _Last);                           // 0x424d60
};
rgl::vector4 face_normal(const rgl::vector4* a, const rgl::vector4* b, const rgl::vector4* c);   // 0x424650

struct ground_store {                    // the object at world+0xb0
    std::vector<ground_v36> verts;   // +0
    vec136 groups;              // +0x10
    int add(int n, const rgl::vector4* pts);                // 0x424220: one polygon -> n vertices + a group; returns the group index
};

// ---- the world's members ------------------------------------------------------------------------------------------
int2* min2(int2* out, const int2* a, const int2* b);        // 0x423e70
int2* max2(int2* out, const int2* a, const int2* b);        // 0x423ea0

#endif

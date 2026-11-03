// entity.h: classes of entity.cpp (0x41f1d0..0x41fba0): the
// effect element FxElem (vtable 0x54b454), the scene's teardown, the chunked free-list pool, and the element types
// of the STL instantiations the linker placed in this object. Engine (rgl) code; all names provisional.
// Every offset below is read by a function of this module (cited); the rest of each object is padding.
#ifndef GAME_ENTITY_H
#define GAME_ENTITY_H
#include <vector>
#include <list>
#include "../rgl/rgl_math.h"
#include "./mat4.h"
#include "../include/mesh_prims.h"
#include "../include/mesh_parts.h"             // rgl::render_info (and the other mesh records)
#include "./tstring.h"

// ---- hand-written VC7.1 std::vector layout (empty allocator slot, first, last, end) -----------------------
// The game's own container for the collision-shape arrays (also s0040d7d0): clear() frees the block, unlike
// std::vector::clear. The out-of-line instance vec<mface>::clear is 0x41f4b0.
template<class T> struct vec {
    std::allocator<T> al;
    T* first; T* last; T* end;
    vec() : first(0), last(0), end(0) {}
    ~vec() { tidy(); }
    unsigned size() const { return first == 0 ? 0 : last - first; }
    struct iter {                                       // std::_Ptrit
        T* cur;
        iter() : cur(0) {}
        iter(T* p) : cur(p) {}
        T& operator*() const { return *cur; }
        iter& operator+=(int n) { cur += n; return *this; }
        iter operator+(int n) const { iter t = *this; return t += n; }
    };
    iter begin() { return iter(first); }
    T& operator[](unsigned i) { return *(begin() + i); }
    void tidy() { if (first != 0) operator delete(first); first = 0; last = 0; end = 0; }
    void clear();
};
template<class T> void vec<T>::clear() { if (first != 0) operator delete(first); first = 0; last = 0; end = 0; }

#include "../include/shape.h"                         // mface: the 0x20-byte face record (s0040d7d0), with the shapes

namespace rgl {

// ---- mesh (vtable 0x54a6f4, 0x260 bytes; see s00412a90 mesh.h): the fields this module reads -------------
struct mesh {
    char pad0[0xb0];
    std::vector<mesh*> kids;    // +0xb0 (first at +0xb4)            0x41f2a0
    void* data;                 // +0xc0 (owned block)                0x41f250
    char padc4[0x1a8 - 0xc4];
    int id;                     // +0x1a8                             0x41f2a0
};

// ---- resource manager (global object 0x5d6fa0) --------------------------------------------------------
struct resmgr {
    void release(int handle);                               // 0x4175f0
};
extern resmgr g_resmgr;                                     // 0x5d6fa0

#include "../include/poly.h"                               // Poly: anything deleted through vtable slot 0
struct Obj { virtual ~Obj(); int owner; int id; };          // element of std::vector<Obj*> (0x41f720, 0x41fa00)

// ---- scene (see mesh.h of s00412a90: list at +0x194, cache at +0x1c8); dtor 0x41f4e0 --------------------
struct scene {
    char pad0[0x154];
    int handle;                 // +0x154 resource handle, released through g_resmgr
    scene* child;               // +0x158
    char pad15c[0x194 - 0x15c];
    vec<void*> list;            // +0x194 (first at +0x198)
    char pad1a4[0x1b4 - 0x1a4];
    vec<void*> v1b4;            // +0x1b4 (first at +0x1b8)
    char pad1c4[0x1c8 - 0x1c4];
    vec<Poly*> cache;           // +0x1c8 (first at +0x1cc)
    ~scene();                   // 0x41f4e0
};


// rgl::entity (vtable 0x54b460) is the one class of src/rgl/scene/scene_entity.h; this module's functions on it are in
// src/rgl/scene/entity_base.cpp. (This header used to declare a second, partial entity with other names for the same slots.)

} // namespace rgl

// four arrays torn down together (dtor 0x41f5d0; users 0x421390, 0x479980)
struct four_vecs {
    vec<rgl::vector4> a;             // +0
    vec<rgl::vector4> b;             // +0x10
    vec<mface> c;               // +0x20
    vec<int> d;                 // +0x30
    ~four_vecs();               // 0x41f5d0
};

// ---- effect element (0x1dc bytes, vtable 0x54b454): FxElem, one definition in src/include/fx.h ---------------------
#include "../include/fx.h"

// ---- chunked free-list pool: a list of chunk pointers, per_chunk cells in each, chained through `next` ----
struct cell12 { cell12* next; int a, b; };
struct cell16 { cell16* next; int a, b, c; };
template<class T> struct pool {
    std::list<T*> chunks;       // +0 (head at +4)
    int per_chunk;              // +0xc
    T* free_head;               // +0x10
    void link_chunks();         // 0x41f350 (cell12), 0x41f3d0 (cell16)
};

// ---- element types of the STL instantiations (only their identity matters: these are the names the
// neighbouring slices registered for the ICF-folded callees, so the resolver maps them) -------------------
#include "../include/pod_elems.h"   // E8 / E12 / E100 / rec36 / rec136 (shared)

// ---- the world's entity list (vector at +0x7c; the rest of the world object is in the previous TU) ----------
namespace rgl { struct entity; }
#include "./world.h"                                        // world: ents +0x7c, erase_entity 0x41f450

#endif // GAME_ENTITY_H

// entity.h: classes of entity.cpp (0x41f1d0..0x41fee0): the scene entity (rgl::entity, vtable 0x54b460), the
// effect element FxElem (vtable 0x54b454), the scene's teardown, the chunked free-list pool, and the element types
// of the STL instantiations the linker placed in this object. Engine (rgl) code; all names provisional.
// Every offset below is read by a function of this module (cited); the rest of each object is padding.
#ifndef GAME_ENTITY_H
#define GAME_ENTITY_H
#include <vector>
#include <list>
#include "rgl.h"
#include "./mat4.h"
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

struct mface { int a, flags, n, idx[4], m4; };          // 0x20-byte face record (s0040d7d0)

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

struct Poly { virtual ~Poly(); };                           // anything deleted through vtable slot 0
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

// ---- the scene entity (vtable 0x54b460, ctor 0x41fba0, 0x15c bytes) -----------------------------------
struct smat : mat4 {
    void scale_rows(const vector4* k);                      // 0x41fee0 (next TU)
};

// the optional frame source behind entity::parent (vtable slots used: 8, 10, 11)
struct frame_src {
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3();
    virtual void s4(); virtual void s5(); virtual void s6(); virtual void s7();
    virtual void apply(const mat4* m);                      // slot 8  (+0x20)
    virtual void s9();
    virtual vector4 get_a();                                // slot 10 (+0x28)
    virtual vector4 get_b();                                // slot 11 (+0x2c)
};

// Axis-aligned box with centre and radius (0x34 bytes; ctor 0x40eaf0, see s00416ef0)
struct aabb {
    vector4 min, max, center;
    float radius;
    aabb();                                                 // 0x40eaf0
};

struct lod_mesh2;
struct render_info { char valid; float depth; };

struct entity {
    virtual ~entity();                                  // 0: 0x4214e0
    virtual int slot1();                                // 1: 0x408020
    virtual tstring type_name();                        // 2: 0x41fcb0  "entity"
    virtual void sync();                                // 3: 0x41fcd0
    virtual void recalc();                              // 4: 0x41ffe0
    virtual float get_fbc();                            // 5: 0x41fd80
    virtual void set_fbc(const float* p);               // 6: 0x41fd90
    virtual void commit(int unused);                    // 7: 0x41fda0
    virtual void set_f144(const float* p);              // 8: 0x41fdd0
    virtual void render(scene* sc, render_info* out);   // 9: 0x420310
    virtual void s10();                                 // 10: 0x4280e0
    virtual void s11();                                 // 11: 0x4280e0
    virtual const vector4* local_origin();              // 12: 0x41fde0
    virtual const vector4* get_vac();                   // 13: 0x41fdf0
    virtual const vector4* get_v9c();                   // 14: 0x41fe00
    virtual int is_dirty();                             // 15: 0x41fe10
    virtual void s16();                                 // 16: 0x4284e0
    virtual int slot17();                               // 17: 0x408030
    virtual void set_mode(unsigned m);                  // 18: 0x41fe30
    virtual void clear();                               // 19: 0x420960
    virtual void set_local(const mat4* m);              // 20: 0x41fe50
    virtual void set_local_dirty(const mat4* m);        // 21: 0x41fe70
    virtual void set_local_scaled(const mat4* m, const vector4* k);        // 22: 0x41fe90
    virtual void set_local_scaled_dirty(const mat4* m, const vector4* k);  // 23: 0x41ff40

    int f4, f8, fc, f10, f14;   // +8: id (world::remove_entity 0x41f0d0 compares it)
    mat4 world;                 // 0x18
    smat local;                 // 0x58
    frame_src* parent;          // 0x98
    vector4 v9c;                // 0x9c
    vector4 vac;                // 0xac
    float fbc;                  // 0xbc
    float fc0;                  // 0xc0
    vector4 vc4;                // 0xc4
    aabb box;                   // 0xd4
    std::vector<lod_mesh2*> lods;   // 0x108
    std::vector<mesh*> meshes;      // 0x118
    std::vector<Obj*> v128;         // 0x128
    int f138, f13c;
    char f140, f141, f142;
    float f144;
    unsigned mode;              // 0x148
    vector4 scale;              // 0x14c

    entity();                                           // 0x41fba0
};

} // namespace rgl

// four arrays torn down together (dtor 0x41f5d0; users 0x421390, 0x479980)
struct four_vecs {
    vec<rgl::vector4> a;             // +0
    vec<rgl::vector4> b;             // +0x10
    vec<mface> c;               // +0x20
    vec<int> d;                 // +0x30
    ~four_vecs();               // 0x41f5d0
};

// ---- effect element (0x1dc bytes, vtable 0x54b454; see FxElem in s00421110 fx.h) ---------------------------
struct FxElem {
    virtual ~FxElem();                                      // 0x41f1d0
    char pad4[0x19c - 4];
    std::vector<rgl::mesh*> psys;   // +0x19c (first at +0x1a0)       0x41f250, 0x41f2a0
    char pad1ac[4];
    std::vector<void*> groups;      // +0x1b0 (first at +0x1b4)       0x41f1d0
    int parent;                 // +0x1c0
    int has_arr;                // +0x1c4                              0x41f1d0
    int* arr;                   // +0x1c8                              0x41f1d0
    char pad1cc[0x1dc - 0x1cc];
    void set_data(void* p);                                 // 0x41f250
    int find_kid(int id);                                   // 0x41f2a0
};

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
struct E100 { int v[25]; };                                 // 0x64-byte particle (std::vector<E100>, s0040f9a0)

// ---- the world's entity list (vector at +0x7c; the rest of the world object is in the previous TU) ----------
namespace rgl { struct entity; }
struct world {
    char pad0[0x7c];
    std::vector<rgl::entity*> ents;  // +0x7c (first at +0x80)              0x41f450
    void erase_entity(rgl::entity* e);                           // 0x41f450
};

#endif // GAME_ENTITY_H

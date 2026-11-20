// dynamic_entity.h: the rigid-body entity (src/rgl/dynamic/dynamic_entity.cpp, 0x425370..0x427d80): its state and
// virtuals, the contact resolution and integration, the world's per-cell shape and entity queries behind its collision
// pass, the std::vector<Contact> instantiations, the sort of the query result, and the collision entity's destructor.
// One header for the classes of this object.
//
// rgl::collision_entity (../scene/scene_entity.h, vtable 0x54b548) is the base (one class: the dynamic entity's constructor
// zeroes its contact list through the member initializer, which is where the original does); scene_entity.h only forward-declares
// rgl::dynamic_entity (the entity factory in entity_types.cpp includes this header). The slices' names (rigid_base: the base with the state; dyn.h's Obj,
// entity, cell, cell_ref, enode, contact_list) are the shared classes: the queries below are members of derived views
// are members of rgl::world / rgl::cell (../scene/world.h), defined in dynamic_entity.cpp.
#ifndef RGL_DYNAMIC_DYNAMIC_ENTITY_H
#define RGL_DYNAMIC_DYNAMIC_ENTITY_H
#include <vector>
#include <algorithm>
#include <memory>
#include <new>
#include <stdexcept>
#include "../../include/vec_xlen.h"
#include <string.h>
#include "../rgl_string.h"
#include "../rgl_math.h"
#include "../resource.h"
#include "../scene/scene_math.h"
#include "../scene/frustum.h"
#include "../scene/scene.h"
#include "../scene/emitter.h"
#include "../scene/scene_entity.h"
#include "../scene/world.h"

namespace rgl {

// ---- std::vector<Contact> --------------------------------------------------------------------------------------------
// scene_entity.h's contact_vec (VC7.1 layout, inline _Tidy) with the members this object instantiates. push_back is the
// instance at 0x427450 (fast path: _Uninitialized_fill_n 0x427ca0), insert 0x427540, _Insert_n 0x427590. The stock
// <vector> does not reproduce these bodies with cl 13.10.3052 (it keeps allocator::construct out of line), so they are
// written out with the members of the VC7.1 <vector>; _Xlen stays an external (0x43e2a0).
struct contact_ops : contact_vec {
    typedef unsigned size_type;
    struct iterator { Contact* _Ptr; iterator() {} iterator(Contact* p) : _Ptr(p) {} };
    size_type capacity() const { return first == 0 ? 0 : end - first; }
    size_type size() const { return first == 0 ? 0 : last - first; }
    size_type max_size() const { return std::allocator<Contact>().max_size(); }
    iterator begin() { return iterator(first); }
    iterator end_() { return iterator(last); }
    __forceinline Contact* _Ucopy(Contact* f, Contact* l, Contact* p);
    __forceinline Contact* _Ufill(Contact* p, size_type n, const Contact& v);
    void _Destroy(Contact* f, Contact* l) { for (; f != l; ++f) std::_Destroy(f); }
    __forceinline void _Xlen() const { VEC_XLEN(this); }            // 0x43e2a0 (throws length_error "vector<T> too long")
    void _Insert_n(Contact* _Where, size_type _Count, const Contact& _Val);    // 0x427590
    __declspec(noinline) iterator insert(iterator _Where, const Contact& _Val);    // 0x427540
    void push_back(const Contact& _Val);                            // 0x427450
};

// ---- the shape contact record (shape_contact_count 0x40bfe0 fills it) --------------------------------------------------
struct vec4p { float x, y, z, w; };    // vector4 layout without the (empty) constructor: keeps contact_rec c[4] free of __vec_ctor
struct contact_rec {                   // 0x24 bytes
    vec4p normal;
    vec4p point;
    float depth;
};


} // namespace rgl

#include "./rigid_body.h"                // rgl::rigid_base, rgl::dynamic_entity (the classes, shared with src/game)

// The original keeps std::vector<entity*>'s _Buy and _Tidy out of line (0x43e760 and 0x41f4b0, the latter shared by every
// 4-byte-element vector): an explicit specialization declared without a definition makes every use a call.
template<> bool std::vector<rgl::entity*, std::allocator<rgl::entity*> >::_Buy(size_t);     // 0x43e760
template<> void std::vector<rgl::entity*, std::allocator<rgl::entity*> >::_Tidy();          // 0x41f4b0

int box_overlap2(const rgl::vector4* a, const rgl::vector4* b, const rgl::vector4* c, const rgl::vector4* d); // 0x426700
int box_overlap3(const rgl::vector4* a, const rgl::vector4* b, const rgl::vector4* c, const rgl::vector4* d); // 0x426690
// 0x40bfe0 (shapes.cpp): the shape pair's contact points; contact_rec is the constructor-free twin of shape.h's contact (0x24 bytes), the
// shapes are the shape_base objects the coll_shape stand-in views
struct shape_base;
struct contact;
int __cdecl shape_contact_count(shape_base* a, shape_base* b, int max, contact* out);
// 0x41f4b0 (entity.cpp): vec<mface>::clear, the growable array's release of its block; the contact vector (contact_vec) is released by the same code
struct mface;
template <class T> struct vec { void clear(); };    // global ::vec (not rgl::vec of mesh.h: that one mangles into rgl)
typedef vec<mface> GlobalVecMface;                  // (spelled through a global typedef: inside namespace rgl, `vec` is rgl::vec of mesh.h)
typedef void (GlobalVecMface::*VecClear)();
#define VEC_CLEAR(list) do { VecClear vc_ = &GlobalVecMface::clear; (((GlobalVecMface*)(list))->*vc_)(); } while (0)

#endif
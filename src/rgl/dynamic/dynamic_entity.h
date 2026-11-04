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
#include "./grid_shape.h"
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


// ---- the rigid body ----------------------------------------------------------------------------------------------------
// Fields read or written by the function cited. Its constructor is inline (forced) in dynamic_entity::dynamic_entity
// (0x425370), which is why the vptr store of that function sits between the contact list and the mass.
struct rigid_base : collision_entity {
    float mass;                 // 0x170  ctor 0x425370 (5.0), apply_impulse 0x425550
    float inertia;              // 0x174  (2.0), resolve 0x4259a0
    float dt;                   // 0x178  apply_forces 0x4257e0
    vector4 force;              // 0x17c
    vector4 torque;             // 0x18c
    vector4 vel;                // 0x19c
    vector4 angvel;             // 0x1ac
    float lin_damp;             // 0x1bc  (0.1)
    float ang_damp;             // 0x1c0  (0.8)
    vector4 shift;              // 0x1c4  pending position delta (collide_shape 0x427900, integrate 0x4260a0)
    bool grounded;              // 0x1d4
    bool asleep;                // 0x1d5

    __forceinline rigid_base()
    {
        vel = vector4(0.0f, 0.0f, 0.0f);
        angvel = vector4(0.0f, 0.0f, 0.0f);
        force = vector4(0.0f, 0.0f, 0.0f);
        torque = vector4(0.0f, 0.0f, 0.0f);
        shift = vector4(0.0f, 0.0f, 0.0f);
        asleep = false;
        restitution = 0.9f;
    }
};

// vtable 0x54b668, 0x1d8 bytes
struct dynamic_entity : rigid_base {
    dynamic_entity();                                                      // 0x425370
    virtual ~dynamic_entity();
    virtual int type_id() { return 7; }                                    // 1: 0x49df30 (ICF-shared with Camera::type)
    virtual string type_name();                                           // 2: 0x425490
    virtual void update(float dt);                                         // 7: 0x425730
    virtual bool awake();                                                  // 10: 0x4254b0
    virtual vector4* point_velocity(vector4* out, const vector4* p);       // 25: 0x4254c0 v + w x (p - origin)
    virtual void apply_impulse(const vector4* p, const vector4* j);        // 26: 0x425550
    virtual float impulse_response(const vector4* p, const vector4* n);    // 27: 0x425670
    virtual void apply_forces(float dt);                                   // 28: 0x4257e0
    virtual void collide();                                                // 29: 0x4263d0
    virtual void integrate(float dt);                                      // 30: 0x4260a0
    virtual void resolve();                                                // 31: 0x4259a0
    virtual void add_torque(const vector4* t);                             // 32: 0x4258e0
    virtual void add_torque_at(const vector4* p, const vector4* f);        // 33: 0x425920
    virtual bool on_entity(entity* e);                                     // 34: 0x427000 (the other entity as a rigid body)
    virtual bool on_shape(grid_shape* s);                                  // 35: 0x427900 (the shape as the entity's collision shape)
};

} // namespace rgl

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
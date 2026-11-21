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

// ---- std::vector<Contact>: collision_entity::contacts (scene_entity.h). The members the original keeps out of line are instantiated
// in world_collide.cpp: push_back 0x427450, insert 0x427540, _Insert_n 0x427590 (its helpers 0x4274c0 / 0x427880 / 0x427ca0).

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
#endif
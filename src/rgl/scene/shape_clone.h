// shape_clone.h: the collision-shape classes as entity::load and the shape resource need them. The shape
// hierarchy (sphere 0x54a4f0, capsule 0x54a540, face 0x54a588, composite 0x54a63c, manifold 0x54a5f4) is
// src/include/shape.h (manifold there is a view: its members need src/rgl/shapes/shapes.h's vec<T>, which
// collides with scene.h's vec). Sizes: sphere 0x44, capsule 0x64, face 0xb4, composite 0x28 (operator new sizes of
// shape_resource::clone_shape 0x4285e0).
#ifndef SCENE_SHAPE_CLONE_H
#define SCENE_SHAPE_CLONE_H
#include "../rgl_math.h"
#include "../resource.h"
#include "../../include/shape.h"

// composite's copy constructor is out of line at 0x433470 (game_util.cpp); the body copy 0x4285e0 expands it inline
__forceinline composite::composite(const composite& o) { ((composite*)this)->copy_from(&o); }

// the object resource_manager::get_object60 returns: a name and a shape (what an entity loads at +0x98)
struct shape_resource : named_object {
    shape_base* shape;                                          // 0x18 (== body_part::shape)
    shape_base* clone_shape();                                  // 0x4285e0
};

#endif

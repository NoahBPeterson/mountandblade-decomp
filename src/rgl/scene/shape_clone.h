// shape_clone.h: the collision-shape classes as entity::load and the shape resource need them. The shape
// hierarchy itself (sphere 0x54a4f0, capsule 0x54a540, face 0x54a588, composite 0x54a63c, manifold 0x54a5f4) is
// another object's; only the sizes, the copy constructors and the virtual slots used here are declared.
// ../resource.h's shape_base stops at slot 4: shape_x adds slots 5..14 (slot 14 is copy_from, called by the inline
// composite copy constructor). Sizes: sphere 0x44, capsule 0x64, face 0xb4, composite 0x28 (operator new sizes of
// shape_resource::clone_shape 0x4285e0).
#ifndef SCENE_SHAPE_CLONE_H
#define SCENE_SHAPE_CLONE_H
#include "../rgl_math.h"
#include "../resource.h"

struct shape_x : shape_base {
    virtual void v5(); virtual void v6(); virtual void v7(); virtual void v8(); virtual void v9();
    virtual void v10(); virtual void v11(); virtual void v12(); virtual void v13();
    virtual void copy_from(const shape_base* o);                // slot 14
};
struct shape_ext : shape_x { shape_ext(); };                    // 0x407ff0
struct composite : shape_ext {                                  // vtable 0x54a63c, size 0x28
    shape_base** bodies; int count;
    composite() : bodies(0), count(0) {}
    __forceinline composite(const composite& o) { ((shape_x*)this)->copy_from(&o); }
};
struct sphere_shape : shape_x {                                 // vtable 0x54a4f0, size 0x44
    rgl::vector4 v20, v30; float radius;
    sphere_shape(const sphere_shape& o);                        // 0x40c030
};
struct capsule_shape : shape_x {                                // vtable 0x54a540, size 0x64
    rgl::vector4 p0, p1, p2, p3; float radius;
    capsule_shape(const capsule_shape& o);                      // 0x40a580
};
struct face_shape : shape_x {                                   // vtable 0x54a588, size 0xb4
    char body[0xb4 - sizeof(shape_x)];
    face_shape(const face_shape& o);                            // 0x40ae70
};
struct manifold : shape_x {                                     // vtable 0x54a5f4
    shape_base* build_composite();                              // 0x40cb60
};

// the object resource_manager::get_object60 returns: a name and a shape (what an entity loads at +0x98)
struct shape_resource : named_object {
    shape_base* shape;                                          // 0x18 (== body_part::shape)
    shape_base* clone_shape();                                  // 0x4285e0
};

#endif

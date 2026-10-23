// grid_shape.h: the collision shape as the world's grid and the dynamic entity see it (src/rgl/dynamic). The full hierarchy
// (sphere, capsule, face, manifold, composite) is src/rgl/shapes/shapes.h, which cannot be included next to resource.h
// (scene.h pulls it in and both define shape_base). So the grid works on views of resource.h's shape_base (slots 0..4:
// destructor, shape_type, two stream slots) that add the slots 5..13 and the fields it reads. Offsets and slots as in
// shapes.h (cited there).
#ifndef RGL_DYNAMIC_GRID_SHAPE_H
#define RGL_DYNAMIC_GRID_SHAPE_H
#include "../rgl_math.h"
#include "../resource.h"

// the slots the grid calls: 9 set_mc (0x408040), 10 / 11 the bounding corners get_min / get_max (shapes.h)
struct grid_shape : shape_base {
    virtual void s5(); virtual void s6(); virtual void s7(); virtual void s8();
    virtual void set_mc(const int& v);                      // 9  (the entity id, to tag the shape with its owner)
    virtual rgl::vector4* get_min(rgl::vector4* out);       // 10
    virtual rgl::vector4* get_max(rgl::vector4* out);       // 11
};
struct grid_face : grid_shape {                             // shape_type 2: a convex polygon of at most 4 vertices
    rgl::vector4 w[4];                                      // +0x20 world vertices
    rgl::vector4 v[4];                                      // +0x60 local vertices
    rgl::vector4 normal;                                    // +0xa0
    int count;                                              // +0xb0
};
struct grid_composite : grid_shape {                        // shape_type 3: owns its bodies; 4 is a manifold (not placed)
    grid_shape** bodies;                                    // +0x20
    int count;                                              // +0x24
};

#endif

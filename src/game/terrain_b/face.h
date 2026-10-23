// face.h: the polygon collision shape (vtable 0x54a588, 0xb4 bytes) built by CellGrid::build_faces (0x479980) and
// queried by the ground-height probe (0x470f40), and a view of the generic shape interface the mission code calls
// through. Layout and slot names as in src/rgl/shapes/shapes.h (face, shape_base); that header cannot be included
// next to src/rgl/resource.h (both define a global shape_base), so the pieces used here are declared standalone.
#ifndef TERRAIN_B_FACE_H
#define TERRAIN_B_FACE_H
#include "../../rgl/rgl_math.h"

struct face {
    int m4, m8, mc, min_x, min_y, max_x, max_y;     // +0x04 (m4 >= 0: the polygon is active, 0x470f40)
    face() : m4(-1), m8(8), mc(-1), min_x(100000000), min_y(100000000), max_x(-100000000), max_y(-100000000), count(0) {}   // 0x408980 inline
    virtual ~face();                                // 0
    virtual int shape_type();                       // 1
    virtual void s2(); virtual void s3(); virtual void s4(); virtual void s5(); virtual void s6(); virtual void s7();
    virtual void transform(const rgl::matrix* m);   // 8 (the slices' set_frame)
    virtual void s9();
    virtual rgl::vector4* get_min(rgl::vector4* out);   // 10
    virtual rgl::vector4* get_max(rgl::vector4* out);   // 11
    virtual void s12();
    virtual void or_m8(int v);                      // 13
    rgl::vector4 w[4];                              // +0x20 world vertices (0x470f40 reads w[0])
    rgl::vector4 v[4];                              // +0x60 local vertices (0x479980)
    rgl::vector4 normal;                            // +0xa0 world normal (0x470f40)
    int count;                                      // +0xb0 vertex count
    bool contains_xy(const float* p) const;         // 0x4333c0
};

// The generic shape interface (any shape_base-derived object): only the slots the mission code calls.
struct shape_calls {
    virtual ~shape_calls();                         // 0
    virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4(); virtual void s5(); virtual void s6();
    virtual void s7();
    virtual void transform(const rgl::matrix* m);   // 8
    virtual void s9();
    virtual void get_min(float* out);               // 10 (the slices' bbox_min)
    virtual void get_max(float* out);               // 11 (bbox_max)
    virtual void s12();
    virtual void or_m8(int v);                      // 13 (set_flags)
};

// a body part (name + collision shape; resource.h's body_part): the shape pointer at +0x18 and its out-of-line clone
struct res_shape {
    char pad[0x18];
    shape_calls* shape;                             // +0x18
    shape_calls* clone_shape();                     // 0x4285e0
};

#endif // TERRAIN_B_FACE_H

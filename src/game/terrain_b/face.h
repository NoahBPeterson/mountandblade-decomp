// face.h: the polygon collision shape (vtable 0x54a588, 0xb4 bytes) built by CellGrid::build_faces (0x479980) and
// queried by the ground-height probe (0x470f40), and a view of the generic shape interface the mission code calls
// through. The face is src/include/shape.h's (with shape_base).
#ifndef TERRAIN_B_FACE_H
#define TERRAIN_B_FACE_H
#include "../../rgl/rgl_math.h"

// face (vtable 0x54a588, 0xb4 bytes; m4 >= 0: the polygon is active, 0x470f40; w[0] 0x470f40, v 0x479980) is src/include/shape.h's
#include "../../include/shape.h"

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

#include "../res_shape.h"                // res_shape: a body part (name + collision shape; shared with agent4)

#endif // TERRAIN_B_FACE_H

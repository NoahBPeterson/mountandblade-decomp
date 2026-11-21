// identity_value.h: identity3 (0x40cd60, shapes/shapes.cpp) seen through its hidden return pointer, as rgl_math.h's
// identity_frame is for identity_pos: the script interpreter's temporaries are built in place (Operation::execute 0x49fa10).
// Its own header, not rgl_math.h: one more class there renumbers cl's symbols in map_tooltip.cpp (0x51c090 off by 4 bytes).
#ifndef RGL_IDENTITY_VALUE_H
#define RGL_IDENTITY_VALUE_H
#include "./rgl_math.h"
namespace rgl {
struct mat3_value : mat3 {};                    // the rotation as a value with a base class (built in place, not copied once more)
mat3_value __cdecl identity3_value();           // defined in shapes/shapes.cpp: identity3 into the result
}
#endif // RGL_IDENTITY_VALUE_H

// collision_vminmax.cpp (0x40af80, 0x40b080): the component-wise vector minimum / maximum the face bounds (face::get_min /
// get_max, collision_shapes.cpp) call. Its own object: face::get_min / get_max only match when cl 13.10 sees these two as
// declarations (with the bodies visible it keeps `count` in a register across the call and takes `this` in ecx), so the original
// compiled them apart (docs/tu_map.md, visibility-twin splits). 0x40af80 and 0x40b080 sit between the two callers.
#include "game/common/iface.h"
#include "engine/physics/collision_shapes.h"
using namespace rgl;

static inline float fmin_(float a, float b) { return a < b ? a : b; }
static inline float fmax_(float a, float b) { return a > b ? a : b; }
// @ 0x40af80
__declspec(noinline) void __cdecl vmin(vector4* out, const vector4* a, const vector4* b)
{
    out->x = fmin_(a->x, b->x);
    out->y = fmin_(a->y, b->y);
    out->z = fmin_(a->z, b->z);
}
// @ 0x40b080
__declspec(noinline) void __cdecl vmax(vector4* out, const vector4* a, const vector4* b)
{
    out->x = fmax_(a->x, b->x);
    out->y = fmax_(a->y, b->y);
    out->z = fmax_(a->z, b->z);
}

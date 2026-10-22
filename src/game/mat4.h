// mat4.h: the engine's 64-byte frame (rows s(ide), f(orward), u(p) and origin o; the Module System's `pos`).
// Same layout as rgl::matrix in match/include/rgl.h; this header adds the out-of-line operations the game modules
// call. Shared by src/game/entity.cpp and src/game/renderer.cpp. Names provisional (rgl engine code).
// Verified: entity (0x41fcd0 / 0x41fda0 / 0x41fe50 copy the 0x40-byte frame at +0x18 / +0x58), renderer
// (0x461630, 0x462170: hidden-return-pointer calls of 0x40cd00 / 0x4329d0 / 0x40e870), frustum::to_clip
// (0x4621f0: 0x407f70 on the frame at frustum+0xc8).
#ifndef GAME_MAT4_H
#define GAME_MAT4_H
#include "rgl.h"

namespace rgl {

struct rot3 { vector4 s, f, u; };                          // 3x3 rotation, rows padded to 4 floats

struct mat4 {
    vector4 s, f, u, o;
    mat4() {}
    mat4(const rot3& r, const vector4& origin) { *(rot3*)this = r; o = origin; }   // renderer::set_view (0x461630)
    mat4 operator*(const mat4& b) const;                    // 0x40e870 (thiscall, this = left operand, hidden return)
    vector4 transform_point(const vector4& p) const;        // 0x408460
    vector4 transform_point_to_local(const vector4& p) const;   // 0x407f70 (rgl::matrix::transform_point_to_local)
    mat4 relative(const mat4& base) const;                  // 0x4329d0 (hidden return + base)
};
mat4 identity();                                            // 0x40cd00 (cdecl, hidden return pointer, returns it)

} // namespace rgl

#endif // GAME_MAT4_H

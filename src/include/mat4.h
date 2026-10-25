// mat4.h: the engine's frame types (namespace rgl) and the Module System `pos`.
//   rot3 / mat3 : 3x3 rotation, rows s(ide), f(orward), u(p) padded to 4 floats (48 bytes). The slices called both
//                 names; mat3 is the one with the by-value constructor (src/rgl/shapes/shapes.cpp builds results with it).
//   mat4        : rgl::matrix + the out-of-line operations the modules call. Same 64-byte layout everywhere:
//                 rows s, f, u and origin o. Verified by: entity (0x41fcd0 / 0x41fda0 / 0x41fe50 copy the 0x40-byte
//                 frame at +0x18 / +0x58), renderer (0x461630, 0x462170: hidden-return-pointer calls of 0x40cd00 /
//                 0x4329d0 / 0x40e870), frustum::to_clip (0x4621f0: 0x407f70 on the frame at frustum+0xc8), entity::render
//                 0x420310 (transform_point), FxDef::skin 0x4291c0 (rot_t, rot3), game_util 0x4329d0 (relative).
//   smat        : a frame whose rows can be scaled in place (entity::local, 0x41fee0).
//   pos         : global-namespace twin used by src/rgl/shapes and src/game/terrain_b (same function addresses as
//                 mat4: transform_point 0x408460, rot3 0x40a920, relative 0x4329d0).
// Names provisional (rgl engine code). Needs rgl::vector4 / rgl::matrix (rgl_base.h picks the module's own).
#ifndef MB_MAT4_H
#define MB_MAT4_H
#include "./rgl_base.h"

namespace rgl {

struct mat4;
struct rot3 { vector4 s, f, u; };                                   // 3x3 rotation, rows padded to 4 floats

struct mat3 {                                                       // same rows as rot3
    vector4 s, f, u;
    mat3() {}
    __forceinline mat3(const vector4& s_, const vector4& f_, const vector4& u_) : s(s_), f(f_), u(u_) {}
    mat4 operator*(const mat4& b) const;                    // 0x40e870 (the same function as mat4::operator*; char_sheet/save_panel.cpp 0x4ea0c0 calls it
                                                            // with a mat3 left operand, the 0x30-byte identity3 result, so the caller passes &mat3 in ECX)
};

struct mat4 : matrix {                                      // no constructors: the rgl scene code keeps it an aggregate
    mat4 operator*(const mat4& b) const;                    // 0x40e870 (thiscall, this = left operand, hidden return)
    vector4 transform_point(const vector4& p) const;        // 0x408460 (hidden return pointer)
    mat3 rot3(const matrix& m) const;                       // 0x40a920 (rotation part of this * m; hidden mat3 return)
    mat3* rot_t(mat3* out) const;                           // 0x429670 (transposed rotation of this, returns out)
    mat4 relative(const mat4& base) const;                  // 0x4329d0 (hidden return + base)
    void rotate_about_s(const float* angle);                     // 0x40e570 (thiscall, ret 4: char_sheet/facegen_misc.cpp, cam_view.cpp)
    void rotate_about_f(const float* angle);                     // 0x40e670 (thiscall, ret 4: char_sheet/cam_view.cpp)
};

// a frame whose rows can be scaled in place (entity::local)
struct smat : mat4 {
    void scale_rows(const vector4* k);                      // 0x41fee0
};

mat4 identity_pos();                                            // 0x40cd00 (cdecl, hidden return pointer, returns it)

} // namespace rgl

// the Module System position as src/rgl/shapes and src/game/terrain_b see it: rgl::matrix plus
struct pos : rgl::matrix {
    __declspec(noinline) rgl::vector4 transform_point(const rgl::vector4& p) const;      // 0x408460
    rgl::mat3 rot3(const rgl::matrix& m) const;                     // 0x40a920 (rotation rows of m through this)
    __declspec(noinline) rgl::vector4 rot(const rgl::vector4& p) const;                  // 0x40a9c0 (no translation)
    __forceinline rgl::vector4 transform_point_inl(const rgl::vector4& p) const     // the body as callers inline it
    {
        return rgl::vector4(s.x * p.x + f.x * p.y + u.x * p.z + o.x,
                            s.y * p.x + f.y * p.y + u.y * p.z + o.y,
                            s.z * p.x + f.z * p.y + u.z * p.z + o.z);
    }
    rgl::matrix relative(const rgl::matrix& o) const;               // 0x4329d0
};

#endif // MB_MAT4_H

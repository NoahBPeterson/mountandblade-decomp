// scene_math.h: frame types of the scene modules. rgl::matrix (../rgl_math.h) is the 64-byte frame {s, f, u, o};
// the scene code adds the point transform (0x408460), the rotation product (0x40a920), the transposed rotation
// (0x429670) and the in-place row scaling (0x41fee0) on top of it. Same layout everywhere: members only.
//
// Verified by: entity::render 0x420310 (transform_point on entity::local and mesh::m_cc), FxDef::skin 0x4291c0
// (rot_t, rot3), entity::set_local_scaled 0x41fe90 (scale_rows), recalc 0x41ffe0 (identity3).
#ifndef SCENE_MATH_H
#define SCENE_MATH_H
#include "../rgl_math.h"

namespace rgl {

struct mat3 { vector4 s, f, u; };                               // 3x3 rotation, rows padded to 4 floats

struct mat4 : matrix {
    vector4 transform_point(const vector4& p) const;            // 0x408460 (hidden return pointer)
    mat3 rot3(const matrix& m) const;                           // 0x40a920 (rotation part of this * m; hidden mat3 return)
    mat3* rot_t(mat3* out) const;                               // 0x429670 (transposed rotation of this, returns out)
};

// a frame whose rows can be scaled in place (entity::local)
struct smat : mat4 {
    void scale_rows(const vector4* k);                          // 0x41fee0
};

mat3* identity3(mat3* out);                                     // 0x40cd60 (cdecl; returns out)
matrix* identity_pos(matrix* out);                              // 0x40cd00 (cdecl; returns out)

// the point transform of a frame as the callers inline it
inline vector4 tp(const matrix& m, const vector4& p)
{
    return vector4(m.s.x * p.x + m.f.x * p.y + m.u.x * p.z + m.o.x,
                   m.s.y * p.x + m.f.y * p.y + m.u.y * p.z + m.o.y,
                   m.s.z * p.x + m.f.z * p.y + m.u.z * p.z + m.o.z);
}

} // namespace rgl
#endif

// scene_math.h: frame helpers of the scene modules. rgl::matrix (../rgl_math.h) is the 64-byte frame {s, f, u, o};
// the frame types the scene code builds on it (mat3, smat with scale_rows 0x41fee0, and matrix's transform_point 0x408460 / rot3
// 0x40a920 / rot_t 0x429670) live in rgl_math.h, the cdecl identity writers rgl::identity3 / rgl::identity_pos in src/include/mat4.h.
// This header keeps the inline point transform of the scene modules.
//
// Verified by: entity::render 0x420310 (transform_point on entity::local and mesh::m_cc), FxDef::skin 0x4291c0
// (rot_t, rot3), entity::set_local_scaled 0x41fe90 (scale_rows), recalc 0x41ffe0 (identity3).
#ifndef SCENE_MATH_H
#define SCENE_MATH_H
#include "../rgl_math.h"
#include "../../include/mat4.h"

namespace rgl {

// the point transform of a frame as the callers inline it
inline vector4 tp(const matrix& m, const vector4& p)
{
    return vector4(m.s.x * p.x + m.f.x * p.y + m.u.x * p.z + m.o.x,
                   m.s.y * p.x + m.f.y * p.y + m.u.y * p.z + m.o.y,
                   m.s.z * p.x + m.f.z * p.y + m.u.z * p.z + m.o.z);
}

} // namespace rgl
#endif

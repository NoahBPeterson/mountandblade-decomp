// scene_math.h: frame helpers of the scene modules. rgl::matrix (../rgl_math.h) is the 64-byte frame {s, f, u, o};
// the frame types the scene code builds on it (mat3, mat4 with transform_point 0x408460 / rot3 0x40a920 / rot_t
// 0x429670, smat with scale_rows 0x41fee0) are shared with src/game and live in src/include/mat4.h. This header keeps
// the cdecl identity writers and the inline point transform of the scene modules.
//
// Verified by: entity::render 0x420310 (transform_point on entity::local and mesh::m_cc), FxDef::skin 0x4291c0
// (rot_t, rot3), entity::set_local_scaled 0x41fe90 (scale_rows), recalc 0x41ffe0 (identity3).
#ifndef SCENE_MATH_H
#define SCENE_MATH_H
#include "../rgl_math.h"
#include "../../include/mat4.h"

// The writers are global-scope functions (rgl/shapes/shapes.cpp defines identity3 as `rgl::mat3* identity3(rgl::mat3*)` and identity_pos
// on the global `pos`, mat4.h); the rgl:: spellings the scene code uses forward to them.
rgl::mat3* __cdecl identity3(rgl::mat3* out);                   // 0x40cd60 (cdecl; returns out)
pos* __cdecl identity_pos(pos* out);                            // 0x40cd00 (cdecl; returns out)

namespace rgl {

// (identity3 has no rgl:: spelling: callers inside namespace rgl or under `using namespace rgl` find the global writer above)
__forceinline matrix* identity_pos(matrix* out) { return (matrix*)::identity_pos((pos*)out); }

// the point transform of a frame as the callers inline it
inline vector4 tp(const matrix& m, const vector4& p)
{
    return vector4(m.s.x * p.x + m.f.x * p.y + m.u.x * p.z + m.o.x,
                   m.s.y * p.x + m.f.y * p.y + m.u.y * p.z + m.o.y,
                   m.s.z * p.x + m.f.z * p.y + m.u.z * p.z + m.o.z);
}

} // namespace rgl
#endif

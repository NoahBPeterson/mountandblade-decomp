// fx_anim.h: the effect definition's animation (src/rgl/dynamic/fx_anim.cpp, fx_keys.cpp; 0x421df0..0x423c00): the
// per-frame update of FxDef (include/fx.h), its pose and envelope passes, the quaternion helpers, the key-frame
// evaluators and the key table. The classes are the shared ones: FxDef / FxElem / EnvRec (include/fx.h), key_array
// (include/key_table.h), skel_anim (../resource.h), quaternion (include/quaternion.h). The slices' FxAnim / Animator /
// Curve (FxDef), AnimRec / Rec / CurveRec (EnvRec), FxBone / QBlock (FxElem) and KeyObj (skel_anim) are folded.
#ifndef RGL_DYNAMIC_FX_ANIM_H
#define RGL_DYNAMIC_FX_ANIM_H
#include <vector>
#include "../../include/layout32.h"
#include <math.h>
#include <stddef.h>
#include "../rgl_string.h"
#include "../rgl_math.h"
#include "../scene/scene_math.h"
#include "../scene/frustum.h"
#include "../scene/scene.h"
#include "../scene/emitter.h"
#include "../scene/scene_entity.h"
#include "../scene/world.h"
#include "../scene/fx.h"

#include "../../include/quaternion.h"
__declspec(noinline) void quat_to_matrix(rgl::matrix* m, const quaternion* q);                                  // 0x423080
__declspec(noinline) void quat_to_axis_angle(rgl::vector4* axis, float* angle, const quaternion* q);            // 0x423160


extern unsigned __int64 g_bit[];                   // 0x5a8e68 g_bit[i] == 1 << i

#include "../../include/key_table.h"   // key_array (find 0x423a50 is defined in fx_keys.cpp)
#include "../../include/fx.h"
#include "../resource.h"                // skel_anim (EnvRec::table)

__declspec(noinline) void slerp(quaternion* out, const quaternion* a, const quaternion* b, const float* t);      // 0x4238c0
__declspec(noinline) void nlerp(quaternion* out, const quaternion* a, const quaternion* b, const float* t);      // 0x4239c0

#endif

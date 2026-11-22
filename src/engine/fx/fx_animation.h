// fx_animation.h: the effect definition's animation (src/engine/fx/fx_animation.cpp, fx_key_frames.cpp; 0x421df0..0x423c00): the
// per-frame update of FxDef (engine/fx/fx.h), its pose and envelope passes, the quaternion helpers, the key-frame
// evaluators and the key table. The classes are the shared ones: FxDef / FxElem / EnvRec (engine/fx/fx.h), key_array
// (game/input/key_table.h), skel_anim (../resource.h), quaternion (engine/math/quaternion.h). The slices' FxAnim / Animator /
// Curve (FxDef), AnimRec / Rec / CurveRec (EnvRec), FxBone / QBlock (FxElem) and KeyObj (skel_anim) are folded.
#ifndef RGL_DYNAMIC_FX_ANIM_H
#define RGL_DYNAMIC_FX_ANIM_H
#include <vector>
#include "engine/core/layout32.h"
#include <math.h>
#include <stddef.h>
#include "engine/core/rgl_string.h"
#include "engine/math/rgl_math.h"
#include "engine/math/scene_math.h"
#include "engine/scene/frustum_rgl_scene.h"
#include "engine/scene/scene.h"
#include "engine/scene/emitter.h"
#include "engine/scene/scene_entity.h"
#include "engine/scene/world.h"
#include "engine/fx/fx_rgl_scene.h"

#include "engine/math/quaternion.h"
__declspec(noinline) void quat_to_matrix(rgl::matrix* m, const quaternion* q);                                  // 0x423080
__declspec(noinline) void quat_to_axis_angle(rgl::vector4* axis, float* angle, const quaternion* q);            // 0x423160


extern unsigned __int64 g_bit[];                   // 0x5a8e68 g_bit[i] == 1 << i

#include "game/input/key_table.h"   // key_array (find 0x423a50 is defined in fx_key_frames.cpp)
#include "engine/fx/fx.h"
#include "engine/resource/resource.h"                // skel_anim (EnvRec::table)

__declspec(noinline) void slerp(quaternion* out, const quaternion* a, const quaternion* b, const float* t);      // 0x4238c0
__declspec(noinline) void nlerp(quaternion* out, const quaternion* a, const quaternion* b, const float* t);      // 0x4239c0

#endif

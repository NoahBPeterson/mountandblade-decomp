#ifndef CAM_VIEW_H
#define CAM_VIEW_H
// Slice s004ea8e0: the character-sheet camera/rotation widget. 0x4ea8e0 updates the model preview
// (zoom slider, clipboard pose dump, reset keys); 0x4eb050 is the per-frame orbit/zoom step.
// Names provisional; all offsets come from the disassembly of this range.
#include "platform/image_addrs.h"
#include <windows.h>
#include <math.h>
#include "engine/math/rgl_math.h"

using rgl::vector4;

#include "engine/math/mat4_include.h"
using rgl::mat4;
using rgl::mat3;

#include "game/ui/screens/character_sheet/cs_common.h"
#include "game/ui/screens/character_sheet/cs_mat.h"                                // identity_pos, UiD, WindowMgrW
#include "game/ui/screens/face_generator/face_gen.h"                              // FaceGen: the face-generator window record (the per-frame update 0x4ea8e0 is its slot 3)
#include "game/input/key_names.h"

// key state / settings globals

// globals of the face-generator update (0x4ea8e0)
extern float g_scroll;                                  // 0x590d7c

#include "engine/render/skin.h"                  // Skin, g_anim.skin_vec (shared with src/game/editor/editor_selection_and_translation.h)

#include "engine/scene/world.h"                      // rgl::world: the 0x344-byte scene (step 0x427f40, reset 0x421390, create_entity 0x428440)
#include "engine/core/tstring.h"
#include "game/data/str_table.h"      // 0x43a1e0
#include "game/ui/window_mgr.h"

#include "engine/io/text_stream.h"                    // mb_snprintf (0x415100)
#include "game/agent/animator.h"                              // Animator, g_anim
#include "game/ui/ui_globals.h"      // UI / core globals

#define SKIN_INT(field) (g_anim.skin_vec[g_anim.cur].field)
#define g_dummy_mat_arr (g_anim.weights)

#endif

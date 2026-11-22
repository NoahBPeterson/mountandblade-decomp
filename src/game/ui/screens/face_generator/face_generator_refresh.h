#ifndef FACEGEN_MISC_H
#define FACEGEN_MISC_H
// Slice s004eb270: the face-generator window (FaceGen, ctor 0x4f49d0; layouts shared with s004f49d0):
// the camera/frustum refresh, the skin randomiser/packer, the character-record array erase helper, the
// event forwarding thunk, the save/load UI entry and the "menu variables loaded" handler.
// Names provisional; offsets are from this range.
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "engine/math/rgl_math.h"
#include "engine/scene/scene_entity.h"   // rgl::entity / rgl::strategic_entity (the preview entity)
#include "game/ui/screens/character_sheet/cs_common.h"
#include "game/ui/screens/character_sheet/cs_mat.h"

using rgl::vector4;

// ---- engine matrix / frustum (s004f49d0, s004291c0) -----------------------------------------------------
#include "engine/math/mat4_include.h"
using rgl::mat4;
// (identity_pos 0x40cd00 is cs_common.h's)

#include "engine/scene/frustum.h"                   // rgl::frustum (0x120 bytes; the slices called it rgl::frustum)
// ---- the animation state ----
#include "game/agent/animator.h"                              // Animator, g_anim (the global character-builder/animation state, 0x8ed4f60), PackedState

// ---- tstring --------------------------------------------------------------------------------------------
#include "engine/core/tstring.h"
#include "game/ui/gui_manager.h"

// ---- save/load menu interfaces --------------------------------------------------------------------------
#include "platform/dxframework/d3dfont.h"               // CD3DFont (DrawText 0x404f00)
#include "engine/render/render_batch.h"                           // render_batch, g_batch (0x5fe898): frame 0x4183c0, render 0x47e520
#include "game/config/core.h"                          // GameApp g_app_obj (0x8ee3c48): the editor restart and the device-object handlers

// ---- UI widget (vtable family 0x597050; s004f49d0 / s004dada0) ------------------------------------------
#include "game/ui/widgets/ui_widget.h"                 // MissionUi (the UI widget interface)
struct Ui { MissionUi* vt; };
namespace rgl { struct world; }

#include "game/campaign/map_widget.h"                 // MapWidget (the slice's face_body: the 0x358-byte camera body, ctor 0x4ed630)
typedef MapWidget face_body;

// the preview entity: world::create_entity(9) makes a strategic_entity (vtable 0x54b5d0); refresh calls its
// clear() (slot 19, +0x4c, 0x420960) and pushes LOD records onto lods (std::vector<lod_mesh*> at +0x108) through
// std::vector<lod_mesh*>::push_back 0x41b750 (cs_common.h; ICF-folded with resource_manager.cpp's vector<named_object*> one)
#include "engine/math/vec.h"                      // vector2 (= Vec2)

// ---- skin record table (0x8ed4f7c, 0x1f8-byte records) -------------------------------------------------
#include "engine/render/skin.h"                  // Skin, g_anim.skin_vec (shared with src/game/editor/editor_selection_and_translation.h)
#include "game/data/module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in game/app/options_dialog_and_module_tables.cpp)

// ---- face-gen window: game/ui/screens/face_generator/face_gen.h ----------------------------------------------------------------
#include "game/ui/screens/face_generator/face_gen.h"
#include "game/data/game_data.h"            // GameData (g_game): the one definition (was a Game view here)
#include "game/ui/ui_globals.h"      // UI / core globals

// (0x432510 is clamp_index, cs_common.h)

extern const float g_590c54;                            // 0x590c54 = 7.0f
extern const float g_598588;                            // 0x598588 = 63.0f
extern "C" int __cdecl sprintf(char*, const char*, ...);            // 0x536f8f
extern "C" char* __cdecl _itoa(int, char*, int);                    // 0x5371f7
extern "C" double __cdecl floor(double);                            // 0x536b10

// integer part, +1 when the fraction is above 0.5 (same idiom as s004ed6f0)
static inline int grid_count(float v)
{
    double fl = floor((double)v);
    int f = (int)fl;
    if (v - (float)fl <= 0.5f)
        return f;
    return f + 1;
}

#endif

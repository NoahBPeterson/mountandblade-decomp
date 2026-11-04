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
#include "../../rgl/rgl_math.h"
#include "../../rgl/scene/scene_entity.h"   // rgl::entity / rgl::strategic_entity (the preview entity)
#include "./cs_common.h"
#include "./cs_mat.h"

using rgl::vector4;

// ---- engine matrix / frustum (s004f49d0, s004291c0) -----------------------------------------------------
#include "../../include/mat4.h"
using rgl::mat4;
// (identity_pos 0x40cd00 is cs_common.h's)

#include "../../include/frustum.h"                   // rgl::frustum (0x120 bytes; the slices called it rgl::frustum)
// ---- the animation state ----
#include "../animator.h"                              // Animator, g_anim (the global character-builder/animation state, 0x8ed4f60), PackedState

// ---- tstring --------------------------------------------------------------------------------------------
#include "../tstring.h"
#include "../strat/gui_manager.h"

// ---- save/load menu interfaces --------------------------------------------------------------------------
#include "../../dxframework/d3dfont.h"               // CD3DFont (DrawText 0x404f00)
#include "../render_batch.h"                           // render_batch, g_batch (0x5fe898): frame 0x4183c0, render 0x47e520
#include "../config/core.h"                          // GameApp g_app_obj (0x8ee3c48): the editor restart and the device-object handlers

// ---- UI widget (vtable family 0x597050; s004f49d0 / s004dada0) ------------------------------------------
#include "../ui_widget.h"                 // MissionUi (the UI widget interface)
struct Ui { MissionUi* vt; };
namespace rgl { struct world; }

#include "../map_widget.h"                 // MapWidget (the slice's face_body: the 0x358-byte camera body, ctor 0x4ed630)
typedef MapWidget face_body;

// the preview entity: world::create_entity(9) makes a strategic_entity (vtable 0x54b5d0); refresh calls its
// clear() (slot 19, +0x4c, 0x420960) and pushes LOD records onto lods (std::vector<lod_mesh2*> at +0x108) through
// std::vector<named_object*>::push_back 0x41b750 (cs_common.h)
#include "../../include/vec.h"                      // vector2 (= Vec2)

// ---- skin record table (0x8ed4f7c, 0x1f8-byte records) -------------------------------------------------
#include "../../include/skin_set.h"              // SkinSet, g_anim.skin_vec.first (shared with src/rgl/mesh2/gui_edit.h)
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)

// ---- face-gen window: game/face_gen.h ----------------------------------------------------------------
#include "../face_gen.h"
#include "../../include/game_data.h"            // GameData (g_game): the one definition (was a Game view here)
#include "../ui_globals.h"      // UI / core globals

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

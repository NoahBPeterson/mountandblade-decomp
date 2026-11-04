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
#include "../../include/msg_log.h"

// ---- save/load menu interfaces --------------------------------------------------------------------------
#include "../../dxframework/d3dfont.h"               // CD3DFont (DrawText 0x404f00)
#include "../render_batch.h"                           // render_batch, g_batch (0x5fe898): frame 0x4183c0, render 0x47e520
#include "../map_editor_obj.h"                      // MapEditor, instance at 0x8ee3c48
extern MapEditor g_editor_obj;                      // 0x8ee3c48
#define g_editor g_editor_obj
extern MsgLog g_msglog;                                 // 0x5d7080

// ---- UI widget (vtable family 0x597050; s004f49d0 / s004dada0) ------------------------------------------
#include "../ui_widget.h"                 // Widget (the UI widget interface)
typedef Widget UiVt;                    // the slice's name for it
struct Ui { UiVt* vt; };
struct World;

#include "../map_widget.h"                 // MapWidget (the slice's face_body: the 0x358-byte camera body, ctor 0x4ed630)
typedef MapWidget face_body;

// the preview entity: world::create_entity(9) makes a strategic_entity (vtable 0x54b5d0); refresh calls its
// clear() (slot 19, +0x4c, 0x420960) and pushes LOD records onto lods (std::vector<lod_mesh2*> at +0x108) through
// std::vector<named_object*>::push_back 0x41b750 (cs_common.h)
typedef rgl::strategic_entity Entity;
#include "../../include/vec.h"                      // vector2 (= Vec2)

// ---- skin record table (0x8ed4f7c, 0x1f8-byte records) -------------------------------------------------
#include "../../include/skin_set.h"              // SkinSet, g_anim.skins (shared with src/rgl/mesh2/gui_edit.h)
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)

// ---- face-gen window (ctor 0x4f49d0) --------------------------------------------------------------------
struct FaceGen {
    char pad0[0x14];
    int troop;                                          // +0x14
    char pad18[0x2c - 0x18];
    UiVt* ui;                                           // +0x2c
    face_body* body;                                    // +0x30
    rgl::frustum fr;                                     // +0x34 (0x120 bytes)
    float v154[4];                                      // +0x154
    float f164;                                         // +0x164
    float f168;                                         // +0x168
    float f16c;                                         // +0x16c zoom
    unsigned char f170;                                 // +0x170
    char pad171[0x174 - 0x171];
    int skin;                                           // +0x174
    World* world;                                       // +0x178
    UiVt* slider;                                       // +0x17c
    UiVt* deform;                                       // +0x180
    UiVt* buttons;                                      // +0x184
    UiVt* cam_btn;                                      // +0x188
    UiB* deform_rows[0x30];                             // +0x18c (one row per deform key; 0x30 slots up to key_labels)
    UiVt* key_labels[0x30];                             // +0x24c (up to b_next_beard)
    UiVt* b_next_beard;                                 // +0x30c
    UiVt* b_next_hair;                                  // +0x310
    UiVt* b_next_face;                                  // +0x314
    UiVt* b_prev_beard;                                 // +0x318
    UiVt* b_prev_hair;                                  // +0x31c
    UiVt* b_prev_face;                                  // +0x320
    UiVt* t_change_beard;                               // +0x324
    UiVt* t_change_hair;                                // +0x328
    UiVt* t_change_face;                                // +0x32c
    UiB* sl_age;                                        // +0x330
    UiB* sl_hair_color;                                 // +0x334
    UiVt* t_age;                                        // +0x338
    UiVt* t_hair_color;                                 // +0x33c
    UiVt* b_done;                                       // +0x340
    UiVt* b_reset;                                      // +0x344
    UiVt* b_randomize;                                  // +0x348
    Entity* ent;                                        // +0x34c
    int f350;                                           // +0x350
    int f354;                                           // +0x354
    int f358;                                           // +0x358
    int f35c;                                           // +0x35c
    float f360;                                         // +0x360
    float f364;                                         // +0x364
    PackedState state;                                  // +0x368
    char keys[4][0x28];                                 // +0x388
    void update_view(int unused);                       // 0x4eb270 (ret 4)
    void refresh();                                     // 0x4eb3c0
};
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

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
#include "rgl.h"
#define CS_REAL_FACEGEN                                 // (this header defines FaceGen)
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
struct SceneRegistry {};                                       // the render-pass object at 0x5fe898: RenderPass
struct RenderPass {
    bool render(int pass);   // 0x47e520
};
extern SceneRegistry g_renderer;                             // 0x5fe898
#include "../map_editor_obj.h"                      // MapEditor, instance at 0x8ee3c48
#define g_editor (*(MapEditor*)0x8ee3c48)
extern MsgLog g_msglog;                                 // 0x5d7080

// ---- UI widget (vtable family 0x597050; s004f49d0 / s004dada0) ------------------------------------------
#include "../ui_widget.h"                 // Widget (the UI widget interface)
typedef Widget UiVt;                    // the slice's name for it
struct Ui { UiVt* vt; };
struct Entity;
struct World;

#include "../map_widget.h"                 // MapWidget (the slice's face_body: the 0x358-byte camera body, ctor 0x4ed630)
typedef MapWidget face_body;

struct ui_vec {                                         // engine vector of void*
    int lead; void** first; void** last; void** end;
    // (push_back 0x41b750 is std::vector<named_object*>::push_back, cs_common.h)
};
struct Entity {                                         // 0x34c entity (EChar-like)
    UiVt* vt;                                           // +0x000
    char pad4[0x108 - 4];
    ui_vec lods;                                        // +0x108
};
#include "../../include/vec.h"                      // vector2 (= Vec2)

// ---- skin record table (0x8ed4f7c, 0x1f8-byte records) -------------------------------------------------
#include "../../include/skin_set.h"              // SkinSet, g_skins_begin (shared with src/rgl/mesh2/gui_edit.h)

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
    UiVt* deform_rows[0x1c];                            // +0x18c (one row per deform key)
    char pad1fc[0x24c - 0x1fc];
    UiVt* key_labels[0x1c];                             // +0x24c
    char pad2bc[0x30c - 0x2bc];
    UiVt* b_next_beard;                                 // +0x30c
    UiVt* b_next_hair;                                  // +0x310
    UiVt* b_next_face;                                  // +0x314
    UiVt* b_prev_beard;                                 // +0x318
    UiVt* b_prev_hair;                                  // +0x31c
    UiVt* b_prev_face;                                  // +0x320
    UiVt* t_change_beard;                               // +0x324
    UiVt* t_change_hair;                                // +0x328
    UiVt* t_change_face;                                // +0x32c
    UiVt* sl_age;                                       // +0x330
    UiVt* sl_hair_color;                                // +0x334
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

extern int g_cur;                                       // 0x8ed4f68
extern int g_x8ed4f64;                                  // 0x8ed4f64
extern int g_x8ed4f74;                                  // 0x8ed4f74
extern int g_char_count;                                // 0x8ed4f88
extern float g_dummy_mat;                               // 0x8ed4f8c
extern float g_dummy0;                                  // 0x8ed5088
extern float g_dummy1;                                  // 0x8ed508c
// (0x432510 is clamp_index, cs_common.h)

extern unsigned g_xa181ac;                              // 0xa181ac (shader-on flag)
extern const float g_590c54;                            // 0x590c54 = 7.0f
extern const float g_598588;                            // 0x598588 = 63.0f
extern const float g_549d70;                            // 0x549d70 = 0.5f
extern "C" int __cdecl sprintf(char*, const char*, ...);            // 0x536f8f
extern "C" char* __cdecl _itoa(int, char*, int);                    // 0x5371f7
extern "C" double __cdecl floor(double);                            // 0x536b10

// integer part, +1 when the fraction is above 0.5 (same idiom as s004ed6f0)
static inline int grid_count(float v)
{
    double fl = floor((double)v);
    int f = (int)fl;
    if (v - (float)fl <= g_549d70)
        return f;
    return f + 1;
}

#endif

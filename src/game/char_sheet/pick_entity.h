#ifndef PICK_ENTITY_H
#define PICK_ENTITY_H
// Slice s004ed8d0: the cursor pick.
//   0x4ed8d0  pick_entity - one contact test per scene object against the view cell; the nearest hit is
//             selected (GuiCtx::f2cc) and its label drawn.
// Class and member names are provisional.
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <vector>
#include "rgl.h"

using rgl::vector4;

// ---- constants / callees ------------------------------------------------------------------------------------
extern const float g_549cdc;            // 1.0f
extern const float g_549d70;            // 0.5f
extern const float g_3f000000;          // 0.5f   (contact radius)
extern const float g_3f8ccccd;          // 1.1f   (pick tolerance)
extern const float g_4e6e6b28;          // 1e9f   (far reach)
extern const float g_3ac49ba6;          // 1.5e-3f (capsule radius)

// ---- globals ------------------------------------------------------------------------------------------------
extern int g_menu_mode;                 // 0x5e2930+0x1a20c
extern int g_sel_entity;                // 0x5e2930+0x1a1b4
extern int g_tasks[];                   // 0x5e292c scene (0x65dc)
extern int g_map_rows[];                // 0x5e28f8 map rows (0x20 bytes)
extern int g_map_count;                 // 0x5e29e8
extern int g_point_rows[];              // 0x5e29dc point rows (0x78 bytes)
extern void* g_overlay;                 // 0x5d75e8
extern int g_cursor_id;                 // 0x5d8760
extern int g_cursor_color;              // 0x5d8764
extern int g_list0;                     // 0x5d8020
extern int g_list1;                     // 0x5d8024
extern float g_list2;                   // 0x5d8028
extern int g_sel_ent;                   // 0x5d8780

// ---- types --------------------------------------------------------------------------------------------------
#include "../tstring.h"
extern tstring g_strs[];                // 0x5d7850 .. 0x5d7d50

#include "../../include/shape.h"


using rgl::smat;
void identity_pos(float* m);                                // 0x40cd00
void capsule_ctor_inline(capsule* c, float r, const vector4* a, const vector4* b);  // 0x433310
int __cdecl shape_contact_count(shape_base* a, void* b, int max, void* out);    // 0x40bfe0
void __stdcall make_contact_box(void* out);                 // 0x408420

#include "../mission.h"
struct scene_row {                      // 0x1c8 bytes
    char pad000[0x18];
    vector4 mat[4];                     // +0x18 transform
    vector4 origin;                     // +0x58
    float radius;                       // +0x68
    char pad06c[0x188 - 0x6c];
    int kind;                           // +0x188
    int ref;                            // +0x18c
    int caption[4];                     // +0x190 tstring
    int point;                          // +0x1a0
    int f1a4;                           // +0x1a4
};

#include "../../include/vec.h"                      // vector2 (= Vec2)

#include "../view_rect.h"                    // ViewRect
struct OverlayItf {                     // vtable 0x590064
    virtual void release(int a);
};
#include "./gui_ctx.h"                // GuiCtx (was Ctx; shared with render_shape.h)

// ------------------------------------------------------------------------------------------------------------
#endif

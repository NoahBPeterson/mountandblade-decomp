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
#include "./cs_common.h"
#include "./cs_mat.h"

using rgl::vector4;

// ---- constants / callees ------------------------------------------------------------------------------------
extern const float g_549cdc;            // 1.0f
extern const float g_549d70;            // 0.5f
// compile-time constants: the original stores / pushes them as immediates (0x4ed921, 0x4ed99d, 0x4ed9a9), no image address
const float g_3f000000 = 0.5f;          // 0x3f000000 (contact radius)
const float g_3f8ccccd = 1.1f;          // 0x3f8ccccd (pick tolerance)
const float g_4e6e6b28 = 1e9f;          // 0x4e6e6b28 (far reach)
const float g_3ac49ba6 = 1.5e-3f;       // 0x3ac49ba6 (capsule radius)

// ---- globals ------------------------------------------------------------------------------------------------
// (the menu-mode flag is g_game->f1a20c, the selected entity g_game->f1a1b4, the scene is *g_mission: all fields of the pointed-to
//  objects, not globals of their own)
extern int* g_ptypes;                 // 0x5e28f8 pointer to the map rows (0x20 bytes)
extern int g_ptype_count;                 // 0x5e29e8
extern int* g_propkinds;              // 0x5e29dc pointer to the point rows (0x78 bytes)
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
// (identity_pos 0x40cd00: cs_mat.h; capsule::capsule(float, const vector4*, const vector4*) 0x433310 and sphere::transform 0x408420: shape.h)
struct contact;
int __cdecl shape_contact_count(shape_base* a, shape_base* b, int max, contact* out);    // 0x40bfe0
struct camera_ray {
    void screen_to_world(rgl::vector4* a, rgl::vector4* b);   // 0x4bd2f0 (the view rectangle's owner at GuiCtx+0x60)
};

#include "../mission.h"
#include "../../include/game_data.h"   // g_game
struct scene_row {                      // 0x1c8 bytes
    char pad000[0x18];
    vector4 mat[4];                     // +0x18 transform
    vector4 origin;                     // +0x58
    float radius;                       // +0x68
    char pad06c[0x158 - 0x6c];
    vector4 scale;                      // +0x158 (xyz: the object's scale; scale_rows 0x41fee0 takes it)
    char pad168[0x188 - 0x168];
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

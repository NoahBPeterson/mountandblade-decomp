#ifndef RENDER_SHAPE_H
#define RENDER_SHAPE_H
// Slice s004ed6f0: the render context `GuiCtx` (global object at 0x5d7080).
//   0x4ed6f0  GuiCtx::render_shape_frame - grid cell extents from the frame size, then one render of the extent
//             (with the caller's clock value +1) and the follow pass.
// Class and member names are provisional.
#include <math.h>
#include <vector>
#include "rgl.h"

using rgl::vector4;

// ---- constants / callees ------------------------------------------------------------------------------------
extern const float g_549cdc;            // 1.0f
extern float g_549d70;            // 0.5f
double __cdecl floor(double v);         // 0x536b10  CRT floor
inline int grid_count(float v)          // integer part (__ftol2 of floor), +1 when the fraction is above 0.5
{
    double fl = floor((double)v);
    int f = (int)fl;
    if (v - fl <= g_549d70) return f;
    return f + 1;
}

// ---- globals ------------------------------------------------------------------------------------------------
extern int g_view_z;                    // 0x5d75c8  width factor
extern int g_view_w;                    // 0x5d75cc  height factor
extern float g_div;                     // 0x5d7594  1/radius

#include "../../include/vec.h"                      // vector2 (= Vec2)

// ---- the render context (`this` = the global at 0x5d7080) ---------------------------------------------------
#include "../view_rect.h"                    // ViewRect

#include "./gui_ctx.h"                // GuiCtx (was Ctx), Vec1c

struct RenderScene {                    // the scene object at GuiCtx+0x178 (renamed from Scene: party/game.h Scene is the party manager at game+0xf8)
    char pad000[0xb8];
    Vec1c fb8;                          // +0xb8 (copied into GuiCtx+0x310)
    void render(int* cells, char a);              // 0x429890 renders the +0x17c extent
};


#endif

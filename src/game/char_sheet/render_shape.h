#ifndef RENDER_SHAPE_H
#define RENDER_SHAPE_H
// Slice s004ed6f0: the 3D view widget MapWidget (src/game/map_widget.h).
//   0x4ed6f0  MapWidget::render - grid cell extents from the frame size, then one render of the extent
//             (with the caller's clock value +1) and the follow pass.
// Class and member names are provisional.
#include <math.h>
#include <vector>
#include "../../rgl/rgl_math.h"
#include "../../rgl/scene/world.h"

using rgl::vector4;

// ---- constants / callees ------------------------------------------------------------------------------------
extern float g_549d70;            // 0.5f
double __cdecl floor(double v);         // 0x536b10  CRT floor
inline int grid_count(float v)          // integer part (__ftol2 of floor), +1 when the fraction is above 0.5
{
    double fl = floor((double)v);
    int f = (int)fl;
    if (v - fl <= 0.5f) return f;
    return f + 1;
}

// ---- globals ------------------------------------------------------------------------------------------------
extern float g_div;                     // 0x5d7594  1/radius

#include "../../include/vec.h"                      // vector2 (= Vec2)

// ---- the render context (`this` = the global at 0x5d7080) ---------------------------------------------------
#include "../view_rect.h"                    // ViewRect

#include "../map_widget.h"
#include "../ui_globals.h"      // UI / core globals
#include "../float_consts.h"

#endif

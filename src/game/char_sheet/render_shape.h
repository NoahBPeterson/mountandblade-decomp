#ifndef RENDER_SHAPE_H
#define RENDER_SHAPE_H
// Slice s004ed6f0: the render context `Ctx` (global object at 0x5d7080).
//   0x4ed6f0  Ctx::render_shape_frame - grid cell extents from the frame size, then one render of the extent
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

struct Vec1c { char b[0x1c]; void assign(const Vec1c* o); };   // 0x43f0a0 (thiscall operator=, 28-byte vector-like)

struct Scene {                          // the scene object at Ctx+0x178
    char pad000[0xb8];
    Vec1c fb8;                          // +0xb8 (copied into Ctx+0x310)
    void render(int* cells, char a);              // 0x429890 renders the +0x17c extent
};

struct Ctx {
    char pad000[0x60];
    int f60;                            // +0x60 owner of the view rectangle (f0e0)
    char pad064[0xe0 - 0x64];
    float f0e0, f0e4;                   // +0xe0, +0xe4
    char pad0e8[0xf0 - 0xe8];
    float f0f0, f0f4;                   // +0xf0, +0xf4
    char pad0f8[0x178 - 0xf8];
    Scene* f178;                        // +0x178 scene object
    int cell_x0, cell_y0, cell_x1, cell_y1;     // +0x17c..+0x188 grid extent
    int f18c;                           // +0x18c
    float f190;                         // +0x190
    char pad194[0x2dc - 0x194];
    char f2dc;                          // +0x2dc
    char pad2dd[0x310 - 0x2dd];
    Vec1c f310;                         // +0x310
    int f32c;                           // +0x32c
    char pad330[0x354 - 0x330];
    char f354;                          // +0x354  "next frame does nothing" flag
    void follow(int a, vector4* v, int b, int c);  // 0x488720
    void render_shape_frame(int a, vector4* v, int b, int c);   // 0x4ed6f0
};

#endif

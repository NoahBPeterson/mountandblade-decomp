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
extern const float g_549d70;            // 0.5f
double __cdecl floor(double v);         // 0x536b10  CRT floor
float ftol(int dummy);                  // 0x535a64  int conversion; the real argument arrives on the x87 stack
inline int grid_count(float v)          // integer part, +1 when the fraction is above 0.5
{
    int f = (int)floor(v);
    return ((v - (float)floor((double)v) > g_549d70) ? f + 1 : f);
}

// ---- globals ------------------------------------------------------------------------------------------------
extern int g_view_z;                    // 0x5d75c8  width factor
extern int g_view_w;                    // 0x5d75cc  height factor
extern float g_div;                     // 0x5d7594  1/radius

struct vector2 { float x, y; };

// ---- the render context (`this` = the global at 0x5d7080) ---------------------------------------------------
struct ViewRect {                       // the view rectangle of the current frame (this = &Ctx::f0e0)
    void cell_rect(int c0[4], int c1[4]);       // 0x4bd2f0
};

struct Ctx {
    char pad000[0x60];
    int f60;                            // +0x60 owner of the view rectangle (f0e0)
    char pad064[0xe0 - 0x64];
    float f0e0, f0e4, f0f0, f0f4;
    char pad0f8[0x178 - 0xf8];
    int f178;                           // +0x178 scene object
    int cell_x0, cell_y0, cell_x1, cell_y1;     // +0x17c..+0x188 grid extent
    int f18c;                           // +0x18c
    float f190;                         // +0x190
    char pad194[0x2c8 - 0x194];
    int f2c8, f2cc;                     // +0x2c8 / +0x2cc picked ids
    char pad2d0[0x2dc - 0x2d0];
    char f2dc;                          // +0x2dc
    char pad2dd[0x2f8 - 0x2dd];
    char f2f8;                          // +0x2f8
    char pad2f9[0x310 - 0x2f9];
    std::vector<int> f310;              // +0x310
    int f32c;                           // +0x32c
    char pad330[0x354 - 0x330];
    char f354;                          // +0x354  "next frame does nothing" flag
    char pad355[0x568 - 0x355];
    void* f568;                         // +0x568
    char pad56c[0x16e8 - 0x56c];
    float f16e8, f16ec, f16f0;          // +0x16e8..+0x16f0
    void __stdcall render_scene(int* cells, char a);         // 0x429890 renders the +0x17c extent
    void __stdcall follow(int a, vector4* v, int b, int c);  // 0x488720
    void __fastcall reset_all(char reinit);                  // 0x42cdb0
    void __fastcall set_pair(const vector2* p);              // 0x4c5190
};

// @ 0x4ed6f0
void __fastcall render_shape_frame(Ctx* self, int a, int b, vector4* v, int c, int d)
{
    if (self->f354) {
        self->f354 = 0;
        return;
    }
    self->cell_x0 = grid_count((float)g_view_z * self->f0e0);
    self->cell_y0 = grid_count((g_549cdc / g_div) * (g_div - (self->f0e4 + self->f0f4)) * (float)g_view_w);
    self->cell_x1 = grid_count((float)g_view_z * self->f0f0);
    self->cell_y1 = grid_count((g_549cdc / g_div) * (float)g_view_w * self->f0f4);
    self->f32c = self->f178 + 0xd8;
    self->f18c = 0;
    self->f190 = 1.0f;
    self->f2dc = 1;
    self->f310 = *(std::vector<int>*)(self->f178 + 0xb8);
    self->render_scene(&self->cell_x0, 1);
    v->x += 1.0f;
    self->follow(a, v, b, c);
}

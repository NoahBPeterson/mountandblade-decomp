// gui_ctx.h: the GUI root object at 0x5d7080 (g_gui, the gui_manager of src/game/strat/gui_manager.h) as char_sheet's
// render_shape_frame (0x4ed6f0) and pick_entity (0x4ed8d0) see it. ONE definition for those two modules: they each declared a
// partial view named `Ctx`, which also collided with strat's Ctx (the scene/world object of StratEnt, 0x42a1a0..); renamed GuiCtx.
// The inline scene at +0x44 puts the scene fields (f178, the cell extent at +0x17c) inside the root object; the draw/reset
// methods are the same functions as inventory_panel's gui_manager views (draw_string 0x4e43e0, reset 0x42cdb0, set_view 0x4c5190).
// The includer must have rgl::vector4 declared (rgl.h).
#ifndef CHAR_SHEET_GUI_CTX_H
#define CHAR_SHEET_GUI_CTX_H

struct tstring;
struct scene_row;
#include "../../include/vec.h"       // vector2 (= Vec2)
struct RenderScene;
struct Vec1c { char b[0x1c]; void assign(const Vec1c* o); };   // 0x43f0a0 (thiscall operator=, 28-byte vector-like)

struct GuiCtx {
    char pad000[0x60];
    int f60;                            // +0x60 owner of the view rectangle (f0e0)
    char pad064[0xe0 - 0x64];
    float f0e0, f0e4;                   // +0xe0, +0xe4
    char pad0e8[0xf0 - 0xe8];
    float f0f0, f0f4;                   // +0xf0, +0xf4
    char pad0f8[0x178 - 0xf8];
    RenderScene* f178;                        // +0x178 scene object
    int cell_x0, cell_y0, cell_x1, cell_y1;     // +0x17c..+0x188 grid extent
    int f18c;                           // +0x18c
    float f190;                         // +0x190
    char pad194[0x2c8 - 0x194];
    int f2c8, f2cc;                     // +0x2c8 / +0x2cc (pick_entity 0x4ed8d0: the picked entity index)
    char pad2d0[0x2dc - 0x2d0];
    char f2dc;                          // +0x2dc
    char pad2dd[0x2f8 - 0x2dd];
    char f2f8;                          // +0x2f8
    char pad2f9[0x310 - 0x2f9];
    Vec1c f310;                         // +0x310
    int f32c;                           // +0x32c
    char pad330[0x354 - 0x330];
    char f354;                          // +0x354  "next frame does nothing" flag
    char pad355[0x568 - 0x355];
    void* f568;                         // +0x568
    char pad56c[0x16e8 - 0x56c];
    float f16e8, f16ec, f16f0;          // +0x16e8
    void follow(int a, rgl::vector4* v, int b, int c);  // 0x488720
    void render_shape_frame(int a, rgl::vector4* v, int b, int c);   // 0x4ed6f0
    void __fastcall draw_string(tstring* s, float a, int color, char b);    // 0x4e43e0
    void __fastcall select_entity(scene_row* row, int ent, int flags);      // 0x4ea0c0
    void __fastcall reset_all(char reinit);                                 // 0x42cdb0
    void __fastcall set_pair(const vector2* p);                             // 0x4c5190
};

#endif // CHAR_SHEET_GUI_CTX_H

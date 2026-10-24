#ifndef PICK_ENTITY_H
#define PICK_ENTITY_H
// Slice s004ed8d0: the cursor pick.
//   0x4ed8d0  pick_entity - one contact test per scene object against the view cell; the nearest hit is
//             selected (Ctx::f2cc) and its label drawn.
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

struct shape_base {                     // collision shape base
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    virtual ~shape_base() {}
    virtual int shape_type();
    virtual void s2(void* m);           // set transform
    virtual void s3(); virtual void s4(); virtual void s5(); virtual void s6();
    virtual void s7(); virtual void s8(); virtual void s9();
    virtual vector4* get_min(vector4* out);
    virtual vector4* get_max(vector4* out);
};
struct shape_ext : shape_base { shape_ext(); };
struct capsule : shape_ext {            // size 0x64
    vector4 p0, p1, p2, p3;
    float radius;
    capsule(float r, const vector4* a, const vector4* b);    // 0x433310
};
struct sphere_shape : shape_base {      // size 0x44
    vector4 v20, v30; float radius;
    sphere_shape(const vector4* centre, float r);           // 0x408770
};
struct smat : rgl::matrix {
    void scale_rows(const vector4* k);                      // 0x41fee0
};
void identity_pos(float* m);                                // 0x40cd00
void capsule_ctor_inline(capsule* c, float r, const vector4* a, const vector4* b);  // 0x433310
int __cdecl shape_contact_count(shape_base* a, void* b, int max, void* out);    // 0x40bfe0
void __stdcall make_contact_box(void* out);                 // 0x408420

struct scene {
    char pad000[0x65dc];
    int f65dc;                          // +0x65dc entity being built
    int f65e0;                          // +0x65e0 entity list
    char pad5e4[0x668c - 0x65e4];
    int f668c;                          // +0x668c scene objects
    int f6690;                          // +0x6690 end
};
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

struct vector2 { float x, y; };

struct ViewRect {                       // this = Ctx::f60
    void cell_rect(int c0[4], int c1[4]);        // 0x4bd2f0
};
struct OverlayItf {                     // vtable 0x590064
    virtual void release(int a);
};
struct Ctx {
    char pad000[0x60];
    int f60;
    char pad064[0xe0 - 0x64];
    float f0e0, f0e4, f0f0, f0f4;
    char pad0f8[0x178 - 0xf8];
    int f178;
    int cell_x0, cell_y0, cell_x1, cell_y1;
    int f18c;
    float f190;
    char pad194[0x2c8 - 0x194];
    int f2c8, f2cc;
    char pad2d0[0x2dc - 0x2d0];
    char f2dc;
    char pad2dd[0x2f8 - 0x2dd];
    char f2f8;
    char pad2f9[0x310 - 0x2f9];
    int f310;
    char pad314[0x32c - 0x314];
    int f32c;
    char pad330[0x354 - 0x330];
    char f354;
    char pad355[0x568 - 0x355];
    void* f568;
    char pad56c[0x16e8 - 0x56c];
    float f16e8, f16ec, f16f0;
    void __fastcall draw_string(tstring* s, float a, int color, char b);    // 0x4e43e0
    void __fastcall select_entity(scene_row* row, int ent, int flags);      // 0x4ea0c0
    void __fastcall reset_all(char reinit);                                 // 0x42cdb0
    void __fastcall set_pair(const vector2* p);                             // 0x4c5190
};

// ------------------------------------------------------------------------------------------------------------
#endif

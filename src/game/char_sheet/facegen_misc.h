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

using rgl::vector4;

// ---- engine matrix / frustum (s004f49d0, s004291c0) -----------------------------------------------------
#include "../../include/mat4.h"
using rgl::mat4;
mat4* identity_pos(mat4* out);                          // 0x40cd00 (cdecl)

struct eng_frustum {
    unsigned char on;                                   // +0x000
    char pad1[0xf8 - 1];
    float f_f8[4];                                      // +0x0f8
    float f108;                                         // +0x108
    float f10c;                                         // +0x10c
    float f110;                                         // +0x110
    float f114;                                         // +0x114
    float f118;                                         // +0x118
    float f11c;                                         // +0x11c
    void build_planes();                                // 0x41e4f0
};

// ---- tstring --------------------------------------------------------------------------------------------
#include "../tstring.h"
#include "../../include/msg_log.h"

// ---- save/load menu interfaces --------------------------------------------------------------------------
struct CD3DFont { void DrawText(float, float, unsigned long, const char*, unsigned long); }; // 0x404f00
struct Renderer { void func_47e520(int); };             // 0x47e520, instance at 0x5fe898
extern Renderer g_renderer;                             // 0x5fe898
struct MapEditor {                                      // instance at 0x8ee3c48
    void restart();                                     // 0x4c0520
    void func_4c1a60();                                 // 0x4c1a60
    void func_4c1940();                                 // 0x4c1940
};
#define g_editor (*(MapEditor*)0x8ee3c48)
extern MsgLog g_msglog;                                 // 0x5d7080

// ---- UI widget (vtable family 0x597050; s004f49d0 / s004dada0) ------------------------------------------
struct UiVt {
    virtual void s00(); virtual void s01(); virtual void s02(); virtual void s03(); virtual void s04();
    virtual void s05(); virtual void s06(); virtual void s07(); virtual void s08(); virtual void s09();
    virtual void s10(); virtual void s11(); virtual void s12(); virtual void s13();
    virtual float value();                              // +0x38
    virtual void s15(); virtual void s16(); virtual void s17(); virtual void s18();
    virtual void refresh_anim();                        // +0x4c
    virtual void s20(); virtual void s21(); virtual void s22(); virtual void s23(); virtual void s24();
    virtual void s25(); virtual void s26(); virtual void s27(); virtual void s28(); virtual void s29();
    virtual void s30(); virtual void s31(); virtual void s32(); virtual void s33(); virtual void s34();
    virtual void s35(); virtual void s36(); virtual void s37(); virtual void s38(); virtual void s39();
    virtual void s40();
    virtual void set_text(const void* a, const void* b);   // +0xa4
};
struct Ui { UiVt* vt; };
struct Entity;
struct World;

// the 0x358-byte camera body (ctor 0x4ed630): widget + world link + frustum copy
struct face_body {
    Ui ui;                                              // +0x000 (vptr)
    char pad004[0x178 - 0x4];
    World* world;                                       // +0x178
    char pad17c[0x1ac - 0x17c];
    eng_frustum view;                                   // +0x1ac (0x120 bytes)
};

struct ui_vec {                                         // engine vector of void*
    int lead; void** first; void** last; void** end;
    void push_back(void* const& v);                     // 0x41b750 (ret 4)
};
struct Entity {                                         // 0x34c entity (EChar-like)
    UiVt* vt;                                           // +0x000
    char pad4[0x108 - 4];
    ui_vec lods;                                        // +0x108
};
struct vector2 { float x, y; };

// ---- skin record table (0x8ed4f7c, 0x1f8-byte records) -------------------------------------------------
#include "../../include/skin_set.h"              // SkinSet, g_skins_begin (shared with src/rgl/mesh2/gui_edit.h)

// ---- face-gen window (ctor 0x4f49d0) --------------------------------------------------------------------
struct PackedState { unsigned __int64 a, b, c, d; };
struct FaceGen {
    char pad0[0x14];
    int troop;                                          // +0x14
    char pad18[0x2c - 0x18];
    UiVt* ui;                                           // +0x2c
    face_body* body;                                    // +0x30
    eng_frustum fr;                                     // +0x34 (0x120 bytes)
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
struct Game { char pad0[0x13a00]; char* chars; };
extern Game* g_game;                                    // 0x5e2930

// the global character-builder/animation state (0x8ed4f60; s0047f650, s004f49d0)
struct Animator {
    int f0;                                             // +0x00
    int f4;                                             // +0x04
    int cur;                                            // +0x08
    int f0c;                                            // +0x0c
    int f10;                                            // +0x10
    int f14;                                            // +0x14
    char pad18[0x1c - 0x18];
    SkinSet* skins;                                     // +0x1c
    char pad20[0x28 - 0x20];
    int char_count;                                     // +0x28
    float values[64];                                   // +0x2c
    float f12c;                                         // +0x12c
    void randomise();                                   // 0x4eb320
    void collect(bool shader, void** a, void** b, void** c);    // 0x47fb10 (ret 0x10)
    void pack_state(PackedState* out);                  // 0x4eb660 (ret 4)
};
extern Animator g_anim;                                 // 0x8ed4f60
extern int g_cur;                                       // 0x8ed4f68
extern int g_x8ed4f64;                                  // 0x8ed4f64
extern int g_x8ed4f74;                                  // 0x8ed4f74
extern int g_char_count;                                // 0x8ed4f88
extern float g_dummy_mat;                               // 0x8ed4f8c
extern float g_dummy0;                                  // 0x8ed5088
extern float g_dummy1;                                  // 0x8ed508c
int gen_face_seed(int* flags, int* zero, int* count);   // 0x432510 (cdecl)

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

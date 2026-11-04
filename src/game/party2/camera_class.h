// camera_class.h: the one declaration of the 0x4a8-byte view object built by the constructor 0x49deb0 (Camera). Its members are defined
// in party2/camera.cpp: type 0x49df30, mask 0x49df40, move 0x49df60, update 0x49dfe0, scalar deleting destructor 0x49e580, render 0x49e5b0.
//
// Layout: the engine object base (Object, 0x188 bytes, constructed by 0x48ae20), the render scene (rgl::scene, 0x1d8 bytes at +0x188),
// the box frustum (rgl::frustum, 0x120 bytes at +0x360), two follow objects at +0x480 and the followed shape at +0x490.
// The scrolling-list widgets of the other modules (party screen skills list, the editor / quest list 0x4f62f0, the HUD tree, the
// party-create item list) are built by the same constructor and use +0x484 (follow[1]) and +0x490 (target) for their scroll bar and
// parent / row count.
// Include after the translation unit's declaration of rgl::scene (src/rgl/mesh.h, or src/game/rgl_mesh.h in the rgl.h universe: both are
// the 0x1d8-byte class); a translation unit that only passes Camera pointers around forward-declares it.
#ifndef MB_CAMERA_CLASS_H
#define MB_CAMERA_CLASS_H
#include <stddef.h>
#include "../../include/frustum.h"
#include "../../include/timer.h"

struct follow_iface {                           // the two follow objects at +0x480
    virtual void  s0(); virtual void  s1(); virtual void  s2(); virtual void  s3();
    virtual void  s4(); virtual void  s5(); virtual void  s6(); virtual void  s7();
    virtual void  s8(); virtual void  s9(); virtual void  s10(); virtual void  s11();
    virtual void  s12(); virtual void  s13(); virtual float s14(); virtual void  s15();
};                                              // s14 = [this+0x38]: the follow weight (0..1)

struct sub_iface {                              // the target's sub-object (*(target+0x17c))
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3();
    virtual void s4(); virtual void s5(); virtual void s6(); virtual void s7();
    virtual void s8(); virtual void s9(); virtual void s10(); virtual void s11();
    virtual void s12(); virtual void s13(); virtual void s14(); virtual void s15();
    virtual void s16(); virtual void s17(); virtual void s18(); virtual void s19();
    virtual void s20(); virtual void s21(); virtual void s22(); virtual void s23();
    virtual void s24(); virtual void s25(); virtual void s26();
    virtual void s27(void* v);                                          // [this+0x6c] zoom
    virtual void s28(); virtual void s29(); virtual void s30(); virtual void s31();
    virtual void s32(); virtual void s33(); virtual void s34(); virtual void s35();
    virtual void s36();
    virtual void s37(void* v);                                          // [this+0x94] colour reset
    virtual void s38(void* v);                                          // [this+0x98] colour set
    char pad_s[0x14c - 4];
    unsigned colour;                            // +0x14c
};

struct float2 {                                 // a 2-float vector (position/displacement)
    float x, y;
    float2() {}
    float2(float a, float b) : x(a), y(b) {}
};

struct shape_ref {                              // the object the view follows (an entity/agent)
    struct iface {                              // s24 = [this+0x60]: "position changed"
        virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3();
        virtual void s4(); virtual void s5(); virtual void s6(); virtual void s7();
        virtual void s8(); virtual void s9(); virtual void s10(); virtual void s11();
        virtual void s12(); virtual void s13(); virtual void s14(); virtual void s15();
        virtual void s16(); virtual void s17(); virtual void s18(); virtual void s19();
        virtual void s20(); virtual void s21(); virtual void s22(); virtual void s23();
        virtual void s24();
    };
    char pad0[0xe0];
    float2 pos;                                 // +0xe0  world position, written by move()
    float2 disp;                                // +0xe8  displacement for this frame
    float sa[2];                                // +0xf0  shake target (near)
    float sb[2];                                // +0xf8  shake base (far)
    char pad100[0x17c - 0x100];
    sub_iface** sub;                            // +0x17c  pointer to the sub-object pointer
};

// ---- the engine object base (constructor 0x48ae20, destructor body 0x490520) ------------------------------------------
struct Object {
    void*   vptr;                               // +0x000  descriptor
    int     f004;                               // +0x004
    Timer   timer[6];                           // +0x008 .. +0x068
    float   lim[6];                             // +0x068  per-timer fade length (-1 = idle)
    float   w40, w41;                           // +0x080  timer 5 blend weights (near)
    float   w20, w21;                           // +0x088  timer 4 blend weights (near)
    unsigned char lv[8];                        // +0x090  colour levels (rendered)
    char    pad098[0x0a0 - 0x098];
    float   w42, w43;                           // +0x0a0  timer 5 blend weights (far)
    float   w22, w23;                           // +0x0a8  timer 4 blend weights (far)
    unsigned char le[8];                        // +0x0b0  colour levels (target)
    char    pad0b8[0x0c0 - 0x0b8];
    unsigned char flag0, flag1, f_c2, f_c3, flag2, flag3;   // +0x0c0
    char    pad0c6[0x0c8 - 0x0c6];
    int     f0c8, f0cc, f0d0, f0d4;             // +0x0c8
    float   f_d8, f_dc;                         // +0x0d8  zoom levels written by update()
    float   px, py;                             // +0x0e0  view position
    float   f_e8, f_ec;                         // +0x0e8
    float   w, h;                               // +0x0f0  view box size
    float   f_f8, f_fc;                         // +0x0f8
    int     f100;                               // +0x100
    unsigned flags;                             // +0x104  drawing flags tested by mask()
    int     arr108[5];                          // +0x108
    unsigned char b11c, b11d, b11e, b11f, b120; // +0x11c
    unsigned char f121, f122;                   // +0x121
    char    pad123[0x124 - 0x123];
    int     f124;                               // +0x124
    char    tstr[0x10];                         // +0x128  an engine string
    float   f138;                               // +0x138
    int     f13c;                               // +0x13c
    float   f140, f144;                         // +0x140
    int     f148;                               // +0x148
    unsigned colour;                            // +0x14c  view colour (AARRGGBB)
    unsigned colour2;                           // +0x150
    int     f154;                               // +0x154
    int     f158, f15c, f160;                   // +0x158
    int     f164;                               // +0x164
    int     f168, f16c;                         // +0x168
    float   f170;                               // +0x170
    int     f174, f178;                         // +0x174
    int     sub0, sub1, sub2;                   // +0x17c

    void base_ctor();                           // 0x48ae20 (MissionUi's constructor)
    void dtor_common();                         // 0x490520
    void update();                              // 0x48d470
    void terrain_refresh(void* p);              // 0x48f120  (p = &w20)
    void set_color(unsigned v);                 // 0x48d350
    void set_alpha(unsigned v);                 // 0x48d3c0
    void bounds_of(void* scene, float* pos, void* c, void* d);   // 0x490710
};

// ---- the view itself ----------------------------------------------------------------------------------------------
struct Camera : Object {
    rgl::scene   scene;                         // +0x188  (0x429740 / 0x41f4e0)
    rgl::frustum fr;                            // +0x360  (0x41ef60)
    follow_iface* follow[2];                    // +0x480  the two follow objects (the list widgets keep their scroll bar in follow[1], +0x484)
    void*   follow_shape[2];                    // +0x488  their shapes
    shape_ref* target;                          // +0x490  the object the view tracks (the list widgets keep their parent / row count here)
    float   shake_x;                            // +0x494
    float   shake_y;                            // +0x498
    float2  tail;                               // +0x49c
    int     f4a4;                               // +0x4a4

    Camera();                                   // 0x49deb0
    int  type();                                // 0x49df30
    unsigned mask(int a);                       // 0x49df40
    void move(shape_ref* e);                    // 0x49df60
    void update(int unused);                    // 0x49dfe0
    void* destroy(unsigned flags);              // 0x49e580
    void render(int a, float* pos, void* c, void* d);   // 0x49e5b0
};

// compile-time layout check (VC7.1 has no static_assert)
#define MB_CAMERA_ASSERT(name, cond) typedef char camera_assert_##name[(cond) ? 1 : -1]
MB_CAMERA_ASSERT(object_size, sizeof(Object) == 0x188);
MB_CAMERA_ASSERT(scene_off, offsetof(Camera, scene) == 0x188);
MB_CAMERA_ASSERT(frustum_off, offsetof(Camera, fr) == 0x360);
MB_CAMERA_ASSERT(follow_off, offsetof(Camera, follow) == 0x480);
MB_CAMERA_ASSERT(follow1_off, offsetof(Camera, follow) + sizeof(follow_iface*) == 0x484);
MB_CAMERA_ASSERT(target_off, offsetof(Camera, target) == 0x490);
MB_CAMERA_ASSERT(flags_off, offsetof(Camera, flags) == 0x104);
MB_CAMERA_ASSERT(camera_size, sizeof(Camera) == 0x4a8);

#endif // MB_CAMERA_CLASS_H

// Slice s0049deb0: the mission "view" object.  It is an engine object (base ctor 0x48ae20, base dtor
// 0x490520, descriptor 0x591c80) with a scene subobject at +0x188, a box frustum at +0x360 (0x120
// bytes) and the per-frame shake/colour/zoom fields that update() drives.  The view's own descriptor
// is 0x5931a8 and its size is 0x4a8.
//
// ctor 0x49deb0, type 0x49df30, mask 0x49df40, move 0x49df60, update 0x49dfe0,
// scalar deleting dtor 0x49e580, render 0x49e5b0.
#include <windows.h>
#include <string.h>
#include <math.h>
#include "rgl.h"
using rgl::vector4;

// ---- callees that live in other slices -----------------------------------------------------------
void make_identity(float* m);                   // 0x40cd00  out-only 4x4 identity (cdecl)
extern "C" double _floor(double v);             // 0x536b10  lib_crt::_floor
float grid_round(float v);                      // 0x535a64  __ftol2 (int in eax, x87 arg)

// ---- float / int constants (named by address, matching the game's own tables) ---------------------
float g_tick_scale = 1e-5f;                    // 0x54a6d0  1e-05f  (tick -> seconds)
extern const float g_549cdc;                    // 0x549cdc  1.0f
extern const float g_549cd8;                    // 0x549cd8  4294967296.0f (unsigned->float bias)
extern const float g_549d70;                    // 0x549d70  0.5f
extern const float g_590cd0;                    // 0x590cd0  0.02f

// int globals: the view's z/w scale factors (zero in the image, set at startup by the game).
extern int g_view_z;                            // 0x5d75c8
extern int g_view_w;                            // 0x5d75cc
// tick counters, 8 bytes each; timers index this array through their own key field.
extern __int64 g_ticks[];                       // 0x5e2948
// ---- stub interfaces for the virtual calls -------------------------------------------------------
struct follow_iface {                           // the two follow objects at +0x480
    virtual void  s0(); virtual void  s1(); virtual void  s2(); virtual void  s3();
    virtual void  s4(); virtual void  s5(); virtual void  s6(); virtual void  s7();
    virtual void  s8(); virtual void  s9(); virtual void  s10(); virtual void  s11();
    virtual void  s12(); virtual void  s13(); virtual float s14(); virtual void  s15();
};                                              // s14 = [this+0x38]: the follow weight (0..1)

struct view_iface {                             // `this` seen as its own descriptor (0x5931a8)
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3();
    virtual void s4(); virtual void s5(); virtual void s6(); virtual void s7();
    virtual void s8(); virtual void s9(); virtual void s10(); virtual void s11();
    virtual void s12(); virtual void s13(); virtual void s14(); virtual void s15();
    virtual void s16(); virtual void s17(); virtual void s18(); virtual void s19();
    virtual void s20(); virtual void s21();
    virtual void s22(void* v);                                          // [this+0x58] shake
    virtual void s23(void* v);                                          // [this+0x5c] follow target
};

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

// ---- the sub-objects -----------------------------------------------------------------------------
struct Scene {                                  // the scene subobject, 0x1d8 bytes at +0x188
    int   viewport[4];                          // +0x000  x, y, w, h (pixels)
    int   f10;                                  // +0x010
    float f14;                                  // +0x014
    char  pad18[0x18c - 0x18];
    int   flags;                                // +0x18c
    char  tail[0x1d8 - 0x190];
    void ctor();                                // 0x429740
    void dtor();                                // 0x41f4e0
    void reset();                               // 0x428f30
};

struct Frustum {                                // the box frustum, 0x120 bytes at +0x360
    unsigned char first;                        // +0x000
    char  pad1[0x108 - 0x1];
    float box[6];                               // +0x108  half extents + the far plane
    void ctor();                                // 0x41ef60
    void build();                               // 0x41e4f0  recompute the 6 planes
    // the box setter (inline): left, right, bottom, top, near, far
    void set_box(float l, float r, float b, float t, float n, float f)
    {
        first = 0;
        box[0] = l;
        box[1] = r;
        box[3] = b;
        box[2] = t;
        box[4] = n;
        box[5] = f;
    }
};

struct Timer {                                  // one of six 0x10-byte timers
    int     key;                                // +0x00  index into g_ticks
    int     pad;                                // +0x04
    __int64 stamp;                              // +0x08  value of g_ticks[key] when started
};

// ---- the engine object base (0x48ae20) ------------------------------------------------------------
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

    void base_ctor();                           // 0x48ae20
    void base_dtor();                           // 0x490520
    void reset_timer_a();                       // 0x48d470
    void reset_timer_b(void* p);                // 0x48f120  (p = &w20)
    void reset_timer_c(unsigned v);             // 0x48d350
    void reset_timer_d(unsigned v);             // 0x48d3c0
    void scene_render(void* scene, float* pos, void* c, void* d);   // 0x490710
};

// ---- the view itself -----------------------------------------------------------------------------
struct Camera : Object {
    Scene   scene;                              // +0x188
    Frustum fr;                                 // +0x360
    follow_iface* follow[2];                    // +0x480  the two follow objects
    void*   follow_shape[2];                    // +0x488  their shapes
    shape_ref* target;                          // +0x490  the object the view tracks
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

// Blend one colour level from `a` (target) to `b` (rendered) over a fade of length `lim` that has run for
// `d`, rounded half-up.  The first colour byte is read as a masked dword (unsigned), the others as bytes.
static __forceinline int blend_u(const float& lim, const float& d, unsigned a, unsigned b)
{
    float v = ((float)a * (lim - d) + (float)b * d) * (1.0f / lim);
    float fl = (float)floor(v);
    int i = (int)fl;
    if (!(v - fl <= 0.5f))
        i++;
    return i;
}
static __forceinline int blend_i(const float& lim, const float& d, int a, int b)
{
    float v = ((lim - d) * (float)a + (float)b * d) * (1.0f / lim);
    float fl = (float)floor(v);
    int i = (int)fl;
    if (!(v - fl <= 0.5f))
        i++;
    return i;
}

// @ 0x49deb0
Camera::Camera()
{
    base_ctor();
    sub0 = 0;
    sub1 = 0;
    sub2 = 0;
    vptr = (void*)0x5931a8;                     // the view's own class descriptor
    scene.ctor();
    fr.ctor();
    shake_x = 0;
    shake_y = 0;
    follow[0] = 0;
    follow[1] = 0;
    target = 0;
    tail = float2(0, 0);
}

// @ 0x49df30
int Camera::type()
{
    return 7;
}

// @ 0x49df40
unsigned Camera::mask(int a)
{
    if (a == 0)
        return flags & 0x10;
    return flags & 0x20;
}

// @ 0x49df60
// Place the followed object at the view position (plus its own displacement), undo the current shake
// if it is the tracked object, then tell it it moved.
void Camera::move(shape_ref* e)
{
    e->pos = float2(e->disp.x + px, e->disp.y + py);
    if (e == target) {
        e->pos.x = e->pos.x - shake_x;
        e->pos.y = e->pos.y - shake_y;
    }
    ((shape_ref::iface*)e)->s24();
}

// @ 0x49dfe0
// Per-frame view update: the two shake axes follow the tracked object while its follow objects report
// a weight, then the four fades (zoom, colour, level, shake) are advanced.
void Camera::update(int)
{
    float2 v;

    shake_x = target->sb[0];
    shake_y = target->sb[1];
    for (int i = 2; i != 0; i--) {
        int k = 2 - i;
        if (follow[k] && follow_shape[k]) {
            float r = follow[k]->s14();
            (&shake_x)[k] = (target->sa[k] - target->sb[k] - (&w)[k]) * r + target->sb[k];
        }
    }
    ((view_iface*)this)->s23(target);

    float d0 = (float)(g_ticks[timer[0].key] - timer[0].stamp) * g_tick_scale;    // colour
    float d1 = (float)(g_ticks[timer[1].key] - timer[1].stamp) * g_tick_scale;    // level
    float d4 = (float)(g_ticks[timer[4].key] - timer[4].stamp) * g_tick_scale;    // zoom
    float d5 = (float)(g_ticks[timer[5].key] - timer[5].stamp) * g_tick_scale;    // shake

    // zoom: blend the target's zoom from the near weights toward the far ones
    if (d4 < lim[4]) {
        float inv = 1.0f / lim[4];
        float rest = lim[4] - d4;
        float xn = d4 * w20 + rest * w22;
        float yn = rest * w23 + d4 * w21;
        float x = xn * inv;
        float y = yn * inv;
        v.x = x * 0.02f;
        v.y = y * 0.02f;
        (*target->sub)->s27(&v);
        f_d8 = x;
        f_dc = y;
        reset_timer_a();
    } else if (flag2) {
        flag2 = 0;
        reset_timer_b(&w20);
    }

    // colour: the three level bytes blended from the target levels and packed under the inherited
    // alpha byte, then the target sub-object's colour slot
    if (d0 < lim[0]) {
        int i0 = blend_u(lim[0], d0, *(unsigned*)le & 0xff, *(unsigned*)lv & 0xff);
        int i1 = blend_i(lim[0], d0, le[1], lv[1]);
        int i2 = blend_i(lim[0], d0, le[2], lv[2]);
        unsigned col = (colour | (*target->sub)->colour) & 0xff000000u;
        col |= (unsigned)((i2 << 16) + (i1 << 8) + i0);
        (*target->sub)->s38(&col);
        (*target->sub)->s37(&v);
    } else if (flag0) {
        flag0 = 0;
        reset_timer_c(colour ^ ((colour ^ *(unsigned*)lv) & 0xffffff));
    }

    // level
    if (d1 < lim[1]) {
        reset_timer_d(blend_i(lim[1], d1, le[7], lv[7]));
    } else if (flag1) {
        flag1 = 0;
        reset_timer_d(lv[7]);
    }

    // shake: blend toward the far weights and hand the offset to the object
    if (d5 < lim[5]) {
        float inv = g_549cdc / lim[5];
        float rest = lim[5] - d5;
        v.x = (d5 * w40 + rest * w42) * inv;
        v.y = (d5 * w41 + rest * w43) * inv;
        ((view_iface*)this)->s22(&v);
        reset_timer_a();
    } else if (flag3) {
        ((view_iface*)this)->s22(&w40);
        reset_timer_a();
        flag3 = 0;
    }
}

// @ 0x49e580  (scalar deleting destructor)
void* Camera::destroy(unsigned flags)
{
    scene.dtor();
    base_dtor();
    if (flags & 1)
        ::operator delete(this);
    return this;
}

// @ 0x49e5b0
// Place the view: size the scene viewport from the view box, rebuild the box frustum, install it as
// the scene's volume and the view matrix, then hand the frame to the scene.  The view matrix is the
// identity with its translation row set from a position vector whose w is never initialised (the
// original copies that stack slot into m[15]).
struct V4 { float x, y, z, w; };
void Camera::render(int a, float* pos, void* c, void* d)
{
    scene.reset();
    scene.viewport[0] = (int)((float)g_view_z * px);
    scene.viewport[1] = (int)((0.75f - (py + h)) * (float)g_view_w * 1.3333334f);
    scene.viewport[2] = (int)((float)g_view_z * w);
    scene.viewport[3] = (int)((float)g_view_w * h * 1.3333334f);
    scene.f14 = 1.0f;
    scene.f10 = 0;
    scene.flags |= 0x1000;
    Frustum* f = &fr;
    f->set_box(w * -0.5f, w * 0.5f, h * -0.5f, h * 0.5f, 0, 501.0f);
    f->build();
    float m[16];
    make_identity(m);
    V4 v;
    v.z = 400.0f;
    v.x = w * 0.5f + px;
    v.y = h * 0.5f + py;
    *(V4*)&m[12] = v;
    memcpy((char*)f + 0xc8, m, 0x40);
    f->build();
    memcpy((char*)this + 0x1b8, f, 0x120);
    *pos = *pos + 1.0f;
    scene_render(&scene, pos, c, d);
}

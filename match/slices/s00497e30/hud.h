// hud.h: the mission HUD widget family that slice s00497e30 decompiles. The three functions run on two
// related objects:
//   * 0x497e30 and 0x498390 are methods of the two-bar/one-colour widget (the same class as slice
//     s00497880; the layout matches up to the mesh pointers) — 0x497e30 is the animating refresh of the
//     bars/colour/position with a "force the frame mesh to rebuild" marker (0x497e30, 0x498390).
//   * 0x498420 is a method of the outer HUD object that owns that widget (this+0x484) plus a scroll
//     object (this+0x490) and the two scrollbar meshes.
// Names are provisional; the offsets are what the functions read and write (OFFSET_CHECK keeps them
// honest).
#ifndef MB_HUD_H
#define MB_HUD_H
#include <vector>
#include "rgl.h"

using rgl::vector4;

extern __int64 g_ticks[];                        // 0x5e2948 (100 kHz clocks)

// 0x432540, verified in s00432130: floor, then "> 0.5 rounds up"
static inline int round_float(float* p)
{
    float fl = (float)floor((double)*p);
    int i = (int)fl;
    return (*p - fl <= 0.5f) ? i : i + 1;
}

struct Timer {                                   // 0x10 bytes at hud+0x08
    int clock;
    int pad4;
    __int64 start;
    float elapsed() const { return (float)(g_ticks[clock] - start) * 1e-5f; }
};

union Color4 {
    unsigned dword;
    struct { unsigned char r, g, b, a; };
};

// the bar mesh (vtable 0x54a6f4, ctor 0x412a90); only what these functions touch
struct mesh {
    virtual ~mesh();                             // scalar deleting dtor via vtable slot 0 (ret 4)
    char pad4[0x38];
    unsigned color;                              // +0x3c: packed ARGB
    void scale(vector4 v);                       // 0x47af30 (this, vector4 by value, ret 0x10)
    mesh* clone();                               // 0x41c6d0 (this, ret 0)
};
#define OFFSET_CHECK(t, m, o) typedef char offset_check_##t##_##m[(offsetof(t, m) == (o)) ? 1 : -1]
OFFSET_CHECK(mesh, color, 0x3c);

// only the slots these functions call are typed
struct HudBase {
    virtual void s00();
    virtual void s01();
    virtual void s02();
    virtual void invalidate();                   // +0x0c
    virtual void s04();
    virtual void s05();
    virtual void s06();
    virtual void s07();
    virtual void s08();
    virtual void s09();
    virtual void s10();
    virtual void s11();
    virtual void s12();
    virtual void s13();
    virtual void s14();
    virtual void s15();
    virtual void s16();
    virtual void s17();
    virtual void s18();
    virtual void s19();
    virtual void s20();
    virtual void s21();
    virtual void set_pos(vector4* v);            // +0x58
    virtual void s23();
    virtual void s24();
    virtual void s25(vector4* v);                // +0x64
    virtual void s26();
    virtual void s27(vector4* v);                // +0x6c
    virtual void s28(vector4* v);                // +0x70
    virtual void s29();
    virtual void s30();
    virtual void s31();
    virtual void s32(unsigned a, mesh* m);       // +0x80
    virtual void s33();
    virtual void s34();
    virtual void s35();
    virtual void s36();
    virtual void s37(unsigned* c);               // +0x94 set_colour (aggregate)
    virtual void s38(unsigned* c);               // +0x98 set_colour (group 1)
    virtual void s39(unsigned* c);               // +0x9c set_colour (group 2)
};

struct Hud2 : HudBase {
    char pad04[4];                               // +0x04
    Timer timers[6];                             // +0x08 .. 0x68
    float dur[6];                                // +0x68 .. 0x80
    vector4 box;                                 // +0x80 (x,y: size; z,w: second size)
    Color4 col_a, alpha_a, col_a2, alpha_a2;     // +0x90, +0x94, +0x98, +0x9c
    vector4 target;                              // +0xa0
    Color4 col_b, alpha_b, col_b2, alpha_b2;     // +0xb0 .. 0xbc
    char c0, c1, c2, c3, c4, c5;                 // +0xc0 .. 0xc5
    char padc6[0xd8 - 0xc6];
    float d8, dc;                                // +0xd8, +0xdc: the last applied scale
    char pade0[0xf0 - 0xe0];
    float speed[4];                              // +0xf0: per-lane scale animation
    char pad100[0x108 - 0x100];
    mesh* m108;                                  // +0x108: bar mesh
    mesh* m10c;                                  // +0x10c: the widget's current-frame mesh
    mesh* m110;                                  // +0x110: bar mesh
    char pad114[0x14c - 0x114];
    unsigned c14c;                               // +0x14c: the colour word the bar colour keeps
    unsigned c150;                               // +0x150
    char pad154[0x168 - 0x154];
    float f168, f16c, f170;                      // +0x168 frame / +0x16c min / +0x170 max
    char pad174[0x178 - 0x174];
    int idx;                                     // +0x178: which colour component the frame scales
    float f17c;                                  // +0x17c: the frame scale
    char pad180[0x18c - 0x180];
    mesh* src;                                   // +0x18c: the mesh the frame colour is applied to
    char pad190[0x194 - 0x190];
    float f194;                                  // +0x194: the frame the current mesh was built for
    vector4 v198;                                // +0x198: the colour the frame scales
    unsigned mask;                               // +0x1a8: the alpha bits the frame mesh's colour keeps

    void update2(int arg);                       // 0x497e30
    void scale_lane(float arg);                  // 0x498390
    void refresh();                              // 0x497d10 (slice s00497880)
};
OFFSET_CHECK(Hud2, timers, 0x08);
OFFSET_CHECK(Hud2, dur, 0x68);
OFFSET_CHECK(Hud2, box, 0x80);
OFFSET_CHECK(Hud2, col_a, 0x90);
OFFSET_CHECK(Hud2, alpha_a, 0x94);
OFFSET_CHECK(Hud2, col_a2, 0x98);
OFFSET_CHECK(Hud2, alpha_a2, 0x9c);
OFFSET_CHECK(Hud2, target, 0xa0);
OFFSET_CHECK(Hud2, col_b, 0xb0);
OFFSET_CHECK(Hud2, alpha_b, 0xb4);
OFFSET_CHECK(Hud2, col_b2, 0xb8);
OFFSET_CHECK(Hud2, alpha_b2, 0xbc);
OFFSET_CHECK(Hud2, c0, 0xc0);
OFFSET_CHECK(Hud2, c5, 0xc5);
OFFSET_CHECK(Hud2, d8, 0xd8);
OFFSET_CHECK(Hud2, dc, 0xdc);
OFFSET_CHECK(Hud2, speed, 0xf0);
OFFSET_CHECK(Hud2, m108, 0x108);
OFFSET_CHECK(Hud2, m10c, 0x10c);
OFFSET_CHECK(Hud2, m110, 0x110);
OFFSET_CHECK(Hud2, c14c, 0x14c);
OFFSET_CHECK(Hud2, c150, 0x150);
OFFSET_CHECK(Hud2, f168, 0x168);
OFFSET_CHECK(Hud2, f16c, 0x16c);
OFFSET_CHECK(Hud2, f170, 0x170);
OFFSET_CHECK(Hud2, idx, 0x178);
OFFSET_CHECK(Hud2, f17c, 0x17c);
OFFSET_CHECK(Hud2, src, 0x18c);
OFFSET_CHECK(Hud2, f194, 0x194);
OFFSET_CHECK(Hud2, v198, 0x198);
OFFSET_CHECK(Hud2, mask, 0x1a8);

// 16 bytes; the engine's interned string (ctor 0x42bbe0, dtor 0x42bc40).
struct RString {
    char* p; int size, cap, extra;
    RString(const char* s);                      // 0x42bbe0
    ~RString();                                  // 0x42bc40
};

// the engine's mesh/asset registry (the `this` 0x419540 looks the string up in is the global at 0x5d6fa0).
struct MeshMgr {
    mesh* find(RString* s);                      // 0x419540
};
extern MeshMgr g_meshes;                         // 0x5d6fa0

// a vtable object reached as *(hud+0x490)->+0x17c (two pointer hops); only the two slots 0x498420 calls.
struct ScrollTarget {
    virtual void t00();
    virtual void t01();
    virtual void t02();
    virtual void t03();
    virtual void t04();
    virtual void t05();
    virtual void t06();
    virtual void t07();
    virtual void t08();
    virtual void t09();
    virtual void t10();
    virtual void t11();
    virtual void t12();
    virtual void t13();
    virtual void t14();
    virtual void t15();
    virtual void t16();
    virtual void t17();
    virtual void t18();
    virtual void t19();
    virtual void t20();
    virtual void t21();
    virtual void t22();
    virtual void t23();
    virtual void t24();
    virtual void t25(vector4* arg);              // +0x64
    virtual void t26();
    virtual void t27(void* p);                   // +0x6c
};

struct ScrollHolder {                            // the object at hud+0x490
    char pad[0x17c];
    ScrollTarget** target;                       // +0x17c
};

// the outer mission HUD that owns the bar widget and the scrollbar
struct HudOuter {
    char pad[0x484];
    Hud2* bar;                                   // +0x484
    char pad488[0x490 - 0x488];
    ScrollHolder* scroll;                        // +0x490
    float f49c;                                  // +0x49c
    float f4a0;                                  // +0x4a0

    void scroll_refresh(vector4* arg);           // 0x498420
    void on_scroll();                            // 0x48d470
};

#endif

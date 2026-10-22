// Slice s00496d30: one frame-update of a mission HUD bar widget (3 bar meshes, two colour groups).
// It lerps three meshes' scale, two groups of bar colours and alpha, plus the widget position, from
// the elapsed timers, then tells the widget base to refresh itself.
// Names provisional; layout recovered from this function and its neighbours.
#include <windows.h>
#include <math.h>
#include "rgl.h"

using rgl::vector4;

extern __int64 g_ticks[];                        // 0x5e2948 (100 kHz clocks)

// 0x432540, verified in s00432130 (the pointer taking form is what gets inlined here)
static inline int round_float(float* p)
{
    float fl = (float)floor((double)*p);
    int i = (int)fl;
    return (*p - fl <= 0.5f) ? i : i + 1;
}

struct Timer {
    int clock;
    int pad4;
    __int64 start;
    float elapsed() const { return (float)(g_ticks[clock] - start) * 1e-5f; }
};

namespace rgl {
struct mesh {
    char pad0[0x3c];
    unsigned color;                              // +0x3c
    void scale(vector4 v);                       // 0x47af30 (ret 0x10)
};
}

// widget base: only the slots this slice calls are typed
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
    virtual void s25();
    virtual void s26();
    virtual void s27(vector4* v);                // +0x6c
    virtual void s28(vector4* v);                // +0x70
    virtual void s29();
    virtual void s30();
    virtual void s31();
    virtual void s32();
    virtual void s33();
    virtual void s34();
    virtual void s35();
    virtual void s36();
    virtual void s37(vector4* v);                // +0x94
    virtual void set_color(unsigned* c);         // +0x98
    virtual void s39(unsigned* c);               // +0x9c
};

union Color4 {
    unsigned dword;
    struct { unsigned char r, g, b, a; };
};

struct Hud1 : HudBase {
    char pad04[4];                               // +0x04
    Timer timers[6];                             // +0x08 .. 0x68
    float dur[6];                                // +0x68 .. 0x80
    vector4 box;                                 // +0x80 (x,y: position; z,w: second position)
    Color4 col_a;                                // +0x90 colour
    Color4 alpha_a;                              // +0x94 colour
    Color4 col_a2;                               // +0x98 colour
    Color4 alpha_a2;                             // +0x9c colour
    vector4 target;                              // +0xa0
    Color4 col_b;                                // +0xb0 colour
    Color4 alpha_b;                              // +0xb4 colour
    Color4 col_b2;                               // +0xb8 colour
    Color4 alpha_b2;                             // +0xbc colour
    char c0, c1, c2, c3, c4, c5;                 // +0xc0
    char padc6[0xd8 - 0xc6];
    float d8, dc;                                // +0xd8, +0xdc
    char pade0[0x10c - 0xe0];
    rgl::mesh* m10c;                             // +0x10c
    rgl::mesh* m110;                             // +0x110
    rgl::mesh* m114;                             // +0x114
    char pad118[0x14c - 0x118];
    unsigned c14c;                               // +0x14c
    unsigned c150;                               // +0x150
    void update(int arg);                        // 0x496d30
};

// @ 0x496d30
void Hud1::update(int)
{
    float e0 = timers[0].elapsed();
    float e1 = timers[1].elapsed();
    float e2 = timers[2].elapsed();
    float e3 = timers[3].elapsed();
    float e4 = timers[4].elapsed();
    float e5 = timers[5].elapsed();
    unsigned char dirty = 0;

    if (e4 < dur[4]) {
        float k = 1.0f / dur[4];
        float x = (e4 * box.z + (dur[4] - e4) * target.z) * k;
        float y = ((dur[4] - e4) * target.w + e4 * box.w) * k;
        m110->scale(vector4(x / d8, y / dc, 1.0f));
        m10c->scale(vector4(x / d8, y / dc, 1.0f));
        m114->scale(vector4(x / d8, y / dc, 1.0f));
        float sy = y * 0.02f;
        vector4 v(sy, sy, 1.0f);
        s27(&v);
        v.x = sy;
        v.y = y * 0.013f;
        s28(&v);
        invalidate();
        d8 = x;
        dc = y;
        dirty = 1;
    } else if (c4) {
        m110->scale(vector4(box.z / d8, box.w / dc, 1.0f));
        m10c->scale(vector4(box.z / d8, box.w / dc, 1.0f));
        m114->scale(vector4(box.z / d8, box.w / dc, 1.0f));
        float sy = box.w * 0.02f;
        vector4 w(sy, sy, 1.0f);
        s27(&w);
        w.x = box.w * 0.02f;
        w.y = box.w * 0.013f;
        s28(&w);
        invalidate();
        c4 = 0;
        d8 = box.z;
        dc = box.w;
        dirty = 1;
    }

    if (e0 < dur[0]) {
        unsigned col = 0;
        float v = ((col_b.dword & 0xff) * (dur[0] - e0) + (col_a.dword & 0xff) * e0) * (1.0f / dur[0]);
        col = round_float(&v);
        v = (col_b.g * (dur[0] - e0) + col_a.g * e0) * (1.0f / dur[0]);
        col += round_float(&v) << 8;
        v = (col_b.b * (dur[0] - e0) + col_a.b * e0) * (1.0f / dur[0]);
        col += round_float(&v) << 16;
        col |= c14c & 0xff000000;
        set_color(&col);
        dirty = 1;
    } else if (c0) {
        unsigned col = (c14c ^ col_a.dword) & 0xffffff ^ c14c;
        c0 = 0;
        set_color(&col);
        dirty = 1;
    }

    if (e1 < dur[1]) {
        unsigned col = 0;
        float v = (alpha_b.a * (dur[1] - e1) + alpha_a.a * e1) * (1.0f / dur[1]);
        col = round_float(&v) << 24;
        m110->color = (m110->color & 0xffffff) | col;
        m10c->color = (m10c->color & 0xffffff) | col;
        m114->color = (m114->color & 0xffffff) | col;
        col |= c14c & 0xffffff;
        set_color(&col);
        dirty = 1;
    } else if (c1) {
        c1 = 0;
        unsigned a = alpha_a.dword;
        m110->color = (m110->color & 0xffffff) | a;
        m10c->color = (m10c->color & 0xffffff) | a;
        m114->color = (m114->color & 0xffffff) | a;
        unsigned col = (c14c ^ a) & 0xffffff ^ a;
        set_color(&col);
        dirty = 1;
    }

    if (e2 < dur[2]) {
        unsigned col = 0;
        float v = ((col_b2.dword & 0xff) * (dur[2] - e2) + (col_a2.dword & 0xff) * e2) * (1.0f / dur[2]);
        col = round_float(&v);
        v = (col_b2.g * (dur[2] - e2) + col_a2.g * e2) * (1.0f / dur[2]);
        col += round_float(&v) << 8;
        v = (col_b2.b * (dur[2] - e2) + col_a2.b * e2) * (1.0f / dur[2]);
        col += round_float(&v) << 16;
        col |= c150 & 0xff000000;
        s39(&col);
        dirty = 1;
    } else if (c2) {
        unsigned col = (c150 ^ col_a2.dword) & 0xffffff ^ c150;
        c2 = 0;
        s39(&col);
        dirty = 1;
    }

    if (e3 < dur[3]) {
        unsigned col = 0;
        float v = (alpha_b2.a * (dur[3] - e3) + alpha_a2.a * e3) * (1.0f / dur[3]);
        col = round_float(&v) << 24;
        col |= c150 & 0xffffff;
        s39(&col);
        dirty = 1;
    } else if (c3) {
        unsigned a = alpha_a2.dword;
        unsigned col = (c150 ^ a) & 0xffffff ^ a;
        c3 = 0;
        s39(&col);
        dirty = 1;
    }

    if (e5 < dur[5]) {
        float k = 1.0f / dur[5];
        vector4 v;
        v.x = (e5 * box.x + (dur[5] - e5) * target.x) * k;
        v.y = ((dur[5] - e5) * target.y + e5 * box.y) * k;
        set_pos(&v);
    } else if (c5) {
        set_pos(&box);
        c5 = 0;
    }

    if (dirty) {
        vector4 v;
        s37(&v);
    }
}

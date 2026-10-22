// Slice s004966b0: same per-frame HUD bar update as s00496100 but the bar is drawn with two meshes
// (+0x110 and +0x10c): scale/position lerps and colour/alpha are pushed to both. Names provisional.
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
    virtual void s27();
    virtual void s28();
    virtual void s29();
    virtual void s30();
    virtual void s31();
    virtual void s32();
    virtual void s33();
    virtual void s34();
    virtual void s35();
    virtual void s36();
    virtual void s37();
    virtual void set_color(unsigned* c);         // +0x98
    virtual void s39();
};

union Color4 {
    unsigned dword;
    struct { unsigned char r, g, b, a; };
};

struct Hud2 : HudBase {
    char pad04[4];                               // +0x04
    Timer timers[6];                             // +0x08 .. 0x68
    float dur[6];                                // +0x68 .. 0x80
    vector4 box;                                 // +0x80 (x,y: position; z,w: second position)
    Color4 col_a;                                // +0x90 colour
    Color4 alpha_a;                              // +0x94 colour
    Color4 f98;                                  // +0x98
    Color4 f9c;                                  // +0x9c
    vector4 target;                              // +0xa0
    Color4 col_b;                                // +0xb0 colour
    Color4 alpha_b;                              // +0xb4 colour
    Color4 fb8;                                  // +0xb8
    Color4 fbc;                                  // +0xbc
    char c0, c1, c2, c3, c4, c5;                 // +0xc0
    char padc6[0xd8 - 0xc6];
    float d8, dc;                                // +0xd8, +0xdc
    char pade0[0x10c - 0xe0];
    rgl::mesh* m2;                               // +0x10c
    rgl::mesh* m1;                               // +0x110
    char pad114[0x14c - 0x114];
    unsigned c14c;                               // +0x14c
    void update(int arg);                        // 0x4966b0
};

// @ 0x4966b0
void Hud2::update(int)
{
    float e0 = timers[0].elapsed();
    float e1 = timers[1].elapsed();
    float e2 = timers[4].elapsed();
    float e3 = timers[5].elapsed();

    if (e2 < dur[4]) {
        float k = 1.0f / dur[4];
        float x = (e2 * box.z + (dur[4] - e2) * target.z) * k;
        float y = ((dur[4] - e2) * target.w + e2 * box.w) * k;
        m1->scale(vector4(x / d8, y / dc, 1.0f));
        m2->scale(vector4(x / d8, y / dc, 1.0f));
        invalidate();
        d8 = x;
        dc = y;
    } else if (c4) {
        c4 = 0;
        m1->scale(vector4(box.z / d8, box.w / dc, 1.0f));
        m2->scale(vector4(box.z / d8, box.w / dc, 1.0f));
        invalidate();
        d8 = box.z;
        dc = box.w;
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
        m1->color = col;
        m2->color = col;
        set_color(&col);
    } else if (c0) {
        unsigned col = (c14c ^ col_a.dword) & 0xffffff ^ c14c;
        c0 = 0;
        m1->color = col;
        m2->color = col;
        set_color(&col);
    }

    {
        unsigned col;
        if (e1 < dur[1]) {
            float v = (alpha_b.a * (dur[1] - e1) + alpha_a.a * e1) * (1.0f / dur[1]);
            col = round_float(&v) << 24;
            m1->color = (m1->color & 0xffffff) | col;
        } else if (c1) {
            col = alpha_a.dword;
            c1 = 0;
            m1->color = (m1->color & 0xffffff) | col;
        } else {
            goto alpha_done;
        }
        m2->color = (m2->color & 0xffffff) | col;
        col |= c14c & 0xffffff;
        set_color(&col);
    }
alpha_done:

    if (e3 < dur[5]) {
        float k = 1.0f / dur[5];
        vector4 v;
        v.x = (e3 * box.x + (dur[5] - e3) * target.x) * k;
        v.y = ((dur[5] - e3) * target.y + e3 * box.y) * k;
        set_pos(&v);
    } else if (c5) {
        set_pos(&box);
        c5 = 0;
    }
}

// Slice s00497880: the per-frame update of a mission HUD bar widget (0x497880: it lerps two bar meshes'
// scale from the elapsed timers, one colour group's alpha, and the widget position, then invalidates
// itself) and the refresh of its animated frame mesh (0x497d10: it rebuilds the frame mesh's colour and
// scale from the current frame). Layout: hud.h. Names are provisional.
#include <windows.h>
#include <math.h>
#include "rgl.h"
#include "hud.h"

// @ 0x497880
void Hud2::update(int)
{
    float t1 = timers[1].elapsed();
    float t4 = timers[4].elapsed();
    float t5 = timers[5].elapsed();

    if (t4 < dur[4]) {
        float k = 1.0f / dur[4];
        float x = (t4 * box.z + (dur[4] - t4) * target.z) * k;
        float y = (t4 * box.w + (dur[4] - t4) * target.w) * k;
        vector4 v(x / d8, y / dc, 1.0f);
        m108->scale(v);
        m110->scale(v);
        invalidate();
        v.x = x * 0.25f;
        v.y = f4;
        s25(&v);
        d8 = x;
        dc = y;
    } else if (c4) {
        vector4 v(box.z / d8, box.w / dc, 1.0f);
        c4 = 0;
        m108->scale(v);
        m110->scale(v);
        invalidate();
        v.x = box.z * 0.25f;
        v.y = f4;
        s25(&v);
        d8 = box.z;
        dc = box.w;
    }

    if (t1 < dur[1]) {
        float f = (alpha_a.a * t1 + alpha_b.a * (dur[1] - t1)) * (1.0f / dur[1]);
        unsigned col = round_float(&f) << 24;
        m108->color = (m108->color & 0xffffff) | col;
        m110->color = (m110->color & 0xffffff) | col;
        col |= c14c & 0xffffff;
        s38(&col);
    } else if (c1) {
        c1 = 0;
        unsigned col = alpha_a.dword;
        m108->color = (m108->color & 0xffffff) | col;
        m110->color = (m110->color & 0xffffff) | col;
        col |= c14c & 0xffffff;
        s38(&col);
    }

    if (t5 < dur[5]) {
        float k = 1.0f / dur[5];
        vector4 v;
        v.x = (t5 * box.x + (dur[5] - t5) * target.x) * k;
        v.y = (t5 * box.y + (dur[5] - t5) * target.y) * k;
        set_pos(&v);
    } else if (c5) {
        set_pos(&box);
        c5 = 0;
    }
}

// @ 0x497d10
void Hud2::refresh()
{
    if (m10c) {
        if (f194 == f168)
            return;
    }
    delete m10c;
    if (src) {
        mesh* m = src->clone();
        float f = (1.0f / f0) * f17c;
        vector4 col = v198;
        ((float*)&col)[idx] = ((1.0f - 2.0f * f) * ((f168 - f16c) / (f170 - f16c)) + f) * ((float*)&col)[idx];
        m10c = m;
        m->scale(col);
        m10c->color = (m10c->color & 0xffffff) | mask;
        f17c = v198.x * 0.020544001f;            // 0x59272c (0.020544f rounds one ulp short)
    }
    f194 = f168;
}

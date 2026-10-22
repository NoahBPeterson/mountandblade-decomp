// Slice s00497e30: three methods of the mission HUD widget family.
//   0x497e30  the animating refresh of a two-bar/one-colour widget: it lerps the bar meshes' scale, the
//             colour group's alpha and the widget position, writes the frame colour vector, and marks the
//             frame mesh stale (f194 = 0xcbb14326) so the next refresh rebuilds it.
//   0x498390  a one-lane scale animation: it scales m110 along idx by speed[idx]/f17c*arg and advances f17c.
//   0x498420  the outer HUD's scrollbar reload: it drives the scroll target, retargets the bar, drops and
//             rebuilds the "scrollbar"/"scrollbar_handle" meshes and installs them on the bar.
// Layout: hud.h. Names are provisional.
#include <windows.h>
#include <math.h>
#include "rgl.h"
#include "hud.h"

// @ 0x497e30
void Hud2::update2(int)
{
    float t1 = timers[1].elapsed();
    float t4 = timers[4].elapsed();
    float t5 = timers[5].elapsed();

    if (t4 < dur[4]) {
        float k = 1.0f / dur[4];
        float x = (t4 * box.z + (dur[4] - t4) * target.z) * k;
        float y = (t4 * box.w + (dur[4] - t4) * target.w) * k;
        f194 = -23234124.0f;                     // 0xcbb14326: never a real frame, so refresh rebuilds
        vector4 c(x * 0.38940808f, y, 1.0f);
        v198 = c;
        m108->scale(vector4(x / d8, y / dc, 1.0f));
        m110->scale(vector4(x / d8, y / dc, 1.0f));
        invalidate();
        vector4 s(x * 0.25f, speed[1], 1.0f);
        s25(&s);
        d8 = x;
        dc = y;
    } else if (c4) {
        vector4 c(box.z * 0.38940808f, box.w, 1.0f);
        v198 = c;
        c4 = 0;
        f194 = -23234124.0f;
        m108->scale(vector4(box.z / d8, box.w / dc, 1.0f));
        m110->scale(vector4(box.z / d8, box.w / dc, 1.0f));
        invalidate();
        vector4 s(box.z * 0.25f, speed[1], 1.0f);
        s25(&s);
        d8 = box.z;
        dc = box.w;
    }

    if (t1 < dur[1]) {
        float f = (alpha_a.a * t1 + alpha_b.a * (dur[1] - t1)) * (1.0f / dur[1]);
        unsigned col = round_float(&f) << 24;
        mask = col;
        f194 = -23234124.0f;
        m108->color = (m108->color & 0xffffff) | col;
        m110->color = (m110->color & 0xffffff) | col;
        col |= c14c & 0xffffff;
        s38(&col);
    } else if (c1) {
        unsigned col = alpha_a.dword;
        mask = col;
        f194 = -23234124.0f;
        c1 = 0;
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

// @ 0x498390
void Hud2::scale_lane(float arg)
{
    if (m110) {
        vector4 v(1.0f, 1.0f, 1.0f);
        ((float*)&v)[idx] = speed[idx] / f17c * arg;
        m110->scale(v);
        f17c = arg * speed[idx];
    }
}

// @ 0x498420
void HudOuter::scroll_refresh(vector4* arg)
{
    ScrollTarget* t = *scroll->target;
    t->t25(arg);
    t->t27((char*)t + 0x140);

    f49c = arg->x;
    f4a0 = arg->y;

    vector4 v1;
    v1.x = 0.01f;
    v1.y = arg->y;
    bar->s25(&v1);
    bar->f17c = arg->y * 1.5576323f * 0.31f;

    delete bar->m108;
    bar->m108 = 0;
    delete bar->m110;
    bar->m110 = 0;

    vector4 scale(0.5f, arg->y * 1.5576323f, 1.0f);
    RString a_name("scrollbar");
    mesh* a = g_meshes.find(&a_name)->clone();
    a->scale(scale);
    RString b_name("scrollbar_handle");
    mesh* b = g_meshes.find(&b_name)->clone();
    b->scale(scale);
    bar->s32(0, a);
    bar->s32(2, b);

    on_scroll();
}

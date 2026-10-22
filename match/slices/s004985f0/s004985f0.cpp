// Slice s004985f0: the mission HUD bar widget's full per-frame update (0x4985f0). One call advances all
// of the widget's animations at once: the two bar meshes' scale with their two companion vectors
// (0x6c/0x70), the four colour groups (0x98 and 0x9c twice each) and the widget position (0x58), and
// finally commits the frame through 0x94 if anything changed. Each animation lerps from the widget's
// current value to its target over an elapsed/total fraction; the per-frame 32-bit colour words are
// re-packed from the two source colours with `round_float`. Layout: hud.h. Names are provisional.
#include <windows.h>
#include <math.h>
#include "rgl.h"
#include "hud.h"

// @ 0x4985f0
void Hud2::update_full(int)
{
    float t0 = timers[0].elapsed();
    float t1 = timers[1].elapsed();
    float t2 = timers[2].elapsed();
    float t3 = timers[3].elapsed();
    float t4 = timers[4].elapsed();
    float t5 = timers[5].elapsed();
    bool changed = false;

    // bars: lerp the (x, z) pair over dur[4], then the two companion vectors from its y component
    if (t4 < dur[4]) {
        float k = 1.0f / dur[4];
        float x = (t4 * box.z + (dur[4] - t4) * target.z) * k;
        float y = (t4 * box.w + (dur[4] - t4) * target.w) * k;
        m110->scale(vector4(x / d8, y / dc, 1.0f));
        m10c->scale(vector4(x / d8, y / dc, 1.0f));
        vector4 v(y * 0.02f, y * 0.02f, 1.0f);
        s27(&v);
        v.x = 0.0f;
        v.y = y * 0.009f;
        s28(&v);
        invalidate();
        d8 = x;
        dc = y;
        changed = true;
    } else if (c4) {
        c4 = 0;
        m110->scale(vector4(box.z / d8, box.w / dc, 1.0f));
        m10c->scale(vector4(box.z / d8, box.w / dc, 1.0f));
        vector4 v(box.w * 0.02f, box.w * 0.02f, 1.0f);
        s27(&v);
        v.x = 0.0f;
        v.y = box.w * 0.009f;
        s28(&v);
        invalidate();
        d8 = box.z;
        dc = box.w;
        changed = true;
    }

    // colour group 1: RGB (from col_a/col_b) with the alpha the widget keeps in c14c
    unsigned col = 0;
    if (t0 < dur[0]) {
        float f = ((col_a.dword & 0xff) * t0 + (col_b.dword & 0xff) * (dur[0] - t0)) * (1.0f / dur[0]);
        col = round_float(&f);
        f = (col_a.g * t0 + col_b.g * (dur[0] - t0)) * (1.0f / dur[0]);
        col += round_float(&f) << 8;
        f = (col_a.b * t0 + col_b.b * (dur[0] - t0)) * (1.0f / dur[0]);
        col += round_float(&f) << 16;
        col |= c14c & 0xff000000;
        s38(&col);
        changed = true;
    } else if (c0) {
        col = (col_a.dword & 0xffffff) | (c14c & 0xff000000);
        c0 = 0;
        s38(&col);
        changed = true;
    }

    // colour group 1 alpha: it also tints both bar meshes
    if (t1 < dur[1]) {
        float f = (alpha_a.a * t1 + alpha_b.a * (dur[1] - t1)) * (1.0f / dur[1]);
        col = round_float(&f) << 24;
        m110->color = (m110->color & 0xffffff) | col;
        m10c->color = (m10c->color & 0xffffff) | col;
        col |= c14c & 0xffffff;
        s38(&col);
        changed = true;
    } else if (c1) {
        col = alpha_a.dword;
        m110->color = (m110->color & 0xffffff) | col;
        m10c->color = (m10c->color & 0xffffff) | col;
        c1 = 0;
        col = (col & 0xff000000) | (c14c & 0xffffff);
        s38(&col);
        changed = true;
    }

    // colour group 2: the same two passes with the second colour pair and the second kept word (c150)
    if (t2 < dur[2]) {
        float f = ((col_a2.dword & 0xff) * t2 + (col_b2.dword & 0xff) * (dur[2] - t2)) * (1.0f / dur[2]);
        col = round_float(&f);
        f = (col_a2.g * t2 + col_b2.g * (dur[2] - t2)) * (1.0f / dur[2]);
        col += round_float(&f) << 8;
        f = (col_a2.b * t2 + col_b2.b * (dur[2] - t2)) * (1.0f / dur[2]);
        col += round_float(&f) << 16;
        col |= c150 & 0xff000000;
        s39(&col);
        changed = true;
    } else if (c2) {
        col = (col_a2.dword & 0xffffff) | (c150 & 0xff000000);
        c2 = 0;
        s39(&col);
        changed = true;
    }

    if (t3 < dur[3]) {
        float f = (alpha_a2.a * t3 + alpha_b2.a * (dur[3] - t3)) * (1.0f / dur[3]);
        col = (c150 & 0xffffff) | (round_float(&f) << 24);
        s39(&col);
        changed = true;
    } else if (c3) {
        c3 = 0;
        col = alpha_a2.dword;
        col = (col & 0xff000000) | (c150 & 0xffffff);
        s39(&col);
        changed = true;
    }

    // position: lerp the (x, y) pair over dur[5]
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

    if (changed) {
        vector4 v;
        s37(&v);
    }
}

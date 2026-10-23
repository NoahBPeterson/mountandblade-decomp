//-----------------------------------------------------------------------------
// File: ui_slider_key.cpp  (0x48b5a0..0x48b6e0)
//
// Desc: the slider widget's key (mouse) handler: grab the thumb, drag it, release. Names provisional.
//       Flags: /O2 /MT /Gy /TP.
//
// Boundary: key calls set_value (0x48b6e0) forward, so the object ends before 0x48b6e0; it calls value()
// (0x48b550) as an external function (see ui_slider.cpp).
//-----------------------------------------------------------------------------
#include "../tstring.h"
#include "./ui_common.h"
#include "./item_ref.h"
#include "./mission_ui.h"
#include "./ui_widgets.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../include/keyframes.h"
#include "../../include/mat4.h"
#include "../../include/mesh_prims.h"
#include "../../include/vec.h"

// @ 0x48b5a0
char UiB::key(unsigned p)
{
    char b = 0;
    if (f180 == 0) {
        if (p != 0x10)
            goto tail;
        b = 1;
        ::play_sound(&g_sound);
        int i = f178;
        float t = g_ui_target[i];
        float v = value() + e[i];
        float c = f17c + 0.01f;
        if (v - c < t && t < v + c) {
            f180 = 1;
            g_ui_active = this;
            f184 = t - v;
            goto tail;
        }
        f184 = 0;
        set_value(t - e[i]);
        set_key_flags(0x10000);
    } else {
        if (p == 8) {
            g_ui_active = 0;
            b = 1;
            f180 = 0;
            set_key_flags(0x10000);
            goto tail;
        }
        set_value((g_ui_target[f178] - f184) - e[f178]);
        set_key_flags(0x10000);
    }
tail:
    set_key_flags(p);
    refresh();
    return b;
}

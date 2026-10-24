//-----------------------------------------------------------------------------
// File: ui_d_key.cpp  (0x48b960..0x48baa0)
//
// Desc: the progress bar widget UiD (vtable 0x592140): its key handler (click / drag of the bar). The constructor and
//       show are in src/game/ui/ui_widgets_b.cpp, the value getter in ui_d_value.cpp, the value setter in
//       src/game/config/ui_widgets_c.cpp. Names provisional. Flags: /O2 /MT /Gy /TP.
//
// Boundary: key calls value (0x48b930) out of line and keeps its locals in callee-saved registers across the call, as for an
// external callee; a same-object value() is inlined by cl (the merged build is 13 bytes longer), so value is another object
// (the UiB::value / UiB::key precedent in src/game/ui/MIGRATION.md, medium). key calls set_value (0x48baa0) forward: the
// object ends before it (high).
//-----------------------------------------------------------------------------
#include "../tstring.h"
#include "../ui/ui_common.h"
#include "../ui/item_ref.h"
#include "../ui/mission_ui.h"
#include "../ui/ui_widgets.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../include/keyframes.h"
#include "../../include/mat4.h"
#include "../../include/mesh_prims.h"
#include "../../include/vec.h"
#include "../../include/mesh_parts.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../rgl/mesh.h"      // (listed directly: the resolution audit scans only the source's own includes)

// @ 0x48b960
char UiD::key(unsigned p)
{
    char b = 0;
    if (f180 == 0) {
        if (p != 0x10)
            goto tail;
        b = 1;
        ::play_sound(&g_ui_sound);
        int i = f178;
        float t = g_ui_target[i];
        float v = value() + e[i];
        if (v - 0.01f < t && t < (f17c + 0.01f) + v) {
            f180 = 1;
            g_ui_active = this;
            f184 = t - v;
            goto tail;
        }
        f184 = 0;
        set_value(t - e[i]);
    } else {
        if (p == 8) {
            g_ui_active = 0;
            b = 1;
            f180 = 0;
            set_key_flags(0x10000);
            goto tail;
        }
        set_value((g_ui_target[f178] - f184) - e[f178]);
    }
    set_key_flags(0x10000);
tail:
    set_key_flags(p);
    refresh();
    return b;
}

//-----------------------------------------------------------------------------
// File: progress_bar_input.cpp  (0x48b960..0x48baa0)
//
// Desc: the progress bar widget UiD (vtable 0x592140): its key handler (click / drag of the bar). The constructor and
//       show are in src/game/ui/widgets/label_and_progress_bar.cpp, the value getter in progress_bar_value.cpp, the value setter in
//       src/game/ui/widgets/text_editor_widget.cpp. Names provisional. Flags: /O2 /MT /Gy /TP.
//
// Boundary: key calls value (0x48b930) out of line and keeps its locals in callee-saved registers across the call, as for an
// external callee; a same-object value() is inlined by cl (the merged build is 13 bytes longer), so value is another object
// (the UiB::value / UiB::key precedent in docs/migration/game_ui.md, medium). key calls set_value (0x48baa0) forward: the
// object ends before it (high).
//-----------------------------------------------------------------------------
#include "engine/core/tstring.h"
#include "game/ui/gui_manager.h"             // g_gui (0x5d7080): ui_active / mouse position
#include "game/ui/widgets/ui_common.h"
#include "game/data/item_ref_game_ui.h"
#include "game/ui/widgets/widget_base.h"
#include "game/ui/widgets/ui_widgets.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "engine/fx/keyframes.h"
#include "engine/math/mat4_include.h"
#include "engine/render/mesh_prims.h"
#include "engine/math/vec.h"
#include "engine/render/mesh_parts.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "engine/render/mesh.h"      // (listed directly: the resolution audit scans only the source's own includes)

// @ 0x48b960
char UiD::key(unsigned p)
{
    char b = 0;
    if (f180 == 0) {
        if (p != 0x10)
            goto tail;
        b = 1;
        g_gui.play_sound();
        int i = f178;
        float t = g_gui.mouse[i];
        float v = value() + position[i];
        if (v - 0.01f < t && t < (f17c + 0.01f) + v) {
            f180 = 1;
            g_gui.ui_active = this;
            f184 = t - v;
            goto tail;
        }
        f184 = 0;
        set_value(t - position[i]);
        set_key_flags(0x10000);
    } else {
        if (p == 8) {
            g_gui.ui_active = 0;
            b = 1;
            f180 = 0;
            set_key_flags(0x10000);
            goto tail;
        }
        set_value((g_gui.mouse[f178] - f184) - position[f178]);
        set_key_flags(0x10000);
    }
tail:
    set_key_flags(p);
    refresh();
    return b;
}

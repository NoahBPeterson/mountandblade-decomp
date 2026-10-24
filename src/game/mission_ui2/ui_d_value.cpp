//-----------------------------------------------------------------------------
// File: ui_d_value.cpp  (0x48b930..0x48b960)
//
// Desc: the progress bar widget UiD (vtable 0x592140): its value getter (the bar position in screen units). Names
//       provisional. Flags: /O2 /MT /Gy /TP.
//
// Boundary: UiD::show (0x48b8d0, src/game/ui/ui_widgets_b.cpp) calls value forward, so a TU starts here (high). The
// next function, UiD::key (0x48b960, ui_d_key.cpp), calls it out of line with callee-saved registers kept live across the
// call, which a same-object callee would not get (cl inlines it), so key is another object (medium).
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

// @ 0x48b930
float UiD::value()
{
    return (screen[f178] - f17c) * ((v168 - f16c) / (f170 - f16c));
}

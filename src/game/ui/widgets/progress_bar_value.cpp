//-----------------------------------------------------------------------------
// File: progress_bar_value.cpp  (0x48b930..0x48b960)
//
// Desc: the progress bar widget UiD (vtable 0x592140): its value getter (the bar position in screen units). Names
//       provisional. Flags: /O2 /MT /Gy /TP.
//
// Boundary: UiD::show (0x48b8d0, src/game/ui/widgets/label_and_progress_bar.cpp) calls value forward, so a TU starts here (high). The
// next function, UiD::key (0x48b960, progress_bar_input.cpp), calls it out of line with callee-saved registers kept live across the
// call, which a same-object callee would not get (cl inlines it), so key is another object (medium).
//-----------------------------------------------------------------------------
#include "engine/core/tstring.h"
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

// @ 0x48b930
float UiD::value()
{
    return (rect_max[f178] - f17c) * ((v168 - f16c) / (f170 - f16c));
}

//-----------------------------------------------------------------------------
// File: slider_value.cpp  (0x48b550..0x48b5a0)
//
// Desc: the slider widget's value readout and its type code. Names provisional. Flags: /O2 /MT /Gy /TP.
//
// Boundary: UiB::show (0x48b4f0, previous object) calls value() forward, so value() starts an object; and
// UiB::key (0x48b5a0) calls value() out of line, which cl would have inlined had it seen the body (it does when
// both are in one file: key then differs by 299 bytes), so key is in the next object (slider_input.cpp).
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

// @ 0x48b550
float UiB::value()
{
    return (rect_max[f178] - (f17c + f17c)) * ((v168 - f16c) / (f170 - f16c)) + f17c;
}

// @ 0x48b590
int UiB::kind() { return 2; }

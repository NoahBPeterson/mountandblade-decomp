//-----------------------------------------------------------------------------
// File: ui_slider.cpp  (0x48b550..0x48b5a0)
//
// Desc: the slider widget's value readout and its type code. Names provisional. Flags: /O2 /MT /Gy /TP.
//
// Boundary: UiB::show (0x48b4f0, previous object) calls value() forward, so value() starts an object; and
// UiB::key (0x48b5a0) calls value() out of line, which cl would have inlined had it seen the body (it does when
// both are in one file: key then differs by 299 bytes), so key is in the next object (ui_slider_key.cpp).
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

// @ 0x48b550
float UiB::value()
{
    return (screen[f178] - (f17c + f17c)) * ((v168 - f16c) / (f170 - f16c)) + f17c;
}

// @ 0x48b590
int UiB::type2() { return 2; }

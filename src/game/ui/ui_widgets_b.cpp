//-----------------------------------------------------------------------------
// File: ui_widgets_b.cpp  (0x48b6e0..0x48b930)
//
// Desc: the slider's value setter, the label widget (constructor, show, type code) and the progress bar's
//       constructor and show. Names provisional. Functions in ascending address order (cl's bottom-up
//       emission). Flags: /O2 /MT /Gy /TP.
//
// Boundary: UiB::key (0x48b5a0, previous object) calls set_value (0x48b6e0) forward; UiD::show (0x48b8d0) calls
// UiD::value (0x48b930) forward, so the object ends before 0x48b930. UiB|UiC and UiC|UiD are class changes only
// (no evidence either way), so the three stay one object.
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
#include "../../include/mesh_parts.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../rgl/mesh.h"      // (listed directly: the resolution audit scans only the source's own includes)

// @ 0x48b6e0
void UiB::set_value(float x)
{
    float v = (f170 - f16c) * ((x - f17c) / (screen[f178] - (f17c + f17c))) + f16c;
    v168 = v;
    if (v < f16c) {
        v168 = f16c;
        return;
    }
    if (f170 < v)
        v168 = f170;
}

// @ 0x48b760
UiC::UiC()
{
    v198 = rgl::vector4(1.0f, 1.0f, 1.0f);
    src = 0;
    f190 = 1;
    mask = 0xff000000;
    f194 = 1.0f;
}

// @ 0x48b7f0
void UiC::render(rgl::scene* a, float* b, int c, int d)
{
    b11c[0] = b11c[1] = b11c[2] = 1;
    update_children();
    if (tracks[2]) {
        int i = f178;
        float v = (screen[i] - (f17c + f17c)) * ((v168 - f16c) / (f170 - f16c)) + f17c;
        *(float*)((char*)tracks[2] + 0xfc + i * 4) = v;
    }
    MissionUi::render(a, b, c, d);
}

// @ 0x48b880
int UiC::kind() { return 3; }

// @ 0x48b890
UiD::UiD()
{
    f178 = 0;
    f180 = 0;
    f184 = 0;
    f17c = 0.1f;
    v168 = 1.0f;
}

// @ 0x48b8d0
void UiD::render(rgl::scene* a, float* b, int c, int d)
{
    b11c[0] = b11c[2] = 1;
    if (tracks[2])
        *(float*)((char*)tracks[2] + 0xfc + f178 * 4) = value();
    MissionUi::render(a, b, c, d);
}

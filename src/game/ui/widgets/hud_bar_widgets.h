// hud_bar_widgets.h: the mesh helpers (clone, frame transform, alpha, uv normalise), the slider widget factory and the mission HUD bar
// widgets that sit between 0x4958f0 and 0x499050 (translation units 85 rgl_mesh and 86 hud2 of the TU map). Names provisional.
//
// The HUD bar is a MissionUi (src/game/ui/widgets/widget_base.h) with the track fields it animates: timers = (clockN, tN) pairs at +0x08,
// durations = periodA..F (+0x68), box = f80..f8c (+0x80), the colour pairs = valA..valD (+0x90) / srcA..srcD (+0xb0), target =
// a0..ac (+0xa0), flags = c0..c5, applied scale = fD8/fDC, meshes = tracks[0..3] (+0x108..+0x114), kept colours =
// color14c / color150. The classes below add only members beyond MissionUi (HudBar, +0x178..) or are views.
#ifndef GAME_WORLD_DATA_MESH_HUD_H
#define GAME_WORLD_DATA_MESH_HUD_H
#include "platform/image_addrs.h"
#include <math.h>
#include <stddef.h>
#include "game/ui/widgets/widget_manager.h"
#include "game/data/raw_place.h"
#include "engine/render/mesh.h"
#include "engine/render/mesh.h"        // rgl::mesh: compute_bounds 0x419300, set_bone_index 0x47b360
#include "engine/math/rgl_math.h"
#include "game/ui/widgets/widget_base.h"
#include "engine/core/clock_table.h"
#include "game/ui/widgets/ui_widgets.h"

using rgl::vector4;

#define WD_FLD(T, off) (*(T*)((char*)this + (off)))

namespace rgl { vector4 unpack_color(const unsigned* c); }      // 0x40e4e0 (collision_shapes.cpp: the colour by value, hidden return pointer)
extern char vtbl_591fb0[];                                     // 0x591fb0 (UiB's vtable: the slider widget)
#include "engine/resource/resource.h"                           // resmgr g_resmgr (0x5d6fa0): get_object 0x419540

// 0x432540: floor, then "> 0.5 rounds up" (inlined here)
static inline int round_float(float* p)
{
    float fl = (float)floor((double)*p);
    int i = (int)fl;
    return (*p - fl <= 0.5f) ? i : i + 1;
}

namespace rgl { struct mesh; struct entity; }
rgl::mesh* __stdcall build_legend_mesh(rgl::entity* parent);    // 0x495b20 (ret 4: the label mesh, added to the entity)

// ---- the HUD bar widget (MissionUi-derived; the fields past +0x178 are the slider-like frame state) ---------------------------------
// the animate overrides of the widget classes whose tables the audit does not cover (update_1); the others are their classes' own
// (UiB::animate 0x497880, UiC::animate 0x497e30, XyzButton::animate 0x4966b0, UiE::animate 0x4985f0)
struct HudBar : MissionUi {
    rgl::mesh* msh(int i) { return (rgl::mesh*)tracks[i]; }
};

// The mesh overlay widget (vtable 0x5922e8, 0x178 bytes: MissionUi's members only): the placed-mesh and tableau overlays of
// widget_mgr::create_mesh_overlay (object_table_spawners.cpp). Its table is MissionUi's with animate 0x496100 (one mesh) and kind 4 (0x48c460).
struct MeshWidget : HudBar {
    virtual void animate(int arg);                              // +0x08 0x496100 (one mesh)
    virtual int kind() { return 4; }                            // +0xc0 0x48c460 (ICF-folded with the shapes' type codes)
};

// ---- the scrolling list (Camera::scroll_refresh 0x498420 is defined in hud_bar_widgets.cpp) ----
#include "game/mission/camera_class.h"     // Camera

#endif // GAME_WORLD_DATA_MESH_HUD_H

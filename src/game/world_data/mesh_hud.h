// mesh_hud.h: the mesh helpers (clone, frame transform, alpha, uv normalise), the slider widget factory and the mission HUD bar
// widgets that sit between 0x4958f0 and 0x499050 (translation units 85 rgl_mesh and 86 hud2 of the TU map). Names provisional.
//
// The HUD bar is a MissionUi (src/game/ui/mission_ui.h) with the track fields it animates: timers = (clockN, tN) pairs at +0x08,
// durations = periodA..F (+0x68), box = f80..f8c (+0x80), the colour pairs = valA..valD (+0x90) / srcA..srcD (+0xb0), target =
// a0..ac (+0xa0), flags = c0..c5, applied scale = fD8/fDC, meshes = tracks[0..3] (+0x108..+0x114), kept colours =
// color14c / color150. The classes below add only members beyond MissionUi (HudBar, +0x178..) or are views.
#ifndef GAME_WORLD_DATA_MESH_HUD_H
#define GAME_WORLD_DATA_MESH_HUD_H
#include "../../include/image_addrs.h"
#include <math.h>
#include <stddef.h>
#include "./widget_mgr_w.h"
#include "./raw_place.h"
#include "../../rgl/mesh.h"
#include "../../rgl/mesh.h"        // rgl::mesh: compute_bounds 0x419300, set_bone_index 0x47b360
#include "../../rgl/rgl_math.h"
#include "../ui/mission_ui.h"
#include "../../include/clock_table.h"
#include "../ui/ui_widgets.h"

using rgl::vector4;

#define WD_FLD(T, off) (*(T*)((char*)this + (off)))

namespace rgl { vector4 unpack_color(const unsigned* c); }      // 0x40e4e0 (shapes.cpp: the colour by value, hidden return pointer)
extern char vtbl_591fb0[];                                     // 0x591fb0 (UiB's vtable: the slider widget)
// mission_ui2's FieldPanel::update (0x48d470), what the outer HUD's refresh calls
struct FieldPanel { void update(); };
#include "../../rgl/resource.h"                           // resmgr g_resmgr (0x5d6fa0): get_object 0x419540

// 0x432540: floor, then "> 0.5 rounds up" (inlined here)
static inline int round_float(float* p)
{
    float fl = (float)floor((double)*p);
    int i = (int)fl;
    return (*p - fl <= 0.5f) ? i : i + 1;
}

struct ColourOwner {};
struct Poser {
    void add_extra(int m);   // 0x442110 (thiscall, ret 4)
};
rgl::mesh* __stdcall build_legend_mesh(ColourOwner* parent);    // 0x495b20 (ret 4)

// ---- the HUD bar widget (MissionUi-derived; the fields past +0x178 are the slider-like frame state) ---------------------------------
// the animate overrides of the widget classes whose tables the audit does not cover (update_1); the others are their classes' own
// (UiB::animate 0x497880, UiC::animate 0x497e30, XyzButton::animate 0x4966b0, UiE::animate 0x4985f0)
struct HudBar : MissionUi {
    rgl::mesh* msh(int i) { return (rgl::mesh*)tracks[i]; }
    void s03() { update_bounds(); }                                       // vtable +0x0c (invalidate)
    void s22(void* v) { move_a((float*)v); }                               // vtable +0x58 (set the position)
    void s25(void* v) { set_xy((float*)v); }                           // vtable +0x64
    void s27(void* v) { set_size((float*)v); }                         // vtable +0x6c
    void s28(void* v) { set_offset((float*)v); }                       // vtable +0x70
    void s32(int a, void* m) { set_track(a, (rgl::mesh*)m); }                // vtable +0x80
    void s37(void* v) { get_xy((float*)v); }                           // vtable +0x94
    void s38(unsigned* c) { set_color(c); }                     // vtable +0x98
    void s39(unsigned* c) { set_target(c); }                           // vtable +0x9c

};

// The mesh overlay widget (vtable 0x5922e8, 0x178 bytes: MissionUi's members only): the placed-mesh and tableau overlays of
// WidgetMgrW::create_mesh_overlay (obj_tbl.cpp). Its table is MissionUi's with animate 0x496100 (one mesh) and kind 4 (0x48c460).
struct MeshWidget : HudBar {
    virtual void animate(int arg);                              // +0x08 0x496100 (one mesh)
    virtual int kind() { return 4; }                            // +0xc0 0x48c460 (ICF-folded with the shapes' type codes)
};

// ---- the outer HUD that owns the bar widget and the scrollbar (widget_mgr.h WItem::terrain_refresh_mesh 0x498420) --------------------------
struct ScrollVt {
    virtual void t00(); virtual void t01(); virtual void t02(); virtual void t03(); virtual void t04();
    virtual void t05(); virtual void t06(); virtual void t07(); virtual void t08(); virtual void t09();
    virtual void t10(); virtual void t11(); virtual void t12(); virtual void t13(); virtual void t14();
    virtual void t15(); virtual void t16(); virtual void t17(); virtual void t18(); virtual void t19();
    virtual void t20(); virtual void t21(); virtual void t22(); virtual void t23(); virtual void t24();
    virtual void t25(vector4* arg);                             // +0x64
    virtual void t26();
    virtual void t27(void* p);                                  // +0x6c
};
struct ScrollHolder {                                           // the object at hud+0x490
    char pad[0x17c];
    ScrollVt** target;                                          // +0x17c
};
struct HudOuter : WItem {
    char pad178[0x484 - 0x178];
    UiD* bar;                                                   // +0x484 the scrollbar slider
    char pad488[0x490 - 0x488];
    ScrollHolder* scroll;                                       // +0x490
    char pad494[0x49c - 0x494];
    float f49c;                                                 // +0x49c
    float f4a0;                                                 // +0x4a0
    void scroll_refresh(vector4* arg);                          // 0x498420 (ret 4)
};

#endif // GAME_WORLD_DATA_MESH_HUD_H

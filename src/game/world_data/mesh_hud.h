// mesh_hud.h: the mesh helpers (clone, frame transform, alpha, uv normalise), the slider widget factory and the mission HUD bar
// widgets that sit between 0x4958f0 and 0x499050 (translation units 85 rgl_mesh and 86 hud2 of the TU map). Names provisional.
//
// The HUD bar is a MissionUi (src/game/ui/mission_ui.h) with the track fields it animates: timers = (clockN, tN) pairs at +0x08,
// durations = periodA..F (+0x68), box = f80..f8c (+0x80), the colour pairs = valA..valD (+0x90) / srcA..srcD (+0xb0), target =
// a0..ac (+0xa0), flags = c0..c5, applied scale = fD8/fDC, meshes = tracks[0..3] (+0x108..+0x114), kept colours =
// color14c / color150. The classes below add only members beyond MissionUi (HudBar, +0x178..) or are views.
#ifndef GAME_WORLD_DATA_MESH_HUD_H
#define GAME_WORLD_DATA_MESH_HUD_H
#include <math.h>
#include <stddef.h>
#include "./widget_mgr_w.h"
#include "./raw_place.h"
#include "../../rgl/mesh.h"
#include "../../rgl/mesh_x.h"        // mesh_x: compute_bounds 0x419300, set_bone_index 0x47b360
#include "../../rgl/rgl_math.h"
#include "../ui/mission_ui.h"

using rgl::vector4;

#define WD_FLD(T, off) (*(T*)((char*)this + (off)))

// ---- rgl::mesh members the shared header lacks (same object, 0x260 bytes) -------------------------------------------------------
struct MeshX : gui_mesh {
    float& uv(int i) { return WD_FLD(float, 0xf0 + 4 * i); }    // +0xf0 .. +0xfc the uv rectangle (0x495fa0)
    MeshX* copy_of();                                           // 0x4958f0 (thiscall, ret 0)
    void set_part(const rgl::matrix& mtx);                      // 0x495ba0 (ret 4)
    void set_alpha(float a);                                    // 0x495f00 (ret 4)
    void normalize_uv(MeshX* src);                              // 0x495fa0 (ret 4)
};
namespace rgl { vector4 unpack_color(const unsigned* c); }      // 0x40e4e0 (shapes.cpp: the colour by value, hidden return pointer)
extern char vtbl_5922e8[];                                     // 0x5922e8 (the mesh widget's vtable)
extern char vtbl_591fb0[];                                     // 0x591fb0 (UiB's vtable: the slider widget)
// mission_ui2's FieldPanel::update (0x48d470), what the outer HUD's refresh calls
struct FieldPanel { void update(); };
// the resource manager's mesh lookup (0x419540, resmgr::get_object of rgl/core/resmgr.h; widget_mgr.h calls its view resmgr_gui)
struct resmgr { rgl::mesh* get_object(const rgl::string* name); };

union Color4 {
    unsigned dword;
    struct { unsigned char r, g, b, a; };
};

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
struct HudBar : MissionUi {
    int idx;                                                    // +0x178 which colour component the frame scales (0x497d10, 0x498390)
    float f17c;                                                 // +0x17c the frame scale
    char pad180[0x18c - 0x180];
    rgl::mesh* src;                                             // +0x18c the mesh the frame is cloned from (0x497d10)
    char pad190[4];
    float f194;                                                 // +0x194 the frame the current mesh was built for
    vector4 v198;                                               // +0x198 the colour the frame scales
    unsigned mask;                                              // +0x1a8 the alpha bits the frame mesh keeps
    char pad1ac[0x1b0 - 0x1ac];

    float elapsed(int i) const { return (float)(g_ticks[*(const int*)((const char*)this + 8 + i * 0x10)] - *(const __int64*)((const char*)this + 0x10 + i * 0x10)) * 1e-5f; }
    float& dur(int i) { return (&periodA)[i]; }                 // +0x68 + 4i
    Color4& ca() { return *(Color4*)&valA; }                    // +0x90
    Color4& aa() { return *(Color4*)&valB; }                    // +0x94
    Color4& ca2() { return *(Color4*)&valC; }                   // +0x98
    Color4& aa2() { return *(Color4*)&valD; }                   // +0x9c
    Color4& cb() { return *(Color4*)&srcA; }                    // +0xb0
    Color4& ab() { return *(Color4*)&srcB; }                    // +0xb4
    Color4& cb2() { return *(Color4*)&srcC; }                   // +0xb8
    Color4& ab2() { return *(Color4*)&srcD; }                   // +0xbc
    gui_mesh* msh(int i) { return (gui_mesh*)tracks[i]; }
    void s03() { d03(); }                                       // vtable +0x0c (invalidate)
    void s22(void* v) { d22(v); }                               // vtable +0x58 (set the position)
    void s25(void* v) { set_xy((float*)v); }                           // vtable +0x64
    void s27(void* v) { set_size((float*)v); }                         // vtable +0x6c
    void s28(void* v) { set_offset((float*)v); }                       // vtable +0x70
    void s32(int a, void* m) { d32(a, (int)m); }                // vtable +0x80
    void s37(void* v) { get_xy((float*)v); }                           // vtable +0x94
    void s38(unsigned* c) { set_color(c); }                     // vtable +0x98
    void s39(unsigned* c) { d39(c); }                           // vtable +0x9c

    void update_1(int arg);                                     // 0x496100 (one mesh)
    void update_2(int arg);                                     // 0x4966b0 (two meshes)
    void update_3(int arg);                                     // 0x496d30 (three meshes, two colour groups)
    void update_4(int arg);                                     // 0x497880 (two bar meshes, one colour group)
    void rebuild_frame();                                       // 0x497d10 (rebuild the frame mesh)
    void update_5(int arg);                                     // 0x497e30 (the animating refresh)
    void scale_lane(float arg);                                 // 0x498390
    void update_full(int arg);                                  // 0x4985f0
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
    HudBar* bar;                                                // +0x484
    char pad488[0x490 - 0x488];
    ScrollHolder* scroll;                                       // +0x490
    char pad494[0x49c - 0x494];
    float f49c;                                                 // +0x49c
    float f4a0;                                                 // +0x4a0
    void scroll_refresh(vector4* arg);                          // 0x498420 (ret 4)
};

#endif // GAME_WORLD_DATA_MESH_HUD_H

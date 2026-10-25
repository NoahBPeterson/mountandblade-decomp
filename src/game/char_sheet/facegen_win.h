#ifndef FACEGEN_WIN_H
#define FACEGEN_WIN_H
// clang-format off
// Slice s004f1460: the character face-generator window -- its initialiser (0x4f1460) and the terrain
// "region type" name table (0x4f2610).  The rest of the window's members (0x4f2730, 0x4f28a0, 0x4f28f0,
// 0x4f2960, 0x4f29b0, 0x4f2a10, 0x4f2ae0) are incomplete; see partial.txt.  Widget slots, the tstring
// helpers and the resource manager follow the verified neighbours (s0048add0, s004dfd50, s004e05c0,
// s004dd830).  Layouts and names are provisional.
#include <windows.h>
#include <stdlib.h>
#include <math.h>
#include <new>
#include <vector>

using std::vector;

// ---- engine types -----------------------------------------------------------------------------------
// rgl::string: 16 bytes { pos, buf, cap, len }; ctor 0x42bbd0 zeroes the four words.
#include "../tstring.h"
#include "./cs_common.h"                                // new_container, MissionUi, gui_manager, NodeVec, vtbl_*

// cloned rgl mesh; only the members this slice reaches
#include "../rgl_mesh.h"
using rgl::mesh;

// resource manager object at 0x5d6fa0
#include "../res_mgr.h"                         // resmgr (0x5d6fa0): the one definition for the rgl.h-universe modules
extern resmgr g_alloc;                                      // 0x5d6fa0

// world / terrain object at 0xa76630 (the terrain generator)
struct TerrainGen {
    void reset();                                       // 0x4647e0 (Terrain::reset: no symbol of that class exists in src; see cs_common.h)
};
extern TerrainGen g_terrain_gen;                            // 0xa76630

// the renderer's view frustum: 0x344-byte record, ctor 0x478ad0, set_fov 0x488cd0
struct frustum_t {
    char pad0[0x108];
    float f108, f10c, f110, f114, f118, f11c;               // +0x108
    char pad120[0x344 - 0x120];
    frustum_t(float fov, float aspect, float zmin, float zmax);     // 0x478ad0
    void set_fog(float h, float v, float a, float b, float c);      // 0x488cd0
};

// ---- the widget interface (slot offsets in comments) -----------------------------------------------
struct tvec2 {
    float x, y;
};

#include "../ui_widget.h"                 // Widget (the UI widget interface)
typedef Widget UiWidget;                // the slice's name for it

// slider (ctor 0x48b4b0, 0x188 bytes; see s0048add0)
struct UiSlider : UiWidget {
    char pad[0x188 - 4];
    UiSlider();
};

// a cloned, scaled mesh (the original inlines these two steps)
static __forceinline mesh* load_mesh(const char* name, float s)
{
    tstring n(name);
    mesh* m = g_alloc.get_object((const rgl::string*)&n)->clone();
    m->scale(s);
    return m;
}

// the face-generator window ("this"); every field is reached by its explicit offset
struct facegen_win : UiWidget {
    char pad0[0x68 - 4];
    UiWidget* board;                                        // +0x68
    UiWidget* panel;                                        // +0x6c
    char pad70[0x1ac - 0x70];
    void* terrain;                                          // +0x1ac
    UiWidget* panel_center;                                 // +0x1b0
    UiWidget* key;                                          // +0x1b4
    UiWidget* copy;                                         // +0x1b8
    UiWidget* paste;                                        // +0x1bc
    UiSlider* sliders[6];                                   // +0x1c0
    UiSlider* seeds[3];                                     // +0x200
    float f20c[3];                                          // +0x20c
    UiWidget* w218;                                         // +0x218
    UiWidget* w21c;                                         // +0x21c
    UiWidget* w220;                                         // +0x220
    UiWidget* w224;                                         // +0x224
    UiWidget* region_type;                                  // +0x228
    UiWidget* region_detail;                                // +0x22c
    UiWidget* generate;                                     // +0x230
    UiWidget* close;                                        // +0x234

    void init();                                            // 0x4f1460
};

// ---- the other classes the window is made of -------------------------------------------------------
struct UiWindow : UiWidget {                                // ctor 0x4931a0, vptr 0x592848, 0x190 bytes
    char pad[0x188 - 4];
    char f188;
    UiWindow();
};
struct UiContainer : UiWidget {                             // ctor 0x4931a0, 0x188 bytes
    char pad[0x188 - 4];
    UiContainer();
};
struct FaceBoard : UiWidget {                               // ctor 0x4931a0 (+ vptr 0x597050), 0x188 bytes
    char pad[0x188 - 4];
    FaceBoard();
};

// ---- helpers / globals ------------------------------------------------------------------------------
extern char g_gui[];                                        // 0x5d7080 (the GUI root object)
// (reset 0x42d1b0 is gui_manager::reset, gui_push 0x49b120 is NodeVec::push_back: cs_common.h)
extern unsigned char g_gui_dirty;                           // 0x5d8788
extern int g_region_names[];                                // 0x59394c, 8 entries ending at 0x59396c

extern "C" int __cdecl sprintf(char*, const char*, ...);    // 0x536f8f
extern "C" int __cdecl rand(void);                          // 0x536a9f

struct UiB; struct DropDown;
UiB* make_slider(MissionUi* parent, float x, float y, float k);                          // 0x4dfd50 (cls_map)
DropDown* make_dropdown(MissionUi* parent, const tstring* name, float x, float y);       // 0x4dfec0 (cls_map)
UiWidget* make_medium(void* parent, tstring* a, float x, float y, float w, float h,
                      tstring* b, tstring* c);                                          // 0x4dcc50
UiWidget* make_short(void* parent, tstring* a, float x, float y, float w, float h,
                     tstring* b, tstring* c);                                           // 0x4dada0

#endif

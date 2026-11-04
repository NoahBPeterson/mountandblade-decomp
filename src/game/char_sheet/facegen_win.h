#ifndef FACEGEN_WIN_H
#define FACEGEN_WIN_H
// clang-format off
// Slice s004f1460: the terrain-generator window (the "facegen_board" page) -- its initialiser (0x4f1460), the view
// frustum set-up (0x4f25a0) and the terrain "region type" name table (0x4f2610). The window object is the terrain
// page of terrain_page.h (TerrainPage: the same offsets; its update 0x4f2730 / refresh_hex 0x4f2960 run at the end of
// init). MissionUi slots, the tstring helpers and the resource manager follow the verified neighbours (s0048add0,
// s004dfd50, s004e05c0, s004dd830).
#include "../../include/image_addrs.h"
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <new>
#include <vector>

using std::vector;

// ---- engine types -----------------------------------------------------------------------------------
// rgl::string: 16 bytes { pos, buf, cap, len }; ctor 0x42bbd0 zeroes the four words.
#include "../tstring.h"
#include "../head_rec.h"                                // head_rec: the screen records' base (the vptr at +0)
#include "../ui/container.h"                            // NodeA, text_box
#include "./cs_common.h"                                // new_container, MissionUi, gui_manager, NodeVec, CellGrid, new_sized_button, vtbl_*

// cloned rgl mesh; only the members this slice reaches
#include "../../rgl/mesh.h"
using rgl::mesh;

// resource manager object at 0x5d6fa0
#include "../res_mgr.h"                         // resmgr (0x5d6fa0): the one definition for the rgl.h-universe modules

// ---- the widget interface (slot offsets in comments) -----------------------------------------------
struct tvec2 {
    float x, y;
};

#include "../ui_widget.h"                 // MissionUi (the UI widget interface)
#include "../map_widget.h"                // MapWidget (0x358 bytes, ctor 0x4ed630): the 3D preview
#include "../terrain_page.h"              // TerrainPage: the same object (update 0x4f2730, refresh_hex 0x4f2960)
#include "../int_vec.h"                   // IntVec (the entity's mesh list at +0x118: _Insert_n 0x410610)
#include "../ui_globals.h"      // UI / core globals

// a container widget's children vector (ContainerBase::kids), pushed onto as the original inlines it
static __forceinline void push_child(std::vector<MissionUi*>& c, MissionUi* const& v)
{
    unsigned n = c._Myfirst == 0 ? 0 : (unsigned)(c._Mylast - c._Myfirst);
    if (c._Myfirst != 0 && n < (unsigned)(c._Myend - c._Myfirst)) {
        *c._Mylast = v;
        ++c._Mylast;
    } else {
        ((SceneObjVec*)&c)->_Insert_n(std::vector<SceneObject*>::iterator((SceneObject**)c._Mylast), 1, (SceneObject* const&)v);   // 0x4927d0
    }
}

// a 4-float temporary as the original copies it (the w lane is never written: the copy carries whatever the stack held)
struct v4raw {
    float x, y, z, w;
};

// the terrain-generator window ("this"); TerrainPage (terrain_page.h) names the same offsets
struct DropDown;
struct facegen_win : head_rec {
    char pad04[0x2c - 4];
    int terrain_on, terraingen_on, water_on, river_on;      // +0x2c..+0x38 (cleared)
    float camera_x, camera_y, camera_z;                     // +0x3c / +0x40 (150) / +0x44 (100)
    float fog;                                              // +0x48 (0.5)
    float water_height;                                     // +0x4c (0.8)
    int water_level;                                        // +0x50 (30)
    int layer_bits[3];                                      // +0x54 (rand() % 0x1b5781)
    int terrain_type;                                       // +0x60 (3)
    int map_size;                                           // +0x64 (0)
    NodeA* board;                                            // +0x68 the window (0x190, vptr 0x592848)
    MapWidget* view;                                        // +0x6c the 3D preview (0x358)
    rgl::frustum cam;                                       // +0x70 (0x120 bytes; its frame's origin row is +0x168)
    v4raw view_pos;                                         // +0x190 eye position
    float yaw, pitch, zoom;                                 // +0x1a0 / +0x1a4 / +0x1a8
    CellGrid* scene;                                        // +0x1ac the preview scene (0x344, ctor 0x478ad0)
    text_box* panel;                                         // +0x1b0 the board panel (0x188, vptr 0x597050)
    MissionUi* key;                                          // +0x1b4 the hex code text
    MissionUi* copy;                                         // +0x1b8
    MissionUi* paste;                                        // +0x1bc
    MissionUi* sliders[6];                                   // +0x1c0 size x / size y / hill height / valley / ruggedness / vegetation
    char pad1d8[0x200 - 0x1d8];
    MissionUi* seeds[3];                                     // +0x200 the seed edit boxes (0x188, vptr 0x592208)
    MissionUi* randomize[3];                                 // +0x20c the "randomize" buttons
    MissionUi* w218;                                         // +0x218 shade occluded
    MissionUi* w21c;                                         // +0x21c disable grass
    MissionUi* w220;                                         // +0x220 place river
    MissionUi* w224;                                         // +0x224 deep water
    DropDown* region_type;                                   // +0x228
    DropDown* region_detail;                                 // +0x22c
    MissionUi* generate;                                     // +0x230
    MissionUi* close;                                        // +0x234

    void init();                                            // 0x4f1460
};

// ---- helpers / globals ------------------------------------------------------------------------------
extern char g_gui[];                                        // 0x5d7080 (the GUI root object)
// (reset 0x42d1b0 is gui_manager::reset, gui_push 0x49b120 is NodeVec::push_back: cs_common.h)
extern char vtbl_592208[];                                  // 0x592208 (the single-line editor UiE, config/ui_widgets_c.h)

extern "C" int __cdecl sprintf(char*, const char*, ...);    // 0x536f8f
extern "C" int __cdecl rand(void);                          // 0x536a9f

struct UiB; struct DropDown;
UiB* make_slider(MissionUi* parent, float x, float y, float k);                          // 0x4dfd50 (cls_map)
DropDown* make_dropdown(MissionUi* parent, const tstring* name, float x, float y);       // 0x4dfec0 (cls_map)
void region_name(int i, char* out);                                                      // 0x4f2610

#endif

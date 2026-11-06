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
#include "../std_vec_insert_n.h"           // the entity's mesh list at +0x118: _Insert_n 0x410610
#include "../ui_globals.h"      // UI / core globals

// a 4-float temporary as the original copies it (the w lane is never written: the copy carries whatever the stack held)
struct v4raw {
    float x, y, z, w;
};

struct DropDown;
typedef TerrainPage facegen_win;     // the window object is the terrain page (terrain_page.h): init is its slot 2

// ---- helpers / globals ------------------------------------------------------------------------------
// (reset 0x42d1b0 is gui_manager::reset, gui_push 0x49b120 is NodeVec::push_back: cs_common.h)

extern "C" int __cdecl sprintf(char*, const char*, ...);    // 0x536f8f
extern "C" int __cdecl rand(void);                          // 0x536a9f

struct UiB; struct DropDown;
UiB* make_slider(MissionUi* parent, float x, float y, float k);                          // 0x4dfd50 (cls_map)
DropDown* make_dropdown(MissionUi* parent, const tstring* name, float x, float y);       // 0x4dfec0 (cls_map)
void region_name(int i, char* out);                                                      // 0x4f2610

#endif

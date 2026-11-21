// widget_mgr.h: the editor window's widget manager (factory methods that build buttons, a progress bar, a
// drop-down and a party button, hook them into the scene and keep them in a vector), the preset placement
// helpers, and the widget classes that have no out-of-line members of their own. Used by widget_mgr.cpp
// (0x499830..0x49ad20) and widget_node.cpp (0x49ad20..). ALL NAMES PROVISIONAL; every offset cites a function.
//
// The widgets are MissionUi-derived (../ui/mission_ui.h; base ctor 0x48ae20); the factories call its virtuals by name.
#ifndef GAME_CONFIG_WIDGET_MGR_H
#define GAME_CONFIG_WIDGET_MGR_H
#include <new>
#include <vector>
#include "../tstring.h"
#include "../ui/ui_widgets.h"
#include "../ui/container.h"
#include "./ui_widgets_c.h"
#include "../../rgl/resource.h"

using rgl::vector4;
#include "../../include/vec.h"


// ---- rgl::mesh (shared header) ----
#include "../../rgl/mesh.h"      // rgl::mesh::set_color 0x47af30

#include "../../rgl/resource.h"                    // resmgr g_resmgr (0x5d6fa0): get_object 0x419540 (a mesh resource by name; callers clone it)

// a 2-float vector with a constructor, for temporaries passed by pointer
// V2 (a vec2 with constructors) is in ../../include/vec.h

// ---- the mesh table g_meshes (0x8ed4f58): 0x48-byte records, placed(i) -> mesh (0x499050, another object) ---------------
#include "../../include/mesh_table.h"      // MeshTable, g_meshes

// ---- widget classes (vtable addresses in the comments); the button family (UiA, MediumButton, XyzButton, UiA) is in ../ui/ui_widgets.h ----
// The list container a drop-down opens (NodeA, vtable 0x592848) and the container it derives from are in ../ui/container.h.

// The stores of +0x178 and +0x121 come before DropDown's vptr store (0x4931d0), so they belong to an intermediate base's inline constructor.
struct DropMid : MissionUi {
    int f178;                                   // +0x178 list mode (0x4932a0)
    DropMid() { f178 = 0; b121 = 0; }
};
struct DropDown : DropMid {                     // ctor 0x4931d0, vtable 0x592570 (compiler-emitted), size 0x1d0
    char f180;                                  // +0x180 open (0x49aca0)
    int f184;                                   // +0x184
    NodeA* f188;                                // +0x188 the open list container (0x49aca0)
    std::vector<tstring> rows;                                // +0x18c the text rows (allocator word, first at +0x190; 0x49aca0, 0x49ad20, 0x493400)
    int f19c;                                   // +0x19c
    rgl::mesh* m1a0;                            // +0x1a0 (0x499d90)
    rgl::mesh* m1a4;                            // +0x1a4
    rgl::mesh* m1a8;                            // +0x1a8
    int f1ac;                                   // +0x1ac
    V2 button_size;                             // +0x1b0 the row button's size (combobox_child_button_size_x / _y; 0x49ad20 scales it by the widget size)
    float button_x;                             // +0x1b8 the rows' indent (combobox_child_button_position_x)
    V2 text_size;                               // +0x1bc the row text's size (combobox_child_button_text_size_x / _y: set_size of the child)
    V2 text_pos;                                // +0x1c4 the row text's offset (combobox_child_button_text_position_x / _y: set_offset of the child)
    int f1cc;                                   // +0x1cc
    DropDown() throw();                         // 0x4931d0 (world_data.cpp)
    // vtable 0x592570: MissionUi's slots, these overridden (the implicit destructor: its deleting form 0x4933d0 is the rows' _Tidy, ~MissionUi, then operator delete)
    virtual bool is_active() { return true; }   // +0x04 0x48b170 (ICF-shared with every `return true`)
    virtual void animate(int unused);           // +0x08 0x496d30 (world_data/mesh_hud.cpp: three meshes, two colour groups)
    virtual void render(rgl::scene* sc, float* count, float* c, float* d);   // +0x28 0x4932a0 (list mode switch, then MissionUi::render)
    virtual void set15c(int v);                 // +0x2c 0x493310 (clamps the selected index, then set_text on its row)
    virtual char key(unsigned ev);              // +0x8c 0x49aca0
    virtual void hide() { b121 = 0; f104 |= 1; }          // +0xb0 0x48b1a0
    virtual void flag2_set() { b121 = 0; f104 |= 2; }     // +0xb8 0x48b180
    virtual int kind();                         // +0xc0 0x493300 (5)
    virtual void reset_tail();                  // +0xc4 0x4933b0 (new slot 49)
    virtual void append(const tstring* name);   // +0xc8 0x493400 (ret 4): adds a row (new slot 50)
    NodeA* create();                            // 0x49ad20
};

struct Font;
#include "../font.h"
extern Font* g_font;                            // 0x5e2934

#include "../presentation.h"      // widget_mgr: the presentation whose overlays these factories build (widgets at +0, scene at +0x2c)

#endif

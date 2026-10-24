#ifndef CONV_OPEN_H
#define CONV_OPEN_H
// clang-format off
// Slice s004f66e0: the conversation (dialogue) screen's constructor 0x4f66e0 -- opens the screen: builds the
// talk overlay (+0x2c), the board/bar (+0x180), the window (+0x30), the speaker and line labels, the question
// (+0x188) and choice (+0x18c) boxes, the press label (+0x17c), 1024 body buttons (+0x190), the message
// window (+0x11e0), the relation bar/pointer (+0x11e8/+0x11e4) and five sentence boxes (+0x11ec); then resets
// the camera and re-targets the talk. Widget layouts/vcall slots, the tstring idioms, the config-table lookup
// and the ConvScreen offsets follow the neighbouring slice s004f73c0 (same `this`). The widget classes were
// identified by disassembling each constructor this function calls and dumping its vtable. Names provisional.
//
// The widget vcall slots in the original are set_pos=+0x58, set_size=+0x64, set_y=+0x68, set_scale2=+0x6c,
// set_flag=+0x74, set_mesh=+0x7c, set_track=+0x80, set_text=+0xa4, advance=+0xb0, add_child=+0xc8.
// The `Widget` declaration below orders the filler slots so those land on the right offsets.
//
// Identified widget classes (all constructed through ::operator new + a ctor that stores the vtable):
//   base Widget   0x178 bytes, ctor 0x48ae20, vtable 0x591c80
//   Panel         0x190 bytes, ctor 0x4931a0, vtable 0x592848, extra byte field +0x188
//   TextBox       0x188 bytes, ctor 0x4931a0, vtable 0x597050, vector<Widget*> at +0x178
//   Choices       0x188 bytes, ctor 0x48ae20, vtable 0x5924a0, three ints +0x17c/+0x180/+0x184
//   ViewWindow    0x358 bytes, ctor 0x4ed630, vtable 0x598698
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <new>
#include <vector>
#include "rgl.h"

using rgl::vector4;

// ---- string / tables -----------------------------------------------------------------------------------
#include "../tstring.h"
struct entry { int f0; char* value; char pad[0x2c - 8]; };  // config-table entry (0xa4425c + 0x2c*i)
extern int g_n_strings;                                  // 0xa70240
extern entry g_strings;                                  // 0xa44240 (indexed from 0xa4425c)
extern tstring g_empty_string;                           // 0x8ee30f0
struct ConfigTable {
    int find(const tstring& key);                        // 0x4bd5a0 (ordered search)
};
extern ConfigTable g_config;                             // 0xa44240
extern unsigned char g_ui_dirty;                         // 0x5d8788

// lookup of one setting (0x4bd5a0 + atof), 0.0f when the key is missing
static float config_float(const char* name)
{
    tstring key(name);
    int idx = g_config.find(key);
    float r = 0.0f;
    if (idx >= 0) {
        entry* e = (entry*)((char*)&g_strings + 0x1c) + idx;
        r = (float)atof(e->value ? e->value : (char*)&e->value);
    }
    return r;
}

// the fourth..seventh lookups at the end of 0x4f66e0 use an explicit linear `equals` scan instead of `find`
static float scan_float(const char* name)
{
    tstring key(name);
    int idx = -1;
    int i = 0;
    do {
        if (i >= g_n_strings)
            break;
        if (key.equals(((tstring*)((char*)&g_strings + 0x1c))[i]))
            idx = i;
        i++;
    } while (idx == -1);
    float r = 0.0f;
    if (idx >= 0) {
        entry* e = (entry*)((char*)&g_strings + 0x1c) + idx;
        r = (float)atof(e->value ? e->value : (char*)&e->value);
    }
    return r;
}

struct base_mesh;
struct resmgr {
    base_mesh* get_object(const tstring& name);          // 0x419540
};
extern resmgr g_alloc;                                   // 0x5d6fa0

extern rgl::matrix identity3();                          // 0x40cd60 (0x40 bytes by value)
void build_camera_planes(void* cam);                     // 0x41e4f0 (frustum::build_planes)
// the engine's UI manager singleton (0x5d7080); 0x42d1b0/0x49b120 are its methods.
struct gui_manager_s {
    void reset();                                        // 0x42d1b0
    void add(void* o);                                   // 0x49b120
};
extern gui_manager_s* g_gui;                             // 0x5d7080
void choice_finalize(void* box);                         // 0x488d50
#include "../str_table.h"
extern StrTable g_strtable;                              // 0x5e2a18

// ---- widget slots used here (s004f73c0 / s0042d360 gui.h) ----------------------------------------------
#include "../ui_widget.h"                 // Widget (the UI widget interface)

// ---- widget classes -------------------------------------------------------------------------------------
#include "../overlay.h"                       // Overlay (the slice's Panel), TextBox
typedef Overlay Panel;                         // the slice's name for it
#include "../map_widget.h"                       // MapWidget (the slice's ViewWindow: ctor 0x4ed630, vtable 0x598698, 0x358 bytes)
typedef MapWidget ViewWindow;

void __cdecl label_refresh(Widget* w, tstring name);       // 0x4e4c70 (cdecl, the name by value; the slice declared it as Widget::configure)

// factory helpers (parent is the last argument)
Widget* make_bar(Widget* talk, const tstring& name);                         // 0x4f6160
Widget* make_question(Widget* choice, const tstring& name, Widget* owner);   // 0x4f62f0
Widget* make_body_button(Widget* parent, const tstring& a, const tstring& b, const tstring& c,
                         float x, float y, float w, float h, float f5, float f6,
                         float f7, float f8);                                // 0x4dcc50

// ---- the conversation screen (`this` of 0x4f66e0, 0x4f73c0, 0x4f7d70) --------------------------------
#include "../conv_screen.h"                      // ConvScreen, Choices, base_mesh

// ---- mission / agent list helpers ---------------------------------------------------------------------
#include "../mission.h"


extern Mission* g_mission;                               // 0x5e292c
void screen_refresh(int mode, int* out);                 // 0x4f73c0 (ecx = screen)

#define PI_OVER_EIGHT 0.39269909262657166   // 0x599200 (read as a double constant by fptan)

// the dialogue camera (0x358 bytes, ctor 0x4ed630, vtable 0x598698). Field offsets other than the ones
// written by 0x4f66e0 are unknown, so this is a flat provisional layout, not a widget-derived one.
struct ConvCamera {
    char data[0x358];
    void init();                                         // 0x4ed630
    void configure(const tstring& s);                    // 0x4e4c70
    void set_pos(const float* p);                        // 0x488970 (slot +0x58 of vtable 0x598698)
    void set_size(const float* p);                       // 0x4889b0 (slot +0x64 of vtable 0x598698)
};

// memcpy of n dwords (the original inlines this as `rep movsd`)
static void dword_copy(int n, void* dst, const void* src)
{
    int* d = (int*)dst;
    const int* s = (const int*)src;
    for (int i = 0; i < n; i++)
        d[i] = s[i];
}

// the vector<Widget*>::push_back used at the two call sites (0x4927d0 is the out-of-line _Insert_n)

// ========================================================================================================
#endif

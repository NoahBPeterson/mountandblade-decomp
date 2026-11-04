#ifndef FACEGEN_INIT_H
#define FACEGEN_INIT_H
// s004f49d0 (ds1 slice 190): the "face_gen_window" construction (face generator menu).
// Provisional names; every offset comes from the disassembly of 0x4f49d0..0x4f5d17.
#include <windows.h>
#include <new>
#include <math.h>
#include "../../rgl/rgl_math.h"
#include "./cs_common.h"                                // widget constructors, make_* / label_refresh / new_button ..., FaceGenome, TextPart
#include "../res_mgr.h"                                 // res_manager()
#include "../../rgl/mesh_x.h"                           // mesh_x::clone 0x41c6d0

#define MB_NOTHROW throw()

extern "C" int _fltused = 0;

// ---- engine string (16 bytes) ----------------------------------------------------------------------------
#include "../tstring.h"

// ---- widgets ---------------------------------------------------------------------------------------------
struct Entity;

#include "../ui/container.h"                         // MissionUi, ContainerBase (the 0x188-byte element), NodeA (the 0x190-byte NodeA)
#include "../party2/camera_class.h"      // Camera (draw 0x488d50)

// the engine view frustum (0x120 bytes)
#include "../../include/frustum.h"                   // rgl::frustum (0x120 bytes; the slices called it rgl::frustum)
struct World;

// the 0x358-byte camera body (ctor 0x4ed630): widget + world link + frustum copy
#include "../map_widget.h"                 // MapWidget (the slice's face_body)
typedef MapWidget face_body;

#include "../face_gen.h"                              // FaceGen: the face-generator window record (ONE definition, ctor 0x4f49d0 is its slot 2)

// ---- engine vector of ints (lead dword, then first/last/end) ---------------------------------------------
#include "../engvec.h"                                 // engvec<T>

// ---- globals ---------------------------------------------------------------------------------------------
#include "../../include/msg_log.h"
extern MsgLog g_msglog;                                 // 0x5d7080
struct gui_mgr {
    void set_window(MissionUi** w);   // 0x49b120
};
extern gui_mgr g_gui;                                   // 0x5d7080
#include "../str_table.h"   // 0x43a1e0
#include "../ini_file.h"
#include "../ui_globals.h"      // UI / core globals
extern "C" double atof(const char* s);                  // 0x537352

#include "../../include/skin_set.h"              // SkinSet, g_anim.skins (shared with src/rgl/mesh2/gui_edit.h)
#include "../../include/game_data.h"            // GameData (g_game): the one definition (was a Game view here)

#include "../animator.h"                              // Animator, g_anim

struct TextPart {
    void get_text(int row, int index, tstring* out);   // 0x4f5d90 (editor_ui.cpp; the animator's text table at 0x8ed4f60)
};

// ---- callees: label_refresh, make_slider / make_scrollbar / make_editor_list, new_key_label, make_*_arrow, new_button, new_sized_button,
// clamp_index, TextPart::get_text are the definitions' own declarations (cs_common.h)
#include "../scene_world.h"                              // World: the 0x344-byte scene (ctor 0x478ad0)

// the 0x7c-byte wind track (ctor 0x461480)
#include "../wind.h"                              // wind (shared with terrain_a/wind.cpp)

static void push_item(ContainerBase* w, MissionUi* o)
{
    w->kids.push_back(o);
}

static float config_float(const char* key)
{
    float f = 0.0f;
    tstring t(key);
    int idx = g_game_vars.find(t);
    if (idx >= 0) {
        // the original: add eax, 0xa4425c / mov ecx, [eax+4] -> the value tstring's buf (entry +0x20), or the
        // inline bytes at &buf when it is null; not the entry's +0x1c dword
        f = (float)atof(g_game_vars.entries[idx].value.c_str());
    }
    return f;
}

#endif

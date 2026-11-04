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

// the face-generator window, ctor 0x4f49d0
struct FaceGen {
    char pad0[0x14];
    int troop;                                          // +0x14
    char pad18[0x2c - 0x18];
    NodeA* ui;                                        // +0x2c
    face_body* body;                                    // +0x30
    rgl::frustum fr;                                     // +0x34 (0x120 bytes)
    float v154[4];                                      // +0x154
    float f164;                                         // +0x164
    float f168;                                         // +0x168
    float f16c;                                         // +0x16c
    unsigned char f170;                                 // +0x170
    char pad171[0x174 - 0x171];
    int skin;                                           // +0x174
    World* world;                                       // +0x178
    ContainerBase* slider;                                       // +0x17c
    ContainerBase* deform;                                       // +0x180
    ContainerBase* buttons;                                      // +0x184
    ContainerBase* cam_btn;                                      // +0x188
    ContainerBase* deform_rows[0x30];                            // +0x18c (one row per deform key; 0x30 slots up to key_labels)
    MissionUi* key_labels[0x30];                           // +0x24c (up to b_next_beard)
    MissionUi* b_next_beard;                               // +0x30c
    MissionUi* b_next_hair;                                // +0x310
    MissionUi* b_next_face;                                // +0x314
    MissionUi* b_prev_beard;                               // +0x318
    MissionUi* b_prev_hair;                                // +0x31c
    MissionUi* b_prev_face;                                // +0x320
    MissionUi* t_change_beard;                             // +0x324
    MissionUi* t_change_hair;                              // +0x328
    MissionUi* t_change_face;                              // +0x32c
    ContainerBase* sl_age;                                       // +0x330
    ContainerBase* sl_hair_color;                                // +0x334
    MissionUi* t_age;                                      // +0x338
    MissionUi* t_hair_color;                               // +0x33c
    MissionUi* b_done;                                     // +0x340
    MissionUi* b_reset;                                    // +0x344
    MissionUi* b_randomize;                                // +0x348
    Entity* ent;                                        // +0x34c
    int f350;                                           // +0x350
    int f354;                                           // +0x354
    int f358;                                           // +0x358
    int f35c;                                           // +0x35c
    float f360;                                         // +0x360
    float f364;                                         // +0x364
    void init();                                        // 0x4f49d0
    void refresh();                                     // 0x4eb3c0
};

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

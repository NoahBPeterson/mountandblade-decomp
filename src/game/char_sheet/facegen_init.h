#ifndef FACEGEN_INIT_H
#define FACEGEN_INIT_H
// s004f49d0 (ds1 slice 190): the "face_gen_window" construction (face generator menu).
// Provisional names; every offset comes from the disassembly of 0x4f49d0..0x4f5d17.
#include <windows.h>
#include <new>
#include <math.h>
#include "rgl.h"
#define CS_REAL_FACEGEN                                 // (this header defines FaceGen)
#include "./cs_common.h"                                // widget constructors, make_* / label_refresh / new_button ..., FaceGenome, TextPart
#include "./cs_res.h"                                   // resmgr g_alloc, mesh_x

#define MB_NOTHROW throw()

extern "C" int _fltused = 0;

// ---- engine string (16 bytes) ----------------------------------------------------------------------------
#include "../tstring.h"

// ---- widgets ---------------------------------------------------------------------------------------------
struct Entity;

// the UI element base (0x178 bytes, ctor 0x48ae20): src/game/widget.h
#include "../widget.h"

// engine vector of UI elements: {lead, first, last, end}
struct objvec {
    int lead;                                           // +0x00
    widget** first;                                     // +0x04
    widget** last;                                      // +0x08
    widget** end;                                       // +0x0c
    // (insert_n 0x4927d0 is std::vector<SceneObject*>::_Insert_n, cs_common.h)
};

// the 0x188-byte UI element (ctor 0x4931a0; "deform"/"buttons" vptr 0x597050)
struct elem : widget {
    elem() MB_NOTHROW;                                  // 0x4931a0
    objvec items;                                       // +0x178
};

// the 0x190-byte overlay window (vptr 0x592848, ctor 0x4931a0)
struct overlay : elem {
    unsigned char f188;                                 // +0x188
    char pad189[0x190 - 0x189];
};

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
    overlay* ui;                                        // +0x2c
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
    elem* slider;                                       // +0x17c
    elem* deform;                                       // +0x180
    elem* buttons;                                      // +0x184
    elem* cam_btn;                                      // +0x188
    elem* deform_rows[0x30];                            // +0x18c (one row per deform key; 0x30 slots up to key_labels)
    widget* key_labels[0x30];                           // +0x24c (up to b_next_beard)
    widget* b_next_beard;                               // +0x30c
    widget* b_next_hair;                                // +0x310
    widget* b_next_face;                                // +0x314
    widget* b_prev_beard;                               // +0x318
    widget* b_prev_hair;                                // +0x31c
    widget* b_prev_face;                                // +0x320
    widget* t_change_beard;                             // +0x324
    widget* t_change_hair;                              // +0x328
    widget* t_change_face;                              // +0x32c
    elem* sl_age;                                       // +0x330
    elem* sl_hair_color;                                // +0x334
    widget* t_age;                                      // +0x338
    widget* t_hair_color;                               // +0x33c
    widget* b_done;                                     // +0x340
    widget* b_reset;                                    // +0x344
    widget* b_randomize;                                // +0x348
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
extern unsigned char g_msg_tick;                        // 0x5d8788
struct gui_mgr {
    void set_window(widget** w);   // 0x49b120
};
extern gui_mgr g_gui;                                   // 0x5d7080
#include "../str_table.h"   // 0x43a1e0
extern StrTable g_strtable;                             // 0x5e2a18
#include "../ini_file.h"
extern ini_file g_config;                               // 0xa44240
extern "C" double atof(const char* s);                  // 0x537352

#include "../../include/skin_set.h"              // SkinSet, g_skins_begin (shared with src/rgl/mesh2/gui_edit.h)
#include "../../include/game_data.h"            // GameData (g_game): the one definition (was a Game view here)
extern int g_no_cam;                                    // 0x5a9fb0

#include "../animator.h"                              // Animator, g_anim
extern float g_dummy_mat;                               // 0x8ed4f8c
extern float g_dummy0;                                  // 0x8ed5088
extern float g_dummy1;                                  // 0x8ed508c
extern int g_char_count;                                // 0x8ed4f88
extern float g_k0_01;                                   // 0x54a6f0 = 0.01f
extern float g_fov_k;                                   // 0x5e20a8
extern const double g_tan_ang;                          // 0x598e90

struct TextPart {
    void get_text(int row, int index, tstring* out);   // 0x4f5d90 (editor_ui.cpp; the animator's text table at 0x8ed4f60)
};

// ---- callees: label_refresh, make_slider / make_scrollbar / make_editor_list, new_key_label, make_*_arrow, new_button, new_sized_button,
// clamp_index, TextPart::get_text are the definitions' own declarations (cs_common.h)
#include "../scene_world.h"                              // World: the 0x344-byte scene (ctor 0x478ad0)

// the 0x7c-byte wind track (ctor 0x461480)
#include "../wind.h"                              // wind (shared with terrain_a/wind.cpp)

// World::tracks (scene_world.h TrackVec) is a std::vector<int>: its push_back's out-of-line _Insert_n is vector<int>::_Insert_n (0x410610,
// protected: reached through a derived class), not the TrackVec view's
namespace std { template<> void vector<int, allocator<int> >::_Insert_n(iterator where, size_type n, const int& v); }   // 0x410610
struct IntVecIns : std::vector<int> { using std::vector<int>::_Insert_n; };
static void push_track(World::TrackVec& t, int v)
{
    if (t.first == 0 || (unsigned)(t.last - t.first) >= (unsigned)(t.end - t.first))
        ((IntVecIns*)&t)->_Insert_n(IntVecIns::iterator(t.last), 1, v);
    else
        *t.last++ = v;
}

static void push_item(elem* w, widget* o)
{
    if (w->items.first == 0 ||
        (unsigned)(w->items.last - w->items.first) >= (unsigned)(w->items.end - w->items.first))
        ((SceneObjVec*)&w->items)->_Insert_n(SceneObjVec::iterator((SceneObject**)w->items.last), 1, (SceneObject* const&)o);   // 0x4927d0
    else
        *w->items.last++ = o;
}

static float config_float(const char* key)
{
    float f = 0.0f;
    tstring t(key);
    int idx = g_config.find(t);
    if (idx >= 0) {
        // the original: add eax, 0xa4425c / mov ecx, [eax+4] -> the value tstring's buf (entry +0x20), or the
        // inline bytes at &buf when it is null; not the entry's +0x1c dword
        f = (float)atof(g_config.entries[idx].value.c_str());
    }
    return f;
}

#endif

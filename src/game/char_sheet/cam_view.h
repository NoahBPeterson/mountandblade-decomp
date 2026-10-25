#ifndef CAM_VIEW_H
#define CAM_VIEW_H
// Slice s004ea8e0: the character-sheet camera/rotation widget. 0x4ea8e0 updates the model preview
// (zoom slider, clipboard pose dump, reset keys); 0x4eb050 is the per-frame orbit/zoom step.
// Names provisional; all offsets come from the disassembly of this range.
#include <windows.h>
#include <math.h>
#include "rgl.h"

using rgl::vector4;

#include "../../include/mat4.h"
using rgl::mat4;
using rgl::mat3;

#include "./cs_common.h"
#include "./cs_mat.h"                                // identity_pos, UiD, WindowMgrW, FaceRace, FaceGen

// key state / settings globals
extern int g_a17204;                                    // 0xa17204
extern int g_a17214;                                    // 0xa17214
extern int g_a1720c;                                    // 0xa1720c
extern int g_a171fc;                                    // 0xa171fc
extern int g_a17454;                                    // 0xa17454
extern int g_a17854;                                    // 0xa17854
extern int g_a17458;                                    // 0xa17458
extern float g_5d7094;                                  // 0x5d7094
extern float g_5d7098;                                  // 0x5d7098
extern float g_a178d4;                                  // 0xa178d4
extern float g_a178d8;                                  // 0xa178d8

// globals of the face-generator update (0x4ea8e0)
extern int g_no_cam;                                    // 0x5a9fb0
extern int g_a17148;                                    // 0xa17148
extern int g_a17348;                                    // 0xa17348
extern int g_a1711c;                                    // 0xa1711c
extern int g_a1751c;                                    // 0xa1751c
extern int g_world_time;                                // 0x5d8790
extern float g_scroll;                                  // 0x590d7c
extern char* g_game;                                    // 0x5e2930
extern float g_dummy_mat;                               // 0x8ed4f8c (float; one per deform key)
extern float g_dummy1;                                  // 0x8ed508c

#include "../../include/skin_set.h"              // SkinSet, g_skins_begin (shared with src/rgl/mesh2/gui_edit.h)
extern int g_cur;                                       // 0x8ed4f68

#include "../scene_world.h"                              // World::step 0x427f40
#include "../tstring.h"
#include "../str_table.h"      // 0x43a1e0
extern StrTable g_strtable;                             // 0x5e2a18
#include "../main_menu.h"
#define g_main_menu (*(main_menu*)0xa73980)

#include "../../rgl/text_stream.h"                    // mb_snprintf (0x415100)
#include "../animator.h"                              // Animator, g_anim

// a UI element; only the vtable slots this function calls are named
struct Ctrl {
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
    virtual void v10(); virtual void v14(); virtual void v18(); virtual void v1c();
    virtual void v20(); virtual void v24(); virtual void v28(); virtual void v2c();
    virtual void v30();
    virtual void v34(float v);                          // +0x34
    virtual float v38();                                // +0x38
    virtual void v3c(); virtual void v40();
    virtual char v44(int key);                          // +0x44
    virtual void v48(int key);                          // +0x48
    virtual void v4c(); virtual void v50(); virtual void v54();
    virtual void v58(); virtual void v5c(); virtual void v60(); virtual void v64();
    virtual void v68(); virtual void v6c(); virtual void v70(); virtual void v74();
    virtual void v78(); virtual void v7c(); virtual void v80(); virtual void v84();
    virtual void v88(); virtual void v8c(); virtual void v90(); virtual void v94();
    virtual void v98(); virtual void v9c(); virtual void va0(); virtual void va4();
    virtual void va8(); virtual void vac();
    virtual void vb0();                                 // +0xb0
    virtual void vb4();                                 // +0xb4
    // (set_value 0x48baa0 is UiD's: ((UiD*)ctrl)->set_value(v))
};

// the character-sheet / face-generator preview (ctor 0x4f49d0); offsets from this range
struct CamView {
    char pad0[0x08];
    int  state;                                         // +0x08
    char pad0c[0x14 - 0x0c];
    int  troop;                                         // +0x14
    int  choice;                                        // +0x18
    char pad1c[0x30 - 0x1c];
    char* target;                                       // +0x30
    char pad34[0x164 - 0x34];
    float yaw;                                          // +0x164
    float pitch;                                        // +0x168
    float zoom;                                         // +0x16c
    unsigned char follow;                               // +0x170
    char pad171[0x174 - 0x171];
    int  skin;                                          // +0x174
    World* world;                                       // +0x178
    Ctrl* slider;                                       // +0x17c
    char pad180[0x188 - 0x180];
    Ctrl* cam_btn;                                      // +0x188
    Ctrl* rows[0x1c];                                   // +0x18c
    char pad25c[0x30c - 0x1fc];
    Ctrl* b_next_beard;                                 // +0x30c
    Ctrl* b_next_hair;                                  // +0x310
    Ctrl* b_next_face;                                  // +0x314
    Ctrl* b_prev_beard;                                 // +0x318
    Ctrl* b_prev_hair;                                  // +0x31c
    Ctrl* b_prev_face;                                  // +0x320
    char pad324[0x330 - 0x324];
    Ctrl* sl_age;                                       // +0x330
    Ctrl* sl_hair_color;                                // +0x334
    char pad338[0x340 - 0x338];
    Ctrl* b_done;                                       // +0x340
    Ctrl* b_reset;                                      // +0x344
    Ctrl* b_randomize;                                  // +0x348
    void* ent;                                          // +0x34c
    int  f350;                                          // +0x350
    int  f354;                                          // +0x354
    int  f358;                                          // +0x358
    int  f35c;                                          // +0x35c
    char pad360[0x368 - 0x360];
    float pose[8];                                      // +0x368 (x..., 8 pose floats)
    char buf388[0x28];                                  // +0x388
    char buf3b0[0x28];                                  // +0x3b0
    char buf3d8[0x28];                                  // +0x3d8
    char buf400[0x10];                                  // +0x400

    void update(float dt);                              // 0x4eb050
    // (setup 0x4eb270 and refresh 0x4eb3c0 are FaceGen's update_view / refresh, cs_common.h)
    void update_model(float dt);
};

#define SKIN_INT(off) (*(int*)((char*)g_skins_begin + g_cur * 0x1f8 + (off)))
#define g_dummy_mat_arr (&g_dummy_mat)

#endif

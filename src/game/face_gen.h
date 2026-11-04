// face_gen.h: the face generator window (vtable 0x595c68, record 29 of the window manager, 0x428 bytes as 0x4d0620 allocates it): a
// head_rec whose slot 1 deletes the preview world (rgl/mesh2/mesh_2.cpp 0x4cb680), slot 2 builds the window (char_sheet/facegen_init.cpp
// 0x4f49d0) and slot 3 is the per-frame update (char_sheet/cam_view.cpp 0x4ea8e0). ONE definition for char_sheet (facegen_init: the
// construction, facegen_misc: update_view 0x4eb270 / refresh 0x4eb3c0, cam_view: the per-frame update and the orbit step 0x4eb050);
// the three slices each saw part of the layout and called it FaceGen / CamView. head_rec's own words are the screen state: w08 the
// tutorial state (0: not shown yet, 1: shown, -1: left through the tutorial), w14 the troop whose face is edited (-1: a new one), w18
// the menu state Done returns to. Needs rgl::frustum, MapWidget and MissionUi from the includer's headers (included below).
#ifndef MB_GAME_FACE_GEN_H
#define MB_GAME_FACE_GEN_H
#include "./head_rec.h"
#include "./ui/container.h"
#include "./map_widget.h"
#include "./animator.h"                       // PackedState
#include "../include/frustum.h"
namespace rgl { struct strategic_entity; }   // the preview entity (rgl/scene/scene_entity.h, included by the TUs that call into it)

struct World;

struct FaceGen : head_rec { MB_ZERO_TAIL(FaceGen)   // vtable 0x595c68
    NodeA* ui;                                          // +0x2c
    MapWidget* body;                                    // +0x30 the 3D preview widget (ctor 0x4ed630)
    rgl::frustum fr;                                    // +0x34 (0x120 bytes)
    float v154[4];                                      // +0x154
    float yaw;                                          // +0x164
    float pitch;                                        // +0x168
    float zoom;                                         // +0x16c
    unsigned char follow;                               // +0x170 the mouse drags the camera
    int skin;                                           // +0x174
    World* world;                                       // +0x178 the preview scene; reset_screen deletes it
    ContainerBase* slider;                              // +0x17c
    ContainerBase* deform;                              // +0x180
    ContainerBase* buttons;                             // +0x184
    ContainerBase* cam_btn;                             // +0x188
    ContainerBase* deform_rows[0x30];                   // +0x18c (one row per deform key; 0x30 slots up to key_labels)
    MissionUi* key_labels[0x30];                        // +0x24c (up to b_next_beard)
    MissionUi* b_next_beard;                            // +0x30c
    MissionUi* b_next_hair;                             // +0x310
    MissionUi* b_next_face;                             // +0x314
    MissionUi* b_prev_beard;                            // +0x318
    MissionUi* b_prev_hair;                             // +0x31c
    MissionUi* b_prev_face;                             // +0x320
    MissionUi* t_change_beard;                          // +0x324
    MissionUi* t_change_hair;                           // +0x328
    MissionUi* t_change_face;                           // +0x32c
    ContainerBase* sl_age;                              // +0x330
    ContainerBase* sl_hair_color;                       // +0x334
    MissionUi* t_age;                                   // +0x338
    MissionUi* t_hair_color;                            // +0x33c
    MissionUi* b_done;                                  // +0x340
    MissionUi* b_reset;                                 // +0x344
    MissionUi* b_randomize;                             // +0x348
    rgl::strategic_entity* ent;                         // +0x34c
    int f350;                                           // +0x350
    int f354;                                           // +0x354
    int f358;                                           // +0x358
    int f35c;                                           // +0x35c
    float f360;                                         // +0x360
    float f364;                                         // +0x364
    union {
        PackedState state;                              // +0x368 the face as the troop record packs it (Animator::pack_state 0x4eb660)
        float pose[8];                                  // +0x368 the same 32 bytes, written back to the troop record by Done
    };
    char keys[4][0x28];                                 // +0x388 the four hex strings of the face code (the clipboard text)

    virtual void reset_screen();                        // slot 1: 0x4cb680 (rgl/mesh2/mesh_2.cpp)
    virtual void init_screen();                         // slot 2: 0x4f49d0 (char_sheet/facegen_init.cpp)
    virtual void frame_update(float dt);                // slot 3: 0x4ea8e0 (char_sheet/cam_view.cpp)
    virtual char pure_7() { return 1; }                 // slot 7: 0x48b170
    void update(float dt);                              // 0x4eb050 (cam_view.cpp: the orbit / zoom step)
    void update_view(int unused);                       // 0x4eb270 (ret 4)
    void refresh();                                     // 0x4eb3c0
};

#endif // MB_GAME_FACE_GEN_H

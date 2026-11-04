// campaign_screen.h: the campaign map screen (CampaignScreen: a head_rec, vtable 0x595bd8; the slices called it Screen and head_cam / particle_cam): the map bar overlay at +0x2c, the map widget at
// +0x30, the menu buttons / hints / banners at +0x34..+0x64, the map camera's frustum at +0x170 (0x120 bytes), and the camera
// state (target +0x290, velocity +0x2a0, yaw / pitch / zoom +0x2b0.., pan and zoom rates +0x2bc..). ONE definition for char_sheet
// (screen_init: init_screen 0x4ec4d0 and camera_mark_entities 0x4ecce0; screen_camera: camera_update 0x4ece20) and
// inventory_panel (mission_ctx: frame_update 0x514d40, camera_move 0x515d80, 0x5191e0). The mission_ctx slice named the camera
// state yaw / speed / height / x / z / vy / y (anonymous unions below keep those spellings next to the screen_camera ones) and
// typed the widgets MissionUi*; the widgets are the MissionUi interface of src/game/ui_widget.h.
// Needs rgl::frustum (src/include/frustum.h) and the includer's tstring.
#ifndef MB_GAME_CAMPAIGN_SCREEN_H
#define MB_GAME_CAMPAIGN_SCREEN_H
#include "./head_rec.h"                  // head_rec: the vptr, +4 / +8, the name at +0x1c
#include "./ui_widget.h"
#include "./ui/container.h"
#include "./map_widget.h"
#include "../include/frustum.h"

struct CampaignScreen : head_rec {
    NodeA* overlay;                           // +0x2c   map bar
    MapWidget* f30;                             // +0x30   the map widget (the 3D panel)
    MissionUi* f34;                                // +0x34   menu buttons
    MissionUi* f38;                                // +0x38
    MissionUi* f3c;                                // +0x3c   ui_party
    MissionUi* f40;                                // +0x40   ui_inventory
    MissionUi* f44;                                // +0x44   ui_character
    int f48;                                    // +0x48   ui_terrain quad (a widget pointer in mission_ctx)
    MissionUi* f4c;                                // +0x4c   ui_camp
    MissionUi* f50;                                // +0x50   ui_quests
    MissionUi* f54;                                // +0x54   paused banner
    MissionUi* f58;                                // +0x58   wait hint
    MissionUi* f5c;                             // +0x5c   first hint
    MissionUi* f60;                             // +0x60   second hint
    char f64;                                   // +0x64
    char pad65[0x170 - 0x65];
    rgl::frustum cam;                           // +0x170  the camera frustum (0x120 bytes; copied into the map widget at +0x1ac)
    rgl::vector4 target;                        // +0x290  camera target (x, y, z, w)
    rgl::vector4 velocity;                      // +0x2a0  camera velocity
    union { float f2b0; float yaw; };           // +0x2b0  yaw (rad)
    union { float f2b4; float speed; };         // +0x2b4  pitch (rad)
    union { float f2b8; float height; };        // +0x2b8  zoom / height
    union { float f2bc; float x; };             // +0x2bc  pan rate x
    union { float f2c0; float z; };             // +0x2c0  pan rate y
    union { float f2c4; float vy; };            // +0x2c4  yaw rate
    union { float f2c8; float y; };             // +0x2c8  zoom rate
    float f2cc;                                 // +0x2cc
    float f2d0;                                 // +0x2d0
    char f2d4;                                  // +0x2d4
    char f2d5;                                  // +0x2d5
    int f2d8;                                   // +0x2d8  (0x5191e0 stores it into the picked party)
    CampaignScreen();                           // 0x4c49f0 (the record is 0x2dc bytes: 0x4d0620 allocates it)
    virtual void reset_screen();                // slot 1: 0x4c6e00, the GUI reset (options_panel/rec_slots_hc.cpp)
    virtual void init_screen();                 // slot 2: 0x4ec4d0
    virtual void frame_update(float dt);        // slot 3: 0x514d40 (ret 4)
    virtual char pure_7() { return 1; }         // slot 7: 0x48b170
    virtual void write_stream(w_stream* w);     // slot 5: 0x4c4b70
    virtual void read_stream(w_stream* r);      // slot 6: 0x4c4c00
    void camera_mark_entities();                // 0x4ecce0
    void camera_update(float dt);               // 0x4ece20 (ret 4)
    void camera_move(float dt);                 // 0x515d80 (ret 4)
    void update_target();                             // 0x5191e0
};

#endif // MB_GAME_CAMPAIGN_SCREEN_H

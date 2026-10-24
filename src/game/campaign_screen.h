// campaign_screen.h: the campaign map screen (Screen, vtable of 10 screen slots): the map bar overlay at +0x2c, the map widget at
// +0x30, the menu buttons / hints / banners at +0x34..+0x64, the map camera's frustum at +0x170 (0x120 bytes), and the camera
// state (target +0x290, velocity +0x2a0, yaw / pitch / zoom +0x2b0.., pan and zoom rates +0x2bc..). ONE definition for char_sheet
// (screen_init: init_screen 0x4ec4d0 and camera_mark_entities 0x4ecce0; screen_camera: camera_update 0x4ece20) and
// inventory_panel (mission_ctx: frame_update 0x514d40, camera_move 0x515d80, 0x5191e0). The mission_ctx slice named the camera
// state yaw / speed / height / x / z / vy / y (anonymous unions below keep those spellings next to the screen_camera ones) and
// typed the widgets MissionUi*; the widgets are the Widget interface of src/game/ui_widget.h.
// Needs rgl::frustum (src/include/frustum.h) and the includer's tstring.
#ifndef MB_GAME_CAMPAIGN_SCREEN_H
#define MB_GAME_CAMPAIGN_SCREEN_H
#include "./ui_widget.h"
#include "./overlay.h"
#include "./map_widget.h"
#include "../include/frustum.h"

struct Screen {
    virtual void s00(int); virtual void s01(int); virtual void s02(int); virtual void s03(int);
    virtual void s04(int); virtual void s05(int); virtual void s06(int); virtual void s07(int);
    virtual void s08(int); virtual void s09(int);
    int f4;                                     // +0x4
    char pad8[0x2c - 8];
    Overlay* overlay;                           // +0x2c   map bar
    MapWidget* f30;                             // +0x30   the map widget (the 3D panel)
    Widget* f34;                                // +0x34   menu buttons
    Widget* f38;                                // +0x38
    Widget* f3c;                                // +0x3c   ui_party
    Widget* f40;                                // +0x40   ui_inventory
    Widget* f44;                                // +0x44   ui_character
    int f48;                                    // +0x48   ui_terrain quad (a widget pointer in mission_ctx)
    Widget* f4c;                                // +0x4c   ui_camp
    Widget* f50;                                // +0x50   ui_quests
    Widget* f54;                                // +0x54   paused banner
    Widget* f58;                                // +0x58   wait hint
    WidgetObj* f5c;                             // +0x5c   first hint
    WidgetObj* f60;                             // +0x60   second hint
    char f64;                                   // +0x64
    char pad65[0x170 - 0x65];
    rgl::frustum cam;                           // +0x170  the camera frustum (0x120 bytes; copied into the map widget at +0x1ac)
    float f290;                                 // +0x290  camera target x
    float f294;                                 // +0x294  camera target y
    float f298;                                 // +0x298  camera target z
    float f29c;                                 // +0x29c
    float f2a0;                                 // +0x2a0  velocity x
    float f2a4;                                 // +0x2a4  velocity y
    float f2a8;                                 // +0x2a8  velocity z
    float f2ac;                                 // +0x2ac
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
    void init_screen();                         // 0x4ec4d0
    void camera_mark_entities();                // 0x4ecce0
    void camera_update(float dt);               // 0x4ece20 (ret 4)
    void frame_update(float dt);                // 0x514d40 (ret 4)
    void camera_move(float dt);                 // 0x515d80 (ret 4)
    void f5191e0();                             // 0x5191e0  // old name of MapPanel::update_target, removed after the names merge
};

#endif // MB_GAME_CAMPAIGN_SCREEN_H

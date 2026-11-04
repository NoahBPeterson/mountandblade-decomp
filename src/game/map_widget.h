// map_widget.h: the 3D view widget (0x358 bytes, ctor 0x4ed630, dtor 0x4ed6b0, vtable 0x598698): the base widget object,
// the current scene/world link at +0x178, an embedded render scene at +0x17c (ctor 0x429740, dtor 0x41f4e0; its frustum sits at
// +0x1ac), the camera flags at +0x308 and the clear colour at +0x30c. ONE definition for char_sheet (screen_init / screen_camera:
// the campaign map's MapWidget; conv_open: the conversation's ViewWindow and Camera; facegen_init / facegen_misc and
// inventory_panel/view_open: the face-generator and portrait views, face_body; inventory_panel/map_picker: Camera, which
// projects through it, project 0x51a690). Every view constructs it with 0x4ed630 and shares the vtable 0x598698.
// Needs rgl::frustum (src/include/frustum.h).
#ifndef MB_GAME_MAP_WIDGET_H
#define MB_GAME_MAP_WIDGET_H
#include "./ui_widget.h"
#include "../include/frustum.h"
#include "../rgl/scene_rec.h"           // rgl::scene (0x1d8 bytes)

struct MapWidget : MissionUi {
    void* world;                                // +0x178 (screen_init: the current scene g_a732b4; face views: their World; the mission HUD: Mission::scene, 0x4f7f00)
    rgl::scene scene3d;                         // +0x17c the embedded render scene (ctor 0x429740, dtor 0x41f4e0): its frustum scene3d.fr sits at +0x1ac,
                                                //        scene3d.f16a at +0x2e6 (the constructor sets 1), scene3d.flags at +0x308 (0x200), scene3d.f190 +0x30c the clear colour
    char f354;                                  // +0x354 (the mission HUD sets it when the 0x5aa034 option is set: 0x4f7f00)
    char pad355[3];
    MapWidget() throw();                        // 0x4ed630
    void project(rgl::vector4* out, const rgl::vector4* in);   // 0x51a690 (ret 8; src/game/inventory_panel/map_picker.cpp: world point -> screen)
};

#endif // MB_GAME_MAP_WIDGET_H

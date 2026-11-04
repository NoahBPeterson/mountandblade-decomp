// camera_class.h: the one declaration of the 0x4a8-byte view object built by the constructor 0x49deb0 (Camera). Its members are defined
// in party2/camera.cpp: type 0x49df30, mask 0x49df40, move 0x49df60, update 0x49dfe0, scalar deleting destructor 0x49e580, render 0x49e5b0.
//
// Layout: the container (ContainerBase, 0x188 bytes: MissionUi, constructed by 0x48ae20, and the child vector), the render scene (rgl::scene, 0x1d8 bytes at +0x188),
// the box frustum (rgl::frustum, 0x120 bytes at +0x360), two follow objects at +0x480 and the followed shape at +0x490.
// The scrolling-list widgets of the other modules (party screen skills list, the editor / quest list 0x4f62f0, the HUD tree, the
// party-create item list) are built by the same constructor and use +0x484 (follow[1]) and +0x490 (target) for their scroll bar and
// parent / row count.
// Include after the translation unit's declaration of rgl::scene (src/rgl/mesh.h, or src/game/rgl_mesh.h in the rgl.h universe: both are
// the 0x1d8-byte class); a translation unit that only passes Camera pointers around forward-declares it.
#ifndef MB_CAMERA_CLASS_H
#define MB_CAMERA_CLASS_H
#include <stddef.h>
#include "../../include/frustum.h"
#include "../ui/container.h"          // ContainerBase: the view is a container (MissionUi + the child vector at +0x178)

struct float2 {                                 // a 2-float vector (position/displacement)
    float x, y;
    float2() {}
    float2(float a, float b) : x(a), y(b) {}
};

// ---- the view itself ----------------------------------------------------------------------------------------------
struct Camera : ContainerBase {
    rgl::scene   scene;                         // +0x188  (0x429740 / 0x41f4e0)
    rgl::frustum fr;                            // +0x360  (0x41ef60)
    MissionUi* follow[2];                       // +0x480  the two follow objects (the list widgets keep their scroll bar in follow[1], +0x484)
    void*   follow_shape[2];                    // +0x488  their shapes
    ContainerBase* target;                      // +0x490  the object the view tracks (the list widgets keep their parent / row count here)
    float   shake_x;                            // +0x494
    float   shake_y;                            // +0x498
    float2  tail;                               // +0x49c
    int     f4a4;                               // +0x4a4

    Camera();                                   // 0x49deb0
    void* destroy(unsigned flags);              // 0x49e580: slot 0's scalar deleting destructor (the scene's destructor, then ContainerBase's 0x490520; written out, a real ~Camera() would store the vptr first)
    virtual void animate(int unused);           // +0x08 0x49dfe0
    virtual void render(rgl::scene* sc, float* pos, int c, int d);   // +0x28 0x49e5b0
    virtual void on_child_moved(MissionUi* e);  // +0x5c 0x49df60
    virtual int kind();                         // +0xc0 0x49df30 (7)
    unsigned mask(int a);                       // 0x49df40
};

// compile-time layout check (VC7.1 has no static_assert)
#define MB_CAMERA_ASSERT(name, cond) typedef char camera_assert_##name[(cond) ? 1 : -1]
MB_CAMERA_ASSERT(scene_off, offsetof(Camera, scene) == 0x188);
MB_CAMERA_ASSERT(frustum_off, offsetof(Camera, fr) == 0x360);
MB_CAMERA_ASSERT(follow_off, offsetof(Camera, follow) == 0x480);
MB_CAMERA_ASSERT(follow1_off, offsetof(Camera, follow) + sizeof(MissionUi*) == 0x484);
MB_CAMERA_ASSERT(target_off, offsetof(Camera, target) == 0x490);
MB_CAMERA_ASSERT(flags_off, offsetof(Camera, f104) == 0x104);
MB_CAMERA_ASSERT(camera_size, sizeof(Camera) == 0x4a8);

#endif // MB_CAMERA_CLASS_H

#ifndef SCREEN_CAMERA_H
#define SCREEN_CAMERA_H
// Slice s004ece20: the campaign map camera tick and the map widget's constructor / destructor.
//  0x4ece20  Screen::camera_update steps the campaign map camera by dt:
//            * integrates the pan stick (+0x2bc / +0x2c0) and the yaw rate (+0x2c4) at 10x, then the
//              zoom rate (+0x2c8) at 10x into the zoom (+0x2b8), clamped to 3.5 .. 105.  While the
//              tactical-view globals 0xa17148 / 0xa1717c / 0x5a9fbc are all set the pan and yaw are
//              pinned to 0 and the zoom limit becomes 235.
//            * re-marks the parties (0x4ecce0) whenever the zoom changed.
//            * eases the four rates to zero, each through its own (1 - dt*9.8, floored at 0) clamp.
//            * clamps the camera to the +/-50 map rectangle (x: 0x5aa120 / 0x5aa118, y: 0x5aa124 /
//              0x5aa11c) and derives the pitch from the zoom (0.014*zoom + 0.23, clamped 0.1 .. pi/2).
//            * when g_game+0x88 is set, follows the selected party: inside 0.05*zoom of the party
//              centre (+0x30, lifted by 1) it adds 4*dt*delta (delta clamped to +/-10) to the velocity,
//              and outside it the velocity is zeroed.
//            * integrates the velocity into the camera target (+0x290 .. +0x298) and halves it
//              (1 - dt) per frame.
//            * builds the frame: identity (0x40cd00), a +90 degree matrix_rotate_fu (0x40e570), then
//              matrix_rotate_su (0x40e670, -yaw) and matrix_rotate_fu (-pitch).  The eye is the camera
//              target (+0x290) plus zoom * (frame row s + row f), pulled half a row u back; 0x470f40
//              finds the map position under it, which is moved to at least eye.z + 0.5.
//            * writes the frame into the map widget's scene (frustum 0x41e4f0 at +0x1ac), computes the
//              fog distance parameter (from g_game+0x13b3c#0x13b40 and the zoom, with the 1.7 / 1.6 /
//              1.5 range factors), packs the fog colour (0x40e440 at g_game+0x58) and sets the fade
//              (0x4624c0), then stores the inverse frame (scene+0x30c) and the listener position
//              (scene+0x31c), copies the frame into the map label matrix (+0x1ac of the widget) and,
//              the first time only, sets the 3D listener (0x5342dc).
//  0x4ed630  MapWidget ctor  widget base 0x48ae20, vptr 0x598698, embedded scene 0x429740 at +0x17c,
//                            then +0xe0 = 0, +0xe4 = 0, +0xf4 = 1.75, +0xf0 = 0, +0x2e6 = 1, +0x308 = 0x200.
//  0x4ed6b0  MapWidget dtor  vptr 0x598698, scene dtor 0x41f4e0 at +0x17c, widget dtor 0x487c00, then
//                            operator delete when the deleting flag's low bit is set.
// flags /O2 /MT /Gy /TP
#include "../../include/image_addrs.h"
#include <new>
#include <string.h>
#include <math.h>
#include "rgl.h"

using rgl::vector4;
#include "../../include/mat4.h"   // rgl::mat4: the 0x40-byte frame; rotate_fu 0x40e570 / rotate_su 0x40e670 are its members
using rgl::mat4;

// ---- the 16-byte engine string -------------------------------------------------------------------------
#include "../tstring.h"

// ---- widgets and overlays (src/game/ui_widget.h, overlay.h, map_widget.h) -----------------------------------
#include "../ui_widget.h"
#include "../overlay.h"
#include "../map_widget.h"

// ---- the render scene (0x1d8 bytes, ctor 0x429740; see s00428240/world.h) ------------------------------
#include "../../include/frustum.h"
using rgl::frustum;
#include "../rgl_mesh.h"
using rgl::scene;
// the scene-container object g_scene (0xa732b4, Mission +0) as the camera tick sees it: the slice called it `scene` because its first
// 0x150 bytes are laid out like a render scene (frustum at +0x30), but it is much larger (the inverse frame at +0x30c); renamed so that
// `scene` is only the 0x1d8-byte render scene (rgl_mesh.h) that the map widget embeds at +0x17c.
struct WorldCam {
    int f0, f4, f8, fc, f10;                    // 0x00
    float f14;                                  // 0x14
    int f18, f1c, f20, f24, f28;                // 0x18
    float f2c;                                  // 0x2c
    frustum fr;                                 // 0x30
    char pad150[0x30c - 0x150];
    vector4 f30c;                               // +0x30c  the inverse frame
    vector4 f31c;                               // +0x31c  the eye
    char pad32c[0x350 - 0x32c];
    int f350;                                   // +0x350
};

// ---- the campaign screen (`this` of 0x4ece20) ----------------------------------------------------------
#include "../campaign_screen.h"            // Screen (the campaign map screen)

// 0x4903e0 / 0x48ad00: thiscall on the party manager (g_game + 0xf8), one argument each
#include "../../include/party.h"

// ---- game objects -------------------------------------------------------------------------------------
#include "../../include/game_data.h"            // GameData (g_game, 0x5e2930): the one definition (was a Game view here)
// (the 0x5ac8-byte party record is Party, src/include/party.h; the slice view PartyRec was unused here)
// ---- globals ------------------------------------------------------------------------------------------
extern WorldCam* g_level;                       // 0xa732b4
extern int g_a17148;                            // 0xa17148
extern int g_a1717c;                            // 0xa1717c
extern int g_5a9fbc;                            // 0x5a9fbc
extern int g_5a9fd8;                            // 0x5a9fd8  listener already set
extern float g_map_min_x;                            // 0x5aa120  map min x (-50)
extern float g_map_max_x;                            // 0x5aa118  map max x ( 50)
extern float g_map_min_y;                            // 0x5aa124  map min y (-50)
extern float g_map_max_y;                            // 0x5aa11c  map max y ( 50)
extern char g_world0[0x28];                     // the strategy world whose +0x28 cell table 0x470f40 reads

// the by-name float constants (the originals live in the read-only pools noted here)
#define C_549D74 (orig::g_ten)             // 10.0
#define C_549D70 (orig::g_half)             // 0.5
#define C_549CDC (orig::g_one)             // 1.0
#define C_549CA8 (orig::g_zero)             // 0.0
#define C_549D7C (orig::k_0_1f)             // 0.1
#define C_54A534 (orig::k_1_5708f)             // 1.5707964
#define C_54A6D0 (orig::k_1em05f)             // 1e-05
#define C_598690 (orig::k_105f)             // 105.0
#define C_59868C (orig::k_235f)             // 235.0
#define C_598688 (orig::k_m1em05f)             // -1e-05
#define C_590D8C (orig::k_3_5f)             // 3.5
#define C_590AF0 (orig::k_9_8f)             // 9.8
#define C_590F7C (orig::k_0_014f)             // 0.014
#define C_590E68 (orig::k_0_23f)             // 0.23
#define C_54B4D0 (orig::k_0_05f)             // 0.05
#define C_5915A0 (orig::k_m10f)             // -10.0
#define C_5935C8 (orig::g_four)             // 4.0
#define C_54B3B8 (orig::k_0_4f)             // 0.4
#define C_590C74 (orig::k_0_08f)             // 0.08
#define C_590FA4 (orig::k_1_7f)             // 1.7
#define C_591AF4 (orig::k_0_94f)             // 0.94
#define C_590C48 (orig::k_1_6f)             // 1.6
#define C_5911D8 (orig::k_0_98f)             // 0.98
#define C_54B8B0 (orig::k_1_5f)             // 1.5
#define C_549D6C (orig::k_0_9f)             // 0.9

// the listener block (written by 0x5342dc, from the map frame)
struct Listener3D {
    vector4 pos;                                // +0 (0x5aa138)
    vector4 vel;                                // +0x10 (0x5aa148)
    vector4 dir;                                // +0x20 (0x5aa158)
    vector4 up;                                 // +0x30 (0x5aa168)
    float extra[2];                             // +0x40 (0x5aa178)
};
extern Listener3D g_listener;                   // 0x5aa138


// ---- callees ------------------------------------------------------------------------------------------
#include "./cs_common.h"
#include "./cs_mat.h"                                      // identity_pos 0x40cd00
using rgl::pack_color;                                      // 0x40e440 (cdecl, ret 0; declared in rgl_mesh.h)

struct envelope;                                            // the scene-envelope object 0x4624c0 works on

#include "../scene_world.h"                              // World::scene_query 0x470f40: thiscall on the world, two stack args, callee pops 8 bytes

// 0x41e4f0: rgl::frustum::build_planes (src/include/frustum.h), thiscall on the frustum, no arguments

// 0x4624c0: thiscall on the envelope (ecx = g_scene both times), two stack args
struct envelope {
    void set_fade(const float* dist, const int* color);   // 0x4624c0
};

// (scene() 0x429740 and ~scene() 0x41f4e0 are rgl_mesh.h's, the widget base constructor 0x48ae20 is MissionUi's, its destructor 0x487c00 Buffer's: cs_common.h)

#include "../../include/vec.h"                      // vector2 (= Vec2)
void __stdcall listener_set(float* pos, float* vel,
                            float dx, float dy, float dz,
                            float ux, float uy, float uz);  // 0x5342dc (FMOD 3; 8 args)

#endif

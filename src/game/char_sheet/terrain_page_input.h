#ifndef TERRAIN_PAGE_INPUT_H
#define TERRAIN_PAGE_INPUT_H
// Slice s004f3570: the terrain "edit" page's field reading, camera simulation, two small rotation helpers and
// the clipboard "paste".
//   0x4f3570  read_fields: decode the terrain bit words back into the page fields (enables, positions, layer
//             bits, water level/height, terrain type/size)
//   0x4f37f0  simulate(dt): keyboard camera steering (pitch, zoom, strafe, pan), clamping
//   0x4f3b40  rotate a 2x2 rotation matrix (m[0..3]) by an angle
//   0x4f3be0  write a 2x2 identity into four floats
//   0x4f3c30  paste_clipboard: read CF_TEXT from the clipboard, parse the terrain words from it, set the hex
//             field's text, then read the fields back and refresh the page
//   0x4f3cf0  path(dt): build the map camera's frame and hand it to the terrain renderer
// All names provisional. The page layout is the one recovered in slice s004f2d70 (TerrainPage); the terrain bit
// words live in the global object at 0xa76630 (slice s004f25a0 / s00464a60).
#include <windows.h>
#include <stdio.h>
#include <math.h>
#pragma intrinsic(sqrt, fabs, sin, cos, tan, fsin, fcos)
// the x87 inline sin/cos used by rotate_2x2 (inline FP is off in this build, so the intrinsics need the asm form)

#include "rgl.h"

using rgl::vector4;

struct vector2 { float x, y; };                             // rgl::vector2 (the 2x2 camera axes, +0 = strafe)

// ---- tstring / rgl::string ---------------------------------------------------------------------------
#include "../tstring.h"

// ---- controls -----------------------------------------------------------------------------------------
// Only the accessor this slice calls is declared.  cl 13.10 numbers virtuals in declaration order, one slot per
// declaration, but a reference parameter opens a family numbered 9 slots above the no-parameter virtuals.
struct Widget {
    char pad00[0xa0];
    virtual void put(const tstring& s);                     // +0xa4  set_text
};

// ---- terrain bit words (the object at 0xa76630) -------------------------------------------------------
#include "../bit_words.h"
extern unsigned g_type_mask;                                // 0x5a8e88 (= 4: the terrain type is 4 bits wide)
extern unsigned g_size_mask;                                // 0x5a8e78 (= 4: the terrain size is 2 bits wide)

// ---- the world / scene --------------------------------------------------------------------------------
struct FrustumBlock { int d[0x48]; void build_planes(); };   // build_planes = 0x41e4f0 (thiscall)
#include "../scene_world.h"                              // World: the page's +0x6c pointer target (block[] +0x1ac)

// 0x40cd00 has a hidden return pointer and returns it; the two rotation helpers are thiscall on the matrix and
// take a pointer to the angle (thiscall, ret 4).
struct mat_self : rgl::matrix {
    void rotate_fu(const float* angle);                     // 0x40e570 (rows s/f about u)
    void rotate_su(const float* angle);                     // 0x40e670 (rows s/u about f)
};
mat_self identity_pos();                                    // 0x40cd00 (cdecl, hidden return)
void rotate_fu(void* m, const float* angle);                // 0x40e570 (same thiscall, for callers holding a raw ptr)
void rotate_su(void* m, const float* angle);                // 0x40e670

void rotate_2x2(float* m, float angle);                     // 0x4f3b40
void identity_2x2(float* m);                                // 0x4f3be0

// ---- the page -----------------------------------------------------------------------------------------
struct TerrainPage {
    char    pad00[0x2c - 0x00];
    int     terrain_on;                                     // +0x2c  "Enable terrain" checkbox
    int     terraingen_on;                                  // +0x30  "Enable terrain generation" checkbox
    int     water_on;                                       // +0x34  "Enable water" checkbox
    int     river_on;                                       // +0x38  "Enable river" checkbox
    float   camera_x;                                       // +0x3c  X position
    float   camera_y;                                       // +0x40  Y position
    float   camera_z;                                       // +0x44  camera height above the ground
    float   fog;                                            // +0x48  fog (0..1)
    float   water_height;                                   // +0x4c  water height (0..1)
    int     water_level;                                    // +0x50  water level
    int     layer_bits[3];                                  // +0x54 / +0x58 / +0x5c  random bits of the water layers
    int     terrain_type;                                   // +0x60  terrain type (index into the type table)
    int     map_size;                                       // +0x64  terrain size
    char    pad68[4];                                       // +0x68
    World*  world;                                          // +0x6c
    int     map_block[(0x138 - 0x70) / 4];                  // +0x70  the 0x120-byte map block (runs on through frame[]; frustum source at
                                                            //        +0xf8 = block+0x88; copied to world +0x1ac)
    float   frame[0x10];                                    // +0x138  the frame path() builds (from a temp)
    char    pad178[0x18];                                   // +0x178
    float   view_x;                                         // +0x190  eye X (clamped to 0..250)
    float   view_y;                                         // +0x194  eye Y (clamped to 0..250)
    float   view_z;                                         // +0x198  eye Z
    char    pad19c[0x1a0 - 0x19c];
    float   yaw;                                            // +0x1a0  camera heading
    float   pitch;                                          // +0x1a4  camera pitch (0.1..1.5)
    float   zoom;                                           // +0x1a8  camera zoom (1.5..400.5)
    char    pad1ac[0x1b4 - 0x1ac];
    Widget* hexbox;                                         // +0x1b4  the hex terrain text field
    char    pad1b8[0x2c0 - 0x1b8];

    void read_fields();                                     // 0x4f3570
    void simulate(float dt);                                // 0x4f37f0
    void paste_clipboard();                                 // 0x4f3c30
    void path(float dt);                                    // 0x4f3cf0
    void update();                                          // 0x4f2730 (slice s004f25a0)
};

// the input flags the camera reads (0xa17xxx block; names provisional)
extern int g_key_17204;                                     // 0xa17204  pitch down
extern int g_key_17214;                                     // 0xa17214  pitch up
extern int g_key_1720c;                                     // 0xa1720c  zoom in
extern int g_key_171fc;                                     // 0xa171fc  zoom out
extern int g_key_1714c;                                     // 0xa1714c  strafe one way (camera-space axis)
extern int g_key_17154;                                     // 0xa17154  strafe the other way
extern int g_key_17150;                                     // 0xa17150  pan one way
extern int g_key_17118;                                     // 0xa17118  pan the other way
extern int g_key_17458;                                     // 0xa17458  turn (heading)
extern int g_key_17454;                                     // 0xa17454  roll with the vertical axis
extern float g_axis_y;                                      // 0xa178d8  second input axis
extern float g_axis_x;                                      // 0xa178d4  first input axis
extern float g_mouse_y;                                     // 0x5d7094  vertical look axis compared with 0.5

#endif

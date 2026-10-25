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
#include "./cs_common.h"
#include "./cs_mat.h"
#include "../../include/frustum.h"                    // rgl::frustum::build_planes (0x41e4f0)

using rgl::vector4;

#include "../../include/vec.h"                      // vector2 (= Vec2)

// ---- tstring / rgl::string ---------------------------------------------------------------------------
#include "../tstring.h"

// ---- controls ----------------------------------------------------------------------------------------------
#include "../ui_widget.h"                 // Widget (the UI widget interface)

// ---- terrain bit words (the object at 0xa76630) -------------------------------------------------------
#include "../bit_words.h"
extern unsigned g_type_mask;                                // 0x5a8e88 (= 4: the terrain type is 4 bits wide)
extern unsigned g_size_mask;                                // 0x5a8e78 (= 4: the terrain size is 2 bits wide)

// ---- the world / scene --------------------------------------------------------------------------------
struct FrustumBlock { int d[0x48]; };                        // the 0x120-byte frustum (copied page +0x70 -> world +0x1ac)
struct DustTerrain {
    void update_particles(const rgl::matrix* view);   // 0x4f3e10 (dust.h: the terrain object 0xa76630)
};
#include "../scene_world.h"                              // World: the page's +0x6c pointer target (block[] +0x1ac)

// 0x40cd00 has a hidden return pointer and returns it; the two rotation helpers are thiscall on the matrix and
// take a pointer to the angle (thiscall, ret 4).
struct mat_self : rgl::matrix {};                            // (rotate_about_s 0x40e570 / rotate_about_f 0x40e670 are rgl::matrix's; identity_pos 0x40cd00: cs_mat.h)

void rotate_2x2(float* m, float angle);                     // 0x4f3b40
void identity_2x2(float* m);                                // 0x4f3be0

// ---- the page: src/game/terrain_page.h ------------------------------------------------------------------
#include "../terrain_page.h"

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
extern float g_look_y;                                     // 0x5d7094  vertical look axis compared with 0.5

#endif

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

#include "../../include/vec.h"                      // vector2 (= Vec2)

// ---- tstring / rgl::string ---------------------------------------------------------------------------
#include "../tstring.h"

// ---- controls ----------------------------------------------------------------------------------------------
#include "../ui_widget.h"                 // Widget (the UI widget interface)

// ---- terrain bit words (the object at 0xa76630) -------------------------------------------------------
struct BitWords {
    unsigned w[8];                                          // w[0..5] are the six hex words
    void set(const unsigned& lo, const int& hi, const unsigned& v);   // 0x464960
    unsigned get(const unsigned& lo, const int& hi);                  // 0x4649b0
    void build_hex_text(tstring& out);                                // 0x4f29b0 (slice s004f25a0)
    void parse_hex(const tstring& s);                                 // 0x4658c0 fills w[0..5] from "0x..."
    void submit_frame(const float* rect);                             // 0x4f3e10 (next slice)
};
extern BitWords g_bits;                                     // 0xa76630
extern unsigned g_type_mask;                                // 0x5a8e88 (= 4: the terrain type is 4 bits wide)
extern unsigned g_size_mask;                                // 0x5a8e78 (= 4: the terrain size is 2 bits wide)

// ---- the world / scene --------------------------------------------------------------------------------
struct Block { int d[0x48]; void build_planes(); };   // build_planes = 0x41e4f0 (thiscall)
struct World {                                              // the page's +0x6c pointer target
    char pad00[0x1ac];
    int block[0x48];                                        // +0x1ac  the 0x120-byte map block (copy of the page's +0x70)
};

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
extern float g_mouse_y;                                     // 0x5d7094  vertical look axis compared with 0.5

#endif

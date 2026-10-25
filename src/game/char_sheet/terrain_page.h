#ifndef TERRAIN_PAGE_H
#define TERRAIN_PAGE_H
// Slice s004f25a0: the "edit/scene" terrain page.
//   0x4f2610  name of a terrain ground type ("Water", "Mountain", ... — same table as s00484590)
//   0x4f2730  page update: refresh every terrain widget from the page fields, then the hex field
//   0x4f28a0  caption a terrain checkbox with "Enabled" / "Disabled"
//   0x4f28f0  caption a terrain number field with "%d"
//   0x4f2a10  format one word as eight hex digits (by hand: digits + 'A'..'F')
//   0x4f2960  build the hex terrain string and hand it to the text box
//   0x4f29b0  build the hex terrain string from the six terrain words (most significant first)
//   0x4f2ae0  write the page values into the terrain bit words at 0xa76630 (the s00464a60 terrain object)
// All names provisional. The page layout follows s004d6b80's video page (+0x18 option, +0x1b4 text box).
//
// 0x4f25a0 (a frustum init) is not a member of any class found so far: the function is a __thiscall on an
// object with a flag at +0, four floats at +0x108..+0x114, near/far at +0x118/+0x11c and a plane array built
// by 0x41e4f0. It sits in this slice only because of its address (see partial.txt).
#include <windows.h>
#include <stdio.h>
#include <math.h>
#pragma intrinsic(sqrt, fabs, sin, cos, tan, fsin, fcos, fptan)
#include "rgl.h"
#include "./cs_common.h"

using rgl::vector4;

// ---- tstring / rgl::string ---------------------------------------------------------------------------
// Kept as this derived view (not src/game/tstring.h): the byte-exact rows of this module (set_enabled_text 0x4f28a0,
// build_hex_text 0x4f29b0) need the inline constructor wrapper `tstring(const char*) : rgl::string(s) {}`; with the
// canonical out-of-line constructor cl hoists the widget's vptr load above the constructor call (56 / 21 bytes differ).
struct tstring : rgl::string {
    tstring(const char* s) : rgl::string(s) {}                              // 0x42bbe0
    tstring();                                                              // 0x42bbd0
    void assign(const char* s);                                             // 0x42bcb0 (the const char* assignment)
    void append(const tstring& o);                                          // 0x42c1e0
    ~tstring();                                                             // 0x42bc40
    const char* c_str() const { return p ? p : (const char*)&p; }
};
extern "C" int __cdecl sprintf(char*, const char*, ...);                    // 0x536f8f
extern "C" int __cdecl sscanf(const char*, const char*, ...);               // 0x535fe4

// ---- controls (layouts verified in s004d5ba0 / s004d63b0 / s004d6b80) --------------------------------
// set_text returns an 8-byte object through a hidden pointer that no caller destroys: a non-POD pair (a user
// constructor, no destructor). 0x4f28a0/0x4f28f0/0x4f2960 reserve exactly 8 bytes for it.
#include "../ui_widget.h"                 // Widget (the UI widget interface; the range widgets are Widgets too: set_value is slot +0x34)
typedef Widget RangeWidget;

// ---- terrain bit words (the object at 0xa76630) -------------------------------------------------------
extern unsigned __int64 g_pow2[];                           // 0x5a8e70  g_pow2[n] = 2 << n
extern const int g_mask_w[1];                               // 0x5a8e78 (= 2: the terrain size is 2 bits wide)
extern const int g_type_mask[1];                            // 0x5a8e88 (= 4: the terrain type is 4 bits wide)
#include "../bit_words.h"

extern int g_use_shaders;                                          // 0x5e28fc (shaders on)
extern int g_size_modes[8];                                 // 0x59394c terrain-size mode table (only [0] is ours)

// ---- the page: src/game/terrain_page.h ------------------------------------------------------------------
#include "../terrain_page.h"

void format_hex(unsigned v, char* out);                     // 0x4f2a10 (argument order recovered from the codegen)
void read_fields(TerrainPage* p);                               // 0x4f3570
void copy_clipboard(TerrainPage* p);                               // 0x4f34c0
void paste_clipboard(TerrainPage* p);                               // 0x4f3c30
void simulate(TerrainPage* p, float dt);                     // 0x4f37f0
void path(TerrainPage* p, float dt);                     // 0x4f3cf0

extern const float g_half;                                  // 0x549d70 (0.5f)
extern const float g_01;                                    // 0x54a6f0 (0.01f)
extern const float g_140;                                   // 0x598e6c (140.0f)
extern const float g_00143;                                 // 0x598e68 (0.0014285714f)
extern const float g_10;                                    // 0x549d74 (10.0f)

extern const char g_fmt_d[];                                // 0x54a0f4 "%d"
extern const char g_str_enabled[];                          // 0x598e7c "Enabled"
extern const char g_str_disabled[];                         // 0x598e70 "Disabled"
extern const char g_hex_sep[];                              // 0x598e84 " "

#endif

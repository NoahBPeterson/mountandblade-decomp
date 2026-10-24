#ifndef TERRAIN_PAGE_UPDATE_H
#define TERRAIN_PAGE_UPDATE_H
// Slice s004f2d70: the terrain "edit" page's per-frame update and its "copy terrain code" button.
//   0x4f2d70  update(this, dt): first the six (checkbox, number) pairs of the three terrain slots and the
//             per-slot selection checkboxes, then exactly one changed-widget chain (terrain flags, the six
//             sliders, terrain type/size, copy/paste), then the "regenerate" button, then the world step.
//   0x4f34c0  copy the hex terrain code (the text of the page's hex field) to the Windows clipboard.
// All names provisional. The page layout is the one recovered in slice s004f25a0 (TerrainPage); the page is a
// widget page whose fields start at +0x18 (the option the menu returns to).
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include "rgl.h"

using rgl::vector4;

// NOTE (slot numbering): cl 13.10 numbers a class's virtuals in DECLARATION order, one slot per declaration,
// but two families are numbered from a base of their own: a parameter given by reference (const vector2&)
// starts 0x24 past the (empty) family, and the (int) family starts 0x24 past too.  So a virtual's slot is
// "#declarations since its own last representative of that family", not its declaration index: that is why the
// binary's (int) accessor changed(0x10000) is at +0x44 (the 2nd (int) after activate) while the getters at
// +0x30/+0x38 are the 3rd/4th () after v00.  The accessors are therefore placed a few declarations apart in
// the block below so that the emitted offsets match the binary (see the comments).

// ---- tstring / rgl::string (layouts verified in s004d6b80 / s004f25a0) --------------------------------
#include "../tstring.h"

extern "C" int __cdecl sscanf(const char*, const char*, ...);               // 0x536f7c

// ---- controls (layouts verified in s004d5ba0 / s004d63b0 / s004d6b80) ---------------------------------
#include "../../include/vec.h"                      // vector2 (= Vec2)
// How this block was measured (cl 13.10, /O2):  within one parameter family (no parameters, (int), a
// reference argument, ...) consecutive virtuals get consecutive slots, and every reference argument family is
// numbered 0x24 above the no-parameter family (see the note above).  The declarations below are laid out so
// that each accessor the page calls compiles to the offset the binary uses:
//
//   0x00  v00 (first virtual; v01..v04 share its slot -> the binary's table is unreachable through any of them)
//   0x28  activate(int)
//   0x30  get_value()            3rd ()
//   0x38  get_float()            4th ()
//   0x44  changed(int)           2nd (int) after activate (declared 7th)
//   0x44  v27/v28 (vector2)
//   0x90  set_default()          31st ()
//   0x94  x94
//   0x98  set_colour(int)
//   0x9c  x9c
//   0xa0  xa0
//   0xa4  put (set_text)
//   0xa8  take (get_text)
//   0xac  put (set_text with a position) -- a reference family of its own
//   0xb0  show
#include "../ui_widget.h"                 // Widget (the UI widget interface)

// ---- terrain bit words (the object at 0xa76630) -------------------------------------------------------
extern unsigned __int64 g_pow2[];                           // 0x5a8e70  g_pow2[n] = 2 << n
struct BitWords {
    unsigned w[8];
    void set(const unsigned& lo, const int& hi, const unsigned& v);   // 0x464960
    unsigned get(const unsigned& lo, const int& hi);                  // 0x4649b0
    void build_hex_text(tstring& out);                                // 0x4f29b0
};
extern BitWords g_bits;                                     // 0xa76630
void terrain_set_a(const float* p);                         // 0x464a60
struct Pct { char pad0[0x10]; unsigned bits; void set_pct(const float& f); };   // 0x4649f0
// 0x4649f0 is a method of this object (the percentage field rides in the low 7 bits of bits at +0x10)
extern Pct g_pct;                                           // 0xa76630

// ---- the world / scene --------------------------------------------------------------------------------
// effect object created by 0x478f30 (0x7c bytes; layout from s00494e40)
#include "../eff_obj.h"                                     // EffObj / EffMgr
struct World {                                              // the page's +0x1ac; scene object 0x421390 resets
    char pad00[0x68];
    vector4 f68;                                            // +0x68
    float   f74;                                            // +0x74
    void reset();                                           // 0x421390
    void step(float dt);                                    // 0x427f40
    EffObj* create_effect() { return ((EffMgr*)this)->create(); }
};
struct terrain_obj {                                        // the object at 0xa76630 (see s00481d10)
    void generate(World* w, int b, int c);                  // 0x481d10
    void regenerate_water(int a, int* out);                 // 0x482170
    void destroy_water();                                   // 0x4814a0
};
struct MainMenu {
    void choose(int option);                                // 0x495700
};

float  clamp_float(const float* x, const float* lo, const float* hi);   // 0x40e3d0
void   free_ptr(void* p);                                   // 0x535712

extern int   g_size_modes[8];                               // 0x59394c terrain-type table (index 0..7)
extern float g_clamp_lo;                                    // 0x593948
extern float g_500;                                         // 0x590b40 (500.0f)
extern float g_140;                                         // 0x598e6c (140.0f)
extern float g_0_01;                                        // 0x54ab28 (0.01f)
extern float g_one;                                         // 0x549d74 (1.0f)
extern float g_half;                                        // 0x549d70 (0.5f)
extern int   g_env2;                                        // 0x5e28fc (shaders on)
extern void* g_a17110;                                      // 0xa17110
extern void* g_a17510;                                      // 0xa17510
extern MainMenu g_main_menu;                                // 0x5e2xxx
extern terrain_obj g_terrain_obj;                           // 0xa76630
extern const char g_fmt_d[];                                // 0x54a0f4 "%d"

// the page: src/game/terrain_page.h (Paged, TerrainPage)
#include "../terrain_page.h"

static inline bool page_changed(Widget* w) { return w->changed(0x10000) != 0; }

// the caption / visibility updates that follow a checkbox change (slice s004f25a0)
void set_enabled_text(Widget* w);                           // 0x4f28a0
void set_hex3_text(int i);                                  // 0x4f28f0
void refresh_hex();                                         // 0x4f2960
void sync_bits();                                           // 0x4f2ae0

#endif

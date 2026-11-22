// module_option_records.h: classes of module_option_records.cpp (0x4c2c50..0x4c48d0): the module/option record classes derived
// from head_rec, the options loader, the renderer setup, the music/sound objects and the tstring array loaders.
// Merged from the slices s004c2b20, s004c3430, s004c3d90 and s004c46d0; names are provisional unless a vtable
// maps them (C4c*, in symbols/slices/module_option_records.txt). Offsets cite the function that proves them.
#ifndef MESH2_REC_TABLES_H
#define MESH2_REC_TABLES_H
#include "platform/image_addrs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>
#include <windows.h>
#include <shlobj.h>
#include <d3d9.h>
#include "engine/core/tstring.h"
#include "engine/math/rgl_math.h"
#include "engine/io/stream.h"
#include "engine/core/ptr_tag.h"
#include "game/ui/gui_manager.h"     // g_log (log_stream)
#include "game/config/ini_file.h"               // ini_file (find 0x4bd5a0), g_game_vars
#include "game/editor/mesh2_common.h"
#include "game/data/head_rec_rgl_mesh2.h"
#include "game/ui/screens/result_window.h"
#include "engine/render/render_flags.h"
#include "engine/render/d3d_caps.h"
#include "game/common/game_globals.h"
#include "game/ui/screens/screen_records.h"

using rgl::vector4;

// ---- engine entry points (mesh2_common.h: engine_alloc/new/free/delete, sub_536f8f) --------------------------
void __cdecl vector_constructor_iterator(void* dst, int size, int n, void* ctor);     // 0x401800

// ---- the C4c* record classes (vtable 0x5947d8 base, own vtable stored after the base ctor) ---------------
// ResultWindow (vtable 0x595740, ctor 0x4c2c50): game/ui/screens/result_window.h

struct M44 { float f[16]; };
namespace rgl { struct matrix; } typedef rgl::matrix pos;                                          // engine/math/mat4_include.h: the 0x40-byte frame, layout of M44

// The 0x130-byte record of constructor 0x4c2d10 is the mission's entry point, MarkRec (game/campaign/mark_rec.h).
#include "game/campaign/mark_rec.h"

// PartyScreen (ex C4c3160, vtable 0x595870, the party exchange window) is declared in game/ui/screens/party_screen_class.h

// 0x18-byte entry of the block read by 0x4c2dd0 (tstring, two ints): config.h's clip_slot (same layout; its constructor
// 0x4be720 is out of line, the array new goes through the vector constructor iterator). module_option_records.cpp includes config.h.
struct clip_slot;
struct Entry18;
// Block with a tstring at +0, two ints and an entry array at +0x18 (0x4c2dd0).
struct Blk2dd0 {
    tstring  s0;             // +0
    int      v10;            // +0x10
    int      v14;            // +0x14
    int      count;          // +0x18
    Entry18* entries;        // +0x1c
    void read(stream_base* s);   // 0x4c2dd0 (thiscall, ret 4)
};

#include "engine/fx/fx.h"   // canonical: g_ticks (0x5e2958)


// ---- options.dat loader (0x4c3a40) and GameApp::InitDeviceObjects (0x4c3510) -----------------------------------
#include "engine/render/renderer_include.h"   // canonical: g_render.in_scene (0xa17e14)


// the renderer at 0xa17910 (renderer::check_shadow_formats 0x4bc350), the device set at 0xa170c8 (init_devices), the render
// batch at 0x5fe898 (init_decls) and the resource manager at 0x5d6fa0 (load_all) are renderer.h's renderer, config.h's InputSet,
// render_batch.h's render_batch and engine/resource/resource.h's resmgr (included by module_option_records.cpp).

// g_game_vars (0xa44240) is ini_file.h's ini_file (entries of 0x2c bytes: key tstring +0, value tstring +0x1c)

// 0x4c0520 is a cdecl function without parameters (game/app/options_dialog_and_module_tables.cpp); the original calls it with ecx = the renderer object, which
// it ignores: module_option_records.cpp calls it through the fastcall type of that call
void load_game_variables();                         // 0x4c0520 (resets one buffer-size group)
// 0x4bab70 user_path (argument in ESI, a static helper of another TU) has a local copy in module_option_records.cpp.

// The binary file reader the options loader builds on its stack (vtable 0x54ae48, bin_reader): its constructor and the
// user-declared empty destructor of the in-memory stream 0x54ad68 are inlined there (stream_inline.h).
#include "engine/io/stream_inline.h"

#include "game/config/options.h"   // options (g_options 0x5aa008; load 0x4c3a40 is module_option_records.cpp)

// ---- music / sound objects (0x4c3d90..0x4c4640; s004c3d90) -------------------------------------------------------
int   __stdcall ftol2(double d);                   // 0x535a64
int   __cdecl round_float(const float* f);             // 0x432540: round to nearest
float clamp(const float* x, const float* lo, const float* hi);   // 0x40e3d0 (the definition: engine/physics/collision_shapes.h, which cannot be included next to this header)

// FMOD 3 entry points used by the sound loader.
#include "engine/audio/fmod_api.h"   // FMOD 3 (one declaration per entry point)

extern const float g_54ab2c;   // 0x54ab2c (0.0625f)
extern const float g_549d70;   // 0x549d70 (0.5f)



// a clock reading: slot into g_time[] and the start tick; elapsed() = (g_time[slot] - start) * 1e-5f
// (engine/core/timer.h's Timer: {clock index, pad, start}; elapsed() is 0x432c10)
#include "engine/core/timer.h"
typedef Timer SndTimer;




#include "game/dialog/dialog_rec.h"      // DialogRec (vdel 0x4609d0: the vector deleting destructor): the shared dialog record
struct tstr_rec { int w0, w4, w8; tstring s; };   // 0x1c bytes (the string sits at +0xc)

#include "game/dialog/dialog_mgr.h"    // DialogMgr: ~DialogMgr 0x4c4090 is defined in module_option_records.cpp




#endif

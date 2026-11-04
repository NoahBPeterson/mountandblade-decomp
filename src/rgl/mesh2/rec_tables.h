// rec_tables.h: classes of rec_tables.cpp (0x4c2c50..0x4c48d0): the module/option record classes derived
// from head_rec, the options loader, the renderer setup, the music/sound objects and the tstring array loaders.
// Merged from the slices s004c2b20, s004c3430, s004c3d90 and s004c46d0; names are provisional unless a vtable
// maps them (C4c*, in symbols/slices/rec_tables.txt). Offsets cite the function that proves them.
#ifndef MESH2_REC_TABLES_H
#define MESH2_REC_TABLES_H
#include "../../include/image_addrs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>
#include <windows.h>
#include <shlobj.h>
#include <d3d9.h>
#include "../../game/tstring.h"
#include "../rgl_math.h"
#include "../stream.h"
#include "../../include/ptr_tag.h"
#include "../../game/strat/gui_manager.h"     // g_log (log_stream)
#include "../../game/ini_file.h"               // ini_file (find 0x4bd5a0), g_game_vars
#include "./mesh2_common.h"
#include "./head_rec.h"
#include "../../game/result_window.h"
#include "../../include/render_flags.h"
#include "../../include/d3d_caps.h"
#include "../../game/game_globals.h"
#include "../../game/screen_records.h"

using rgl::vector4;

// ---- engine entry points (mesh2_common.h: engine_alloc/new/free/delete, sub_536f8f) --------------------------
void __cdecl vector_constructor_iterator(void* dst, int size, int n, void* ctor);     // 0x401800

// ---- the C4c* record classes (vtable 0x5947d8 base, own vtable stored after the base ctor) ---------------
// ResultWindow (vtable 0x595740, ctor 0x4c2c50): game/result_window.h

struct M44 { float f[16]; };
namespace rgl { struct matrix; } typedef rgl::matrix pos;                                          // include/mat4.h: the 0x40-byte frame, layout of M44

// 0x140-byte record: four 0x40-byte matrices filled from 0x40cd00, counters at +0x100/+0x104,
// a mode int at +0x110 and a clock snapshot / rate at +0x118..+0x128 (ctor 0x4c2d10).
struct C4c2d10 {
    M44    blk0;             // +0
    M44    blk1;             // +0x40
    M44    blk2;             // +0x80
    M44    blk3;             // +0xc0
    int    v100;             // +0x100
    int    v104;             // +0x104
    char   pad108[8];        // +0x108
    int    v110;             // +0x110
    int    v114;             // +0x114
    int    v118;             // +0x118
    int    v11c;             // +0x11c
    int    v120;             // +0x120
    float  f124;             // +0x124 (6.0f)
    int    v128;             // +0x128
    C4c2d10();               // 0x4c2d10
};

// PartyScreen (ex C4c3160, vtable 0x595870, the party exchange window) is declared in game/party_screen_class.h

// 0x18-byte entry of the block read by 0x4c2dd0 (tstring, two ints): config.h's clip_slot (same layout; its constructor
// 0x4be720 is out of line, the array new goes through the vector constructor iterator). rec_tables.cpp includes config.h.
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

// ---- stream slots used through raw vtable calls (0x4c2dd0, 0x4c2ea0): slot 0x54 = r_tstr, 0x4c = r_int ----
struct stream_vt { void* const vt; };
typedef void (__fastcall *vt_rec)(void* self, void* dst);
typedef void (__fastcall *vt_int)(void* self, int* dst);
inline void st_str(void* s, void* dst) { ((vt_rec*)(*(char**)s))[0x54 / 4](s, dst); }
inline void st_int(void* s, int* dst) { ((vt_rec*)(*(char**)s))[0x4c / 4](s, dst); }

#include "../../include/fx.h"   // canonical: g_ticks (0x5e2958)


// ---- options.dat loader (0x4c3a40) and GameApp::InitDeviceObjects (0x4c3510) -----------------------------------
#include "../../include/renderer.h"   // canonical: g_render.pad504 (0xa17e14)

struct font_ref { void set_device(IDirect3DDevice9* d); };   // 0x404c00 (CD3DFont::InitDeviceObjects, ret 4)

// layout-only view of the application object (GameApp : CD3DApplication)
struct GameAppView {
    char  pad0[0xa4];
    char  windowed;                 // +0xa4
    char  pada5[0xec - 0xa5];
    HWND  hwnd;                     // +0xec
    int   pad_f0;
    int   width;                    // +0xf4
    IDirect3DDevice9* device;       // +0xf8
    char  padfc[0x22c - 0xfc];
    int   height;                   // +0x22c
    char  pad230[0x348 - 0x230];
    font_ref* font;                 // +0x348
};

// the input object at 0xa17910 (apply_input_settings' argument), the device set at 0xa170c8 (init_devices), the render
// batch at 0x5fe898 (init_decls) and the resource manager at 0x5d6fa0 (load_all) are config.h's InputSet,
// render_batch.h's render_batch and rgl/core/resmgr.h's resmgr (included by rec_tables.cpp).

// g_game_vars (0xa44240) is ini_file.h's ini_file (entries of 0x2c bytes: key tstring +0, value tstring +0x1c)

// 0x4c0520 is a cdecl function without parameters (game/triggers/trigger_vec.cpp); the original calls it with ecx = the renderer object, which
// it ignores: rec_tables.cpp calls it through the fastcall type of that call
void load_game_variables();                         // 0x4c0520 (resets one buffer-size group)
// 0x4bab70 user_path (argument in ESI, a static helper of another TU) has a local copy in rec_tables.cpp.

// The binary file reader the options loader builds on its stack (vtable 0x54ae48, bin_reader): its constructor and the
// user-declared empty destructor of the in-memory stream 0x54ad68 are inlined there (stream_inline.h).
#include "../stream_inline.h"

// 0x9c bytes; the loader reads the fields in the order shown, with the file's tokens in a different order.
// (Field names/layout mirror options::save() at 0x4181c0.)
struct options_record {
    int   mode;          // +0x00
    int   w04;           // +0x04
    float f08;           // +0x08
    float f0c;           // +0x0c
    float f10;           // +0x10
    int   w14;           // +0x14
    int   w18;           // +0x18
    int   w1c;           // +0x1c
    int   w20;           // +0x20
    int   w24;           // +0x24
    int   w28;           // +0x28
    int   w2c;           // +0x2c
    int   w30;           // +0x30
    int   w34;           // +0x34
    int   w38;           // +0x38
    int   w3c;           // +0x3c
    int   w40;           // +0x40
    int   w44;           // +0x44
    int   w48;           // +0x48  must be in [0,3), else forced to 1
    int   w4c;           // +0x4c
    int   w50;           // +0x50
    int   w54;           // +0x54
    int   w58;           // +0x58
    int   w5c;           // +0x5c
    int   w60;           // +0x60
    int   w64;           // +0x64
    int   w68;           // +0x68
    int   w6c;           // +0x6c
    int   w70;           // +0x70
    float f74;           // +0x74
    int   w78;           // +0x78
    int   w7c;           // +0x7c
    float f80;           // +0x80  clamped to [1.4, 2.6], else 2.2
    float f84;           // +0x84  must be in [-0.001, 1.01), else 0.5
    float f88;           // +0x88  must be in [-0.001, 1.01), else 0.5
    int   w8c;           // +0x8c
    int   w90;           // +0x90
    int   w94;           // +0x94
    int   w98;           // +0x98
    void load();                               // 0x4c3a40
    // set_triple_buffering (0x4bc090) and set_texture_quality (0x4bc100) are Config's (config.h): this record is the
    // option block g_cfg 0x5a9f98 (mode == Config::first_time +0, w04 == Config::texture_detail +4)
};

// ---- element type of the 0x58-byte arrays (0x4c3430) ------------------------------------------------------------
#include "../../include/rec_dtors.h"                  // RecD (scalar_delete 0x431480 = ??_ERecD, ret 4), shared with rgl/xmlcfg/records.h
void __stdcall vector_destructor_iterator(void* p, unsigned size, int n, void (__fastcall* d)(void*));  // 0x401830 (d3dapp.h)
// 0x431440 is ~RecD (thiscall, the record in ecx). C++ cannot take a destructor's address, so the callback of the inlined
// vector delete (elem58::vector_deleting_delete) is declared as the equivalent fastcall function; its symbol is not
// ??1RecD (see docs of the relink report: delete[] would call ??_ERecD instead of expanding the loop).
void __fastcall RecD_dtor(void* p);                   // 0x431440

// 0x58-byte element: { tstring s0, tstring s10, tstring s24, void* p34, .., RecD* p50 }.
struct elem58 {
    tstring s0;          // +0
    tstring s10;         // +0x10
    int pad20[1];        // +0x20
    tstring s24;         // +0x24
    void* p34;           // +0x34
    char pad38[0x18];    // +0x38
    RecD* rec50;         // +0x50
    ~elem58() {}         // empty: the ctor/dtor bodies are out of line
    void* vector_deleting_delete(unsigned flags);   // 0x4c3430
};


// ---- music / sound objects (0x4c3d90..0x4c4640; s004c3d90) -------------------------------------------------------
int   __stdcall ftol2(double d);                   // 0x535a64
int   __cdecl round_float(const float* f);             // 0x432540: round to nearest
float clamp(const float* x, const float* lo, const float* hi);   // 0x40e3d0 (the definition: rgl/shapes/shapes.h, which cannot be included next to this header)

// FMOD 3 entry points used by the sound loader.
#include "../../include/fmod_api.h"   // FMOD 3 (one declaration per entry point)

extern const float g_54ab2c;   // 0x54ab2c (0.0625f)
extern const float g_549d70;   // 0x549d70 (0.5f)



// a clock reading: slot into g_time[] and the start tick; elapsed() = (g_time[slot] - start) * 1e-5f
// (include/timer.h's Timer: {clock index, pad, start}; elapsed() is 0x432c10)
#include "../../include/timer.h"
typedef Timer SndTimer;

// Allocator objects of the engine's array helpers (provisional names): construct(dst, src) is a thiscall, ret 8.
struct Alloc204 { void construct(void* dst, const void* src); };   // 0x4c1b40
struct Alloc4   { void construct(void* dst, const void* src); };   // 0x436d80
struct Alloc2c  { void construct(void* dst, const void* src); };   // 0x4c1b20

struct elem48 {
    tstring s0;          // +0
    int w10;             // +0x10
    tstring s14;         // +0x14
    char pad24[0x24];
    // initialised from the stream by prsnt_record::save (0x4bdf60, config.h): the same 0x48-byte record (two tstrings at +0 / +0x14)
};


#include "../../include/dialog_rec.h"      // DialogRec (vdel 0x4609d0: the vector deleting destructor): the shared dialog record
struct tstr_rec { int w0, w4, w8; tstring s; };   // 0x1c bytes (the string sits at +0xc)

#include "../../game/dialog_mgr.h"    // DialogMgr: ~DialogMgr 0x4c4090 is defined in rec_tables.cpp

struct music_rec {
    FSOUND_SAMPLE* sample;   // +0
    int   level;             // +4   file bits: bit0 = looped, bits 4..7 = channel class
    float ratio;             // +8   length / default frequency
    int   freq;              // +0xc default frequency
    int   length;            // +0x10 sample length
    music_rec() : sample(0), level(0), ratio(0.0f), length(0) {}    // +0xc is left alone
};

// load_used (0x4be270) is loaded_snd_table::load_used (config.h): music_set is the same table (count2 +8, names +0xc)

struct music_name {         // 0x98 bytes, only the leading tstring is constructed
    tstring s;
    char pad[0x98 - sizeof(tstring)];
    // loaded by sound_record::load (0x4be200, config.h): the same 0x98-byte record
};

struct music_set {
    int count;                 // +0  number of samples
    music_rec* recs;           // +4  [count]
    int count2;                // +8  number of names
    music_name* names;         // +0xc [count2]
    void load(stream_base* s, int load_samples, const tstring* dir);   // 0x4c4340 (thiscall, ret 0xc)
};



void* copy_uninit_4_range(char* first, char* last, char* dst, Alloc4& al); // 0x4c3ef0
void* copy_uninit_2c(char* first, char* last, char* dst, Alloc2c& al); // 0x4c3e80
void copy_uninit_4(char* dst, unsigned n, const void* arg, Alloc4& al, ptr_tag tag); // 0x4c3e10
#endif

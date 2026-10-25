// rec_tables.h: classes of rec_tables.cpp (0x4c2c50..0x4c48d0): the module/option record classes derived
// from head_rec, the options loader, the renderer setup, the music/sound objects and the tstring array loaders.
// Merged from the slices s004c2b20, s004c3430, s004c3d90 and s004c46d0; names are provisional unless a vtable
// maps them (C4c*, in symbols/slices/rec_tables.txt). Offsets cite the function that proves them.
#ifndef MESH2_REC_TABLES_H
#define MESH2_REC_TABLES_H
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
#include "../../game/strat/gui_manager.h"     // g_log (log_stream)
#include "./mesh2_common.h"
#include "./head_rec.h"

using rgl::vector4;

// ---- engine entry points (mesh2_common.h: engine_alloc/new/free/delete, sub_536f8f) --------------------------
void __cdecl vector_constructor_iterator(void* dst, int size, int n, void* ctor);     // 0x401800

// ---- the C4c* record classes (vtable 0x5947d8 base, own vtable stored after the base ctor) ---------------
// 0x4c2c50 (vtable 0x595740): tstrings +0x54/+0x68/+0x78/+0x88
struct C4c2c50 : head_rec {
    char  pad2c[0x20];       // +0x2c
    int   v4c;               // +0x4c
    int   v50;               // +0x50
    tstring t54;             // +0x54
    int   v64;               // +0x64
    tstring t68;             // +0x68
    tstring t78;             // +0x78
    tstring t88;             // +0x88
    C4c2c50();               // 0x4c2c50
    virtual void vfunc();
    __declspec(noinline) void dtor();   // 0x4c2cd0 (called, not inlined, by the deleting dtor 0x4c2cb0)
};

struct M44 { float f[16]; };
M44* identity_pos(M44* out);                         // 0x40cd00 (fills `out`, returns it)

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

struct C4c2fb0 : head_rec {  // vtable 0x595798
    char  pad2c[0x1c];       // +0x2c
    tstring t48;             // +0x48
    char    t58[0x20];       // +0x58, +0x68: two tstrings (built/destroyed through one pointer)
    C4c2fb0();               // 0x4c2fb0
    virtual void vfunc();
    __declspec(noinline) void dtor();   // 0x4c3020 (called, not inlined, by the deleting dtor 0x4c3000)
};

struct C4c3050 : head_rec {  // vtable 0x5957bc
    char  pad2c[0x24];       // +0x2c
    int   v50;               // +0x50 (set to -1)
    int   v54;               // +0x54
    int   v58;               // +0x58
    tstring t5c;             // +0x5c
    char    t6c[0x60];       // +0x6c (6 tstrings, 0x10 apart; built/destroyed by hand in the ctor/dtor)
    C4c3050();               // 0x4c3050
    virtual void vfunc();
    void dtor();             // 0x4c30e0
};

struct Slot20 {              // 0x20 bytes
    int a, b, c, d, e, f, g, h;
};
struct C4c3160 : head_rec {  // vtable 0x595870
    char   body[0x8e8 - 0x2c];
    Slot20 slots[0x80];      // +0x8e8
    C4c3160();               // 0x4c3160
    virtual void vfunc();
};

struct C4c31c0 : head_rec {  // vtable 0x595924
    char    body[0x224 - 0x2c];  // +0x2c: 9 pairs of tstrings, 0x30 apart (built/destroyed by hand)
    char    t224[0x10];      // +0x224 tstring
    C4c31c0();               // 0x4c31c0
    virtual void vfunc();
    void dtor();             // 0x4c3240
};

struct C4c3290 : head_rec {  // vtable 0x595a20
    char  pad2c[0x24];       // +0x2c
    tstring t50;             // +0x50
    char    t60[0x20];       // +0x60, +0x70: two tstrings (built/destroyed through one pointer)
    C4c3290();               // 0x4c3290
    virtual void vfunc();
    __declspec(noinline) void dtor();   // 0x4c3300 (called, not inlined, by the deleting dtor 0x4c32e0)
};

// 0x18-byte entry of the block read by 0x4c2dd0 (tstring, two ints).
struct Entry18 {
    tstring s0;              // +0
    int  v10;                // +0x10
    int  v14;                // +0x14
    Entry18();               // 0x4be720 (out of line: array new goes through the vector constructor iterator)
};
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

// A half-initialised tstring used by 0x4c2ea0 / 0x4c3350 (vtable pointer 0x54a740, restored to 0x54ad68 on exit).
struct tstr_ll {
    void*      typ;     // +0
    tstring    s;       // +4
    tstr_ll() : typ((void*)0x54a740) { }
    tstr_ll(const char* c) : typ((void*)0x54a740), s(c) { }                // 0x42bbe0
    tstr_ll(const tstring& o) : typ((void*)0x54a740), s(o) { }             // 0x42c110
    ~tstr_ll() { typ = (void*)0x54ad68; }
    const char* text() const { return s.buf ? s.buf : (const char*)&s.buf; }
};

#include "../../include/fx.h"   // canonical: g_ticks (0x5e2958)


// ---- options.dat loader (0x4c3a40) and GameApp::InitDeviceObjects (0x4c3510) -----------------------------------
#include "../../include/renderer.h"   // canonical: g_render.pad504 (0xa17e14)
extern D3DCAPS9 g_caps;                            // 0xa17f2c (GetDeviceCaps fills it here)
extern int   g_renderer_width;                     // 0xa1805c
extern int   g_renderer_height;                    // 0xa18060
extern int   g_renderer_vsync;                     // 0xa180e4
extern int   g_use_shaders;                        // 0x5e28fc (also g_dinput_enabled)
extern int   g_shader_support;                     // 0x5a9fcc "use_vertex_shaders"

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

struct InputGlobal;                                // the object at 0xa17910 (apply_input_settings' argument)
void __fastcall apply_input_settings(InputGlobal* self);   // 0x4bc350
struct InputDevices {                                      // the object at 0xa170c8
    int init_devices(HWND h);                              // 0x4bc430 (thiscall, ret 4)
};
struct RenderSetup {                                       // the object at 0x5fe898
    int init_decls(const int& a0, const int& a1, const int& a2, const int& a3, const int& a4, const int& a5, const int& a6, const int& a7, const int& a8, const int& a9);   // 0x4bc830 (thiscall, ret 0x28)
};
struct ResMgrX {                                           // the object at 0x5d6fa0
    void load_all(int flag, tstring* prefix);       // 0x41c540 (thiscall, ret 8)
};
extern tstring g_module_path;                              // 0x8ee3c28

// the ini table at 0xa44240 (0x2c-byte entries: key tstring +0, value tstring +0x1c); get_float is inline (header)
struct ini_entry_x { tstring key; char pad10[0xc]; tstring value; };
struct ini_file_x {
    ini_entry_x entries[0x1000];                           // +0
    int count;                                             // +0x2c000
    int find(const tstring& key);                          // 0x4bd5a0 (thiscall, ret 4)
    float get_float(const tstring& key)
    {
        float ret = 0.0f;
        int idx = find(key);
        if (idx >= 0)
            ret = (float)atof(entries[idx].value.c_str());
        return ret;
    }
};
extern ini_file_x g_game_vars;                                   // 0xa44240

void __fastcall load_game_variables(void* self);    // 0x4c0520 (resets one buffer-size group)
// 0x4bab70 user_path (argument in ESI, a static helper of another TU) has a local copy in rec_tables.cpp.

// The binary file reader the options loader builds on its stack (vtable 0x54ae48 = bin_reader): the same inline chain
// as rgl/core/resmgr.h (inline ctors, user-declared empty destructor of the in-memory stream 0x54ad68).
struct opt_mem_stream : stream_base {
    STREAM_SLOTS
    __forceinline opt_mem_stream() {}
    ~opt_mem_stream() {}
};
struct opt_reader_b : opt_mem_stream {
    FILE* fp;                                  // +0x30
    char failed;                               // +0x34
    __forceinline opt_reader_b() { failed = 1; fp = 0; state = 0; }
};
struct opt_reader : opt_reader_b {             // vtable 0x54ae48
    __forceinline opt_reader() {}
    STREAM_SLOTS
};

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
    void set_triple_buffering(int mode);                   // 0x4bc090 (thiscall, ret 4)
    void set_texture_quality(int v);                   // 0x4bc100 (thiscall, ret 4)
};

// ---- element type of the 0x58-byte arrays (0x4c3430) ------------------------------------------------------------
#include "../../include/rec_dtors.h"                  // RecD (scalar_delete 0x431480 = ??_ERecD, ret 4), shared with rgl/xmlcfg/records.h
void __stdcall vector_destructor_iterator(void* p, unsigned size, int n, void (__stdcall* d)(void* const*));  // 0x401830
void __stdcall RecD_dtor(void* const* p);             // 0x431440 (the pointer is in ecx: (void*, unused) call)

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
float __cdecl clamp(const float& v, const float& lo, const float& hi);  // 0x40e3d0

// FMOD 3 entry points used by the sound loader.
struct FSOUND_SAMPLE;
FSOUND_SAMPLE* __stdcall FSOUND_Sample_Load(int index, const char* name, unsigned flags, int offset, int length); // 0x534330 (ret 20)
void  __stdcall FSOUND_SetVolume(int channel, int volume);                                                       // 0x5342ca (ret 8)
int   __stdcall FSOUND_Sample_SetDefaults(FSOUND_SAMPLE* s, int freq, int vol, int pan, int pri);                 // 0x53432a (ret 20)
int   __stdcall FSOUND_Sample_GetLength(FSOUND_SAMPLE* s);                                                         // 0x534324 (ret 4)
int   __stdcall FSOUND_Sample_GetDefaults(FSOUND_SAMPLE* s, int* freq, int* vol, int* pan, int* pri);              // 0x53431e (ret 20)

extern const float g_54ab2c;   // 0x54ab2c (0.0625f)
extern const float g_590b60;   // 0x590b60 (250.0f)
extern const float g_549d70;   // 0x549d70 (0.5f)
extern int    g_sound_paused;   // 0x5a9fd4
extern int    g_sound_muted;    // 0x5a9fd8
extern float  g_music_volume;   // 0x5aa010

// a clock reading: slot into g_time[] and the start tick; elapsed() = (g_time[slot] - start) * 1e-5f
struct SndTimer {
    int     slot;       // +0
    int     pad4;
    __int64 start;      // +8
    float elapsed();    // 0x432c10 (thiscall)
};

// the "sound manager" tail (0x2474-byte object, layout-only view)
struct sound_engine {
    int   w0;                       // +0
    char  pad4[0x240c - 4];
    int   w240c;                    // +0x240c  fade-out flag
    char  pad2410[0x2418 - 0x2410];
    SndTimer t2418;                 // +0x2418  last advance of the current piece
    char  pad2428[0x2438 - 0x2428];
    int   cur_index;                // +0x2438  channel driven so far (or -1)
    int   channel;                  // +0x243c  channel of the current piece
    int   last_volume;              // +0x2440
    char  pad2444[0x2450 - 0x2444];
    SndTimer t2450;                 // +0x2450  start of the new piece
    SndTimer t2460;                 // +0x2460  fade end
    int   fading;                   // +0x2470
    void __fastcall advance_piece();               // 0x4c25e0
    void stop(int idx);                      // 0x4be0a0 (thiscall, ret 4)
    void __fastcall tick();                        // 0x4c4100
};

// Allocator objects of the engine's array helpers (provisional names): construct(dst, src) is a thiscall, ret 8.
struct Alloc204 { void construct(void* dst, const void* src); };   // 0x4c1b40
struct Alloc4   { void construct(void* dst, const void* src); };   // 0x436d80
struct Alloc2c  { void construct(void* dst, const void* src); };   // 0x4c1b20

struct elem48 {
    tstring s0;          // +0
    int w10;             // +0x10
    tstring s14;         // +0x14
    char pad24[0x24];
    void save(stream_base* s);   // 0x4bdf60 (thiscall, ret 4)
};
struct elem48_holder {
    elem48* arr;     // +0 (array new, count in front)
    int count;       // +4
};


#include "../../include/dialog_rec.h"      // DialogRec (vdel 0x4609d0: the vector deleting destructor): the shared dialog record
struct tstr_rec { int w0, w4, w8; tstring s; };   // 0x1c bytes (the string sits at +0xc)

struct scenefile_obj {
    int  w000;
    DialogRec* w004;         // +4   (freed first, moving to ecx for the call)
    char pad8[0x24 - 8];
    tstr_rec ts_a[0x400];    // +0x24
    tstr_rec ts_b[0x400];    // +0x7024
    char pad_e024[4];
    tstring s_e028;          // +0xe028
    ~scenefile_obj();
};

struct music_rec {
    FSOUND_SAMPLE* sample;   // +0
    int   level;             // +4   file bits: bit0 = looped, bits 4..7 = channel class
    float ratio;             // +8   length / default frequency
    int   freq;              // +0xc default frequency
    int   length;            // +0x10 sample length
    music_rec() : sample(0), level(0), ratio(0.0f), length(0) {}    // +0xc is left alone
};

void __fastcall load_used(struct music_set* m);                     // 0x4be270

struct music_name {         // 0x98 bytes, only the leading tstring is constructed
    tstring s;
    char pad[0x98 - sizeof(tstring)];
    void load(stream_base& s);   // 0x4be200 (thiscall, ret 4)
};

struct music_set {
    int count;                 // +0  number of samples
    music_rec* recs;           // +4  [count]
    int count2;                // +8  number of names
    music_name* names;         // +0xc [count2]
    void load(stream_base* s, int load_samples, const tstring* dir);   // 0x4c4340 (thiscall, ret 0xc)
};

struct tstring_pair {
    tstring a;       // +0
    tstring b;       // +0x10
};

// the object with two pair arrays: arrays at +0 / +4, counts at +8 / +0xc (destroyed by 0x4c4640)
struct pair_holder {
    tstring_pair* arr1;   // +0
    tstring_pair* arr2;   // +4
    int count1;           // +8
    int count2;           // +0xc
    void load1(stream_base* s);   // 0x4c46d0 (thiscall, ret 4)
    void load2(stream_base* s);   // 0x4c47d0 (thiscall, ret 4)
};


void* copy_uninit_4_range(char* first, char* last, char* dst, Alloc4& al); // 0x4c3ef0
void* copy_uninit_2c(char* first, char* last, char* dst, Alloc2c& al); // 0x4c3e80
void copy_uninit_4(char* dst, unsigned n, const void* arg, Alloc4& al); // 0x4c3e10
#endif

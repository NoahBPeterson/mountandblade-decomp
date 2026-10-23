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
void __cdecl alloc_ctor(void* dst, int size, int n, void* ctor);     // 0x401800

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
};
// Block with tstrings at +0/+0x10 and an entry vector at +0x18 (0x4c2dd0).
struct Blk2dd0 {
    tstring  s0;             // +0
    tstring  s10;            // +0x10
    int      count;          // +0x18
    Entry18* entries;        // +0x1c
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

extern unsigned char g_5d8789;        // 0x5d8789 (g_ui_dirty)
extern int    g_5e2958;               // 0x5e2958  clock snapshot source
extern int    g_5e295c;               // 0x5e295c


// ---- options.dat loader (0x4c3a40) and renderer setup (0x4c3510) ----------------------------------------------
extern IDirect3DDevice9* g_d3d_device;             // 0xa17f28 ("current device")
extern D3DCAPS9 g_caps;                            // 0xa17f2c (GetDeviceCaps fills it here)
extern int   g_renderer_width;                     // 0xa1805c
extern int   g_renderer_height;                    // 0xa18060
extern int   g_renderer_vsync;                     // 0xa180e4
extern char  g_renderer_flag;                      // 0xa17e14
extern int   g_use_shaders;                        // 0x5e28fc
extern int   g_shader_support;                     // 0x5a9fcc "use_vertex_shaders"

struct renderer;
struct font_ref { void set_device(IDirect3DDevice9* d); };   // 0x404c00 (ret 4): the renderer's font object
extern renderer g_render;                          // 0xa17910  ?g_render@@3Urenderer@@A
void __fastcall render_init(renderer* r);          // 0x4bc350 (resets the shader state, creates buffers)
void __fastcall create_shader_constants(renderer* r, int a1, int a2, int a3, int a4, int a5,
                                        int a6, int a7, int a8, int a9, int a10);   // 0x4bc830 (by value)
void __fastcall set_render_buffer_sizes(renderer* r, int a);  // 0x4bc430
extern char g_setup_obj;                           // 0x5d6fa0
void __fastcall setup_obj_init(char* self, int a, void* b);  // 0x41c540

// config values: a tstring-keyed table of 0x2c-byte entries at 0xa44240
extern char g_config;                              // 0xa44240
int __fastcall config_find(char* t, tstring* key); // 0x4bd5a0
#define CFG_ENTRY(idx) (*(const char**)(0xa4425c + (idx) * 0x2c) != 0 ? *(const char**)(0xa4425c + (idx) * 0x2c) : "")

void __fastcall reset_buffer_group(void* self);    // 0x4c0520 (resets one buffer-size group)
// 0x4bab70: rewrites a file name to its full %APPDATA%\Mount&Blade path (helper of another TU).
void user_path(char* name);                        // 0x4bab70

// buffer-size option names, in the order 0x4c3510 reads them (each is copied into the stack tstring)
extern const char g_opt_dx7_regular_ffp_dynamic[]; // 0x595b68
extern const char g_opt_dx7_regular_ffp_static[];  // 0x595b44
extern const char g_opt_normal_map_dynamic[];      // 0x595b24
extern const char g_opt_normal_map_static[];       // 0x595b04
extern const char g_opt_skinning_dynamic[];        // 0x595ae4
extern const char g_opt_skinning_static[];         // 0x595ac8
extern const char g_opt_regular_dynamic[];         // 0x595aac
extern const char g_opt_regular_static[];          // 0x595a90
extern const char g_opt_regular_ffp_dynamic[];     // 0x595a70
extern const char g_opt_regular_ffp_static[];      // 0x595a50

// The stream the options loader builds on its stack (vtable 0x54ae48 = bin_reader, inline ctor; see 0x4c3a40).
// The original never destroys it: with src/rgl/stream.h's members (rgl::string name, buf) the implicit destructor
// would add an unwind frame the loader does not have, so the loader uses this trivially destructible view
// (same layout: name +0x4, state +0x14, buf +0x20, fp +0x30, failed +0x34).
struct options_stream {
    virtual void reset();
    virtual void rewind();
    virtual void* slot02(); virtual void* slot03(); virtual void* slot04(); virtual void* slot05();
    virtual void* slot06(); virtual void* slot07(); virtual void* slot08(); virtual void* slot09();
    virtual void* slot10(); virtual void* slot11(); virtual void* slot12(); virtual void* slot13();
    virtual void* r_float(float* v);                 // slot 14 (+0x38)
    virtual void* slot15(); virtual void* slot16(); virtual void* slot17();
    virtual void* slot18(); virtual void* r_int(int* v);   // slot 19 (+0x4c)
    virtual void* slot20(); virtual void* slot21();
    virtual void* slot22();
    char name_raw[0x10];                       // +0x04 tstring
    int state;                                 // +0x14
    char autoflush;                            // +0x18
    int flushed;                               // +0x1c
    char buf_raw[0x10];                        // +0x20 tstring
    FILE* fp;                                  // +0x30
    volatile char failed;                      // +0x34
    char is_open;                              // +0x35
    options_stream() {
        CONSTRUCT_TSTRING(name_raw);
        autoflush = 0; flushed = 0;
        CONSTRUCT_TSTRING(buf_raw);
        fp = 0; state = 0; failed = 1;
    }
    bool open_read(const char* path);          // 0x415290
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
    float f84;           // +0x84  clamped to [0.5, 1.01], else 0.5
    float f88;           // +0x88  clamped to [0.5, 1.01], else 0.5
    int   w8c;           // +0x8c
    int   w90;           // +0x90
    int   w94;           // +0x94
    int   w98;           // +0x98
    void load();                               // 0x4c3a40
};
int __fastcall options_set_mode(options_record* o, int mode);  // 0x4bc090 (ret 4)
int __fastcall options_set_spacing(options_record* o, int v);  // 0x4bc100 (ret 4)

// ---- element type of the 0x58-byte arrays (0x4c3430) ------------------------------------------------------------
struct RecD { void* scalar_delete(unsigned flags); };  // 0x431480 (??_ERecD, ret 4)
void __stdcall RecD_free_array(void* p, unsigned size, int n, void (__stdcall* d)(void* const*));  // 0x401830
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
float __fastcall elapsed_since(const __int64* t);  // 0x432c10: (now - *t) * 1e-5f
int   __fastcall round_f(const float* f);          // 0x432540: (int)(*f + 0.5f)
float __cdecl clamp_float3(const float* v, const float* lo, const float* hi);  // 0x40e3d0

// FMOD 3 entry points used by the sound loader.
struct FSOUND_SAMPLE;
FSOUND_SAMPLE* __stdcall FSOUND_Sample_Load(int index, const char* name, unsigned flags, int offset, int length); // 0x534330 (ret 20)
void  __stdcall _FSOUND_SetVolume(int channel, int volume);                                                       // 0x5342ca (ret 8)

extern int    g_sound_paused;   // 0x5a9fd4
extern int    g_sound_muted;    // 0x5a9fd8
extern float  g_music_volume;   // 0x5aa010

// the "sound manager" tail (0x2400-byte object)
struct sound_engine {
    int   w240c;            // +0x240c  fade-out flag
    int   cur_index;        // +0x2438  channel driven so far (or -1)
    int   channel;          // +0x243c  channel of the current piece
    int   last_volume;      // +0x2440
    int   slot;             // +0x2418  tick slot of the current piece
    __int64 start_tick;     // +0x2420
    int   w2450;            // +0x2450  tick slot of the new piece
    __int64 w2458;          // +0x2458  its start (8 bytes)
    __int64 w2460;          // +0x2460  fade end (read by elapsed_since)
    int   fading;           // +0x2470
    void __fastcall advance_piece();               // 0x4c25e0
    void __fastcall stop_piece(int idx);           // 0x4be0a0 (ret 4)
    void __fastcall tick();                        // 0x4c4100
};

// Array helpers (engine template members, provisional names). Signature recovered from 0x4c3d90: the call site
// pushes dst then arg and calls with `this` = ebx = 4th arg, but the callee is `ret 8`, so the argument order below
// is what the call site passes.
void __stdcall construct_204(void* dst, const void* arg);   // 0x4c1b40 (ret 8, this in ecx, may throw)
void __stdcall construct_4(void* dst, const void* arg);     // 0x436d80 (ret 8, this in ecx, may throw)
void __stdcall construct_2c(void* dst, const void* arg);    // 0x4c1b20 (ret 8, this in ecx, may throw)

struct elem48 {
    tstring s0;          // +0
    tstring s14;         // +0x14
    char pad28[0x20];
};

void __stdcall elem48_init(elem48* e, void* arg);   // 0x4bdf60 (ret 4, this in ecx)
void __fastcall free_vec_4609d0(void* p, unsigned flags);   // 0x4609d0 (ret 4): vector-like delete of [this]
struct tstr_rec { tstring s; int w10, w14, w18; };   // 0x1c bytes

struct scenefile_obj {
    int* w001;
    int* w004;               // +4   (freed first, moving to ecx for the call)
    char pad8[0xe028 - 0xc];
    tstring s_e028;          // +0xe028
    tstr_rec ts2[0x400];     // +0xe050
    tstr_rec ts3[0x400];     // +0x15050
    tstr_rec ts4[0x400];     // +0x1c050
    int w1c02c;              // +0x1c02c
    tstring s_e03c;          // +0xe03c   (second live string, placed last so it can hold a callee-saved reg)
    ~scenefile_obj();
};

struct music_rec {
    FSOUND_SAMPLE* sample;   // +0
    int   level;             // +4
    float ratio;             // +8
    int   length;            // +0xc
    int   channels;          // +0x10
};

void __stdcall music_entry_read(music_rec* rec, stream_base& s);   // 0x4be200 (ret 4): tstring name, int, ints
void __fastcall music_set_finish(void* m);                         // 0x4be270
struct music_set {
    int count;                 // +0  number of samples
    music_rec* recs;           // +4  [count]
    int count2;                // +8  number of names
    tstring* names;            // +0xc [count2]
    void __fastcall load(stream_base& s); // 0x4c4340 (ret 0xc)
};

struct tstring_pair {
    tstring a;       // +0
    tstring b;       // +0x10
};


#endif

// Slice s004c3430: the options/config file (options.dat) - the loaded record's array element type, the
// renderer setup that reads the buffer-size options, and the options loader.
//
// The loader (0x4c3a40) builds a text_stream on the stack, reads an options record field by field through
// the stream's virtual slots (r_int = +0x4c, r_float = +0x38, r_tstr = +0x54) and copies it into *this.
// The 0x54a740 stream hierarchy declared here differs from ../s00414960/mbstream.h in one way only: it has
// no implicit destructor, so the loader's text_stream (which the original never destroys) stays trivial and
// the EH frame of the loader does not grow an unwind handler. Stack offsets are the loader's own ordering.
// Names are provisional.
#ifndef MB_STREAM_LOCAL_H
#define MB_STREAM_LOCAL_H
#include <stdio.h>
#include <stddef.h>
#endif
#include <stdlib.h>
#include <windows.h>
#include <shlobj.h>
#include <d3d9.h>

struct tstring {
    int pos;            // +0  read cursor
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring();                                 // 0x42bbd0
    tstring(const char* s);                    // 0x42bbe0
    ~tstring();                                // 0x42bc40
    void assign(const char* s);                // 0x42bcb0
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

struct stream_base {
    virtual void reset();                      // slot 0   0x415220
    virtual void rewind();                     // slot 1   0x415240
    virtual stream_base& w_stream(void* o) = 0;            // 2
    virtual stream_base& w_float(float* v) = 0;            // 3
    virtual stream_base& w_bool(char* v) = 0;              // 4
    virtual stream_base& w_u64(unsigned __int64* v) = 0;   // 5
    virtual stream_base& w_i64(__int64* v) = 0;            // 6
    virtual stream_base& w_uint(unsigned* v) = 0;          // 7
    virtual stream_base& w_int(int* v) = 0;                // 8
    virtual stream_base& w_byte(unsigned char* v) = 0;     // 9
    virtual stream_base& w_cstr(const char* s) = 0;        // 10
    virtual stream_base& w_tstr(const tstring* s) = 0;     // 11
    virtual stream_base& slot12(void* v) = 0;              // 12
    virtual stream_base& r_stream(void* o) = 0;            // 13
    virtual stream_base& r_float(float* v) = 0;            // 14  0x415c80
    virtual stream_base& r_bool(char* v) = 0;              // 15
    virtual stream_base& r_uint(unsigned* v) = 0;          // 16
    virtual stream_base& r_u64(unsigned __int64* v) = 0;   // 17
    virtual stream_base& r_i64(__int64* v) = 0;            // 18
    virtual stream_base& r_int(int* v) = 0;                // 19  0x415930
    virtual stream_base& r_byte(char* v) = 0;              // 20
    virtual stream_base& r_tstr(tstring* v) = 0;           // 21  0x415880
    virtual stream_base& slot22(void* v) = 0;              // 22
};

// ---- engine objects referenced by the renderer setup ----------------------------------------------
extern IDirect3DDevice9* g_d3d_device;             // 0xa17f28 ("current device")
extern D3DCAPS9 g_caps;                            // 0xa17f2c (GetDeviceCaps fills it here)
extern int   g_renderer_width;                     // 0xa1805c
extern int   g_renderer_height;                    // 0xa18060
extern int   g_renderer_vsync;                     // 0xa180e4
extern char  g_renderer_flag;                      // 0xa17e14
extern int   g_use_shaders;                        // 0x5e28fc
extern int   g_shader_support;                     // 0x5a9fcc "use_vertex_shaders"

struct renderer;
extern renderer g_render;                          // 0xa17910  ?g_render@@3Urenderer@@A
void __fastcall render_init(renderer* r);          // 0x4bc350 (resets the shader state, creates buffers)
void __fastcall create_shader_constants(renderer* r, int a1, int a2, int a3, int a4, int a5,
                                        int a6, int a7, int a8, int a9, int a10);   // 0x4bc830 (by value)
void __fastcall set_render_buffer_sizes(renderer* r, int a);  // 0x4bc430
extern char g_setup_obj;                           // 0x5d6fa0
void __fastcall setup_obj_init(char* self, int a, void* b);  // 0x41c540

// config values: a tstring-keyed table of 0x2c-byte entries at 0xa44240
extern char g_config;                              // 0xa44240
// Value of the first field of an entry, as a C string: a C string at +4, else an inline tstring's buffer at
// +0xc, else `""`.
const char* __fastcall config_value(char* t, tstring* key); // 0x4bd5a0 (thiscall, ret 4) + value fetch
int __fastcall config_find(char* t, tstring* key); // 0x4bd5a0
#define CFG_ENTRY(idx) (*(const char**)(0xa4425c + (idx) * 0x2c) != 0 ? *(const char**)(0xa4425c + (idx) * 0x2c) : "")
double __fastcall config_number(const char* key);  // 0x4bd450 (NaN when the key is missing)

void __fastcall reset_buffer_group(void* self);    // 0x4c0520 (resets one buffer-size group)
extern "C" int _sprintf(char*, const char*, ...);  // 0x536f8f
// 0x4bab70: rewrites a file name to its full %APPDATA%\Mount&Blade path (helper of another TU).
void user_path(char* name);                        // 0x4bab70
void __fastcall __user_path_probe(char* name);     // see the note on 0x4c3a40
extern "C" double atof(const char*);               // 0x537352
extern "C" int atoi(const char*);                  // 0x5373d0

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

struct D3DVec4 { float x, y, z, w; };
void __fastcall set_alpha_blend(D3DVec4* v);       // 0x404c00

// text log stream at 0x5a9f60 (see s00418a70 / s00449c60): slot 10 (+0x28) is w_cstr
struct log_stream {
    virtual void reset();
    virtual void rewind();
    virtual void* s2();
    virtual void* s3();
    virtual void* s4();
    virtual void* s5();
    virtual void* s6();
    virtual void* s7();
    virtual void* s8();
    virtual void* s9();
    virtual void* w_cstr(const char* s);
};
extern log_stream g_log;                           // 0x5a9f60

// A text_stream as used by the loader: built on the stack with its concrete vtable (0x54a7d0) read from a
// global; it reads a file and the fields through the virtual slots above. The loader's copy keeps the name in
// a tstring of its own (two of them on the stack: +0x10 is the reader's buffer of the base class at +0x20).
struct options_stream : stream_base {
    tstring name_str;                          // +0x10 (see 0x4c3a40: constructed before the fields are set)
    tstring fmt_str;                           // +0x2c (holds the "%s%s" path format)
    FILE* fp;                                  // +0x30
    volatile char failed;                      // +0x34
    char is_open;                              // +0x35
    virtual stream_base& w_stream(void* o);
    virtual stream_base& w_float(float* v);
    virtual stream_base& w_bool(char* v);
    virtual stream_base& w_u64(unsigned __int64* v);
    virtual stream_base& w_i64(__int64* v);
    virtual stream_base& w_uint(unsigned* v);
    virtual stream_base& w_int(int* v);
    virtual stream_base& w_byte(unsigned char* v);
    virtual stream_base& w_cstr(const char* s);
    virtual stream_base& w_tstr(const tstring* s);
    virtual stream_base& slot12(void* v);
    virtual stream_base& r_stream(void* o);
    virtual stream_base& r_float(float* v);
    virtual stream_base& r_bool(char* v);
    virtual stream_base& r_uint(unsigned* v);
    virtual stream_base& r_u64(unsigned __int64* v);
    virtual stream_base& r_i64(__int64* v);
    virtual stream_base& r_int(int* v);
    virtual stream_base& r_byte(char* v);
    virtual stream_base& r_tstr(tstring* v);
    virtual stream_base& slot22(void* v);
    options_stream* open_read(const char* path);   // 0x415290
};

// ---- the loaded record ---------------------------------------------------------------------------
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
    void load(stream_base& s);                 // 0x4c3a40
};
int __fastcall options_set_mode(options_record* o, int mode);  // 0x4bc090 (ret 4)
int __fastcall options_set_spacing(options_record* o, int v);  // 0x4bc100 (ret 4)

// ---- element type of the 0x58-byte arrays --------------------------------------------------------
struct RecD { void* scalar_delete(unsigned flags); };  // 0x431480 (??_ERecD, ret 4)
void __stdcall RecD_free_array(void* p, unsigned size, int n, void (__stdcall* d)(void* const*));  // 0x401830
void __stdcall RecD_dtor(void* const* p);             // 0x431440 (the pointer is in ecx: (void*, unused) call)

// @ 0x4c3430
// Array `delete[]` for 0x58-byte elements: { tstring s0, tstring s10, tstring s24, void* p34, .., RecD* p50 }.
struct elem58 {
    tstring s0;          // +0
    tstring s10;         // +0x10
    int pad20[1];        // +0x20
    tstring s24;         // +0x24
    void* p34;           // +0x34
    char pad38[0x18];    // +0x38
    RecD* rec50;         // +0x50
    ~elem58() {}         // empty: the ctor/dtor bodies are out of line
    void* vector_deleting_delete(unsigned flags);
};

void* elem58::vector_deleting_delete(unsigned flags)
{
    if (flags & 2) {
        int n = *(int*)((char*)this - 4);
        char* base = (char*)this - 4;
        elem58* cur = (elem58*)((char*)this + n * 0x58);
        while (--n >= 0) {
            cur = (elem58*)((char*)cur - 0x58);
            if (*(void**)((char*)cur + 0x34) != 0)
                free(*(void**)((char*)cur + 0x34));
            RecD* r = *(RecD**)((char*)cur + 0x50);
            if (r != 0) {
                RecD_free_array(r, 0x78, *(int*)((char*)r - 4), RecD_dtor);
                free((char*)r - 4);
            }
            cur->s24.~tstring();
            cur->s10.~tstring();
            cur->s0.~tstring();
        }
        if (flags & 1)
            free(base);
        return base;
    }
    if (*(void**)((char*)this + 0x34) != 0)
        free(*(void**)((char*)this + 0x34));
    if (*(void**)((char*)this + 0x50) != 0)
        (*(RecD**)((char*)this + 0x50))->scalar_delete(3);
    ((elem58*)this)->s24.~tstring();
    ((elem58*)this)->s10.~tstring();
    ((elem58*)this)->s0.~tstring();
    if (flags & 1)
        operator delete(this);
    return this;
}
// @ 0x4c3510
// Renderer setup: hands the font object the device, publishes the window size, queries the caps, decides
// whether shaders can be used, then reads the ten buffer-size options into create_shader_constants.
//
// Complete but NOT byte-exact: the original frame ([esp+8] arguments, a 0xc8-byte sub, `this` in esi, the
// 0x348 offset in ecx and the device kept in [esp+0x10]) shows this function was built into an object without
// /Gy (identical code in a non-COMDAT section) and with different inlining decisions, so cl 13.10 under this
// slice's flags cannot reproduce it. See nonmatching.txt.
void __fastcall renderer_setup(void* self)
{
    IDirect3DDevice9* device = *(IDirect3DDevice9**)((char*)self + 0xf8);
    void* font = *(void**)((char*)self + 0x348);
    BOOL software_vertex_processing = *(char*)((char*)self + 0xa4);
    int height = *(int*)((char*)self + 0x22c);
    float dflt = 0.0f;
    printf("%f", *(const double*)g_opt_dx7_regular_ffp_dynamic);
    if (software_vertex_processing)
        *(int*)((char*)self + 0x22c) = 0x4c357a;
    g_d3d_device = device;
    g_renderer_width = *(int*)((char*)self + 0xf4);
    g_renderer_height = *(int*)((char*)self + 0x22c);
    g_d3d_device->GetDeviceCaps(&g_caps);
    g_renderer_vsync = -1;
    g_use_shaders = 0;
    if (g_caps.PixelShaderVersion >= 0xfffe0200 && g_caps.VertexShaderVersion >= 0xffff0200 &&
        g_caps.MaxTextureBlendStages >= 0x10 && (g_caps.DevCaps & 1) != 0 && g_shader_support != 0)
        g_use_shaders = 1;
    g_renderer_flag = 0;
    render_init(&g_render);
    {
        tstring key1(g_opt_dx7_regular_ffp_dynamic);
        reset_buffer_group(self);
        int idx1 = config_find(&g_config, &key1);
        int dx7_regular_ffp_dynamic = (int)(idx1 >= 0 ? atof(CFG_ENTRY(idx1)) : (double)dflt);
        tstring key2(g_opt_dx7_regular_ffp_static);
        int idx2 = config_find(&g_config, &key2);
        int dx7_regular_ffp_static = (int)(idx2 >= 0 ? atof(CFG_ENTRY(idx2)) : (double)dflt);
        tstring key3(g_opt_normal_map_dynamic);
        int idx3 = config_find(&g_config, &key3);
        int normal_map_dynamic = (int)(idx3 >= 0 ? atof(CFG_ENTRY(idx3)) : (double)dflt);
        tstring key4(g_opt_normal_map_static);
        int idx4 = config_find(&g_config, &key4);
        int normal_map_static = (int)(idx4 >= 0 ? atof(CFG_ENTRY(idx4)) : (double)dflt);
        tstring key5(g_opt_skinning_dynamic);
        int idx5 = config_find(&g_config, &key5);
        int skinning_dynamic = (int)(idx5 >= 0 ? atof(CFG_ENTRY(idx5)) : (double)dflt);
        tstring key6(g_opt_skinning_static);
        int idx6 = config_find(&g_config, &key6);
        int skinning_static = (int)(idx6 >= 0 ? atof(CFG_ENTRY(idx6)) : (double)dflt);
        tstring key7(g_opt_regular_dynamic);
        int idx7 = config_find(&g_config, &key7);
        int regular_dynamic = (int)(idx7 >= 0 ? atof(CFG_ENTRY(idx7)) : (double)dflt);
        tstring key8(g_opt_regular_static);
        int idx8 = config_find(&g_config, &key8);
        int regular_static = (int)(idx8 >= 0 ? atof(CFG_ENTRY(idx8)) : (double)dflt);
        tstring key9(g_opt_regular_ffp_dynamic);
        int idx9 = config_find(&g_config, &key9);
        int regular_ffp_dynamic = (int)(idx9 >= 0 ? atof(CFG_ENTRY(idx9)) : (double)dflt);
        tstring key10(g_opt_regular_ffp_static);
        int idx10 = config_find(&g_config, &key10);
        int regular_ffp_static = (int)(idx10 >= 0 ? atof(CFG_ENTRY(idx10)) : (double)dflt);
        create_shader_constants(&g_render,
                                regular_ffp_static, regular_ffp_dynamic,
                                regular_static, regular_dynamic,
                                skinning_static, skinning_dynamic,
                                normal_map_static, normal_map_dynamic,
                                dx7_regular_ffp_static, dx7_regular_ffp_dynamic);
        setup_obj_init(&g_setup_obj, 1, (void*)0x8ee3c28);
        set_render_buffer_sizes(&g_render, *(int*)((char*)self + 0xec));
    }
}

// @ 0x4c3a40
// Reads options.dat (in the user folder) into the record.
//
// Complete but NOT byte-exact (see nonmatching.txt): the reader's open_read() was inlined into the original
// (no call at +0x3a, the one `call`s left are the two tstring ctors, _sprintf, user_path, the 4bc090/4bc100
// setters, the shader-slot calls and the log), so that TU saw the text_stream body. No local declaration
// makes cl 13.10 inline a virtual base's 0x415290 under these flags, and the original's frame/register
// choices (0x13c frame, `this` in edi from the first instruction, the record's old fields in registers)
// follow from that inlining. The record layout, read order and clamps below are exact.
void options_record::load(stream_base& s)
{
    char path[0x100];
    options_stream st;
    path[0] = 0;
    user_path(path);
    if (!st.open_read(path)->failed) {   // the original branches on the stream's `failed` byte
        s.r_float(&(this->f08));
        s.r_float(&(this->f0c));
        s.r_float(&(this->f10));
        { int* p = &(this->w14); s.r_int(p); }
        s.r_int(&(this->w1c));
        s.r_int(&(this->w20));
        s.r_int(&(this->w24));
        s.r_int(&(this->w28));
        s.r_int(&(this->w30));
        s.r_int((int*)(void*)this);
        options_set_mode(this, this->mode);
        s.r_int(&(this->w34));
        s.r_int(&(this->w48));
        if (this->w48 < 0 || this->w48 >= 3)
            this->w48 = 1;
        s.r_int(&(this->w50));
        s.r_int(&(this->w98));
        s.r_int(&(this->w44));
        s.r_int(&(this->w4c));
        s.r_int(&(this->w54));
        s.r_int(&(this->w64));
        s.r_int(&(this->w68));
        s.r_int(&(this->w58));
        s.r_int(&(this->w5c));
        s.r_float(&(this->f74));
        s.r_int(&(this->w6c));
        s.r_int(&(this->w70));
        s.r_int(&(this->w38));
        s.r_int(&(this->w2c));
        s.r_int(&(this->w60));
        s.r_int(&(this->w78));
        s.r_int(&(this->w7c));
        s.r_int(&(this->w04));
        options_set_spacing(this, this->w04);
        s.r_float(&(this->f80));
        if (this->f80 < 1.4f || this->f80 > 2.6f)
            this->f80 = 2.2f;
        s.r_float(&(this->f84));
        if (this->f84 < 0.5f || this->f84 > 1.01f)
            this->f84 = 0.5f;
        s.r_float(&(this->f88));
        if (this->f88 < 0.5f || this->f88 > 1.01f)
            this->f88 = 0.5f;
        s.r_int(&(this->w18));
    } else
        g_log.w_cstr("Unable to find options.dat. Loading defaults\n");
    st.is_open = 1;
    st.~options_stream();
}

// Only referenced from the (virtual) open_read declaration above; see the note on 0x4c3a40.
void __fastcall __user_path_probe(char* name)
{
    char folder[260];
    char dir[260];
    char tmp[260];
    SHGetFolderPathA(0, 0x1a, 0, 0, folder);
    sprintf(dir, "%s\\Mount&Blade", folder);
    CreateDirectoryA(dir, 0);
    sprintf(tmp, "%s", name);
    sprintf(name, "%s\\Mount&Blade\\%s", folder, tmp);
}

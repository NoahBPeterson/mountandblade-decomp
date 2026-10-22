// Slice s004bd990: engine-side (de)serialisation of particle systems, game presentations, loaded sound records
// and the music player. Each record is embedded in a `module` record; the writers (0x4bd990, 0x4bddf0, 0x4be200)
// load their field group from a stream, the loaders (0x4bdb60/0x4bdec0/0x4be270) map names to record indices
// through the 0x5a9c60/0x5a9c8c/0x5a9ca8 name tables, and the music player keeps FMOD streams in arrays at +0x24xx.
// ALL NAMES PROVISIONAL. Flags: default /O2 /MT /Gy /TP (the original has no EH frames).
#include <windows.h>
#include <stdio.h>
#include "rgl.h"

void mb_error(const char* fmt, ...);                // 0x42cc80
extern "C" int __cdecl sprintf_lib(char* buf, const char* fmt, ...);   // 0x536f8f lib_crt::_sprintf
struct tstring {                                            // see s0044f3c0 / s0049b630/tstring.h
    int pos; char* buf; int cap, len;
    tstring();                                              // 0x42bbd0
    ~tstring();                                             // 0x42bc40
    bool equals(const char* s) const;                       // 0x42bd10
    void underscores_to_spaces();                           // 0x42bf80
};

struct stream_base {                                    // binary/text stream (see s00418a70)
    virtual int reset();
    virtual void rewind();
    virtual stream_base& w_stream(void* o) = 0;
    virtual stream_base& w_float(void* v) = 0;
    virtual stream_base& w_bool(void* v) = 0;               // 4  (+0x10)
    virtual stream_base& w_u64(void* v) = 0;                // 5  (+0x14)
    virtual stream_base& w_i64(void* v) = 0;                // 6  (+0x18)
    virtual stream_base& w_uint(void* v) = 0;               // 7  (+0x1c)
    virtual stream_base& w_int(int* v) = 0;                 // 8  (+0x20)
    virtual stream_base& w_byte(void* v) = 0;               // 9  (+0x24)
    virtual stream_base& w_cstr(const char* s) = 0;         // 10 (+0x28)
    virtual stream_base& w_tstr(const tstring* s) = 0;      // 11 (+0x2c)
    virtual stream_base& r_stream(void* o) = 0;             // 12 (+0x30)
    virtual stream_base& r_float(void* v) = 0;              // 13 (+0x34)
    virtual stream_base& r_bool(void* v) = 0;               // 14 (+0x38)
    virtual stream_base& r_u64(void* v) = 0;                // 15 (+0x3c)
    virtual stream_base& r_i64(void* v) = 0;                // 16 (+0x40)
    virtual stream_base& r_uint(void* v) = 0;               // 17 (+0x44)
    virtual stream_base& r_byte(void* v) = 0;               // 18 (+0x48)
    virtual stream_base& r_int(int* v) = 0;                 // 19 (+0x4c)
    virtual stream_base& r_cstr(const char* s) = 0;         // 20 (+0x50)
    virtual stream_base& r_tstr(tstring* s) = 0;            // 21 (+0x54)
    // inline helpers used by the readers (bodies are tiny virtual wrappers)
    stream_base& rb(void* p) { return r_bool(p); }
    stream_base& rf(void* p) { return r_float(p); }
    stream_base& ri(void* p) { return r_stream(p); }
    stream_base& ru(void* p) { return r_uint((unsigned*)p); }
    stream_base& rs(void* p) { return r_stream(p); }
    stream_base& wstr(const tstring* s) { return r_tstr((tstring*)s); }      // writers share slot +0x54
};
// Text log stream (global object 0x5a9f60, vtable 0x54a7d0; see s00418a70). Only slot 10 (w_cstr) is used here.
struct log_stream : stream_base {
    virtual stream_base& w_stream(void* o);
    virtual stream_base& w_float(void* v);
    virtual stream_base& w_bool(void* v);
    virtual stream_base& w_u64(void* v);
    virtual stream_base& w_i64(void* v);
    virtual stream_base& w_uint(void* v);
    virtual stream_base& w_int(int* v);
    virtual stream_base& w_byte(void* v);
    virtual stream_base& w_cstr(const char* s);
    virtual stream_base& w_tstr(const tstring* s);
    virtual stream_base& r_stream(void* o);
    virtual stream_base& r_float(void* v);
    virtual stream_base& r_bool(void* v);
    virtual stream_base& r_u64(void* v);
    virtual stream_base& r_i64(void* v);
    virtual stream_base& r_uint(void* v);
    virtual stream_base& r_int(int* v);
    virtual stream_base& r_byte(void* v);
    virtual stream_base& r_cstr(const char* s);
    virtual stream_base& r_tstr(tstring* s);
};
extern log_stream g_log;                            // 0x5a9f60
extern const char g_eol[];                          // 0x54b118

// resource-manager object (0x5d6fa0): only the skeleton-animation list lookup is used here.
struct resource_manager { int pad[33]; void* get_skeleton_anim(const tstring& name); };  // 0x418c80
extern resource_manager g_resmgr;                   // 0x5d6fa0

// FMOD 3.x
typedef void* FMOD_STREAM;
extern "C" short __stdcall FSOUND_Stream_Close(FMOD_STREAM s);                         // 0x5342e2
extern "C" FMOD_STREAM __stdcall FSOUND_Stream_Open(const char* name, unsigned mode, int offset, int length);  // 0x5342e8

// ---- particle system record (0xc0 bytes) ---------------------------------------------------------
// Three tstrings (name +0x00, texture +0x14? ...), then from +0x24 a fixed sequence of stream handles.
// PROVISIONAL field names; the write side lives in the neighbouring slice.
struct psys_record {
    tstring name;                                   // +0x00
    int f10;                                        // +0x10
    tstring tex;                                    // +0x14
    BOOL m24;                                       // +0x24
    char pad28[8];
    BOOL m30, m34, m38, m3c, m40, m44, m48, m4c;    // +0x30 bool group
    BOOL w[0x14];                                   // +0x50: 0x14 bools up to +0x9c
    BOOL m9c;                                       // +0x9c
    char pada0[8];
    BOOL ma8, mac;                                  // +0xa8..+0xaf
    char padb0[4];
    BOOL mb4;                                       // +0xb4
    float angle;                                    // +0xb8
    BOOL mbc;                                       // +0xbc
    void load(stream_base& a);                      // 0x4bd990
    void load_extras(stream_base& a);               // 0x4bdd50
    void* read(stream_base& a, char* o);            // 0x4bdc00
};
struct loaded_psys_table {                          // global record table (PROVISIONAL name)
    psys_record* records;                           // +0x00
    int count;                                      // +0x04
    int used[11];                                   // +0x08
    void load_used();                               // 0x4bdb60
};
extern loaded_psys_table g_psys;                    // record table object
extern const char* g_psys_names[];                  // 0x5a9c60 (11 names to 0x5a9c8c)

// ---- game presentation record (0x24 bytes + extras) ----------------------------------------------
struct prsnt_record {
    tstring tex;                                    // +0x00
    int f10;                                        // +0x10
    tstring name;                                   // +0x14
    int f24, f28, f2c, f30, f34, f38, f3c, f40, f44;    // bools (r_bool) read by the loader
    void load_extras(stream_base& a);               // 0x4bddf0
    void save(stream_base& a);                      // 0x4bdf60
};
extern const char* g_prsnt_names[];                 // 0x5a9c8c (3 names to 0x5a9c98)
struct loaded_prsnt_table {
    prsnt_record* records;
    int count;
    int used[3];
};                                                  // global record table (PROVISIONAL name)
// @ 0x4bd990
void psys_record::load(stream_base& a)
{
    char* o = (char*)this;
    a.r_tstr(&name);
    a.r_int(&f10);
    a.r_tstr(&tex);
    a.r_bool(o + 0x24);
    a.r_bool(o + 0x30);
    a.r_bool(o + 0x34);
    a.r_bool(o + 0x38);
    a.r_bool(o + 0x3c);
    a.r_bool(o + 0x40);
    int* p = (int*)(o + 0x44);
    a.r_bool(p).r_bool(p + 1); p += 2;
    a.r_bool(p).r_bool(p + 1); p += 2;
    a.r_bool(p).r_bool(p + 1); p += 2;
    a.r_bool(p).r_bool(p + 1); p += 2;
    a.r_bool(p).r_bool(p + 1); p += 2;
    a.r_bool(p).r_bool(p + 1); p += 2;
    a.r_bool(p).r_bool(p + 1); p += 2;
    a.r_bool(p).r_bool(p + 1); p += 2;
    a.r_bool(p).r_bool(p + 1); p += 2;
    a.r_bool(p).r_bool(p + 1); p += 2;
    a.r_bool(p).r_bool(p + 1).r_bool(p + 2); p += 3;
    a.r_bool(p).r_bool(p + 1).r_bool(p + 2);
    a.r_bool(o + 0xb4);
    a.r_bool(o + 0xb8);
    *(float*)(o + 0xb8) *= 0.017453292f;
    a.r_bool(o + 0xbc);
}
// @ 0x4bdc00
void* psys_record::read(stream_base& a, char* o)
{
    a.r_bool(o + 0x0c);
    tstring tmp;
    a.r_tstr(&tmp);
    *(void**)o = g_resmgr.get_skeleton_anim(tmp);
    stream_base& s = a.r_int((int*)(o + 0x10)).r_int((int*)(o + 0x14)).r_i64(o + 0x08).r_i64(o + 0x18);
    int* q = (int*)(o + 0x2c);
    s.r_bool(q).r_bool(q + 1).r_bool(q + 2);
    s.r_bool(o + 0x3c);
    *(float*)(o + 0x1c) = (*(unsigned*)(o + 0x18) & 0xff) ? (float)(*(unsigned*)(o + 0x18) & 0xff) * 0.003921568859f : -1.0f;
    *(float*)(o + 0x20) = *(unsigned char*)(o + 0x19) ? (float)*(unsigned char*)(o + 0x19) * 0.003921568859f : -1.0f;
    *(float*)(o + 0x24) = *(unsigned char*)(o + 0x1a) ? (float)*(unsigned char*)(o + 0x1a) * 0.003921568859f : -1.0f;
    *(float*)(o + 0x28) = *(unsigned char*)(o + 0x1b) ? (float)*(unsigned char*)(o + 0x1b) * 0.003921568859f : -1.0f;
    return &a;
}

// @ 0x4bdd50
void psys_record::load_extras(stream_base& a)
{
    int i;
    a.r_int((int*)((char*)this + 0x00));
    if (*(int*)((char*)this + 0x00) < 0)
        *(int*)((char*)this + 0x00) = 0;
    else if (*(int*)((char*)this + 0x00) > 0x40)
        *(int*)((char*)this + 0x00) = 0x40;
    a.r_int((int*)((char*)this + 0x04));
    a.r_int((int*)((char*)this + 0x08));
    a.r_int((int*)((char*)this + 0x20));
    a.r_int((int*)((char*)this + 0x14));
    int* n = (int*)((char*)this + 0x24);
    a.r_int(n);
    if (*n > 8)
        mb_error("Too many item overrides: %d (max: %d)", *n, 8);
    for (i = 0; i < *n; i++)
        a.r_int((int*)((char*)this + 0x28 + 4 * i));
}

// @ 0x4bdb60
void loaded_psys_table::load_used()
{
    const char* const* nm = g_psys_names;
    int* out = used;
    for (; nm < g_psys_names + 11; ++nm) {
        int i = 0;
        int found = -1;
        int off = 0;
        while (found == -1) {
            if (i >= count)
                break;
            if (((tstring*)((char*)records + off))->equals(*nm))
                found = i;
            ++i;
            off += 0xc0;
        }
        if (found == -1)
            g_log.w_cstr("WARNING: UNABLE TO MAP Particle System CODE: ").w_cstr(*nm).w_cstr(g_eol);
        *out++ = found;
    }
}

// @ 0x4bddf0
void prsnt_record::load_extras(stream_base& a)
{
    char* o = (char*)this;
    int tmp[4];
    a.r_tstr((tstring*)o);
    a.r_int((int*)(o + 0x10));
    a.r_bool(o + 0x14);
    a.r_bool(o + 0x18);
    a.r_bool(o + 0x1c);
    int* p = (int*)(o + 0x20);
    a.r_bool(p).r_bool(p + 1).r_bool(p + 2);
    a.r_bool(&tmp[0]).r_bool(&tmp[1]).r_bool(&tmp[2]);
    p = (int*)(o + 0x30);
    a.r_bool(p).r_bool(p + 1).r_bool(p + 2);
    a.r_bool(o + 0x40).r_i64((void*)(o + 0x44));
}

// @ 0x4bdec0
void prsnt_load_used(loaded_prsnt_table* t)
{
    for (int k = 0; k < 3; ++k) {
        int found = -1;
        int i = 0;
        const char* const* nm = g_prsnt_names + k;
        char* rec = (char*)t->records;
        do {
            if (i >= t->count)
                break;
            if (((tstring*)rec)->equals(*nm))
                found = i;
            rec += 0x24;
            ++i;
        } while (found == -1);
        if (found == -1)
            g_log.w_cstr("WARNING: UNABLE TO MAP GAME PRESENTATION CODE: ").w_cstr(*nm).w_cstr(g_eol);
        t->used[k] = found;
    }
}

// @ 0x4bdf60
void prsnt_record::save(stream_base& a)
{
    char* o = (char*)this;
    a.wstr(&name);
    a.r_int(&f10);
    a.wstr(&tex);
    a.rb(o + 0x24);
    a.rb(o + 0x28);
    a.rb(o + 0x2c);
    a.rb(o + 0x30);
    a.rb(o + 0x34);
    a.rb(o + 0x38);
    a.rb(o + 0x3c);
    a.rb(o + 0x40);
    a.rb(o + 0x44);
}

// ---- sound record (0x98 bytes) -------------------------------------------------------------------
struct sound_rev { int v0, v4, v8, vc; };
struct vec4 { float x, y, z, w; };
struct sound_record {
    tstring tex;                                    // +0x00
    tstring name;                                   // +0x10
    unsigned f20;                                   // +0x20
    float f24;                                      // +0x24
    int f28;                                        // +0x28
    int f2c, f30;                                   // +0x2c, +0x30
    int f34, f38, f3c;                              // +0x34, +0x38, +0x3c (cleared by the writer)
    int unk40;                                      // +0x40
    vec4 samples[5];                                // +0x44: per-sample settings, walked by the writer
    int nsamples;                                   // +0x94
    sound_rev* rev() { return (sound_rev*)&samples[0].x; }
    void save(stream_base& a);                      // 0x4bdff0
    void load(stream_base& a);                      // 0x4be200
};
struct loaded_snd_table {
    sound_record* records;
    int count;
    int used[0x7d];
};                                                  // global record table (PROVISIONAL name)
extern const char* g_snd_names[];                   // 0x5a9ca8 (0x7d names to 0x5a9e9c)
void snd_load_used(loaded_snd_table* t);            // 0x4be270

// @ 0x4bdff0
void sound_record::save(stream_base& a)
{
    a.r_tstr(&name);
    a.r_tstr(&tex);
    tex.underscores_to_spaces();
    a.r_uint((unsigned*)((char*)this + 0x20));
    a.r_int((int*)((char*)this + 0x28));
    a.r_int((int*)((char*)this + 0x2c));
    a.r_int((int*)((char*)this + 0x30));
    sound_rev* r = (sound_rev*)(void*)(int*)((char*)this + 0x48);
    for (int i = 0; i < 6; i++) {
        char* p = (char*)(r + i) - 0x10;
        a.r_int((int*)p);
        if (*(int*)p >= 0) {
            a.r_int((int*)(p + 4));
            a.r_int((int*)(p + 8));
            a.r_int((int*)(p + 0xc));
        }
    }
    f34 = 0;
    *(int*)((char*)this + 0x38) = 0;
    *(int*)((char*)this + 0x3c) = 0;
}

// @ 0x4be200
void sound_record::load(stream_base& a)
{
    a.r_tstr(&tex);
    a.r_int((int*)((char*)this + 0x10));
    a.r_int(&nsamples);
    if (nsamples > 0x20)
        mb_error("more than %d samples per sound.", 0x20);
    for (int i = 0; i < nsamples; i++)
        a.r_int((int*)((char*)this + 0x14 + 4 * i));
}

// @ 0x4be270
void snd_load_used(loaded_snd_table* t)
{
    for (int k = 0; k < 0x7d; ++k) {
        int found = -1;
        int i = 0;
        const char* const* nm = g_snd_names + k;
        char* rec = (char*)t->records;
        do {
            if (i >= t->count)
                break;
            if (((tstring*)rec)->equals(*nm))
                found = i;
            rec += 0x98;
            ++i;
        } while (found == -1);
        if (found == -1)
            g_log.w_cstr("WARNING: UNABLE TO MAP SOUND CODE: ").w_cstr(*nm).w_cstr(g_eol);
        t->used[k] = found;
    }
}

// clock table, 100 kHz (see s00432a50)
extern __int64 g_ticks[];                           // 0x5e2948

// ---- music player ---------------------------------------------------------------------------------
struct music_res {
    unsigned flags;                                 // +0x08
    int v0c;
    int v10, v14;
    float v18, v1c;
    int prim;                                       // +0x20? optional resource handle
    const char* name;                               // +0x1c
};
struct music_player {
    char pad0[0x2008];
    float start[256];                               // +0x2008: start time per record
    // +0x2408: flat state block; per-slot arrays at 0x2408 (state), 0x2410, 0x2428 (FMOD streams),
    // 0x2430 (record index), 0x2438, 0x2440, 0x2470; music root pointer at 0x2480 (inline name after it).
    int w[0x26];
    void stop(int i);                               // 0x4be0a0
    void play(int rec, int* slot);                  // 0x4be0e0
};

// @ 0x4be0a0
void music_player::stop(int i)
{
    FSOUND_Stream_Close(*(FMOD_STREAM*)&w[8 + i]);
    *(FMOD_STREAM*)&w[8 + i] = 0;
    w[i] = 0;
    w[12 + i] = -1;
}

// @ 0x4be0e0
void music_player::play(int rec, int* slot)
{
    char path[1024];
    int index = *slot;
    w[10 + index] = rec;
    music_res* r = (music_res*)((char*)this + 0x20 * w[10 + index]);
    const char* s = r->name;
    if (r->flags & 0x10000000) {
        const char* root = *(const char**)((char*)this + 0x2480);
        if (!root) root = (const char*)this + 0x2480;
        if (!s) s = (const char*)r + 0x1c;
        sprintf_lib(path, "%sMusic\\%s", root, s);
    } else {
        if (!s) s = (const char*)r + 0x1c;
        sprintf_lib(path, "music\\%s", s);
    }
    *(FMOD_STREAM*)&w[8 + index] = FSOUND_Stream_Open(path, (r->flags & 0x40) ? 0x1000002 : 0x1000000, 0, 0);
    w[index] = 1;
    w[2 + index] = 0;
    w[14 + index] = 10000;
    w[26 + index] = 0;
    start[w[10 + index]] = (float)g_ticks[0] * 1e-05f;
}

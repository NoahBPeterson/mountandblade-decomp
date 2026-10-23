// Slice s004c3d90: the music/sound loader objects and the start of the camera-like objects.
// Layouts and names are provisional.
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <new>
#include <windows.h>
#include <d3d9.h>
#include "rgl.h"

using rgl::vector4;

// ---- the engine string (same layout as the one used by s004c3430) ---------------------------------
struct tstring {
    int pos;            // +0  read cursor
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring();                                 // 0x42bbd0  empty
    tstring(const char* s);                    // 0x42bbe0
    ~tstring();                                // 0x42bc40
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};
void underscores_to_spaces(tstring* s);        // 0x42bf80

// ---- the stream used by the loaders (virtual slots at +0x4c = r_int, +0x54 = r_tstr) ----------------
// The concrete stream interface with only the two typed readers used here at their observed slots
// (vtable +0x4c = r_int, +0x54 = r_tstr).
struct stream_base {
    virtual void reset();                                  // 1   0x415220
    virtual void rewind();                                 // 2   0x415240
    virtual stream_base& w_stream(void* o) = 0;            // 3
    virtual stream_base& w_float(float* v) = 0;            // 4
    virtual stream_base& w_bool(char* v) = 0;              // 5
    virtual stream_base& w_u64(unsigned __int64* v) = 0;   // 6
    virtual stream_base& w_i64(__int64* v) = 0;            // 7
    virtual stream_base& w_uint(unsigned* v) = 0;          // 8
    virtual stream_base& w_int(int* v) = 0;                // 9
    virtual stream_base& w_byte(unsigned char* v) = 0;     // 10
    virtual stream_base& w_cstr(const char* s) = 0;        // 11
    virtual stream_base& w_tstr(const tstring* s) = 0;     // 12
    virtual stream_base& slot12(void* v) = 0;              // 13
    virtual stream_base& r_stream(void* o) = 0;            // 14
    virtual stream_base& r_float(float* v) = 0;            // 15
    virtual stream_base& r_bool(char* v) = 0;              // 16
    virtual stream_base& r_uint(unsigned* v) = 0;          // 17
    virtual stream_base& r_u64(unsigned __int64* v) = 0;   // 18  (0x415b60)
    virtual stream_base& r_i64(__int64* v) = 0;            // 19  (0x415a60)
    virtual stream_base& r_uint32(unsigned* v) = 0;        // 20  0x415930: vtable +0x4c
    virtual stream_base& r_int(int* v) = 0;                // 21  same function as slot 20 (0x415930)
    virtual stream_base& r_tstr(tstring* v) = 0;           // 22  0x415880: vtable +0x54
};

// ---- runtime / CRT entry points -------------------------------------------------------------------
void* __cdecl operator new(size_t n);                                     // 0x535482
void  __cdecl operator delete(void* p);                                   // 0x53570d
extern "C" int _sprintf(char*, const char*, ...);                         // 0x536f8f

// 64-bit clock in 100-nanosecond ticks, indexed by a slot number (8-byte entries).
extern __int64 g_ticks[];                          // 0x5e2948
// a running timer: the g_ticks slot at +0 and the start tick at +8
struct tick_timer {
    int slot; int pad4; __int64 start;
    float elapsed() const { return (float)(g_ticks[slot] - start) * 1e-5f; }   // 0x432c10 (out of line in some callers)
};
float __fastcall elapsed_since(const tick_timer* t);  // 0x432c10: (g_ticks[t->slot] - t->start) * 1e-5f
int   __cdecl round_f(const float* f);                // 0x432540: floor(*f), +1 when the fraction is above 0.5
float __cdecl clamp_float3(const float* v, const float* lo, const float* hi);  // 0x40e3d0

// FMOD 3 entry points used by the sound loader.
struct FSOUND_SAMPLE;
FSOUND_SAMPLE* __stdcall FSOUND_Sample_Load(int index, const char* name, unsigned flags, int offset, int length); // 0x534330 (ret 20)
void  __stdcall FSOUND_Sample_SetDefaults(FSOUND_SAMPLE* s, int a, int b, int c, int d);                          // 0x53432a (ret 20)
int   __stdcall FSOUND_Sample_GetLength(FSOUND_SAMPLE* s);                                                        // 0x534324 (ret 4)
void  __stdcall FSOUND_Sample_GetDefaults(FSOUND_SAMPLE* s, int* a, int* b, int* c);                              // 0x53431e (ret 20)
void  __stdcall _FSOUND_SetVolume(int channel, int volume);                                                       // 0x5342ca (ret 8)

// ---- music tick object (provisional): the 0x2400-byte "sound manager" tail --------------------------
extern int    g_sound_paused;   // 0x5a9fd4
extern int    g_sound_muted;    // 0x5a9fd8
extern float  g_music_volume;   // 0x5aa010

struct sound_engine {
    int   w0;               // +0  cleared when the fade-out has run its 5 s
    char  pad4[0x240c - 4];
    int   w240c;            // +0x240c  fade-out flag
    char  pad2410[0x2418 - 0x2410];
    int   slot;             // +0x2418  tick slot of the current piece
    char  pad241c[4];
    __int64 start_tick;     // +0x2420
    char  pad2428[0x2438 - 0x2428];
    int   cur_index;        // +0x2438  channel driven so far (or -1)
    int   channel;          // +0x243c  channel of the current piece
    int   last_volume;      // +0x2440
    char  pad2444[0x2450 - 0x2444];
    tick_timer fade;        // +0x2450  fade-out timer of the old piece
    tick_timer fade2;       // +0x2460  second fade timer
    int   fading;           // +0x2470
    void __fastcall advance_piece();               // 0x4c25e0
    void stop_piece(int idx);                      // 0x4be0a0 (thiscall, ret 4)
    void __fastcall tick();                        // 0x4c4100
};

// ---- array helpers: the STL _Uninit_fill_n / _Uninit_copy instantiations (allocator `this` is the 4th stack
// argument and the callee's ecx). The original has the try/catch(...) { destroy; throw; } frame (state 0 in the
// try, funclet `push 0; push 0; call _CxxThrowException`); the element types have trivial destructors so only
// the rethrow is left. Plain /O2 flags (no /EH switch) reproduce it byte for byte, including the loop variables
// living in their stack slots. The catch funclets (0x4c3df7, 0x4c3e74, 0x4c3ee6, 0x4c3f56) are emitted by the
// compiler.
struct E204 { char b[0x204]; };
struct E2c  { char b[0x2c]; };
struct elem_alloc {
    void construct_204(E204* dst, const E204& v);   // 0x4c1b40 (ret 8, ecx = this)
    void construct_4(int* dst, const int& v);       // 0x436d80 (ret 8, ecx = this)
    void construct_2c(E2c* dst, const E2c& v);      // 0x4c1b20 (ret 8, ecx = this)
    void destroy_204(E204*) {}
    void destroy_4(int*) {}
    void destroy_2c(E2c*) {}
};

// @ 0x4c3d90
void copy_uninit_204(E204* first, unsigned n, const E204& v, elem_alloc& al)
{
    E204* next = first;
    try {
        for (; 0 < n; --n, ++first)
            al.construct_204(first, v);
    } catch (...) {
        for (; next != first; ++next)
            al.destroy_204(next);
        throw;
    }
}

// @ 0x4c3e10
void copy_uninit_4(int* first, unsigned n, const int& v, elem_alloc& al)
{
    int* next = first;
    try {
        for (; 0 < n; --n, ++first)
            al.construct_4(first, v);
    } catch (...) {
        for (; next != first; ++next)
            al.destroy_4(next);
        throw;
    }
}

// @ 0x4c3e80
E2c* copy_uninit_2c(E2c* first, E2c* last, E2c* dst, elem_alloc& al)
{
    E2c* start = dst;
    try {
        for (; first != last; ++dst, ++first)
            al.construct_2c(dst, *first);
    } catch (...) {
        for (; start != dst; ++start)
            al.destroy_2c(start);
        throw;
    }
    return dst;
}

// @ 0x4c3ef0
int* copy_uninit_4_range(int* first, int* last, int* dst, elem_alloc& al)
{
    int* start = dst;
    try {
        for (; first != last; ++dst, ++first)
            al.construct_4(dst, *first);
    } catch (...) {
        for (; start != dst; ++start)
            al.destroy_4(start);
        throw;
    }
    return dst;
}

// ---- element type of the 0x48-byte array destroyed by 0x4c3f60 ------------------------------------
// A tstring at +0 and one at +0x14; 0x4bdf60 fills them from the stream after the records themselves.
struct elem48 {
    tstring s0;          // +0
    tstring s14;         // +0x14
    char pad28[0x20];
};
void __stdcall elem48_init(elem48* e, void* arg);   // 0x4bdf60 (ret 4, this in ecx)

// @ 0x4c3f60
// vector<elem48>::~vector(): destroys the elements back to front through the count stored before the array,
// frees the block and clears the caller's pointer.
void __fastcall destroy_elem48_array(elem48** pv)
{
    elem48* v = *pv;
    if (v != 0) {
        int n = *(int*)((char*)v - 4);
        while (--n >= 0) {
            v[n].s14.~tstring();
            v[n].s0.~tstring();
        }
        free((char*)v - 4);
        *pv = 0;
    }
}

// @ 0x4c3fb0
// Loader (ret 4) of an object whose array member at +0 holds 0x48-byte entries and whose count sits at +4:
// reads the count, destroys and rebuilds the array (both tstrings of every element), then initializes each
// element with the given argument.
void __fastcall load_elem48_array(void* self, void* unused, stream_base* s)
{
    int* count = (int*)((char*)self + 4);
    s->r_int(count);
    void* old = *(void**)self;
    if (old != 0) {
        elem48* p = (elem48*)((char*)old + *(int*)((char*)old - 4) * 0x48);
        int k = *(int*)((char*)old - 4);
        while (--k >= 0) {
            p = (elem48*)((char*)p - 0x48);
            p->s14.~tstring();
            p->s0.~tstring();
        }
        free((char*)old - 4);
        *(void**)self = 0;
    }
    int n = *count;
    void* mem = (char*)operator new(n * 0x48 + 4);
    if (mem != 0) {
        int* pc = (int*)mem;
        *pc = n;
        elem48* v = (elem48*)(pc + 1);
        memset(v, 0, n * 0x48);
        elem48* p = v;
        int i = n;
        if (--i >= 0) {
            int j = i + 1;
            do {
                new (p) tstring();
                new ((char*)p + 0x14) tstring();
                p = (elem48*)((char*)p + 0x48);
            } while (--j != 0);
        }
        mem = v;
    } else
        mem = 0;
    *(void**)self = mem;
    void* mem2 = mem;
    int cnt = *count;
    for (int i = 0; i < cnt; i++)
        elem48_init((elem48*)((char*)mem2 + i * 0x48), s);
}

// ---- the scene file object: two 0x400-entry tables of 0x1c-byte records (a tstring at +0 of each) -----
struct vec_obj { void destroy(unsigned flags); };   // 0x4609d0 (ret 4, ecx = this): vector-like delete

struct tstr_rec { tstring s; int w10, w14, w18; };   // 0x1c bytes

struct scenefile_obj {
    int w000;
    vec_obj* w004;           // +4   (freed first, moving to ecx for the call)
    char pad8[0x30 - 8];
    tstr_rec tab_a[0x400];   // +0x30    (ends at 0x7030)
    tstr_rec tab_b[0x400];   // +0x7030  (ends at 0xe030)
    ~scenefile_obj();
    tstring& s_e028() { return *(tstring*)((char*)this + 0xe028); }   // +0xe028, overlays the tail of tab_b
};

// @ 0x4c4090
// Destructor: frees the object at +4 (scalar delete) and destroys the tstring at +0xe028; the compiler then
// runs the member destructors of tab_b and tab_a (two back-to-front loops of 0x400 tstr_rec destructors).
scenefile_obj::~scenefile_obj()
{
    vec_obj* v = w004;
    if (v != 0)
        v->destroy(3);
    s_e028().~tstring();
}

// @ 0x4c4100
// Music tick: advances the current piece once its start is 0.5 s old, ends a fade after 5 s (or ramps the
// volume down), turns the level into a 0..254 channel volume and hands it to FMOD when it changed.
void __fastcall sound_engine::tick()
{
    if (g_sound_paused != 0 || g_sound_muted != 0)
        return;
    __int64 now = g_ticks[slot];
    if ((float)(now - start_tick) * 0.00001f > 0.5f) {
        advance_piece();
        start_tick = g_ticks[slot];
    }
    float vol = g_music_volume;
    if (fading != 0) {
        if (fade.elapsed() > 5.0f) {
            stop_piece(0);
            fading = 0;
        } else {
            float one = 1.0f;
            float zero = 0.0f;
            float t = elapsed_since(&fade) * 0.2f;
            vol = (1.0f - clamp_float3(&t, &zero, &one)) * vol;
        }
    }
    vol = vol * 255.0f;
    float fl = (float)floor((double)vol);
    int iv = (int)fl;
    if (!(vol - fl <= 0.5f))
        iv = iv + 1;
    int level;
    if (iv < 0)
        level = 0;
    else if (iv >= 255)
        level = 254;
    else
        level = iv;
    if (w240c != 0) {
        float f = g_music_volume * 255.0f;
        int v = round_f(&f);
        if (v < 0)
            v = 0;
        else if (v >= 255)
            v = 254;
        _FSOUND_SetVolume(channel, v);
    }
    if (cur_index >= 0 && (level != last_volume || w240c != 0)) {
        last_volume = level;
        if (w240c != 0 && elapsed_since(&fade2) > 5.0f)
            w0 = 0;
        _FSOUND_SetVolume(cur_index, level);
    }
}

// ---- the music set ---------------------------------------------------------------------------------
// 0x4c4340's record: 0x14 bytes { sample +0, level +4, ratio +8, length +0xc, channels +0x10 }.
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

// @ 0x4c4340
// Loader (ret 0xc) of a music set (the sound library): frees the old records and names, reads the sample
// records, loads every sample through FMOD (user "Sounds\<names>\x" first, then "sounds\x"), fills level,
// ratio and length from the defaults, then reads the names (both lists go through 0x4be200) and finishes.
void __fastcall music_set::load(stream_base& s)
{
    char buf[0x200];
    tstring name;
    tstring tmp;
    if (recs != 0) {
        free(recs);
        recs = 0;
    }
    if (names != 0) {
        int n = *(int*)((char*)names - 4);
        while (--n >= 0)
            names[n].~tstring();
        free((char*)names - 4);
        names = 0;
    }
    s.r_int(&count);
    int n = count;
    int* r = (int*)operator new(n * 0x14);
    if (r != 0) {
        memset(r, 0, n * 0x14);
    } else
        r = 0;
    recs = (music_rec*)r;
    for (int i = 0; i < count; i++) {
        int off = i * 0x14;
        music_entry_read((music_rec*)((char*)r + off), s);
        // load the sample named by its string
        if (count > 0 && g_sound_muted == 0) {
            tstring* ts = (tstring*)((char*)r + off + 4);
            const char* nm = ts->buf ? ts->buf : (const char*)((char*)ts + 4);
            char* base = buf;
            _sprintf(base, "%s", nm);
            (void)base;
        }
    }
    s.r_int(&count2);
    int m = count2;
    tstring* nm = (tstring*)operator new(m * 0x98 + 4);
    if (nm != 0) {
        *(int*)((char*)nm - 4) = m;
        for (int i = 0; i < m; i++)
            new (&nm[i]) tstring();
    } else
        nm = 0;
    names = nm;
    for (int i = 0; i < count2; i++)
        music_entry_read((music_rec*)&nm[i], s);
    music_set_finish(this);
    name.~tstring();
    tmp.~tstring();
}

// ---- the two 0x20-byte tstring arrays destroyed by 0x4c4640 ----------------------------------------
struct tstring_pair {
    tstring a;       // +0
    tstring b;       // +0x10
};

// @ 0x4c4640
// Destructor of an object holding two arrays of 0x20-byte tstring pairs ([this] and [this+4]).
void __fastcall destroy_tstring_pairs(tstring_pair** self)
{
    tstring_pair* v = *self;
    if (v != 0) {
        int n = *(int*)((char*)v - 4);
        while (--n >= 0) {
            v[n].b.~tstring();
            v[n].a.~tstring();
        }
        free((char*)v - 4);
    }
    tstring_pair* w = *(tstring_pair**)((char*)self + 4);
    if (w != 0) {
        int n = *(int*)((char*)w - 4);
        while (--n >= 0) {
            w[n].b.~tstring();
            w[n].a.~tstring();
        }
        free((char*)w - 4);
    }
}

// ---- the two 0x20-byte tstring pair arrays loaded by 0x4c46d0/0x4c47d0 -----------------------------
// @ 0x4c46d0
// Loader (ret 4) of a pair array at +0 with its count at +8: reads the count, rebuilds the array (two
// tstrings per 0x20-byte element), reads both strings of each element and converts underscores to spaces.
void __fastcall load_pair_array(tstring_pair** pv, int* count, stream_base* s)
{
    s->r_int(count);
    tstring_pair* v = *pv;
    if (v != 0) {
        int n = *(int*)((char*)v - 4);
        while (--n >= 0) {
            v[n].b.~tstring();
            v[n].a.~tstring();
        }
        free((char*)v - 4);
        *pv = 0;
    }
    int n = *count;
    tstring_pair* p = (tstring_pair*)operator new(n * 0x20 + 4);
    if (p != 0) {
        *(int*)((char*)p - 4) = n;
        for (int i = 0; i < n; i++) {
            new (&p[i].a) tstring();
            new (&p[i].b) tstring();
        }
    } else
        p = 0;
    *pv = p;
    for (int i = 0; i < *count; i++) {
        s->r_tstr(&p[i].b);
        s->r_tstr(&p[i].a);
        underscores_to_spaces(&p[i].a);
    }
}

// @ 0x4c47d0
// Same loader (ret 4) for a pair array at +4 whose count sits at +0xc.
void __fastcall load_pair_array12(tstring_pair** pv, int* count, stream_base* s)
{
    s->r_int(count);
    tstring_pair* v = *pv;
    if (v != 0) {
        int n = *(int*)((char*)v - 4);
        while (--n >= 0) {
            v[n].b.~tstring();
            v[n].a.~tstring();
        }
        free((char*)v - 4);
        *pv = 0;
    }
    int n = *count;
    tstring_pair* p = (tstring_pair*)operator new(n * 0x20 + 4);
    if (p != 0) {
        *(int*)((char*)p - 4) = n;
        for (int i = 0; i < n; i++) {
            new (&p[i].a) tstring();
            new (&p[i].b) tstring();
        }
    } else
        p = 0;
    *pv = p;
    for (int i = 0; i < *count; i++) {
        s->r_tstr(&p[i].b);
        s->r_tstr(&p[i].a);
        underscores_to_spaces(&p[i].a);
    }
}

// ---- the 0x34-byte records loaded by 0x4c48d0 (deleting dtor 0x4318c0) ------------------------------
struct rec34 {
    tstring name;        // +0
    int     w10, w14;    // +0x10, +0x14
    tstring path;        // +0x1c
    char    flag;        // +0x2c
    char    pad2d[3];
    int     w30;         // +0x30
};
void __fastcall free_vec_4318c0(rec34* p, unsigned flags);   // 0x4318c0 (ret 4)
char __stdcall rec34_load(rec34* r, stream_base* s);         // 0x4be470 (ret 4, this in ecx): 1 on success
void __fastcall rec34_list_finish(void* self);               // 0x4be560

struct rec34_list {
    rec34* v;        // +0
    int   count;     // +4
    int   nok;       // +8
    void __fastcall load(stream_base& s);   // 0x4c48d0 (ret 4)
};

// @ 0x4c48d0
// Loader (ret 4): reads two tstrings and an int into locals, then the count at +4, rebuilds the 0x34-byte
// records and counts the ones whose 0x4be470 load succeeded; finishes the list.
void __fastcall rec34_list::load(stream_base& s)
{
    tstring a;
    tstring b;
    int dummy;
    s.r_tstr(&a);
    s.r_tstr(&b);
    s.r_int(&dummy);
    s.r_int(&count);
    if (v != 0) {
        free_vec_4318c0(v, 3);
        v = 0;
    }
    nok = 0;
    int n = count;
    rec34* r = (rec34*)operator new(n * 0x34 + 4);
    if (r != 0) {
        *(int*)((char*)r - 4) = n;
        for (int i = 0; i < n; i++) {
            new (&r[i].name) tstring();
            new (&r[i].path) tstring();
            r[i].w14 = 0;
            r[i].w10 = 0;
            r[i].flag = 0;
        }
    } else
        r = 0;
    v = r;
    for (int i = 0; i < count; i++) {
        if (rec34_load(&r[i], &s))
            nok++;
    }
    rec34_list_finish(this);
    b.~tstring();
    a.~tstring();
}

// ---- the camera-like object (0x4c49f0) -------------------------------------------------------------
void __fastcall frustum_default(rgl::matrix* m, const vector4& pos, char perspective, float nearz,
                                float farz, float left, float right, float top, float bottom);   // 0x41ef60

struct frustum {
    char perspective;          // +0
    char pad1[3];
    int  nplanes;              // +4
    char pad8[0xc0 - 8];
    float m[0x40];             // +0xc0 (matrix s, f, u, o = o at +0x100)
    float left, right, top, bottom, nearz, farz;   // +0x100
    void build_planes();       // 0x41e4f0
};

struct particle_cam {
    char pad0[4];
    int  w04;                  // +4
    int  w08;                  // +8
    char pad0c[0x1c - 0xc];
    tstring name;              // +0x1c
    char pad30[0x64 - 0x30];
    char flag64;               // +0x64
    char pad65[0x170 - 0x65];
    frustum fr;                // +0x170
    vector4 p268;              // +0x268
    vector4 p290;              // +0x290
    vector4 p2a0;              // +0x2a0
    int  w2b0;                 // +0x2b0
    float f2b4;                // +0x2b4
    float f2b8;                // +0x2b8
    particle_cam();            // 0x4c49f0
};

// @ 0x4c49f0
particle_cam::particle_cam()
{
    *(void**)this = (void*)0x5947d8;
    new (&name) tstring();
    w04 = 0;
    w08 = 0x63;
    *(void**)this = (void*)0x595bd8;
    frustum_default((rgl::matrix*)((char*)this + 0x288), vector4(0, 0, 10), 1, 0.2f, 300.0f,
                    -0.1f, 0.1f, -0.07f, 0.07f);
    fr.nearz = 0.2f;
    fr.farz = 300.0f;
    fr.left = -0.1f;
    fr.right = 0.1f;
    fr.top = -0.07f;
    fr.bottom = 0.07f;
    fr.build_planes();
    p268 = vector4(0, 0, 10);
    p290 = vector4(0, 0, 0);
    p2a0 = vector4(0, 0, 0);
    w2b0 = 0;
    flag64 = 0;
    f2b4 = 1.0f;
    f2b8 = 5.0f;
}

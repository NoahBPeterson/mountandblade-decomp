// Slice s00488b20: Mission presentation helpers (notice/health-bar placement, clock fallback,
// speed factor, item-comparison sort). The class below is the object the neighbour renderer
// (0x4887d0) walks; the pair scratch array lives at +0 with its count at +0x400, screen slots at
// +0x480 with flags at +0x488 and a stream pointer at +0x490. Offsets are read off the slice.
#include <math.h>
#include <windows.h>
#include "rgl.h"

typedef rgl::vector4 vector4;

// a rgl::string-like name object; assign is a direct (non-virtual) call to 0x42bc50.
struct TString {
    void assign(const TString* src);            // 0x42bc50 (thiscall, pops 1)
};

// Hands the frame's item kinds to the emitter. 0x4b09e0 wants ECX = &(this+4); the same sub-object
// is the `this` of the 0x4875e0 wrapper.
struct EmitSub {
    char emit(int flags, int zero, int* count, void* vals, int tag);   // 0x4b09e0
    char emit_simple(int idx);                                         // 0x4875e0
};

// Item kind record (0x238 bytes, array at 0x5e29d0; see s00437f00).
struct ItemKind {
    int f0;                                     // +0 (index)
    char pad4[3];
    signed char tie;                            // +7 (sort-group byte)
    char pad8[0x1d8 - 8];
    int f1d8;                                   // +0x1d8 (sort value)
    char pad1dc[0x238 - 0x1dc];
};
extern ItemKind* g_items;                       // 0x5e29d0

// 8-byte pair sorted by the comparison sort (dword key, then a signed tie byte at +7).
struct Entry8 {
    int key;                                    // +0
    char pad4[3];
    signed char tie;                            // +7
    void check_slots(int n);                    // 0x489070
    void sort_sel(int n);                       // 0x489090
};

struct SortVal { float v; float pad; };         // 8-byte element
extern SortVal g_sort_val[];                    // 0xa75f90
extern __int64 g_ticks[];                       // 0x5e2948

// object behind a screen slot (+0x480): slots 0xb0/0xb4 are draw begin/end, 0xc4 a float setter.
struct DrawObj {
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4();
    virtual void s5(); virtual void s6(); virtual void s7(); virtual void s8(); virtual void s9();
    virtual void s10(); virtual void s11(); virtual void s12(); virtual void s13(); virtual void s14();
    virtual void s15(); virtual void s16(); virtual void s17(); virtual void s18(); virtual void s19();
    virtual void s20(); virtual void s21(); virtual void s22(); virtual void s23(); virtual void s24();
    virtual void s25(); virtual void s26(); virtual void s27(); virtual void s28(); virtual void s29();
    virtual void s30(); virtual void s31(); virtual void s32(); virtual void s33(); virtual void s34();
    virtual void s35(); virtual void s36(); virtual void s37(); virtual void s38(); virtual void s39();
    virtual void s40(); virtual void s41(); virtual void s42(); virtual void s43();
    virtual void s44();                         // 0xb0
    virtual void s45();                         // 0xb4
    virtual void s46(); virtual void s47(); virtual void s48();
    virtual void s49(float f);                  // 0xc4
};

// stream object behind f_490: slot 22 (0x58) takes two float pointers.
struct MemStream {
    virtual void s0();
    virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4(); virtual void s5();
    virtual void s6(); virtual void s7(); virtual void s8(); virtual void s9(); virtual void s10();
    virtual void s11(); virtual void s12(); virtual void s13(); virtual void s14(); virtual void s15();
    virtual void s16(); virtual void s17(); virtual void s18(); virtual void s19(); virtual void s20();
    virtual void s21();
    virtual void s58(float* a, float* b);       // slot 22 (0x58)
};

// vtable of the object itself: slot 25 (0x64) positions the screen box, slot 37 (0x94) reads the
// current screen position back.
struct UiVt {
    virtual void d00(); virtual void d01(); virtual void d02(); virtual void d03(); virtual void d04();
    virtual void d05(); virtual void d06(); virtual void d07(); virtual void d08(); virtual void d09();
    virtual void d10(); virtual void d11(); virtual void d12(); virtual void d13(); virtual void d14();
    virtual void d15(); virtual void d16(); virtual void d17(); virtual void d18(); virtual void d19();
    virtual void d20(); virtual void d21(); virtual void d22(); virtual void d23(); virtual void d24();
    virtual void set_xy(float* v);              // slot 25 (0x64)
    virtual void d26(); virtual void d27(); virtual void d28(); virtual void d29(); virtual void d30();
    virtual void d31(); virtual void d32(); virtual void d33(); virtual void d34(); virtual void d35();
    virtual void d36();
    virtual void set_ctrl(float* v);            // slot 37 (0x94)
};

// slot-0/slot-1 pair object walked by call0/call1 (stride 0x1dc).
struct Slot2 {
    virtual void f0(void* p);                   // slot 0
    virtual void f1(void* p);                   // slot 1
};

struct MissionUi {
    float f_0;                                  // 0x0 (pair[0].a / period)
    void* f_4;                                  // 0x4
    char pad8[0x20 - 8];
    int f_20;                                   // 0x20 (clock index)
    char pad24[4];
    __int64 f_28;                               // 0x28 (last tick)
    __int64 f_30;                               // 0x30
    char pad38[0x64 - 0x38];
    unsigned f_64;                              // 0x64 (alpha in the low byte)
    char pad68[0x80 - 0x68];
    float f_amount[23];                         // 0x80 amount array (0x80..0xdb)
    float f_dc;                                 // 0xdc
    float f_e0;                                 // 0xe0
    float f_e4;                                 // 0xe4
    char f_e8;                                  // 0xe8 (byte flag)
    float f_ec;                                 // 0xec
    float f_screen[2];                          // 0xf0, 0xf4 (screen offsets / base height)
    char pad_f8[0x104 - 0xf8];
    unsigned flags104;                          // 0x104
    char pad108[0x120 - 0x108];
    char f_120;                                 // 0x120
    char pad121[0x124 - 0x121];
    int f_124;                                  // 0x124
    char f_128[0x10];                           // 0x128 rgl::string name
    float f_138;                                // 0x138
    float f_13c;                                // 0x13c
    float f_140;                                // 0x140
    float f_144;                                // 0x144
    unsigned flags148;                          // 0x148
    char pad14c[0x150 - 0x14c];
    int f_150;                                  // 0x150
    char pad154[0x3b4 - 0x154];
    int f_3b4;                                  // 0x3b4
    int f_3b8;                                  // 0x3b8
    char pad3bc[0x400 - 0x3bc];
    int f_400;                                  // 0x400 (count of the +0 pair array)
    char pad404[0x480 - 0x404];
    void* f_slots[2];                           // 0x480, 0x484
    int f_flags[2];                             // 0x488 flag for f_480
    char* f_490;                                // 0x490 (float buffer / stream)

    void set_target(void* p);                   // 0x488b20
    void set_pos(int p);                        // 0x488b30
    void set_text(vector4* out, const void* name, int fmt);   // 0x488b50
    void put(void* out);                        // 0x488c70
    void f_488c90();                            // 0x488c90
    void f_488ca0();                            // 0x488ca0
    void f_488cb0();                            // 0x488cb0
    void f_488cc0();                            // 0x488cc0
    void set_speed(const float* ref, const float* lo, const float* hi);   // 0x488cd0
    void draw();                                // 0x488d50
    void call0(int i, void* p);                 // 0x488e20
    void call1(int i, void* p);                 // 0x488e40
    void push(int a, int b);                    // 0x488e60
    void tick(int idx);                         // 0x488e90
    void f_4892a0(int v);                       // 0x4892a0
    void add(unsigned p, int amt);              // 0x4892d0
    void set_colors(int a, int b);              // 0x489320
    void sub(int amt);                          // 0x489340
    void set_alpha(int a);                      // 0x489370
};

// @ 0x488b20
void MissionUi::set_target(void* p)
{
    f_150 = *(int*)p;
}

// @ 0x488b30
void MissionUi::set_pos(int p)
{
    float v[2];
    f_124 = p;
    ((UiVt*)this)->set_ctrl(v);
}

// @ 0x488b50
void MissionUi::set_text(vector4* out, const void* name, int fmt)
{
    if (flags148 & 2) {
        float v = f_screen[1] * 0.95f;
        f_144 = v;
        f_140 = v;
    }
    ((TString*)((char*)this + 0x128))->assign((const TString*)name);
    unsigned fl = flags148;
    if (fl & 1) {
        f_138 = 0.005f;
        f_13c = f_screen[1] * 0.9f - f_144 * 0.6f;
    }
    if ((fl & 0x200) && !(fl & 0x100)) {
        float c[2];
        c[0] = 1.0f;
        c[1] = 0.75f;
        ((UiVt*)this)->set_xy(c);
    }
    vector4 var;
    ((UiVt*)this)->set_ctrl((float*)&var);
    fl = flags148;
    if (fl & 0x200) {
        float v[2];
        v[0] = (fl & 0x100) ? f_screen[0] : var.x;
        v[1] = var.y;
        ((UiVt*)this)->set_xy(v);
        f_13c = var.y - f_144 * 0.63f;
        ((UiVt*)this)->set_ctrl(v);
    }
    f_120 = 1;
    out->x = var.x;
    out->y = var.y;
}

// @ 0x488c70
void MissionUi::put(void* out)
{
    ((TString*)out)->assign((const TString*)((char*)this + 0x128));
}

// @ 0x488c90
void MissionUi::f_488c90()
{
    flags104 |= 1;
}

// @ 0x488ca0
void MissionUi::f_488ca0()
{
    flags104 &= ~1u;
}

// @ 0x488cb0
void MissionUi::f_488cb0()
{
    flags104 |= 2;
}

// @ 0x488cc0
void MissionUi::f_488cc0()
{
    flags104 &= ~2u;
}

// @ 0x488cd0
void MissionUi::set_speed(const float* ref, const float* lo, const float* hi)
{
    f_e8 = 1;
    f_dc = *ref;
    f_e0 = *lo;
    f_e4 = *hi;
    float f = (1.0f / f_e0) * 3.0f;
    if (f < 0.001f) {
        f_ec = 0.001f;
        return;
    }
    if (f >= 0.95f) {
        f_ec = 0.95f;
        return;
    }
    f_ec = f;
}

// @ 0x488d50
void MissionUi::draw()
{
    for (int i = 0; i < 2; i++) {
        DrawObj* a = (DrawObj*)f_slots[i];
        f_flags[i] = 0;
        if (a) {
            a->s44();
            float d = *(float*)(f_490 + 0xf0 + i * 4) - *(float*)(f_490 + 0xf8 + i * 4);
            float v = f_screen[i];
            if (d > v) {
                f_flags[i] = 1;
                a->s45();
                if (f_flags[i] != 0)
                    a->s49(v / d);
            }
        }
    }
    float x = 0.0f, y = 0.0f;
    ((MemStream*)f_490)->s58(&x, &y);
}

// @ 0x488e20
void MissionUi::call0(int i, void* p)
{
    ((Slot2*)((char*)f_4 + i * 0x1dc))->f0(p);
}

// @ 0x488e40
void MissionUi::call1(int i, void* p)
{
    ((Slot2*)((char*)f_4 + i * 0x1dc))->f1(p);
}

// @ 0x488e60
void MissionUi::push(int a, int b)
{
    if (f_400 >= 0 && f_400 < 0x80) {
        ((int*)this)[f_400 * 2] = a;
        ((int*)this)[f_400 * 2 + 1] = b;
        f_400++;
    }
}

// @ 0x488e90
void MissionUi::tick(int idx)
{
    if (f_0 < -1.0f) {
        double d = floor((double)f_0);
        int i = (int)d;
        if (!(f_0 - (float)d <= 0.5f))
            i++;
        if (idx == i) {
            int zero = 0;
            __int64 buf[16];
            ((EmitSub*)((char*)this + 4))->emit(idx, 0, &zero, buf, -1);
        }
    } else {
        long long d = g_ticks[f_20] - f_28;
        if (!((float)d * 1e-5f < f_0) && idx >= 0) {
            f_28 = g_ticks[f_20];
            ((EmitSub*)((char*)this + 4))->emit_simple(idx);
        }
    }
}

// @ 0x488f80
int __fastcall sort_value(Entry8* e)
{
    int idx = e->key;
    if (idx < 0)
        return 0;
    float x = (float)g_items[idx].f1d8 * g_sort_val[e->tie * 5].v;
    double d = floor((double)x);
    int i = (int)d;
    if (!(x - (float)d <= 0.5f))
        i++;
    return i;
}

// @ 0x488ff0
float __stdcall speed_scale(const float* base, const float* ref, float r)
{
    float d = fabsf(r - *ref);
    if ((r > 1.001f && *ref < 1.001f) || (r < 0.999f && *ref > 0.999f))
        d = d * 5.0f + 3.0f;
    return *base / (d + 1.0f);
}

// @ 0x489060
void __fastcall clear_slots(Entry8* e)
{
    int i = 0;
    while (i < 0x60) {
        e[i].key = -1;
        i++;
    }
}

// @ 0x489070
void Entry8::check_slots(int n)
{
    int i = 0;
    if (n > 0) {
        do {
            if (this[i].key == -1)
                break;
            i++;
        } while (i < n);
    }
}

// @ 0x489090
void Entry8::sort_sel(int n)
{
    for (int j = 0; j < n - 1; j++) {
        int best = j;
        for (int k = j + 1; k < n; k++) {
            if (this[k].key > this[best].key ||
                (this[k].key == this[best].key && g_sort_val[this[k].tie * 5].v > g_sort_val[this[best].tie * 5].v))
                best = k;
        }
        Entry8 tmp = this[best];
        this[best] = this[j];
        this[j] = tmp;
    }
}

// @ 0x4892a0
void MissionUi::f_4892a0(int v)
{
    f_30 &= ~0xf;
    f_30 |= (__int64)v & 0xf;
}

// @ 0x4892d0
void MissionUi::add(unsigned p, int amt)
{
    float v = f_amount[p] + (float)amt;
    f_amount[p] = v;
    if (v < 0.0f) {
        f_amount[p] = 0.0f;
        return;
    }
    if (v > 1000.0f)
        f_amount[p] = 1000.0f;
}

// @ 0x489320
void MissionUi::set_colors(int a, int b)
{
    f_3b8 = (b & 0xff) << 16 | a;
}

// @ 0x489340
void MissionUi::sub(int amt)
{
    f_3b4 -= amt;
    if (f_3b4 < 0)
        f_3b4 = 0;
}

// @ 0x489370
void MissionUi::set_alpha(int a)
{
    int v = a;
    *(unsigned char*)&f_64 = 0;
    f_64 |= v & 0xff;
}

// @ 0x489390
int __cdecl cursor_check(unsigned v)
{
    if ((v & 7) != 0 && v != 7)
        return 0;
    return 1;
}

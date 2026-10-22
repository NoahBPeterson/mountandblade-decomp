// Slice s0048add0: the mission-UI text/slider base object (vptr 0x591c80, ctor 0x48ae20) and its two
// derived widgets -- the multi-value "flag" widget (vptr 0x591d58) and the "slider" widget (vptr 0x591fb0).
// The base carries six timing slots (g_ticks pairs at +0x10..+0x67), a name string at +0x128, text
// metrics at +0x138..+0x170 and a five-entry child list at +0x108.  The widget methods below position the
// text, test the child bounds, run the key state machine and interpolate the slider's value.
// All field names are provisional; offsets are read off this slice and s00488b20 (the same base object).
#include <windows.h>
#include <math.h>
#include <new>

// ---- shared helpers / globals ------------------------------------------------------------------------
struct tstring {                                    // 0x10 bytes, ctor 0x42bbd0
    int pos;
    char* buf;
    int cap;
    int len;
    tstring();                                      // 0x42bbd0
};
extern int g_ticks[];                               // 0x5e2948 (two dwords per clock)
extern int g_5e2930;                                // 0x5e2930 (game data pointer, +0xc8 = screen mode)
extern float g_master_volume;                       // 0x5aa014
extern float g_ui_target[2];                        // 0x5d7094 (slider target positions)
extern void* g_ui_active;                           // 0x5d7090 (slider being dragged)

// FMOD 3 imports (stdcall thunks; names from the import library).
extern "C" {
int __stdcall FSOUND_PlaySoundEx(int channel, void* sample, void* dsp, int startpaused);   // 0x5342d6
int __stdcall FSOUND_SetVolume(int channel, int vol);                                      // 0x5342ca
int __stdcall FSOUND_SetPaused(int channel, int paused);                                   // 0x5342ac
}

struct SoundMgr {                                   // 0x5d7080: the sound object used by the UI
    char pad[0x170c];
    int handle;                                     // +0x170c (the UI click sample)
};
extern SoundMgr g_sound;                            // 0x5d7080
void __fastcall play_sound(SoundMgr* m);            // 0x48b2d0

// ---- virtual interface seen through the object's vptr ------------------------------------------------
// slot 18 (+0x48): set_key_flags(int); slot 25 (+0x64)/37 (+0x94): screen position; slot 27 (+0x6c)/
// 28 (+0x70): box size/position; slot 36 (+0x90): refresh.
struct UiVt {
    virtual void d00(); virtual void d01(); virtual void d02(); virtual void d03(); virtual void d04();
    virtual void d05(); virtual void d06(); virtual void d07(); virtual void d08(); virtual void d09();
    virtual void d10(); virtual void d11(); virtual void d12(); virtual void d13(); virtual void d14();
    virtual void d15(); virtual void d16(); virtual void d17();
    virtual void set_key_flags(int f);              // +0x48
    virtual void d19(); virtual void d20(); virtual void d21(); virtual void d22(); virtual void d23();
    virtual void d24();
    virtual void set_xy(float* v);                  // +0x64
    virtual void d26();
    virtual void set_size(float* v);                // +0x6c
    virtual void set_pos(float* v);                 // +0x70
    virtual void d29(); virtual void d30(); virtual void d31(); virtual void d32(); virtual void d33();
    virtual void d34(); virtual void d35();
    virtual void refresh();                         // +0x90
    virtual void get_xy(float* v);                  // +0x94
};

struct MissionUi {
    int vptr;                                       // 0x000
    int f_4;                                        // 0x004
    int f_8;                                        // 0x008
    int f_c;                                        // 0x00c
    struct Slot { int lo, hi, zero, pad; };         // 0x10-byte clock slots
    Slot t[5];                                      // 0x010..0x05f
    int t5lo, t5hi;                                 // 0x060
    float neg[6];                                   // 0x068: six -1.0f
    char pad80[0xc0 - 0x80];                        // 0x080
    char f_c0, f_c1, f_c2, f_c3, f_c4, f_c5;        // 0x0c0..0x0c5
    char padc6[2];                                  // 0x0c6
    int f_c8, f_cc, f_d0, f_d4;                     // 0x0c8
    float f_d8, f_dc;                               // 0x0d8
    float f_e0[4];                                  // 0x0e0
    float screen[4];                                // 0x0f0
    int f_100;                                      // 0x100
    unsigned flags104;                              // 0x104
    int f_108[5];                                   // 0x108..0x11b (children)
    char f_11c, f_11d, f_11e, f_11f, f_120;         // 0x11c..0x120
    char f_121, f_122;                              // 0x121
    char pad123[0x124 - 0x123];                     // 0x123
    int f_124;                                      // 0x124
    tstring name;                                   // 0x128
    float f_138, f_13c, f_140, f_144;               // 0x138
    unsigned f_148;                                 // 0x148
    unsigned f_14c, f_150;                          // 0x14c
    int f_154;                                      // 0x154
    int f_158, f_15c, f_160;                        // 0x158
    int f_164;                                      // 0x164
    float f_168, f_16c, f_170;                      // 0x168
    int f_174;                                      // 0x174

    MissionUi();
    char always1();                                 // 0x48b170
    void set_text_pos(const float* p);              // 0x48b090
    void flag2();                                   // 0x48b180
    void flag1();                                   // 0x48b1a0
    void bounds(int a, int b, int c, int d);        // 0x488720
    void set_text(void* out, const void* s);        // 0x488b50
    void* make_text(void* out, const void* s);      // 0x48b3a0
};

struct UiA : MissionUi {                            // vptr 0x591d58
    int f_178;                                      // 0x178
    float f_17c;                                    // 0x17c
    char f_180;                                     // 0x180 (bool flag)
    char pad181[3];
    unsigned f_184;                                 // 0x184 (flags)
    UiA();
    char key(unsigned p);                           // 0x48b200
    void update(int a, int b, int c, int d);        // 0x48b320
    int type();                                     // 0x48b1f0
    int type9();                                    // 0x48b390
    int type8();                                    // 0x48b4a0
};

struct UiB : MissionUi {                            // vptr 0x591fb0
    int f_178;                                      // 0x178 (slider index)
    float f_17c;                                    // 0x17c (tolerance)
    int f_180;                                      // 0x180 (dragging)
    float f_184;                                    // 0x184 (grab offset)
    UiB();
    void show(int a, int b, int c, int d);          // 0x48b4f0
    __declspec(noinline) float value()
    {
        return (screen[f_178] - (f_17c + f_17c)) * ((f_168 - f_16c) / (f_170 - f_16c)) + f_17c;
    }
    char key(unsigned p);                           // 0x48b5a0
    void set_value(float v);                        // 0x48b6e0
    int type2();                                    // 0x48b590
};

// 0x488e90: per-child clock tick (0x30-byte child record, emitter sub-object at +4).
struct TimerChild {
    float period;                                   // +0x00
    char pad4[0x1c];
    int clock;                                      // +0x20
    int pad24;
    __int64 last;                                   // +0x28
    void tick(int idx);                             // 0x488e90
};
struct TimerList {                                  // children holder (count + array)
    int count;                                      // +0
    TimerChild* items;                              // +4
    void update(int flag);                          // 0x48add0
};

// @ 0x48add0 -------------------------------------------------------------------------------
void TimerList::update(int flag)
{
    for (int i = 0; i < count; i++) {
        items[i].tick(flag);
        if (flag > 0 && (flag & 1) && *(int*)(g_5e2930 + 0xc8) == 0x12)
            return;
    }
}

// @ 0x48ae20 -------------------------------------------------------------------------------
MissionUi::MissionUi()
{
    float a = 0.005f;
    float b = 0.024f;
    float c = 0.0f;
    float d = 1.0f;

    *(int*)this = 0x591c80;
    f_8 = 0;
    t[0].lo = g_ticks[0]; t[0].hi = g_ticks[1]; t[0].zero = 0;
    t[1].lo = g_ticks[0]; t[1].hi = g_ticks[1]; t[1].zero = 0;
    t[2].lo = g_ticks[0]; t[2].hi = g_ticks[1]; t[2].zero = 0;
    t[3].lo = g_ticks[0]; t[3].hi = g_ticks[1]; t[3].zero = 0;
    t[4].lo = g_ticks[0]; t[4].hi = g_ticks[1]; t[4].zero = 0;
    t5lo = g_ticks[0];
    t5hi = g_ticks[1];
    // name's tstring constructor runs implicitly (0x42bbd0)
    f_e0[0] = 0; f_e0[1] = 0; f_e0[2] = 0; f_e0[3] = 0;
    f_100 = 1;
    f_154 = 0;
    flags104 = 0;
    for (int i = 0; i < 5; i++) {
        f_108[i] = 0;
        (&f_11c)[i] = 0;
    }
    f_138 = a;
    f_140 = b;
    f_13c = c;
    f_14c = 0xff000000;
    f_150 = 0xff000000;
    f_11c = 1;
    f_11e = 1;
    f_164 = 1;
    screen[3] = c;
    f_144 = b;
    screen[2] = c;
    neg[0] = neg[1] = neg[2] = neg[3] = neg[4] = neg[5] = -1.0f;
    screen[0] = c;
    f_d8 = d;
    f_124 = 0;
    f_c8 = f_cc = f_d0 = f_d4 = 0;
    f_158 = f_15c = f_160 = 0;
    f_168 = f_16c = 0;
    f_121 = 0;
    f_122 = 0;
    f_c0 = f_c1 = f_c2 = f_c3 = f_c4 = f_c5 = 0;
    f_148 = 0x11;
    f_170 = d;
    screen[1] = c;
    f_dc = d;
}

// @ 0x48b090 -------------------------------------------------------------------------------
void MissionUi::set_text_pos(const float* p)
{
    float v[2];
    float o[2];
    if (f_148 & 2)
        f_148 &= ~2u;
    f_140 = p[0];
    f_144 = p[1];
    if ((f_148 & 0x200) && !(f_148 & 0x100)) {
        v[0] = 1.0f;
        v[1] = 0.75f;
        ((UiVt*)this)->set_xy(v);
    }
    if (f_148 & 0x200) {
        ((UiVt*)this)->get_xy(v);
        float m = screen[0];
        if (v[0] < m)
            m = v[0];
        o[0] = m;
        o[1] = v[1];
        ((UiVt*)this)->set_xy(o);
        f_13c = v[1] - f_144 * 0.63f;
        ((UiVt*)this)->get_xy(o);
    }
}

// @ 0x48b170 -------------------------------------------------------------------------------
char MissionUi::always1() { return 1; }

// @ 0x48b180 -------------------------------------------------------------------------------
void MissionUi::flag2()
{
    f_121 = 0;
    flags104 |= 2;
}

// @ 0x48b1a0 -------------------------------------------------------------------------------
void MissionUi::flag1()
{
    f_121 = 0;
    flags104 |= 1;
}

// @ 0x48b1c0 -------------------------------------------------------------------------------
UiA::UiA()
{
    f_178 = 0;
    f_121 = 0;
    f_180 = 0;
    f_184 = 0;
    *(int*)this = 0x591d58;
}

// @ 0x48b1f0 -------------------------------------------------------------------------------
int UiA::type() { return 1; }

// @ 0x48b200 -------------------------------------------------------------------------------
char UiA::key(unsigned p)
{
    char b = 0;
    if (f_184 & 1) {
        if (!(p & 0x10))
            goto tail;
        b = 1;
        if (!(f_184 & 2) || f_15c == 0) {
            ::play_sound(&g_sound);
            f_180 = (f_180 == 0);
            ((UiVt*)this)->set_key_flags(0x10000);
            f_15c = (f_15c == 0);
        }
    } else {
        if (p & 4)
            goto clear;
        if (p & 0x10) {
            b = 1;
            ::play_sound(&g_sound);
            f_180 = 1;
            goto tail;
        }
        if (!(p & 8) || f_180 != 1)
            goto tail;
        b = 1;
        ((UiVt*)this)->set_key_flags(0x10000);
clear:
        f_180 = 0;
    }
tail:
    ((UiVt*)this)->set_key_flags(p);
    ((UiVt*)this)->refresh();
    return b;
}

// @ 0x48b2d0 -------------------------------------------------------------------------------
void __fastcall play_sound(SoundMgr* m)
{
    int ch = FSOUND_PlaySoundEx(-1, (void*)m->handle, 0, 1);
    float v = g_master_volume * 155.0f;
    if (!(v > 0.0f))
        v -= 0.99999988f;
    FSOUND_SetVolume(ch, (int)v);
    FSOUND_SetPaused(ch, 0);
}

// @ 0x48b320 -------------------------------------------------------------------------------
void UiA::update(int a, int b, int c, int d)
{
    if (((f_184 & 1) == 0 || f_15c != 0) && ((f_184 & 1) != 0 || f_180 != 0))
        f_178 = 0;
    else
        f_178 = 1;
    f_11f = 0;
    if (f_178 != 0) {
        f_11e = 1;
        f_11d = 0;
    } else {
        f_11e = 0;
        f_11d = 1;
    }
    bounds(a, b, c, d);
}

// @ 0x48b390 -------------------------------------------------------------------------------
int UiA::type9() { return 9; }

// @ 0x48b3a0 -------------------------------------------------------------------------------
void decode_utf8(int* out_len, const tstring* s, int* buf, const int* maxn);   // 0x487b00

void* MissionUi::make_text(void* out, const void* s)
{
    float h = screen[1];
    float scale = h * 0.65f;
    int buf[0x800];
    int len;
    const int maxn = 0x800;
    float avail = screen[0] - h * 1.1805556f;
    decode_utf8(&len, (const tstring*)s, buf, &maxn);
    float w = (float)len * scale * 0.39f;
    if (w > avail)
        scale = avail / w * scale;
    float size[2];
    size[0] = scale;
    size[1] = scale;
    ((UiVt*)this)->set_size(size);
    float pos[2];
    pos[0] = 0.0f;
    pos[1] = h * 0.5f - scale * 0.3f;
    ((UiVt*)this)->set_pos(pos);
    set_text(out, s);
    return out;
}

// @ 0x48b4a0 -------------------------------------------------------------------------------
int UiA::type8() { return 8; }

// @ 0x48b4b0 -------------------------------------------------------------------------------
UiB::UiB()
{
    f_178 = 0;
    f_180 = 0;
    f_184 = 0;
    *(int*)this = 0x591fb0;
    f_17c = 0.008f;
    f_168 = 1.0f;
}

// @ 0x48b4f0 -------------------------------------------------------------------------------
void UiB::show(int a, int b, int c, int d)
{
    f_11e = 1;
    f_11c = 1;
    if (f_108[2]) {
        float v = value();
        *(float*)(f_108[2] + 0xfc + f_178 * 4) = v;
    }
    bounds(a, b, c, d);
}

// @ 0x48b590 -------------------------------------------------------------------------------
int UiB::type2() { return 2; }

// @ 0x48b5a0 -------------------------------------------------------------------------------
char UiB::key(unsigned p)
{
    char b = 0;
    float t;
    if (f_180 == 0) {
        if (p != 0x10)
            goto tail;
        b = 1;
        ::play_sound(&g_sound);
        int i = f_178;
        t = g_ui_target[i];
        float v = value() + f_e0[i];
        float c = f_17c + 0.01f;
        if (v - c < t && t < v + c) {
            f_180 = 1;
            g_ui_active = this;
            f_184 = t - v;
            goto tail;
        }
        f_184 = 0;
        t = t - f_e0[i];
        b = 1;
    } else {
        if (p == 8) {
            g_ui_active = 0;
            b = 1;
            f_180 = 0;
            ((UiVt*)this)->set_key_flags(0x10000);
            goto tail;
        }
        t = (g_ui_target[f_178] - f_184) - f_e0[f_178];
        b = 0;
    }
    set_value(t);
    ((UiVt*)this)->set_key_flags(0x10000);
tail:
    ((UiVt*)this)->set_key_flags(p);
    ((UiVt*)this)->refresh();
    return b;
}

// @ 0x48b6e0 -------------------------------------------------------------------------------
void UiB::set_value(float x)
{
    float v = (f_170 - f_16c) * ((x - f_17c) / (screen[f_178] - (f_17c + f_17c))) + f_16c;
    f_168 = v;
    if (v < f_16c) {
        f_168 = f_16c;
        return;
    }
    if (f_170 < v)
        f_168 = f_170;
}

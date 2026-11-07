// slot.cpp: TU 64 "slot" (0x4875e0..0x488560): pieces of the 0x878-byte input-device record (provisional names) -- a channel play stub, a clock helper, a status-object poll that records 0x80 flags per key, a 0x14-byte
// state block read, a key-edge test, the per-frame update (decay, restart) -- the UTF-8 decoder, and the
// dtor of the DirectSound buffer wrapper (vtable 0x591c80).
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <dinput.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "../tstring.h"
#include "../slot.h"
#include "../ui/mission_ui.h"


#define NOINL __declspec(noinline)     // keeps the definition's name visible to tools/difftest/slice.py's marker scan
#include "../../include/fx.h"
#include "../../include/dialog_rec.h"   // DialogBlock: the 0x1c-byte script block, whose runner emit 0x4b09e0 the channel play calls
#include "../float_consts.h"   // shared float constants kept in memory (0x549cdc ...)

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
Slot g_keyboard;                                    // 0xa170c8 (.bss; constructed by the compiler's static initializer, the original's 0x547b50)
Slot::Slot()
    : t828(g_ticks[0]), t838(g_ticks[0])
{
    memset(down, 0, sizeof down);
    f818 = 0;
    f81c = 0;
    axis[0] = 0;
    axis[1] = 0;
}


// DirectInput-style device interface (vtable: +0x1c poll/acquire, +0x24 GetDeviceState).
struct SomeIface {
    virtual void __stdcall dtor(char) = 0;                  // +0
    virtual void __stdcall s1() = 0;
    virtual void __stdcall s2() = 0;
    virtual void __stdcall s3() = 0;
    virtual void __stdcall s4() = 0;
    virtual void __stdcall s5() = 0;
    virtual void __stdcall s6() = 0;
    virtual int __stdcall GetStatus() = 0;                  // +0x1c
    virtual void __stdcall s8() = 0;
    virtual int __stdcall Read(unsigned n, void* buf) = 0;  // +0x24
};

// A 0x60-byte record (a "vector3-plus-state" used by the slot's clock helpers).
struct TimeKey {
    int mode;                                               // +0x00
    float f04;                                              // +0x04
    float f08;                                              // +0x08
    float f0c;                                              // +0x0c
    int pad10[0x44 / 4];
    float stamp;                                            // +0x54
    void calc();                                            // 0x487620
};


// @ 0x4875e0
char DialogBlock::play(int mode)
{
    char buf[128];
    int n = 0;
    return emit(mode, 0, &n, (__int64*)buf, -1);    // 0x4b09e0
}

// @ 0x487620
void TimeKey::calc()
{
    if (mode == 0)
        stamp = (f04 + f08 + f0c) * 10.0f;
}

// @ 0x487640
int Slot::read_keys()
{
    if (kb) {
        int keys[0x40];
        memset(keys, 0, sizeof keys);
        if (((SomeIface*)kb)->Read(0x100, keys) < 0) {
            if (((SomeIface*)kb)->GetStatus() == 0x8007001e) {
                while (((SomeIface*)kb)->GetStatus() == 0x8007001e)
                    ;
            }
        } else {
            const unsigned char* b = (const unsigned char*)keys;
            for (int i = 0; i < 0xe0; i++)
                down[i] = b[i] & 0x80;
        }
    }
    return 0;
}

// @ 0x4876e0
int Slot::read_state()
{
    if (state) {
        int buf[5] = {};
        if (((SomeIface*)state)->Read(0x14, buf) < 0) {
            if (((SomeIface*)state)->GetStatus() == 0x8007001e) {
                while (((SomeIface*)state)->GetStatus() == 0x8007001e)
                    ;
            }
        } else {
            const unsigned char* b = (const unsigned char*)buf;
            for (int i = 0; i < 8; i++)
                down[0xe0 + i] = b[12 + i] & 0x80;
            down[0xee] = 0;
            axis[0] = (float)buf[0];
            axis[1] = (float)buf[1];
            axis[2] = (float)buf[2];
            down[0xef] = 0;
            if (buf[2] > 0) {
                if ((float)(g_ticks[clock830] - t838) * 0.00001f > 0.5f) {
                    t838 = g_ticks[clock830];
                    down[0xee] = 1;
                }
            } else if (buf[2] < 0) {
                if ((float)(g_ticks[clock830] - t838) * 0.00001f > 0.5f) {
                    t838 = g_ticks[clock830];
                    down[0xef] = 1;
                }
            }
        }
    }
    return 0;
}

// @ 0x487900
int Slot::is_ready(int i)
{
    if (down[i] && !prev[i])
        return 1;
    return 0;
}

// @ 0x487930
void Slot::update(float dt)
{
    bool ch = false;

    for (int i = 0; i < 0x100; i++)
        prev[i] = down[i];
    read_keys();
    read_state();
    float f = 1.0f - dt * 0.14f;
    float fc = f;
    if (0.2f > fc)
        fc = 0.2f;
    float g = f818 * fc;
    f818 = g;
    if (0.2f > f)
        f = 0.2f;
    f81c *= f;
    if (axis[0] != 0.0f || axis[1] != 0.0f) {
        f818 = g * 0.5f + axis[0];
        f81c = f81c * 0.5f + axis[1];
    }
    for (int i = 0; i < 0x100; i++) {
        if (prev[i] != down[i])
            ch = true;
    }
    if (ch)
        t828 = g_ticks[clock820] + 80000;
    char& b = b840;
    b = 0;
    if ((float)(g_ticks[clock820] - t828) * 0.00001f > 0.0f) {
        b = 1;
        t828 = g_ticks[clock820] + 15000;
    }
}

// @ 0x487b00
// The original body uses a static convention cl 13.10 does not choose for this shape: tstring* s in edi,
// int* out_len in edx (cached in ebp), i in eax, n in esi, with buf/maxn on the stack and s->buf reloaded
// per arm. Every argument order and spelling tested gives out_len->edi / s->edx / i->esi (230 bytes vs 252),
// so the declaration is kept at the natural order and only the natural convention is reachable (see
// slot.nonmatching.txt row 00487b00).
const int g_max_chars = 8192;                           // 0x59095c
static NOINL void decode_utf8(const tstring* s, int* out_len, int* buf, const int* maxn)
{
    *out_len = 0;
    int n = s->len;
    int i = 0;
    if (n > 0) {
        do {
            if (*out_len >= *maxn)
                break;
            unsigned char c = s->buf[i];
            int ch;
            if (c >= 0xf0 && c < 0xf5) {
                const unsigned char* q = (const unsigned char*)s->buf + i;
                ch = (((((q[0] & 7) << 6) | (q[1] & 0x3f)) << 6 | (q[2] & 0x3f)) << 6) | (q[3] & 0x3f);
                i += 4;
            } else if (c >= 0xe0 && c < 0xf0) {
                const unsigned char* q = (const unsigned char*)s->buf + i;
                ch = ((((q[0] & 0xf) << 6) | (q[1] & 0x3f)) << 6) | (q[2] & 0x3f);
                i += 3;
            } else if (c >= 0xc2 && c < 0xe0) {
                const unsigned char* q = (const unsigned char*)s->buf + i;
                ch = (((q[0] & 0x1f) << 6) | (q[1] & 0x3f));
                i += 2;
            } else {
                i++;
                if (c >= 0x80)
                    continue;
                ch = (signed char)c;
            }
            buf[*out_len] = ch;
            (*out_len)++;
        } while (i < n);
    }
}
void use_decode(const tstring* s, int* buf) { int len; decode_utf8(s, &len, buf, &g_max_chars); }

// @ 0x487c00
// The widget base's destructor: the five child meshes are deleted through their virtual destructors, then the name string.
// The scalar deleting destructor 0x502300 (slot 0 of every widget table) is the compiler's.
MissionUi::~MissionUi()
{
    for (int i = 0; i < 5; i++) {
        if (tracks[i]) {
            delete tracks[i];
            tracks[i] = 0;
        }
    }
}


// ---- the animated effect (0x150-byte object, vtable of 40 slots): 0x487c40 ----------------------------------------------
extern "C" double floor(double v);     // 0x536b10 CRT floor (x87 double arg on the stack)

// float constants in the image's read-only data (each 4 bytes, cited by address).
extern const float g_half;              // 0x549d70 0.5f
extern const float g_zero;              // 0x549ca8 0.0f
extern const float g_k002;              // 0x590cd0 0.02f
extern const float g_zero;              // 0x549ca8 0.0f

// The tick table's scale: a float at 0x54a6d0 (loaded with fmul, so it must be a global, not a literal).

// @ 0x487c40
// Per-frame per-track animator. Six (clock, start-stamp) pairs produce six elapsed times
// (fild(ticks - stamp) * 1e-5f). Each track then either fades its destination toward its source over the
// period (interpolating a packed colour or a float pair and rounding through CRT floor + __ftol2), or
// takes the "restart" byte branch the matching setter raised (+0xc0..+0xc3, +0xc4, +0xc5), which XORs the
// stored source colour back in. Every active track dispatches one virtual; if anything ran, the trailing
// object refresh (+0x94) is called with the +0x58 track's pair.
void MissionUi::animate(int)
{

    bool changed = false;

    float e0 = (float)(g_ticks[tm[0].clock] - tm[0].start) * 1e-5f;
    float e1 = (float)(g_ticks[tm[1].clock] - tm[1].start) * 1e-5f;
    float e2 = (float)(g_ticks[tm[2].clock] - tm[2].start) * 1e-5f;
    float e3 = (float)(g_ticks[tm[3].clock] - tm[3].start) * 1e-5f;
    float e4 = (float)(g_ticks[tm[4].clock] - tm[4].start) * 1e-5f;
    float e5 = (float)(g_ticks[tm[5].clock] - tm[5].start) * 1e-5f;

    float pair[2];                              // the +0x58/+0x6c argument pair (reused by E and F)
    unsigned col;                               // packed colour word for +0x98/+0x9c

    // --- track E (+0x78): fade +0xa8/+0xac toward +0x88/+0x8c, hand the scaled pair to +0x6c
    if (e4 < periodE) {
        float inv = g_one / periodE;
        float part[2];                          // the fDC partial products
        part[0] = e4 * f8c;
        part[1] = (periodE - e4) * ac;
        fD8 = (e4 * f88 + (periodE - e4) * a8) * inv;
        part[1] = part[1] + part[0];
        fDC = part[1] * inv;
        pair[0] = fD8 * g_k002;
        pair[1] = fDC * g_k002;
        set_size(pair);
        changed = true;
    } else if (c4) {
        fD8 = f88;
        fDC = f8c;
        c4 = 0;
        changed = true;
        pair[0] = fD8 * g_k002;
        pair[1] = fDC * g_k002;
        set_size(pair);
    }

    // --- track A (+0x68): blend the RGB bytes of +0xb0 into +0x90, keep the alpha of +0x14c, call +0x98
    if (e0 < periodA) {
        unsigned char* vs = (unsigned char*)&valA;
        unsigned char* ss = (unsigned char*)&srcA;
        float p;
        float fl;
        int c;

        p = ((float)((unsigned)srcA & 0xff) * (periodA - e0) + (float)((unsigned)valA & 0xff) * e0) * (g_one / periodA);
        fl = floor((double)p);
        c = (int)fl;
        if (!(p - fl <= 0.5f))
            c++;
        col = (unsigned)c;

        p = ((float)ss[1] * (periodA - e0) + (float)vs[1] * e0) * (g_one / periodA);
        fl = floor((double)p);
        c = (int)fl;
        col += (unsigned)(c + !(p - fl <= 0.5f)) << 8;

        p = ((float)ss[2] * (periodA - e0) + (float)vs[2] * e0) * (g_one / periodA);
        fl = floor((double)p);
        c = (int)fl;
        col += (unsigned)(c + !(p - fl <= 0.5f)) << 16;

        col |= color14c & 0xff000000u;
        set_color(&col);
        changed = true;
    } else if (c0) {
        col = color14c ^ ((color14c ^ valA) & 0xffffffu);
        c0 = 0;
        set_color(&col);
        changed = true;
    }

    // --- track B (+0x6c): the alpha byte of +0x94 rides under the RGB of +0x14c, call +0x98
    if (e1 < periodB) {
        unsigned char* ss = (unsigned char*)&srcB;
        unsigned char* vs = (unsigned char*)&valB;
        float p = ((float)ss[3] * (periodB - e1) + (float)vs[3] * e1) * (g_one / periodB);
        float fl = floor((double)p);
        int c = (int)fl;
        if (!(p - fl <= 0.5f))
            c++;
        col = (color14c & 0xffffffu) | ((unsigned)c << 24);
        set_color(&col);
        changed = true;
    } else if (c1) {
        col = ((color14c ^ valB) & 0xffffffu) ^ valB;
        c1 = 0;
        set_color(&col);
        changed = true;
    }

    // --- track C (+0x70): the RGB bytes of +0xb8 into +0x98, alpha of +0x150, call +0x9c
    if (e2 < periodC) {
        unsigned char* vs = (unsigned char*)&valC;
        unsigned char* ss = (unsigned char*)&srcC;
        float p;
        float fl;
        int c;

        p = ((float)((unsigned)srcC & 0xff) * (periodC - e2) + (float)((unsigned)valC & 0xff) * e2) * (g_one / periodC);
        fl = floor((double)p);
        c = (int)fl;
        if (!(p - fl <= 0.5f))
            c++;
        col = (unsigned)c;

        p = ((float)ss[1] * (periodC - e2) + (float)vs[1] * e2) * (g_one / periodC);
        fl = floor((double)p);
        c = (int)fl;
        if (!(p - fl <= 0.5f))
            c++;
        col += (unsigned)c << 8;

        p = ((float)ss[2] * (periodC - e2) + (float)vs[2] * e2) * (g_one / periodC);
        fl = floor((double)p);
        c = (int)fl;
        if (!(p - fl <= 0.5f))
            c++;
        col += (unsigned)c << 16;

        col |= color150 & 0xff000000u;
        set_target(&col);
        changed = true;
    } else if (c2) {
        col = color150 ^ ((color150 ^ valC) & 0xffffffu);
        c2 = 0;
        set_target(&col);
        changed = true;
    }

    // --- track D (+0x74): the alpha byte of +0x9c rides under the RGB of +0x150, call +0x9c
    if (e3 < periodD) {
        unsigned char* ss = (unsigned char*)&srcD;
        unsigned char* vs = (unsigned char*)&valD;
        float p = ((float)ss[3] * (periodD - e3) + (float)vs[3] * e3) * (g_one / periodD);
        float fl = floor((double)p);
        int c = (int)fl;
        if (!(p - fl <= 0.5f))
            c++;
        col = (color150 & 0xffffffu) | ((unsigned)c << 24);
        set_target(&col);
        changed = true;
    } else if (c3) {
        col = ((color150 ^ valD) & 0xffffffu) ^ valD;
        c3 = 0;
        set_target(&col);
        changed = true;
    }

    // --- track F (+0x7c): fade +0xa0/+0xa4 toward +0x80/+0x84 (each half halved when +0x148 says so)
    if (e5 < periodF) {
        float h0 = 0.0f;
        float h1 = 0.0f;
        if (f148 & 0x10)
            h0 = screen[0] * 0.5f;
        if (f148 & 0x1000)
            h1 = screen[1] * 0.5f;
        pair[0] = ((f80 - h0) * e5 + (periodF - e5) * a0) * (g_one / periodF);
        pair[1] = ((periodF - e5) * a4 + (f84 - h1) * e5) * (g_one / periodF);
        move_a(pair);
        changed = true;
    } else if (c5) {
        float h0 = 0.0f;
        float h1 = 0.0f;
        if (f148 & 0x10)
            h0 = screen[0] * 0.5f;
        if (f148 & 0x1000)
            h1 = screen[1] * 0.5f;
        pair[0] = f80 - h0;
        pair[1] = f84 - h1;
        move_a(pair);
        c5 = 0;
        changed = true;
    }

    if (changed)
        get_xy(pair);
}

// Slice s00487c40: methods of a 0x150-byte animated-effect / track object (provisional name Effect).
// 0x487c40 is the per-frame animator (six clock/start track pairs, colour/position interpolation and
// virtual dispatch); 0x488560/0x4885a0 are two track "restart" setters that stamp a start time.
#include <windows.h>
#include <math.h>

extern __int64 g_time[];                // 0x5e2948 clock table (1e-5 s ticks)
extern "C" void _ReadWriteBarrier(void); // compiler barrier (no code emitted): pins the period store
                                         // ahead of the g_time load, as in the original scheduling

extern "C" double _floor(double v);     // 0x536b10 CRT floor (x87 double arg on the stack)

// float constants in the image's read-only data (each 4 bytes, cited by address).
extern const float g_one;               // 0x549cdc 1.0f
extern const float g_half;              // 0x549d70 0.5f
extern const float g_zero;              // 0x549ca8 0.0f
extern const float g_k002;              // 0x590cd0 0.02f

// The tick table's scale: a float at 0x54a6d0 (loaded with fmul, so it must be a global, not a literal).
float g_tick_scale = 1e-5f;             // 0x54a6d0

struct Effect {
    virtual void d00(int release);      // +0x00 slots follow the naming used in slice s004885e0
    virtual void d01();                 // +0x04
    virtual void d02();                 // +0x08
    virtual void d03();                 // +0x0c
    virtual void d04();                 // +0x10
    virtual void d05();                 // +0x14
    virtual void d06();                 // +0x18
    virtual void d07();                 // +0x1c
    virtual void d08();                 // +0x20
    virtual void d09();                 // +0x24
    virtual void d10();                 // +0x28
    virtual void d11();                 // +0x2c
    virtual void d12();                 // +0x30
    virtual void d13();                 // +0x34
    virtual void d14();                 // +0x38
    virtual void d15();                 // +0x3c
    virtual void d16();                 // +0x40
    virtual void d17();                 // +0x44
    virtual void d18();                 // +0x48
    virtual void d19();                 // +0x4c
    virtual void d20();                 // +0x50
    virtual void d21();                 // +0x54
    virtual void d22(void* p);          // +0x58  position/offset pair
    virtual void d23();                 // +0x5c
    virtual void d24();                 // +0x60
    virtual void d25();                 // +0x64
    virtual void d26();                 // +0x68
    virtual void d27(void* p);          // +0x6c  scaled pair
    virtual void d28();                 // +0x70
    virtual void d29();                 // +0x74
    virtual void d30();                 // +0x78
    virtual void d31();                 // +0x7c
    virtual void d32();                 // +0x80
    virtual void d33();                 // +0x84
    virtual void d34();                 // +0x88
    virtual void d35();                 // +0x8c
    virtual void d36();                 // +0x90
    virtual void d37(void* p);          // +0x94  finish/refresh
    virtual void d38(void* p);          // +0x98  colour word
    virtual void d39(void* p);          // +0x9c  colour word (second palette)
    int clock0;                         // +0x08  g_time index of track 0
    __int64 t0;                         // +0x10  g_time[clock0] start stamp
    int clock1;                         // +0x18
    __int64 t1;                         // +0x20
    int clock2;                         // +0x28
    __int64 t2;                         // +0x30
    int clock3;                         // +0x38
    __int64 t3;                         // +0x40
    int clock4;                         // +0x48
    __int64 t4;                         // +0x50
    int clock5;                         // +0x58
    __int64 t5;                         // +0x60
    float periodA;                      // +0x68
    float periodB;                      // +0x6c
    float periodC;                      // +0x70
    float periodD;                      // +0x74
    float periodE;                      // +0x78
    float periodF;                      // +0x7c
    float f80, f84;                     // +0x80, +0x84
    float f88, f8c;                     // +0x88, +0x8c
    int valA;                           // +0x90
    int valB;                           // +0x94
    int valC;                           // +0x98
    int valD;                           // +0x9c
    float a0, a4, a8, ac;               // +0xa0..+0xac
    int srcA;                           // +0xb0
    int srcB;                           // +0xb4
    int srcC;                           // +0xb8
    int srcD;                           // +0xbc
    unsigned char c0, c1, c2, c3, c4, c5;  // +0xc0..+0xc5 restart flags
    char padc6[0xd8 - 0xc6];
    float out0, out1;                   // +0xd8, +0xdc
    char pade0[0xf0 - 0xe0];
    float f0f0, f0f4;                   // +0xf0, +0xf4
    char padf8[0x148 - 0xf8];
    unsigned flags148;                  // +0x148
    unsigned color14c;                  // +0x14c
    unsigned color150;                  // +0x150

    void set_a(float a, int b);         // 0x488560
    void set_b(float a, int b);         // 0x4885a0
    void animate(float dt);             // 0x487c40
};

// @ 0x488560
void Effect::set_a(float a, int b)
{
    periodA = a;
    _ReadWriteBarrier();
    t0 = g_time[clock0];
    valA = b;
    srcA = color14c;
    c0 = 1;
}

// @ 0x4885a0
void Effect::set_b(float a, int b)
{
    periodB = a;
    _ReadWriteBarrier();
    t1 = g_time[clock1];
    valB = b;
    srcB = color14c;
    c1 = 1;
}

// @ 0x487c40
// Per-frame per-track animator. Six (clock, start-stamp) pairs produce six elapsed times
// (fild(ticks - stamp) * 1e-5f). Each track then either fades its destination toward its source over the
// period (interpolating a packed colour or a float pair and rounding through CRT floor + __ftol2), or
// takes the "restart" byte branch the matching setter raised (+0xc0..+0xc3, +0xc4, +0xc5), which XORs the
// stored source colour back in. Every active track dispatches one virtual; if anything ran, the trailing
// object refresh (+0x94) is called with the +0x58 track's pair.
void Effect::animate(float dt)
{
    (void)dt;

    float e0 = (float)(g_time[clock0] - t0) * g_tick_scale;
    float e1 = (float)(g_time[clock1] - t1) * g_tick_scale;
    float e2 = (float)(g_time[clock2] - t2) * g_tick_scale;
    float e3 = (float)(g_time[clock3] - t3) * g_tick_scale;
    float e4 = (float)(g_time[clock4] - t4) * g_tick_scale;
    float e5 = (float)(g_time[clock5] - t5) * g_tick_scale;

    bool changed = false;

    float pair[2];                              // the +0x58/+0x6c argument pair (reused by E and F)
    unsigned col;                               // packed colour word for +0x98/+0x9c

    // --- track E (+0x78): fade +0xa8/+0xac toward +0x88/+0x8c, hand the scaled pair to +0x6c
    if (e4 < periodE) {
        out0 = (e4 * f88 + (periodE - e4) * a8) * (g_one / periodE);
        out1 = ((periodE - e4) * ac + e4 * f8c) * (g_one / periodE);
        pair[0] = out0 * g_k002;
        pair[1] = out1 * g_k002;
        d27(pair);
        changed = true;
    } else if (c4) {
        out0 = f88;
        out1 = f8c;
        c4 = 0;
        pair[0] = out0 * g_k002;
        pair[1] = out1 * g_k002;
        d27(pair);
        changed = true;
    }

    // --- track A (+0x68): blend the RGB bytes of +0xb0 into +0x90, keep the alpha of +0x14c, call +0x98
    if (e0 < periodA) {
        unsigned char* vs = (unsigned char*)&valA;
        unsigned char* ss = (unsigned char*)&srcA;
        float p;
        double fl;
        int c;

        p = ((float)(srcA & 0xff) * (periodA - e0) + (float)(valA & 0xff) * e0) * (g_one / periodA);
        fl = _floor((double)p);
        c = (int)fl;
        if (!(p - fl <= 0.5f))
            c++;
        col = (unsigned)c;

        p = ((float)ss[1] * (periodA - e0) + (float)vs[1] * e0) * (g_one / periodA);
        fl = _floor((double)p);
        c = (int)fl;
        if (!(p - fl <= 0.5f))
            c++;
        col += (unsigned)c << 8;

        p = ((float)ss[2] * (periodA - e0) + (float)vs[2] * e0) * (g_one / periodA);
        fl = _floor((double)p);
        c = (int)fl;
        if (!(p - fl <= 0.5f))
            c++;
        col += (unsigned)c << 16;

        col |= color14c & 0xff000000u;
        d38(&col);
        changed = true;
    } else if (c0) {
        col = color14c ^ ((color14c ^ valA) & 0xffffffu);
        c0 = 0;
        d38(&col);
        changed = true;
    }

    // --- track B (+0x6c): the alpha byte of +0x94 rides under the RGB of +0x14c, call +0x98
    if (e1 < periodB) {
        unsigned char* ss = (unsigned char*)&srcB;
        unsigned char* vs = (unsigned char*)&valB;
        float p = ((float)ss[3] * (periodB - e1) + (float)vs[3] * e1) * (g_one / periodB);
        double fl = _floor((double)p);
        int c = (int)fl;
        if (!(p - fl <= 0.5f))
            c++;
        col = (color14c & 0xffffffu) | ((unsigned)c << 24);
        d38(&col);
        changed = true;
    } else if (c1) {
        col = ((color14c ^ valB) & 0xffffffu) ^ valB;
        c1 = 0;
        d38(&col);
        changed = true;
    }

    // --- track C (+0x70): the RGB bytes of +0xb8 into +0x98, alpha of +0x150, call +0x9c
    if (e2 < periodC) {
        unsigned char* vs = (unsigned char*)&valC;
        unsigned char* ss = (unsigned char*)&srcC;
        float p;
        double fl;
        int c;

        p = ((float)(srcC & 0xff) * (periodC - e2) + (float)(valC & 0xff) * e2) * (g_one / periodC);
        fl = _floor((double)p);
        c = (int)fl;
        if (!(p - fl <= 0.5f))
            c++;
        col = (unsigned)c;

        p = ((float)ss[1] * (periodC - e2) + (float)vs[1] * e2) * (g_one / periodC);
        fl = _floor((double)p);
        c = (int)fl;
        if (!(p - fl <= 0.5f))
            c++;
        col += (unsigned)c << 8;

        p = ((float)ss[2] * (periodC - e2) + (float)vs[2] * e2) * (g_one / periodC);
        fl = _floor((double)p);
        c = (int)fl;
        if (!(p - fl <= 0.5f))
            c++;
        col += (unsigned)c << 16;

        col |= color150 & 0xff000000u;
        d39(&col);
        changed = true;
    } else if (c2) {
        col = color150 ^ ((color150 ^ valC) & 0xffffffu);
        c2 = 0;
        d39(&col);
        changed = true;
    }

    // --- track D (+0x74): the alpha byte of +0x9c rides under the RGB of +0x150, call +0x9c
    if (e3 < periodD) {
        unsigned char* ss = (unsigned char*)&srcD;
        unsigned char* vs = (unsigned char*)&valD;
        float p = ((float)ss[3] * (periodD - e3) + (float)vs[3] * e3) * (g_one / periodD);
        double fl = _floor((double)p);
        int c = (int)fl;
        if (!(p - fl <= 0.5f))
            c++;
        col = (color150 & 0xffffffu) | ((unsigned)c << 24);
        d39(&col);
        changed = true;
    } else if (c3) {
        col = ((color150 ^ valD) & 0xffffffu) ^ valD;
        c3 = 0;
        d39(&col);
        changed = true;
    }

    // --- track F (+0x7c): fade +0xa0/+0xa4 toward +0x80/+0x84 (each half halved when +0x148 says so)
    if (e5 < periodF) {
        float h0 = g_zero;
        float h1 = g_zero;
        if (flags148 & 0x10)
            h0 = f0f0 * g_half;
        if (flags148 & 0x1000)
            h1 = f0f4 * g_half;
        pair[0] = ((f80 - h0) * e5 + (periodF - e5) * a0) * (g_one / periodF);
        pair[1] = ((periodF - e5) * a4 + (f84 - h1) * e5) * (g_one / periodF);
        d22(pair);
        changed = true;
    } else if (c5) {
        float h0 = g_zero;
        float h1 = g_zero;
        if (flags148 & 0x10)
            h0 = f0f0 * g_half;
        if (flags148 & 0x1000)
            h1 = f0f4 * g_half;
        pair[0] = f80 - h0;
        pair[1] = f84 - h1;
        d22(pair);
        c5 = 0;
        changed = true;
    }

    if (changed)
        d37(pair);
}

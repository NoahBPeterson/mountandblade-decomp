// Slice s004885e0: more methods of the animated-effect / track object seen in slice s00487c40
// (provisional name Effect). Four more track "restart" setters (0x4885e0..0x4886c0), one
// per-frame flags/palette updater (0x488720), and a batch of small accessors.
#include <windows.h>

extern __int64 g_time[];                 // 0x5e2948 clock table (1e-5 s ticks)
extern const float g_one;                // 0x549cdc 1.0f
extern "C" void _ReadWriteBarrier(void); // compiler barrier (emits no code)

// 0x40cd00 / 0x413750 are callees of 0x488720 (reached only through that function here).
struct Track                                     // 0x413750's owner (the track objects the Effect drives)
{
    void drive(const void* a, const void* xf);   // 0x413750 __thiscall (this in ecx)
};
extern void __cdecl helper40cd00(void* out);     // 0x40cd00 builds a 0x40-byte transform block

// The class holds __int64 clock stamps; MSVC inserts post-vfptr padding under default packing, which is
// exactly the original layout (clock0 at +0x08).
struct Effect {
    virtual void d00(int release);       // +0x00  delete-ish: called with push 1 (0x488ad0)
    virtual bool d01();                  // +0x04
    virtual void d02();                  // +0x08
    virtual void d03();                  // +0x0c
    virtual void d04();                  // +0x10
    virtual void d05();                  // +0x14
    virtual void d06();                  // +0x18
    virtual void d07();                  // +0x1c
    virtual void d08();                  // +0x20
    virtual void d09();                  // +0x24
    virtual void d10();                  // +0x28
    virtual void d11();                  // +0x2c
    virtual void d12();                  // +0x30
    virtual void d13();                  // +0x34
    virtual void d14();                  // +0x38
    virtual void d15();                  // +0x3c
    virtual void d16();                  // +0x40
    virtual void d17();                  // +0x44
    virtual void d18();                  // +0x48
    virtual void d19();                  // +0x4c
    virtual void d20();                  // +0x50
    virtual void d21();                  // +0x54
    virtual void d22();                  // +0x58
    virtual void d23(float* a);          // +0x5c
    virtual void d24();                  // +0x60
    virtual void d25();                  // +0x64
    virtual void d26();                  // +0x68
    virtual void d27();                  // +0x6c
    virtual void d28();                  // +0x70
    virtual void d29();                  // +0x74
    virtual void d30();                  // +0x78
    virtual void d31();                  // +0x7c
    virtual void d32(int a, int b);      // +0x80
    virtual void d33(int a);             // +0x84
    virtual void d34(void* a);           // +0x88
    virtual void d35();                  // +0x8c
    virtual void d36();                  // +0x90
    virtual void d37(float* a);          // +0x94

    int clock0;                          // +0x08 (filler at +0x04 per the packed layout)
    __int64 t0;                          // +0x10
    int clock1;                          // +0x18
    __int64 t1;                          // +0x20
    int clock2;                          // +0x28
    __int64 t2;                          // +0x30
    int clock3;                          // +0x38
    __int64 t3;                          // +0x40
    int clock4;                          // +0x48
    __int64 t4;                          // +0x50
    int clock5;                          // +0x58
    __int64 t5;                          // +0x60
    float periodA, periodB, periodC, periodD, periodE, periodF;  // +0x68..+0x7c  (24)
    float f80, f84;                      // +0x80, +0x84
    float f88, f8c;                      // +0x88, +0x8c
    int valA, valB, valC, valD;          // +0x90..+0x9c
    float a0, a4, a8, ac;                // +0xa0..+0xac
    int srcA, srcB, srcC, srcD;          // +0xb0..+0xbc
    unsigned char c0, c1, c2, c3, c4, c5; // +0xc0..+0xc5
    unsigned char pad_c6[0xc8 - 0xc6];
    unsigned fC8, fCC, fD0, fD4;         // +0xc8..+0xd4
    float fD8, fDC, fE0, fE4, fE8, fEC;  // +0xd8..+0xec
    float fF0, fF4, fF8, fFC;            // +0xf0..+0xfc
    unsigned char pad_100[0x104 - 0x100];
    unsigned f104;                       // +0x104
    void* tracks[5];                     // +0x108..+0x118 (track objects, [3] is the primary one)
    unsigned char b11c[5];               // +0x11c..+0x120 (per-track "advance" flags)
    unsigned char b121;                  // +0x121
    unsigned char pad_122[0x138 - 0x122];
    float f138, f13c;                    // +0x138, +0x13c
    unsigned char pad_140[0x148 - 0x140];
    unsigned f148;                       // +0x148
    unsigned color14c, color150;         // +0x14c, +0x150
    void* p154;                          // +0x154
    unsigned char pad_158[0x15c - 0x158];
    int v15c;                            // +0x15c
    unsigned char pad_160[0x168 - 0x160];
    float v168;                          // +0x168

    void set_c(float a, int b);          // 0x4885e0
    void set_d(float a, int b);          // 0x488620
    void set_e(float a, float* p);       // 0x488660
    void set_f(float a, float* p);       // 0x4886c0
    void update(void* a, float* t, int c, int d); // 0x488720
    void set15c(int v);                  // 0x488890
    int  get15c();                       // 0x4888a0
    void set168(int v);                  // 0x4888b0
    float get168();                      // 0x4888c0
    bool test_c8(int m);                 // 0x4888d0
    void clear_c8();                     // 0x4888e0
    bool clear_mask(int m);              // 0x4888f0
    void or_flags(int m);                // 0x488910
    void set_d0(int v);                  // 0x488930
    void set_d4(int v);                  // 0x488940
    void or_d4(int m);                   // 0x488950
    void move_a(float* p);               // 0x488970
    void move_f0(float* p);              // 0x4889b0
    void move_f8(float* p);              // 0x4889e0
    void set138(float* p);               // 0x488a10
    void set148(float* p);               // 0x488a40
    void or148(float* p);                // 0x488a70
    void run(int a, int b);              // 0x488aa0
    void set_track(int i, void* p);      // 0x488ad0
    void set14c(float* p);               // 0x488b10
};

// @ 0x4885e0
void Effect::set_c(float a, int b)
{
    periodC = a;
    _ReadWriteBarrier();
    t2 = g_time[clock2];
    valC = b;
    srcC = color150;
    c2 = 1;
}

// @ 0x488620
void Effect::set_d(float a, int b)
{
    periodD = a;
    _ReadWriteBarrier();
    t3 = g_time[clock3];
    valD = b;
    srcD = color150;
    c3 = 1;
}

// @ 0x488660
void Effect::set_e(float a, float* p)
{
    periodE = a;
    _ReadWriteBarrier();
    t4 = g_time[clock4];
    f88 = p[0];
    f8c = p[1];
    a8 = fD8;
    ac = fDC;
    c4 = 1;
}

// @ 0x4886c0
void Effect::set_f(float a, float* p)
{
    periodF = a;
    _ReadWriteBarrier();
    t5 = g_time[clock5];
    f80 = p[0];
    f84 = p[1];
    a0 = fE0;
    a4 = fE4;
    c5 = 1;
}

// @ 0x488890
void Effect::set15c(int v)
{
    v15c = v;
}

// @ 0x4888a0
int Effect::get15c()
{
    return v15c;
}

// @ 0x4888b0
void Effect::set168(int v)
{
    *(int*)&v168 = v;
}

// @ 0x4888c0
float Effect::get168()
{
    return v168;
}

// @ 0x4888d0
bool Effect::test_c8(int m)
{
    return (unsigned char)((fC8 & m) != 0);
}

// @ 0x4888e0
void Effect::clear_c8()
{
    fC8 = 0;
}

// @ 0x4888f0
bool Effect::clear_mask(int m)
{
    bool had = (fC8 & m) != 0;
    fC8 &= ~m;
    return had;
}

// @ 0x488910
void Effect::or_flags(int m)
{
    fC8 |= m;
    fCC |= m;
}

// @ 0x488930
void Effect::set_d0(int v)
{
    fD0 = v;
}

// @ 0x488940
void Effect::set_d4(int v)
{
    fD4 = v;
}

// @ 0x488950
void Effect::or_d4(int m)
{
    fD4 |= m;
}

// @ 0x488970
// Pair into +0xe8/+0xec; when +0x154 is set the callee's virtual +0x5c is tail-called with `this`
// (esp+4 overwritten), else the pair also goes into +0xe0/+0xe4 before `ret 4`.
void Effect::move_a(float* p)
{
    fE8 = p[0];
    fEC = p[1];
    if (p154)
    {
        ((Effect*)p154)->d23((float*)this);
        return;
    }
    fE0 = p[0];
    fE4 = p[1];
}

// @ 0x4889b0
// No else-branch store: the pair goes into +0xf0/+0xf4 and, when +0x154 is set, the callee is tail-called
// with `this` (esp+4 overwritten) and the entry's own `ret 4` epilogue gives the 42-byte size.
void Effect::move_f0(float* p)
{
    fF0 = p[0];
    fF4 = p[1];
    if (p154)
        ((Effect*)p154)->d23((float*)this);
}

// @ 0x4889e0
void Effect::move_f8(float* p)
{
    fF8 = p[0];
    fFC = p[1];
    if (p154)
        ((Effect*)p154)->d23((float*)this);
}

// @ 0x488a10
void Effect::set138(float* p)
{
    if (f148 & 1)
        f148 &= ~1u;
    f138 = p[0];
    f13c = p[1];
}

// @ 0x488a40
void Effect::set148(float* p)
{
    f148 = *(unsigned*)p;
    float t[2];
    d37(t);
}

// @ 0x488a70
void Effect::or148(float* p)
{
    f148 |= *(unsigned*)p;
    float t[2];
    d37(t);
}

// @ 0x488aa0
void Effect::run(int a, int b)
{
    d33(b);
    d32(a, b);
}

// @ 0x488ad0
void Effect::set_track(int i, void* p)
{
    if (tracks[i])
        ((Effect*)tracks[i])->d00(1);
    tracks[i] = p;
    d34(p);
}

// @ 0x488b10
void Effect::set14c(float* p)
{
    color14c = *(unsigned*)p;
}

// @ 0x488720
// Per-frame track driver.  Stack shape `ret 0x10` -> this + three stack words: an opaque pointer a
// (forwarded unchanged to 0x413750), the caller's time accumulator t and a trailing word the caller
// supplies.  0x40cd00 fills the 0x40-byte transform at frame-0x40 whose float 14 (+0x38, the same
// slot as frame-0x58) receives the advanced time; the +0x118 track's colour word at +0x3c is
// refreshed first (masked by 0x77ffffff when f104 bit 1 is set, else chosen by byte +0x121 and
// virtual +0x04).  Then each of the five +0x108 tracks: track 3 whose own +0x114 slot is set, while
// +0x121/d01() hold and track[1] with byte +0x11d is live, takes the plain path and otherwise only
// advances when its +0x11c byte is set; track 2 is skipped outright while +0x121/d01() hold and
// track[0] is set.  Every advance bumps *t by 1.0 and calls 0x413750(track, a, &transform).
void Effect::update(void* a, float* t, int, int)
{
    float xf[19];
    if (f104 & 1)
        return;
    if (tracks[4])
    {
        if (f104 & 2)
            *(unsigned*)((char*)tracks[4] + 0x3c) = color14c & 0x77ffffffu;
        else
            *(unsigned*)((char*)tracks[4] + 0x3c) = (b121 && d01()) ? color150 : color14c;
    }
    helper40cd00(xf);
    xf[0] = fE0;
    xf[1] = fE4;
    xf[16] = fE0;
    xf[17] = fE4;
    xf[18] = fE8;
    for (int i = 0; i < 5; i++)
    {
        if (!tracks[i])
            continue;
        if (i == 3)
        {
            if (!(b121 && d01() && !(tracks[1] && b11c[1])))
                ;                                    // fall through to the b11c[i] gate
            else
            {
                xf[17] = *t + g_one;
                *t = xf[17];
                ((Track*)tracks[3])->drive(a, xf);
                continue;
            }
        }
        else if (i == 2)
        {
            if (b121 && d01() && tracks[3])
                continue;
        }
        if (!b11c[i])
            continue;
        xf[17] = *t + g_one;
        *t = xf[17];
        ((Track*)tracks[i])->drive(a, xf);
    }
}

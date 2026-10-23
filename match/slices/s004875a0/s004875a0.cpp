// Slice s004875a0: pieces of the 0x878-byte input-device record (provisional names) -- a rounding helper,
// a channel play stub, a clock helper, a status-object poll that records 0x80 flags per key, a 0x14-byte
// state block read, a key-edge test, the per-frame update (decay, restart) -- the UTF-8 decoder, and the
// dtor of the DirectSound buffer wrapper (vtable 0x591c80).
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <dinput.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

struct tstring {
    int pos; char* buf; int cap; int len;
    ~tstring();                                                     // 0x42bc40
};

extern __int64 g_time[];        // 0x5e2948: 1e-5 s clock table

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

struct Channel {                                            // element of the channels array
    char pad[0x1c];
    char emit(int script, int zero, int* count, char* buf, int tag);   // 0x4b09e0 (thiscall ret 0x14: ecx = this is passed straight through)
    void play(int arg);                                     // 0x4875e0 (thiscall, ret 4)
};

struct BufferSub {                                          // element of the 5-pointer array at +4
    virtual void dtor(char) = 0;                            // +0
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

struct Slot {
    char pad0[4];
    SomeIface* kb;                                          // +4  (0x100-byte key device)
    SomeIface* state;                                       // +8  (0x14-byte state device)
    int key[0x100];                                         // +0xc  (key[0xe0..0xe7] + edges are filled by read_state)
    int now[0x100];                                         // +0x40c (snapshot of key taken at the top of update)
    float f80c;                                             // +0x80c
    float f810;                                             // +0x810
    float f814;                                             // +0x814
    float f818;                                             // +0x818
    float f81c;                                             // +0x81c
    int clock820;                                           // +0x820
    __int64 t828;                                           // +0x828
    int clock830;                                           // +0x830
    __int64 t838;                                           // +0x838
    char b840;                                              // +0x840
    char pad841[0x878 - 0x841];

    void update(float dt);                                  // 0x487930
    int read_keys();                                        // 0x487640
    int read_state();                                       // 0x4876e0
    int is_ready(int i);                                    // 0x487900
};

struct Buffer {                                             // vtable 0x591c80
    virtual ~Buffer();                                      // 0x487c00
    virtual void s1() {}
    virtual void s2() {}
    virtual void s3() {}
    virtual void s4() {}
    virtual void s5() {}
    virtual void s6() {}
    virtual void s7() {}
    virtual void s8() {}
    virtual void s9() {}
    virtual void s10() {}
    virtual void s11() {}
    virtual void s12() {}
    virtual void s13() {}
    virtual void s14() {}
    virtual void s15() {}
    virtual void s16() {}
    virtual void s17() {}
    virtual void s18() {}
    virtual void s19() {}
    virtual void s20() {}
    virtual void s21() {}
    virtual void s22() {}
    virtual void s23() {}
    virtual void s24() {}
    virtual void s25() {}
    virtual void s26() {}
    virtual void s27() {}
    virtual void s28() {}
    char body[0x108 - 4];
    BufferSub* subs[5];                                     // +0x108
    int pad11c[3];
    tstring name;                                           // +0x128
};

void __stdcall report_line(int idx, int a, int* flag, char* buf, int last);   // 0x4b09e0

// @ 0x4875a0 (same code as round_float 0x432540; a long long is what keeps the add/adc pair)
int __cdecl near_round(const float* p)
{
    float fl = (float)floor((double)*p);
    long long i = (long long)fl;
    if (!(*p - fl <= 0.5f))
        i += 1;
    return (int)i;
}
int __cdecl use_round(const float* p) { return near_round(p); }

// @ 0x4875e0
void Channel::play(int arg)
{
    char buf[128];
    int n = 0;
    emit(arg, 0, &n, buf, -1);
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
        if (kb->Read(0x100, keys) < 0) {
            if (kb->GetStatus() == 0x8007001e) {
                while (kb->GetStatus() == 0x8007001e)
                    ;
            }
        } else {
            const unsigned char* b = (const unsigned char*)keys;
            for (int i = 0; i < 0xe0; i++)
                key[i] = b[i] & 0x80;
        }
    }
    return 0;
}

// @ 0x4876e0
int Slot::read_state()
{
    if (state) {
        int buf[5] = {0};
        if (state->Read(0x14, buf) < 0) {
            if (state->GetStatus() == 0x8007001e) {
                while (state->GetStatus() == 0x8007001e)
                    ;
            }
        } else {
            const unsigned char* b = (const unsigned char*)buf;
            for (int i = 0; i < 8; i++)
                key[0xe0 + i] = b[12 + i] & 0x80;
            key[0xee] = 0;
            f80c = (float)buf[0];
            f810 = (float)buf[1];
            f814 = (float)buf[2];
            key[0xef] = 0;
            if (buf[2] > 0) {
                if ((float)(g_time[clock830] - t838) * 0.00001f > 0.5f) {
                    t838 = g_time[clock830];
                    key[0xee] = 1;
                }
            } else if (buf[2] < 0) {
                if ((float)(g_time[clock830] - t838) * 0.00001f > 0.5f) {
                    t838 = g_time[clock830];
                    key[0xef] = 1;
                }
            }
        }
    }
    return 0;
}

// @ 0x487900
int Slot::is_ready(int i)
{
    if (key[i] && !now[i])
        return 1;
    return 0;
}

// @ 0x487930
void Slot::update(float dt)
{
    memcpy(now, key, sizeof(key));
    read_keys();
    read_state();
    bool ch = false;
    float f = 1.0f - dt * 0.14f;
    if (0.2f > f)
        f = 0.2f;
    f818 *= f;
    if (0.2f > f)
        f = 0.2f;
    f81c *= f;
    if (f80c != 0.0f || f810 != 0.0f) {
        f818 = f818 * 0.5f + f80c;
        f81c = f81c * 0.5f + f810;
    }
    for (int i = 0; i < 0x100; i++) {
        if (now[i] != key[i])
            ch = true;
    }
    if (ch)
        t828 = g_time[clock820] + 80000;
    b840 = 0;
    if ((float)(g_time[clock820] - t828) * 0.00001f > 0.0f) {
        b840 = 1;
        t828 = g_time[clock820] + 15000;
    }
}

// @ 0x487b00  (custom register convention in the original: tstring* s in edi, int* out_len in edx)
const int g_max_chars = 8192;                           // 0x59095c
static __declspec(noinline) void decode_utf8(int* out_len, const tstring* s, int* buf, const int* maxn)
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
void use_decode(const tstring* s, int* buf) { int len; decode_utf8(&len, s, buf, &g_max_chars); }

// @ 0x487c00
Buffer::~Buffer()
{
    for (int i = 0; i < 5; i++) {
        if (subs[i]) {
            subs[i]->dtor(1);
            subs[i] = 0;
        }
    }
}

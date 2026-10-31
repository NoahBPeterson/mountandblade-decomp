// patches/play/no_input_spin.cpp: don't busy-wait on a lost DirectInput device (patch set "play"; issue #1).
//
// Changes behaviour on purpose. The faithful Slot::read_keys (0x487640) and Slot::read_state (0x4876e0) answer a failed
// GetDeviceState with `while (dev->Acquire() == DIERR_INPUTLOST) ;`, the original's own loop. Under CrossOver the device
// is lost often, and a profiled session spent 55-83% of the main thread in that spin in its worst seconds (the 50 -> 5
// FPS drops). Here a failed read calls Acquire() once and returns; the key state stays as it was for this frame, and the
// next frame reads again. The rest of both bodies is src/game/mesh_slot/slot.cpp's, unchanged (read_keys is byte-exact
// there; read_state is equivalence WEAK, so its copied body is as verified as the faithful one, no more).
//
// @ replaces 0x487640
// @ replaces 0x4876e0
#include <string.h>

struct SomeIface {                                          // DirectInput device (slot.cpp's view)
    virtual void __stdcall dtor(char) = 0;                  // +0
    virtual void __stdcall s1() = 0;
    virtual void __stdcall s2() = 0;
    virtual void __stdcall s3() = 0;
    virtual void __stdcall s4() = 0;
    virtual void __stdcall s5() = 0;
    virtual void __stdcall s6() = 0;
    virtual int __stdcall GetStatus() = 0;                  // +0x1c Acquire
    virtual void __stdcall s8() = 0;
    virtual int __stdcall Read(unsigned n, void* buf) = 0;  // +0x24 GetDeviceState
};

extern __int64 g_ticks[];                                   // 0x5e2948 clock table, 1e-5 s ticks

struct Slot {                                               // the fields of slot.cpp's Slot these two read and write
    char pad0[4];
    SomeIface* kb;                                          // +4
    SomeIface* state;                                       // +8
    int key[0x100];                                         // +0xc
    int now[0x100];                                         // +0x40c
    float f80c;                                             // +0x80c
    float f810;                                             // +0x810
    float f814;                                             // +0x814
    float f818;                                             // +0x818
    float f81c;                                             // +0x81c
    int clock820;                                           // +0x820
    __int64 t828;                                           // +0x828
    int clock830;                                           // +0x830
    __int64 t838;                                           // +0x838
    int read_keys();
    int read_state();
};

int Slot::read_keys()
{
    if (kb) {
        int keys[0x40];
        memset(keys, 0, sizeof keys);
        if (kb->Read(0x100, keys) < 0) {
            kb->GetStatus();                                // one Acquire, no spin
        } else {
            const unsigned char* b = (const unsigned char*)keys;
            for (int i = 0; i < 0xe0; i++)
                key[i] = b[i] & 0x80;
        }
    }
    return 0;
}

int Slot::read_state()
{
    if (state) {
        int buf[5] = {0};
        if (state->Read(0x14, buf) < 0) {
            state->GetStatus();                             // one Acquire, no spin
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
                if ((float)(g_ticks[clock830] - t838) * 0.00001f > 0.5f) {
                    t838 = g_ticks[clock830];
                    key[0xee] = 1;
                }
            } else if (buf[2] < 0) {
                if ((float)(g_ticks[clock830] - t838) * 0.00001f > 0.5f) {
                    t838 = g_ticks[clock830];
                    key[0xef] = 1;
                }
            }
        }
    }
    return 0;
}

//-----------------------------------------------------------------------------
// File: ui_widgets_a.cpp  (0x48b2d0..0x48b550)
//
// Desc: the UI click sound, the toggle widget's update, the base widget's text fitting (UTF-8 width measure, size
//       and offset through the vtable), the slider's constructor and show. Names provisional. Functions in
//       ascending address order (cl's bottom-up emission). Flags: /O2 /MT /Gy /TP.
//
// Boundary: UiA::key (0x48b200, previous object) calls play_sound (0x48b2d0) forward, so 0x48b2d0 starts an
// object; UiB::show (0x48b4f0) calls UiB::value (0x48b550) forward, so the object ends before 0x48b550.
//-----------------------------------------------------------------------------
#include "../tstring.h"
#include "./ui_common.h"
#include "./item_ref.h"
#include "./mission_ui.h"
#include "./ui_widgets.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../include/keyframes.h"
#include "../../include/mat4.h"
#include "../../include/mesh_prims.h"
#include "../../include/vec.h"

// FMOD 3 imports (stdcall thunks in the image's import library)
extern "C" {
int __stdcall FSOUND_PlaySoundEx(int channel, void* sample, void* dsp, int startpaused);   // 0x5342d6
int __stdcall FSOUND_SetVolume(int channel, int vol);                                      // 0x5342ca
int __stdcall FSOUND_SetPaused(int channel, int paused);                                   // 0x5342ac
}

// 0x487b00: UTF-8 decoder (another object in the original; cl gave it a custom register convention: text in EDI,
// &out_len in EDX, buffer and &capacity on the stack). A static noinline copy here reproduces that convention.
static __declspec(noinline) void decode_utf8(const tstring* s, int* out_len, int* buf, const int* maxn)   // 0x487b00
{
    *out_len = 0;
    int n = s->len;
    int i = 0;
    if (n > 0) {
        do {
            int cnt = *out_len;
            if (cnt >= *maxn)
                break;
            unsigned char c = s->buf[i];
            int ch;
            if (c >= 0xf0 && c < 0xf5) {
                ch = ((((((unsigned char)s->buf[i] & 7) << 6) | ((unsigned char)s->buf[i+1] & 0x3f)) << 6 | ((unsigned char)s->buf[i+2] & 0x3f)) << 6) | ((unsigned char)s->buf[i+3] & 0x3f);
                i += 4;
            } else if (c >= 0xe0 && c < 0xf0) {
                ch = (((((unsigned char)s->buf[i] & 0xf) << 6) | ((unsigned char)s->buf[i+1] & 0x3f)) << 6) | ((unsigned char)s->buf[i+2] & 0x3f);
                i += 3;
            } else if (c >= 0xc2 && c < 0xe0) {
                ch = ((((unsigned char)s->buf[i] & 0x1f) << 6) | ((unsigned char)s->buf[i+1] & 0x3f));
                i += 2;
            } else {
                i++;
                if (c >= 0x80)
                    continue;
                ch = (signed char)c;
            }
            buf[cnt] = ch;
            *out_len = cnt + 1;
        } while (i < n);
    }
}

// @ 0x48b2d0
void __fastcall play_sound(SoundMgr* m)
{
    int ch = FSOUND_PlaySoundEx(-1, (void*)m->handle, 0, 1);
    float v = g_master_volume * 155.0f;
    if (!(v > 0.0f))
        v -= 0.99999988f;
    FSOUND_SetVolume(ch, (int)v);
    FSOUND_SetPaused(ch, 0);
}

// @ 0x48b320
void UiA::update(void* a, float* b, int c, int d)
{
    if (((f184 & 1) == 0 || v15c != 0) && ((f184 & 1) != 0 || f180 != 0))
        f178 = 0;
    else
        f178 = 1;
    b11c[3] = 0;
    if (f178 != 0) {
        b11c[2] = 1;
        b11c[1] = 0;
    } else {
        b11c[2] = 0;
        b11c[1] = 1;
    }
    render((rgl::scene*)a, b, c, d);
}

// @ 0x48b390
int UiA::type9() { return 9; }

// @ 0x48b3a0
void* MissionUi::make_text(void* out, const tstring* s)
{
    float sw = screen[0];
    float h = screen[1];
    float scale = h * 0.65f;
    int buf[0x800];
    int len;
    const int maxn = 0x800;
    decode_utf8(s, &len, buf, &maxn);
    float w = (float)len * scale * 0.39f;
    float avail = sw - h * 1.1805556f;
    if (w > avail)
        scale = avail / w * scale;
    float size[2];
    size[0] = scale;
    size[1] = scale;
    set_size(size);
    float pos[2];
    pos[0] = 0.0f;
    pos[1] = h * 0.5f - scale * 0.3f;
    set_offset(pos);
    set_text((float*)out, s);
    return out;
}

// @ 0x48b4a0
int UiA::type8() { return 8; }

// @ 0x48b4b0
UiB::UiB()
{
    f178 = 0;
    f180 = 0;
    f184 = 0;
    f17c = 0.008f;
    v168 = 1.0f;
}

// @ 0x48b4f0
void UiB::show(void* a, float* b, int c, int d)
{
    b11c[2] = 1;
    b11c[0] = 1;
    if (tracks[2]) {
        float v = value();
        *(float*)((char*)tracks[2] + 0xfc + f178 * 4) = v;
    }
    render((rgl::scene*)a, b, c, d);
}

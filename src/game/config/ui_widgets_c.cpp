//-----------------------------------------------------------------------------
// File: ui_widgets_c.cpp  (0x48baa0..0x48be00)
//
// Desc: the progress bar's value setter, and the single-line text editor widget UiE (constructor, key handler,
//       update). Names provisional. Functions in ascending address order (cl's bottom-up emission).
//       Flags: /O2 /MT /Gy /TP.
//
// Boundary: UiD::key (0x48b960, previous object) calls set_value (0x48baa0) forward, so 0x48baa0 starts an object.
//-----------------------------------------------------------------------------
#include "../tstring.h"
#include "./ui_widgets_c.h"

// 0x487b00: UTF-8 decoder in another object, custom ABI (string in EDI, &out_len in EDX, buffer and &capacity on
// the stack, caller pops 8). Same 6-instruction asm helper as ../ui/ui_widgets_a.cpp.
extern "C" void decode_utf8_raw();          // 0x487b00
static __forceinline void decode_utf8(int* out_len, const tstring* s, int* buf, const int* maxn)
{
    __asm {
        mov edi, s
        mov edx, out_len
        push maxn
        push buf
        call decode_utf8_raw
        add esp, 8
    }
}

// @ 0x48baa0
void UiD::set_value(float x)
{
    float v = (f170 - f16c) * (x / (screen[f178] - f17c)) + f16c;
    v168 = v;
    if (v < f16c) {
        v168 = f16c;
        return;
    }
    if (f170 < v)
        v168 = f170;
}

// @ 0x48bb10
UiE::UiE()
{
    f178 = 0;
    b121 = 0;
    f180 = 0;
    f181 = 0;
    *(int*)this = 0x592208;
    f184 = 0x1c;
}

// @ 0x48bb50
char UiE::key(unsigned p)
{
    char b = 0;
    if (f180) {
        if (p == 0x1000) {
            char buf[0x2000];
            int out_len;
            int maxn = 0x800;
            b = 1;
            int ch = g_last_char;
            decode_utf8(&out_len, &name, (int*)buf, &maxn);
            if (ch >= 0x20 && out_len < f184) {
                char u[4];
                int n;
                if (ch < 0x80) {
                    u[0] = (char)ch;
                    n = 1;
                } else if (ch < 0x7ff) {
                    u[0] = (char)((ch >> 6) | 0xc0);
                    u[1] = (char)((ch & 0x3f) | 0x80);
                    n = 2;
                } else {
                    u[0] = (char)((ch >> 12) | 0xe0);
                    u[1] = (char)(((ch >> 6) & 0x3f) | 0x80);
                    u[2] = (char)((ch & 0x3f) | 0x80);
                    n = 3;
                }
                for (int i = 0; i < n; i++)
                    name.push_back(u[i]);
            }
            get_xy((float*)&out_len);
            if (g_last_char == 0xd) {
                f180 = 0;
                b = 1;
                g_ui_active = 0;
                ::play_sound(&g_sound);
                set_key_flags(0x10000);
            } else if (g_last_char == 8) {
                if (name.len > 0) {
                    int i = name.len - 1;
                    int n = 1;
                    while (i > 0 && (name.buf[i] & 0xc0) == 0x80 && n < name.len) {
                        i--;
                        n++;
                    }
                    name.erase(name.len - n, n);
                }
                get_xy((float*)&out_len);
            }
        } else if (p == 0x10) {
            f180 = 0;
            b = 1;
            g_ui_active = 0;
            ::play_sound(&g_sound);
            set_key_flags(0x10000);
        }
    } else if (p == 0x10) {
        f180 = 1;
        b = 1;
        g_ui_active = this;
        ::play_sound(&g_sound);
        if (f181) {
            tstring tmp("");
            float sz[2];
            get_text_size(&tmp, sz);
            tmp.~tstring();
            unsigned c = 0xff000000;
            set_color(&c);
            f181 = 0;
        }
    }
    set_key_flags(p);
    refresh();
    return b;
}

// @ 0x48bda0
void UiE::update(void* a, float* b, int c, int d)
{
    if (f180 != 0)
        f178 = 0;
    else
        f178 = 1;
    b11c[3] = 0;
    if (f178 != 0) {
        b11c[2] = 1;
        b11c[1] = 0;
        render((rgl::scene*)a, b, c, d);
        return;
    }
    b11c[2] = 0;
    b11c[1] = 1;
    render((rgl::scene*)a, b, c, d);
}

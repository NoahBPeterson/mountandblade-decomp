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
struct gui_manager;
extern gui_manager g_gui;                   // 0x5d7080 (f1718 = last keyboard character; strat/gui_manager.h clashes with the mesh stubs here)
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../include/keyframes.h"
#include "../../include/mat4.h"
#include "../../include/mesh_prims.h"
#include "../../include/vec.h"
#include "../../include/mesh_parts.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../rgl/mesh.h"      // (listed directly: the resolution audit scans only the source's own includes)

// 0x487b00: UTF-8 decoder (another object in the original; cl gave it a custom register convention: text in EDI,
// out_len in EDX, buffer and &capacity on the stack). A static noinline copy here reproduces that convention.
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

#include "../wret.h"
// view of MissionUi's vtable for the +0xa4 slot: it returns an 8-byte non-POD through a hidden pointer, which the
// header's get_text_size(s, out) does not express
struct TextSizeView {
    virtual void s00(); virtual void s01(); virtual void s02(); virtual void s03(); virtual void s04();
    virtual void s05(); virtual void s06(); virtual void s07(); virtual void s08(); virtual void s09();
    virtual void s10(); virtual void s11(); virtual void s12(); virtual void s13(); virtual void s14();
    virtual void s15(); virtual void s16(); virtual void s17(); virtual void s18(); virtual void s19();
    virtual void s20(); virtual void s21(); virtual void s22(); virtual void s23(); virtual void s24();
    virtual void s25(); virtual void s26(); virtual void s27(); virtual void s28(); virtual void s29();
    virtual void s30(); virtual void s31(); virtual void s32(); virtual void s33(); virtual void s34();
    virtual void s35(); virtual void s36(); virtual void s37(); virtual void s38(); virtual void s39();
    virtual void s40();
    virtual WRet get_text_size(const tstring* s);       // +0xa4
};

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
            int out_len[2];
            b = 1;
            int ch = (*(int*)((char*)&g_gui + 0x1718));
            int maxn = 0x800;
            decode_utf8(&name, out_len, (int*)buf, &maxn);
            if (ch >= 0x20 && out_len[0] < f184) {
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
                get_xy((float*)out_len);
            } else if (ch == 8) {
                if (name.len > 0) {
                    const char* q = name.buf + name.len - 1;
                    int n = 1;
                    while ((*q & 0xc0) == 0x80 && n < name.len) {
                        n++;
                        q--;
                    }
                    name.erase(name.len - n, n);
                }
                get_xy((float*)out_len);
            }
            if ((*(int*)((char*)&g_gui + 0x1718)) == 0xd)
                goto leave_edit;
        } else if (p == 0x10) {
leave_edit:
            f180 = 0;
            b = 1;
            g_ui_active = 0;
            ::play_sound(&g_ui_sound);
            set_key_flags(0x10000);
        }
    } else if (p == 0x10) {
        f180 = 1;
        b = 1;
        g_ui_active = this;
        ::play_sound(&g_ui_sound);
        if (f181) {
            {
                tstring tmp("");
                // +0xa4 returns an 8-byte non-POD (hidden pointer, never destroyed); the header's (s, out) is swapped
                ((TextSizeView*)this)->get_text_size(&tmp);
            }
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

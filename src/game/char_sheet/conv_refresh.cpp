#include "./conv_refresh.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../str_table.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../overlay.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../map_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../conv_screen.h"      // (listed directly: the resolution audit reads only the source's own includes)

#include "../game_globals.h"      // g_dialogs 0x8ed50c8 (listed directly: the resolution audit reads only the source's own includes)

// UTF-8 -> int code points (0x487b00; a static of another object with cl's register convention: text in edi, count out
// in edx, buf and &capacity on the stack, as 0x4f7926 calls it). Local copy (as in cls_map.cpp, ui_widgets_a.cpp).
static __declspec(noinline) void decode_utf8(const tstring* s, int* out_len, int* buf, const int* maxn)   // 0x487b00
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

// the value source behind the question box (+0x484 of the widget): range [f16c, f170], current f168, the animated
// slot table at +0xf0 (read through idx +0x178 / step +0x17c); slot 0x48 is set_key_flags (0x4f7d55)
struct ConvScrollSrc {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09(); virtual void v0a(); virtual void v0b();
    virtual void v0c(); virtual void v0d(); virtual void v0e(); virtual void v0f();
    virtual void v10(); virtual void v11();
    virtual void set_key_flags(int f);              // +0x48
    char pad04[0xf0 - 0x04];
    float slot[1];                                  // +0xf0
    char pad1[0x168 - 0xf4];
    float f168;                                     // +0x168 (current value)
    float f16c;                                     // +0x16c (range low)
    float f170;                                     // +0x170 (range high)
    char pad2[0x178 - 0x174];
    int idx;                                        // +0x178
    float step;                                     // +0x17c
};

// @ 0x4f73c0
void ConvScreen::refresh(int sel, int* out)
{
    if (n_lines > 1) {
        if (sel >= 0) {
            g_dialogs.select(&g_dialogs.list[sel]);
            *out = g_dialogs.get_c8();
            g_dialogs.advance();
        }
    } else if (n_lines == 1) {
        g_dialogs.select(g_dialogs.list);
    } else if (sel == -2) {
        *out = g_dialogs.get_c8();
        g_dialogs.advance();
    }

    speaker_id = g_dialogs.speaker();
    {
        char buf[0x100];
        _snprintf(buf, 0x100, "%s:", troop_text(speaker_id));
        tstring name(buf);
        struct { int a, b; } ret;                        // set_text's 8-byte result slot (original [esp+0x30])
        label_speaker->set_text(&ret, name);
    }
    {
        tstring text = g_empty_string;                   // original: copy ctor from 0x8ee30f0
        struct { int a, b; } ret;
        tstring stripped = text.strip_tags();
        label_text->set_text(&ret, stripped);
    }
    for (int i = 0; i < 0x400; i++)
        buttons[i]->hide();
    g_dialogs.advance_all(&n_lines);                     // the layout below tests the count AFTER the advance
    float acc = 0.0f;                                    // original [esp+0x24]: running y of the stacked buttons
    if (n_lines > 1) {
        input_label->hide();
        float y_a = setting_float("conv_sentence_y_a");
        for (int i = 0; i < n_lines; i++) {
            buttons[i]->show();
            tstring text = g_dialogs.line_text(i).strip_tags();
            unsigned flags = g_dialogs.line_flags(i);
            float x0 = setting_float("conv_sentence_x");
            float text_x = setting_float("conv_sentence_text_x");
            float ratio = setting_float("conv_sentence_width_ratio");
            (void)setting_float("conv_sentence_button_height");
            float size_y = setting_float("conv_sentence_size_y");
            float h = size_y * 0.65f;
            int count;
            int maxn = 0x800;
            int codes[0x800];                            // the 0x2000-byte code-point buffer of the 0x222c frame
            decode_utf8(&text, &count, codes, &maxn);
            float text_px = (float)count * h * 0.39f;    // code points * (size_y * 0.65) * 0.39
            float limit = size_y * 23.5f - 0.02f;
            float X = size_y;                            // original [esp+0x28]
            float v0, v1;
            if (limit < text_px && (flags & 0x80000)) {
                flags = 0x100;
                buttons[i]->set_flag((const int*)&flags);
                float r = text_px;
                do {
                    r -= limit;
                    X += h;
                } while (r > limit);
                v0 = 0.02f;
                v1 = X - h;
            } else {
                int flag = 0x10;
                buttons[i]->set_flag(&flag);
                v0 = 0.0f;
                v1 = size_y * 0.5f - h * 0.3f;
            }
            float sz[2];
            sz[0] = size_y * ratio;
            sz[1] = X;
            buttons[i]->set_scale(sz);                   // +0x64
            float p68[2];
            p68[0] = text_x;
            p68[1] = 0.0f;
            buttons[i]->set_y(p68);                      // +0x68
            acc = y_a + acc;
            float p6c[2];
            p6c[0] = h;
            p6c[1] = h;
            buttons[i]->set_size(p6c);                   // +0x6c
            float p70[2];
            p70[0] = v0;
            p70[1] = v1;
            buttons[i]->set_offset(p70);                 // +0x70
            float pp[2];
            pp[0] = x0;
            pp[1] = -acc;
            buttons[i]->set_pos(pp);                     // +0x58
            struct { int a, b; } ret;
            buttons[i]->set_text(&ret, text);            // +0xa4
            float q = X / size_y;
            base_mesh* bm;
            {
                tstring name("dlg_button");
                bm = (base_mesh*)((mesh_x*)g_alloc.get_object((const rgl::string*)&name))->clone();
            }
            rgl::vector4 col(1.0f, q, 1.0f);             // w is left uninitialised, as in the original
            bm->set_color(col);
            buttons[i]->set_track(2, bm);
            {
                tstring name("dlg_button_down");
                bm = (base_mesh*)((mesh_x*)g_alloc.get_object((const rgl::string*)&name))->clone();
            }
            rgl::vector4 col2(1.0f, q, 1.0f);
            bm->set_color(col2);
            buttons[i]->set_track(1, bm);
        }
    } else {
        input_label->show();
    }
    float min_y = setting_float("conv_choices_min_y");
    float m = (min_y > acc) ? min_y : acc;
    float p[2];
    p[0] = 0.0f;
    p[1] = -m;
    choice->set_y(p);                                    // +0x68 of the choices box (+0x18c)
    ConvScrollSrc* src = *(ConvScrollSrc**)((char*)question + 0x484);
    float v = (src->f170 - src->f16c) * (1000000.0f / (src->slot[src->idx] - src->step)) + src->f16c;
    src->f168 = v;
    if (v < src->f16c)
        src->f168 = src->f16c;
    else if (src->f170 < v)
        src->f168 = src->f170;
    (*(ConvScrollSrc**)((char*)question + 0x484))->set_key_flags(0x10000);
    ((MissionPanel*)question)->draw();                   // 0x488d50
}

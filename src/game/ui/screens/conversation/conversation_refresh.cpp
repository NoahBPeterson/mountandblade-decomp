#include "game/ui/screens/conversation/conversation_refresh.h"
#include "engine/math/vec.h"      // vector2
#include "engine/core/tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "game/data/str_table.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "game/ui/widgets/ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "game/ui/widgets/container.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "game/mission/camera_class.h"      // Camera (draw 0x488d50)
#include "game/campaign/map_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "game/ui/screens/conversation/conv_screen.h"      // (listed directly: the resolution audit reads only the source's own includes)

#include "game/common/game_globals.h"      // g_dialogs 0x8ed50c8 (listed directly: the resolution audit reads only the source's own includes)

// UTF-8 -> int code points (0x487b00; a static of another object with cl's register convention: text in edi, count out
// in edx, buf and &capacity on the stack, as 0x4f7926 calls it). Local copy (as in winmain_and_menu_screens.cpp, toggle_and_text_fit.cpp).
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
                ch = ((((((unsigned char)s->buf[i + 0] & 7) << 6) | ((unsigned char)s->buf[i + 1] & 0x3f)) << 6 | ((unsigned char)s->buf[i + 2] & 0x3f)) << 6) | ((unsigned char)s->buf[i + 3] & 0x3f);
                i += 4;
            } else if (c >= 0xe0 && c < 0xf0) {
                ch = (((((unsigned char)s->buf[i + 0] & 0xf) << 6) | ((unsigned char)s->buf[i + 1] & 0x3f)) << 6) | ((unsigned char)s->buf[i + 2] & 0x3f);
                i += 3;
            } else if (c >= 0xc2 && c < 0xe0) {
                ch = ((((unsigned char)s->buf[i + 0] & 0x1f) << 6) | ((unsigned char)s->buf[i + 1] & 0x3f));
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

// @ 0x4f73c0
void Rec1208::refresh(int sel, int* out)
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
        label_speaker->set_text(name);
    }
    {
        tstring text = g_dialogs.cur_text;                   // original: copy ctor from 0x8ee30f0
        tstring stripped = text.strip_tags();
        label_text->set_text(stripped);
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
            V2 p70;
            if (text_px > limit && (flags & 0x80000)) {
                flags = 0x100;
                buttons[i]->set148(flags);
                float r = text_px;
                do {
                    r -= limit;
                    X += h;
                } while (r > limit);
                p70 = V2(0.02f, X - h);
            } else {
                buttons[i]->set148(0x10);
                p70 = V2(0.0f, size_y * 0.5f - h * 0.3f);
            }
            buttons[i]->set_xy(V2(size_y * ratio, X));                   // +0x64
            buttons[i]->move_f8(V2(text_x, 0.0f));                      // +0x68
            acc = y_a + acc;
            buttons[i]->set_size(V2(h, h));                   // +0x6c
            buttons[i]->set_offset(p70);              // +0x70
            buttons[i]->move_a(V2(x0, -acc));                     // +0x58
            buttons[i]->set_text(text);            // +0xa4
            float q = X / size_y;
            rgl::mesh* bm;
            {
                tstring name("dlg_button");
                bm = ((rgl::mesh*)g_resmgr.get_object(&name))->clone();
            }
            rgl::vector4 col(1.0f, q, 1.0f);             // w is left uninitialised, as in the original
            bm->set_color(col);
            buttons[i]->set_track(2, bm);
            {
                tstring name("dlg_button_down");
                bm = ((rgl::mesh*)g_resmgr.get_object(&name))->clone();
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
    choice->move_f8(V2(0.0f, -m));                                    // +0x68 of the choices box (+0x18c)
    UiD* src = question->follow[1];                 // the scroll bar: range [f16c, f170], value v168, its axis f178 and step f17c
    float v = (src->f170 - src->f16c) * (1000000.0f / (src->rect_max[src->f178] - src->f17c)) + src->f16c;
    src->v168 = v;
    if (v < src->f16c)
        src->v168 = src->f16c;
    else if (src->f170 < v)
        src->v168 = src->f170;
    question->follow[1]->set_key_flags(0x10000);
    question->draw();                               // 0x488d50
}

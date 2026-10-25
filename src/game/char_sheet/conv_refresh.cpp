#include "./conv_refresh.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../str_table.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../overlay.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../map_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../conv_screen.h"      // (listed directly: the resolution audit reads only the source's own includes)

// @ 0x4f73c0
void ConvScreen::refresh(int sel, int* out)
{
    if (n_lines > 1) {
        if (sel < 0)
            goto tail;
        g_dialogs.select(&g_dialogs.list[sel]);
        *out = g_dialogs.get_c8();
        g_dialogs.advance();
    } else {
        if (n_lines != 1) {
            if (sel != -2)
                goto tail;
        } else {
            g_dialogs.select(g_dialogs.list);
            goto tail;
        }
        *out = g_dialogs.get_c8();
        g_dialogs.advance();
    }
    {
        static float one = 1.0f;
        float y_a = 0.0f;                                // original: [esp+0x24] (int slot reused as float)
        float text_px = 0.0f;                            // original: [esp+0x20]
tail:
        speaker_id = g_dialogs.speaker();
        {
            tstring name(troop_text(speaker_id));        // original: snprintf("%s:") into [esp+0x140]
            tstring tmp;
            label_speaker->set_text(&name, tmp);
        }
        {
            tstring text = g_empty_string;               // original: copy ctor from 0x8ee30f0
            tstring out2;
            label_text->set_text(&(text.strip_tags()), out2);
        }
        for (int i = 0; i < 0x400; i++)
            buttons[i]->hide();
        int n = n_lines;                                 // original: pre-advance count stays in [esp+0x10]
        g_dialogs.advance_all(&n_lines);
        if (n > 1) {
            input_label->hide();
            y_a = setting_float("conv_sentence_y_a");
            for (int i = 0; i < n_lines; i++) {
                buttons[i]->show();
                tstring text = g_dialogs.line_text(i).strip_tags();
                unsigned flags = g_dialogs.line_flags(i);
                float ratio = setting_float("conv_sentence_width_ratio");
                (void)setting_float("conv_sentence_button_height");
                float size_y = setting_float("conv_sentence_size_y");
                float h = size_y * 0.65f;
                int count = 0;
                text_px = decode_utf8(&text, &count, 0) * ratio * 0.39f;
                if (size_y * 23.5f - 0.02f < text_px && (flags & 0x80000)) {
                    flags = 0x100;
                    buttons[i]->set_flag((const int*)&flags);
                    do {
                        text_px -= y_a;
                        text_px += ratio;
                    } while (y_a < text_px - ratio);
                    float v = 0.02f;
                    buttons[i]->set_scale(&v);
                } else {
                    int flag = 0x10;
                    buttons[i]->set_flag(&flag);
                    y_a = i * 0.5f - ratio * 0.3f;
                }
                float sz[2];
                sz[0] = i * ratio;                       // original: [esp+0xbc]
                sz[1] = text_px;                         // original: [esp+0xc0]
                buttons[i]->set_scale(sz);
                float pos[2];
                pos[0] = y_a;                            // original: [esp+0xb4]
                pos[1] = 0.0f;                           // original: [esp+0xb8]
                buttons[i]->set_pos(pos);
                float sc[2];
                sc[0] = (float)i;                        // original: [esp+0x5c]
                sc[1] = 0.0f;                            // original: [esp+0x60]
                buttons[i]->set_scale(sc);
                float size[2];
                size[0] = text_px;                       // original: [esp+0xac]
                buttons[i]->set_size(size);
                buttons[i]->set_pos((const float*)&text_px);
                float mv[2];
                mv[0] = (float)i;                        // original: [esp+0x2c]
                mv[1] = -y_a;                            // original: [esp+0x30]
                buttons[i]->set_offset(mv);
                {
                    tstring name("dlg_button");
                    base_mesh* bm = (base_mesh*)((mesh_x*)g_alloc.get_object((const rgl::string*)&name))->clone();
                    float col[4] = { one, text_px, 0.0f, one };   // original: four 1.0f at +0x8c..+0xa4
                    bm->set_color(col);
                    buttons[i]->set_track(2, bm);
                }
                {
                    tstring name("dlg_button_down");
                    base_mesh* bm = (base_mesh*)((mesh_x*)g_alloc.get_object((const rgl::string*)&name))->clone();
                    float col[4] = { one, text_px, 0.0f, one };
                    bm->set_color(col);
                    buttons[i]->set_track(1, bm);
                }
            }
        } else {
            choice->show();
        }
        setting_float("conv_choices_min_y");
        (void)n;
    }
}


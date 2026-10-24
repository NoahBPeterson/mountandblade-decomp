#include "./terrain_page.h"

// @ 0x4f2730
void TerrainPage::update()
{
    w218->activate(enabled3);
    set_enabled_text(w218);
    w21c->activate(water2);
    set_enabled_text(w21c);
    w220->activate(enabled1);
    set_enabled_text(w220);
    w224->activate(enabled2);
    set_enabled_text(w224);

    int mode = -1;
    for (int i = 0; mode == -1; i++) {
        if (i >= 8)
            break;
        if (g_size_modes[i] == type)
            mode = i;
    }
    if (mode >= 0)
        w228->activate(mode);
    w22c->activate(size);

    btn1c0->set_value((f3c - g_140) * g_00143);
    btn1c4->set_value((f40 - g_140) * g_00143);
    btn1c8->set_value((f44 - g_10) * g_01);
    btn1cc->set_value(f48);
    btn1d0->set_value((float)water_level * g_01);
    btn1d4->set_value(f4c);

    set_hex3_text(0);
    set_hex3_text(1);
    set_hex3_text(2);
}
// @ 0x4f28a0
void TerrainPage::set_enabled_text(Widget* w)
{
    if (w->get_value())
        w->set_text(tstring("Enabled"));
    else
        w->set_text(tstring("Disabled"));
}

// @ 0x4f28f0
void TerrainPage::set_hex3_text(int i)
{
    char buf[512];
    sprintf(buf, g_fmt_d, hex3[i]);
    tstring text(buf);
    num2[i]->set_text(text);
}

// @ 0x4f2960
void TerrainPage::refresh_hex()
{
    sync_bits();
    tstring text;
    g_bits.build_hex_text(text);            // 0x4f29b0, this = the bit-word object at 0xa76630
    hexbox->set_text(text);
}

// @ 0x4f29b0
void BitWords::build_hex_text(tstring& out)
{
    char buf[12];
    out.assign(g_hex_sep);
    for (int i = 5; i >= 0; i--) {
        format_hex(w[i], buf);
        out.append(tstring(buf));
    }
}

// @ 0x4f2a10
void format_hex(unsigned v, char* out)
{
    for (int i = 7; i >= 0; i--) {
        unsigned c = v & 0xf;
        v >>= 4;
        char d = (c < 10) ? (char)(c + '0') : (char)(c + 'W');
        out[i] = d;
    }
    out[8] = 0;
}

// @ 0x4f2ae0
void TerrainPage::sync_bits()
{
    g_bits.set(0x7e, 0x7e, enabled3 != 0);
    g_bits.set(0xa2, 0xa3, water2);
    g_bits.set(0x7f, 0x7f, enabled1 != 0);
    g_bits.set(0x3f, 0x3f, enabled2 != 0);
    g_bits.set(0x60, 0x69, (unsigned)(int)f3c);
    g_bits.set(0x6a, 0x73, (unsigned)(int)f40);
    g_bits.set_a(&f44);
    g_bits.set_pct(&f48);
    g_bits.set(0, 0x1e, hex3[0]);
    g_bits.set(0x20, 0x3e, hex3[1]);
    g_bits.set(0x40, 0x5e, hex3[2]);
    g_bits.set(0x8e, 0x94, water_level);
    g_bits.set_b(&f4c);
    g_bits.set(0x9c, 0x9f, type);
    g_bits.set(0xa0, 0xa1, size);
}

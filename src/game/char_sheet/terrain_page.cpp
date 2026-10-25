#include "./terrain_page.h"
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../terrain_page.h"      // (listed directly: the resolution audit reads only the source's own includes)

// @ 0x4f2730
void TerrainPage::update()
{
    water_box->activate(water_on);
    set_enabled_text(water_box);
    river_box->activate(river_on);
    set_enabled_text(river_box);
    terrain_box->activate(terrain_on);
    set_enabled_text(terrain_box);
    terraingen_box->activate(terraingen_on);
    set_enabled_text(terraingen_box);

    int mode = -1;
    for (int i = 0; mode == -1; i++) {
        if (i >= 8)
            break;
        if (g_size_modes[i] == terrain_type)
            mode = i;
    }
    if (mode >= 0)
        type_box->activate(mode);
    size_box->activate(map_size);

    x_slider->set_value((camera_x - g_140) * g_00143);
    y_slider->set_value((camera_y - g_140) * g_00143);
    z_slider->set_value((camera_z - g_10) * g_01);
    fog_slider->set_value(fog);
    level_slider->set_value((float)water_level * g_01);
    height_slider->set_value(water_height);

    set_hex3_text(0);
    set_hex3_text(1);
    set_hex3_text(2);
}
// @ 0x4f28a0
void TerrainPage::set_enabled_text(Widget* w)
{
    if (w->get_value())
        { char r[8]; w->set_text(r, tstring("Enabled")); }
    else
        { char r[8]; w->set_text(r, tstring("Disabled")); }
}

// @ 0x4f28f0
void TerrainPage::set_hex3_text(int i)
{
    char buf[512];
    sprintf(buf, g_fmt_d, layer_bits[i]);
    tstring text(buf);
    { char r[8]; bit_fields[i]->set_text(r, text); }
}

// @ 0x4f2960
void TerrainPage::refresh_hex()
{
    sync_bits();
    tstring text;
    g_bits.build_hex_text(text);            // 0x4f29b0, this = the bit-word object at 0xa76630
    { char r[8]; hexbox->set_text(r, text); }
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
    g_bits.set(0x7e, 0x7e, water_on != 0);
    g_bits.set(0xa2, 0xa3, river_on);
    g_bits.set(0x7f, 0x7f, terrain_on != 0);
    g_bits.set(0x3f, 0x3f, terraingen_on != 0);
    g_bits.set(0x60, 0x69, (unsigned)(int)camera_x);
    g_bits.set(0x6a, 0x73, (unsigned)(int)camera_y);
    ((Terrain*)(void*)&g_bits)->set_field_a(&camera_z);   // 0x464a60
    g_bits.set_pct(fog);
    g_bits.set(0, 0x1e, layer_bits[0]);
    g_bits.set(0x20, 0x3e, layer_bits[1]);
    g_bits.set(0x40, 0x5e, layer_bits[2]);
    g_bits.set(0x8e, 0x94, water_level);
    ((Terrain*)(void*)&g_bits)->set_field_b(&water_height);   // 0x464ac0
    g_bits.set(0x9c, 0x9f, terrain_type);
    g_bits.set(0xa0, 0xa1, map_size);
}

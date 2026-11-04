#include "./terrain_page.h"
#include "../terrain_a/terrain.h"      // g_terrain 0xa76630 (listed directly: the resolution audit reads only the source's own includes)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../terrain_page.h"      // (listed directly: the resolution audit reads only the source's own includes)

// ---- data this module owns (docs/standalone_data.md) ----
const int g_region_types[8] = { 2, 3, 4, 5, 11, 12, 10, 13 };   // 0x59394c
extern unsigned __int64 g_bit[];                            // 0x5a8e68 (game_globals.h): g_bit[2] = 4 (size mask), g_bit[4] = 0x10 (type mask)
unsigned g_8ee41ec = (unsigned)g_bit[2];                       // 0x8ee41ec (no reader in this source; set at startup)
unsigned g_8ee41e8 = (unsigned)g_bit[4];                       // 0x8ee41e8

// @ 0x4f2730
void TerrainPage::update()
{
    water_box->set15c(water_on);
    set_enabled_text(water_box);
    river_box->set15c(river_on);
    set_enabled_text(river_box);
    terrain_box->set15c(terrain_on);
    set_enabled_text(terrain_box);
    terraingen_box->set15c(terraingen_on);
    set_enabled_text(terraingen_box);

    int mode = -1;
    for (int i = 0; mode == -1; i++) {
        if (i >= 8)
            break;
        if (g_region_types[i] == terrain_type)
            mode = i;
    }
    if (mode >= 0)
        type_box->set15c(mode);
    size_box->set15c(map_size);

    x_slider->set168((camera_x - 140.0f) * 0.0014285714f);
    y_slider->set168((camera_y - 140.0f) * 0.0014285714f);
    z_slider->set168((camera_z - 10.0f) * 0.01f);
    fog_slider->set168(fog);
    level_slider->set168((float)water_level * 0.01f);
    height_slider->set168(water_height);

    set_hex3_text(0);
    set_hex3_text(1);
    set_hex3_text(2);
}
// The original calls the string constructor through an inline wrapper in these two functions (cl hoists the widget's vptr load
// above the out-of-line constructor call otherwise): a constructor wrapper over the shared tstring.
struct tstr_c : tstring { tstr_c(const char* s) : tstring(s) {} };      // 0x42bbe0

// @ 0x4f28a0
void TerrainPage::set_enabled_text(MissionUi* w)
{
    if (w->get15c())
        { char r[8]; w->set_text((float*)r, &(tstr_c("Enabled"))); }
    else
        { char r[8]; w->set_text((float*)r, &(tstr_c("Disabled"))); }
}

// @ 0x4f28f0
void TerrainPage::set_hex3_text(int i)
{
    char buf[512];
    sprintf(buf, "%d", layer_bits[i]);
    tstring text(buf);
    { char r[8]; bit_fields[i]->set_text((float*)r, &(text)); }
}

// @ 0x4f2960
void TerrainPage::refresh_hex()
{
    sync_bits();
    tstring text;
    g_terrain.build_hex_text(text);            // 0x4f29b0, this = the bit-word object at 0xa76630
    { char r[8]; hexbox->set_text((float*)r, &(text)); }
}

// @ 0x4f29b0
void Terrain::build_hex_text(tstring& out)
{
    char buf[12];
    out.assign("0x");
    for (int i = 5; i >= 0; i--) {
        format_hex(bits[i], buf);
        out.append(tstr_c(buf));
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
    g_terrain.set_bits(0x7e, 0x7e, water_on != 0);
    g_terrain.set_bits(0xa2, 0xa3, river_on);
    g_terrain.set_bits(0x7f, 0x7f, terrain_on != 0);
    g_terrain.set_bits(0x3f, 0x3f, terraingen_on != 0);
    g_terrain.set_bits(0x60, 0x69, (unsigned)(int)camera_x);
    g_terrain.set_bits(0x6a, 0x73, (unsigned)(int)camera_y);
    g_terrain.set_field_a(&camera_z);   // 0x464a60
    g_terrain.set_pct(fog);
    g_terrain.set_bits(0, 0x1e, layer_bits[0]);
    g_terrain.set_bits(0x20, 0x3e, layer_bits[1]);
    g_terrain.set_bits(0x40, 0x5e, layer_bits[2]);
    g_terrain.set_bits(0x8e, 0x94, water_level);
    g_terrain.set_field_b(&water_height);   // 0x464ac0
    g_terrain.set_bits(0x9c, 0x9f, terrain_type);
    g_terrain.set_bits(0xa0, 0xa1, map_size);
}

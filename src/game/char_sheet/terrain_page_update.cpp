#include "./terrain_page_update.h"
#include "../bit_words.h"      // g_bits 0xa76630 (listed directly: the resolution audit reads only the source's own includes)
#include "../main_menu.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../eff_obj.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../terrain_page.h"      // (listed directly: the resolution audit reads only the source's own includes)

// the original inlines set_enabled_text (0x4f28a0) and refresh_hex (0x4f2960) into the two "Enable terrain" branches
// (they stay calls in the other branches); same behaviour, spelled out so the call sequence matches
#define inline_enabled_text(w) do { if ((w)->get_value()) { char r[8]; (w)->set_text(r, tstring("Enabled")); } \
    else { char r[8]; (w)->set_text(r, tstring("Disabled")); } } while (0)
#define inline_refresh_hex() do { sync_bits(); tstring t_; g_bits.build_hex_text(t_); \
    { char r_[8]; hexbox->set_text(r_, t_); } } while (0)

// @ 0x4f2d70
void TerrainPage::update(float dt)
{
    // the three terrain slots: a "select" checkbox, random bits, the %d caption of the number field
    for (int i = 0; i < 3; i++) {
        if (page_changed(layer_boxes[i])) {
            layer_bits[i] = rand() % 0x1b5781;
            set_hex3_text(i);
            sync_bits();
            tstring text;
            g_bits.build_hex_text(text);
            { char r[8]; hexbox->set_text(r, text); }
        }
        if (page_changed(bit_fields[i])) {
            tstring text;
            sync_bits();
            bit_fields[i]->get_text(&text);
            int v;
            sscanf(text.c_str(), g_fmt_d, &v);
            int lo, hi;
            if (i == 0)      { lo = 0x00; hi = 0x1e; }
            else if (i == 1) { lo = 0x20; hi = 0x3e; }
            else             { lo = 0x40; hi = 0x5e; }
            g_bits.set(lo, hi, v);
            read_fields();
            this->update();         // 0x4f2730: refresh every widget from the page fields
        }
    }

    // the two "Enable terrain" checkboxes also recaption themselves, so they form a chain of their own
    if (page_changed(terrain_box)) {
        terrain_on = terrain_box->get_value();
        inline_enabled_text(terrain_box);
        inline_refresh_hex();
        goto done;
    }
    if (page_changed(terraingen_box)) {
        terraingen_on = terraingen_box->get_value();
        inline_enabled_text(terraingen_box);
        inline_refresh_hex();
        goto done;
    }

    if (page_changed(water_box)) {
        water_on = water_box->get_value();
        set_enabled_text(water_box);
        refresh_hex();
    } else if (page_changed(river_box)) {
        river_on = river_box->get_value();
        set_enabled_text(river_box);
        refresh_hex();
    } else if (page_changed(x_slider)) {
        float zero = 0.0f;
        float v = x_slider->get_float() * g_500 + g_140;
        camera_x = clamp(&v, &zero, &g_clamp_lo);   // (v, 0, 0x593948)
        refresh_hex();
    } else if (page_changed(y_slider)) {
        float zero = 0.0f;
        float v = y_slider->get_float() * g_500 + g_140;
        camera_y = clamp(&v, &zero, &g_clamp_lo);   // (v, 0, 0x593948)
        refresh_hex();
    } else if (page_changed(z_slider)) {
        camera_z = z_slider->get_float() * g_0_01 + g_one;
        refresh_hex();
    } else if (page_changed(fog_slider)) {
        fog = fog_slider->get_float();
        refresh_hex();
    } else if (page_changed(level_slider)) {
        water_level = (int)(level_slider->get_float() * g_0_01);   // fmul; call __ftol2 (0x535a64)
        refresh_hex();
    } else if (page_changed(height_slider)) {
        water_height = height_slider->get_float();
        refresh_hex();
    } else if (page_changed(type_box)) {
        terrain_type = g_size_modes[type_box->get_value()];
        refresh_hex();
    } else if (page_changed(size_box)) {
        map_size = size_box->get_value();
        refresh_hex();
    } else if (page_changed(copy_btn)) {
        copy_clipboard();
    } else if (page_changed(paste_btn)) {
        paste_clipboard();
    }
done:
    if (page_changed(regen_btn)) {
        // "regenerate": reset the scene, arm the flash effect, rebuild the terrain and put the camera on it
        world->reset();                      // 0x421390 (rgl::world::reset)
        vector4 clear_colour(0.4f, 0.4f, 0.6f);
        world->color = clear_colour;
        world->color.w = 1.0f;
        EffObj* flash = (EffObj*)((CellGrid*)(void*)world)->spawn_wind();     // 0x478f30
        flash->pos = vector4(0.9f, 0.9f, 0.9f);
        flash->f14 = vector4(1.0f, 1.0f, 1.0f);
        flash->f24 = vector4(-1000.0f, -1000.0f, 1000.0f);
        flash->f0 = 2;
        flash->f54 = 20.0f;
        vector4 dir(1.0f, 1.0f, -1.0f);
        flash->f44 = dir;
        flash->f44.normalize();                     // inlined in the original (0x407d60 body: (0,1,0) when the length is <= 1e-10)
        sync_bits();
        ((TerrainX*)(void*)&g_terrain_obj)->generate((int)world, (int*)0, 0);   // 0x481d10
        void* water_out;                            // 0x482170 stores the new water mesh here; the original never reads it back
        ((TerrainX*)(void*)&g_terrain_obj)->regenerate_water(&water_out, 0);   // 0x482170
        int owner[4];                               // lanes 1..3 are zeroed just before 0x4814a0 (which ignores its argument); lane 0 is never set
        owner[0] = 0; owner[1] = 0; owner[2] = 0; owner[3] = 0;
        ((TerrainX*)(void*)&g_terrain_obj)->destroy_water((int)owner);   // 0x4814a0
        float x = camera_x * g_half;
        view_x = x;
        float y = camera_y * g_half;
        view_y = y;
        if (owner[1] != 0)
            free_ptr((void*)owner[1]);
    }
    simulate(dt);
    world->step(dt);
    path(dt);
    if (page_changed(back_btn) || (g_a17110 != 0 && g_a17510 == 0))
        g_main_menu.set_state(option);
}

// @ 0x4f34c0
void TerrainPage::copy_clipboard()
{
    tstring s;
    hexbox->get_text(&s);
    if (OpenClipboard(0)) {
        // GMEM_MOVEABLE | GMEM_DDESHARE (0x2002); a failure just leaves the clipboard closed again
        HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, s.len + 1);
        if (h) {
            EmptyClipboard();
            char* p = (char*)GlobalLock(h);
            const char* src = s.buf;
            if (src == 0)
                src = (const char*)&s.buf;            // an empty string reads as the string object itself
            memcpy(p, src, s.len);
            p[s.len] = 0;
            GlobalUnlock(h);
            SetClipboardData(CF_TEXT, h);
        }
        CloseClipboard();
    }
}

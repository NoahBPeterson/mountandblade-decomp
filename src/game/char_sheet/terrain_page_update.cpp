#include "./terrain_page_update.h"
#include "../main_menu.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)

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
            hexbox->put(text);
        }
        if (page_changed(bit_fields[i])) {
            tstring text;
            sync_bits();
            bit_fields[i]->take(text);
            int v;
            sscanf(text.c_str(), g_fmt_d, &v);
            int lo, hi;
            if (i == 0)      { lo = 0x00; hi = 0x1e; }
            else if (i == 1) { lo = 0x20; hi = 0x3e; }
            else             { lo = 0x40; hi = 0x5e; }
            g_bits.set(lo, hi, v);
            read_fields();
            this->update(dt);       // refresh every widget from the page fields
        }
    }

    // the two "Enable terrain" checkboxes also recaption themselves, so they form a chain of their own
    if (page_changed(terrain_box) || page_changed(terraingen_box)) {
        Widget* w;
        if (page_changed(terrain_box)) {
            terrain_on = terrain_box->get_value();
            w = terrain_box;
        } else {
            terraingen_on = terraingen_box->get_value();
            w = terraingen_box;
        }
        set_enabled_text(w);
        refresh_hex();
        goto done;
    }

    if (page_changed(water_box) || page_changed(river_box)) {
        Widget* w = page_changed(water_box) ? water_box : river_box;
        if (w == water_box)
            water_on = water_box->get_value();
        else
            river_on = river_box->get_value();
        set_enabled_text(w);
        refresh_hex();
    } else if (page_changed(x_slider)) {
        float v = x_slider->get_float() * g_500 + g_140;
        camera_x = clamp_float(&v, &g_clamp_lo, &g_140);
        refresh_hex();
    } else if (page_changed(y_slider)) {
        float v = y_slider->get_float() * g_500 + g_140;
        camera_y = clamp_float(&v, &g_clamp_lo, &g_140);
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
        copy_code();
    } else if (page_changed(paste_btn)) {
        paste_code();
    }
done:
    if (page_changed(regen_btn)) {
        // "regenerate": reset the scene, arm the flash effect, rebuild the terrain and put the camera on it
        world->reset();
        vector4 clear_colour(0.4f, 0.4f, 0.6f);
        world->color = clear_colour;
        world->color.w = 1.0f;
        EffObj* flash = ((EffMgr*)world)->create();     // 0x478f30
        flash->pos = vector4(0.9f, 0.9f, 0.9f);
        flash->f14 = vector4(1.0f, 1.0f, 1.0f);
        flash->f24 = vector4(-1000.0f, -1000.0f, 1000.0f);
        flash->f0 = 2;
        flash->f54 = 20.0f;
        vector4 dir(1.0f, 1.0f, -1.0f);
        flash->f44 = dir;
        sync_bits();
        g_terrain_obj.generate(world, 0, 0);
        vector4 owner;                              // 16 zeroed bytes; lane 1 is what 0x4814a0 hands back
        owner.x = 0.0f; owner.y = 0.0f; owner.z = 0.0f; owner.w = 0.0f;
        g_terrain_obj.regenerate_water(0, (int*)&owner.y);
        g_terrain_obj.destroy_water();
        float x = camera_x * g_half;
        view_x = x;
        float y = camera_y * g_half;
        view_y = y;
        if (owner.y != 0.0f)
            free_ptr((void*)(int)owner.y);
    }
    simulate(dt);
    world->step(dt);
    path(dt);
    if (page_changed(back_btn) || (g_a17110 != 0 && g_a17510 == 0))
        g_main_menu.choose(option);
}

// @ 0x4f34c0
void TerrainPage::copy_code()
{
    tstring s;
    hexbox->take(s);
    if (OpenClipboard(0)) {
        // GMEM_MOVEABLE | GMEM_ZEROINIT; a failure just leaves the clipboard closed again
        HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, s.len + 1);
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

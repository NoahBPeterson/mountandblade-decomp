#include "./terrain_page_update.h"
#include "../float_consts.h"
#include "../terrain_a/terrain.h"      // g_terrain 0xa76630 (listed directly: the resolution audit reads only the source's own includes)
#include "../window_mgr.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../terrain_page.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../slot.h"
#include "../terrain_b/cell_grid.h"      // CellGrid (ctor 0x478ad0, dtor 0x475d50, spawn_wind 0x478f30): the one definition

static const float g_clamp_hi = 840.0f;   // 0x593948 (the upper clamp of the camera sliders; passed by address)

// the original inlines set_enabled_text (0x4f28a0) and refresh_hex (0x4f2960) into the two "Enable terrain" branches
// (they stay calls in the other branches); same behaviour, spelled out so the call sequence matches
#define inline_enabled_text(w) do { if ((w)->get15c()) { float r[2]; tstring s_("Enabled"); (w)->set_text(r, &s_); } \
    else { float r[2]; tstring s_("Disabled"); (w)->set_text(r, &s_); } } while (0)
#define inline_refresh_hex() do { sync_bits(); tstring t_; g_terrain.build_hex_text(t_); \
    { float r_[2]; hexbox->set_text(r_, &t_); } } while (0)

// @ 0x4f2d70
void TerrainPage::frame_update(float dt)
{
    // the three terrain slots: a "select" checkbox, random bits, the %d caption of the number field
    for (int i = 0; i < 3; i++) {
        if (page_changed(layer_boxes[i])) {
            layer_bits[i] = rand() % 0x1b5781;
            set_hex3_text(i);
            sync_bits();
            tstring text;
            g_terrain.build_hex_text(text);
            { float r[2]; hexbox->set_text(r, &text); }
        }
        if (page_changed(bit_fields[i])) {
            tstring text;
            sync_bits();
            bit_fields[i]->put(&text);
            unsigned v;
            sscanf(text.c_str(), "%d", &v);
            if (i == 0)      g_terrain.set_bits(0x00, 0x1e, v);
            else if (i == 1) g_terrain.set_bits(0x20, 0x3e, v);
            else             g_terrain.set_bits(0x40, 0x5e, v);
            read_fields();
            this->update();         // 0x4f2730: refresh every widget from the page fields
        }
    }

    // the two "Enable terrain" checkboxes share one recaption path, so the merge runs it once
    MissionUi* w;
    if (page_changed(terrain_box)) {
        terrain_on = terrain_box->get15c();
        w = terrain_box;
        goto enabled;
    }
    if (page_changed(terraingen_box)) {
        terraingen_on = terraingen_box->get15c();
        w = terraingen_box;
        goto enabled;
    }

    if (page_changed(water_box)) {
        water_on = water_box->get15c();
        set_enabled_text(water_box);
        refresh_hex();
    } else if (page_changed(river_box)) {
        river_on = river_box->get15c();
        set_enabled_text(river_box);
        refresh_hex();
    } else if (page_changed(x_slider)) {
        float zero = 0.0f;
        float v = x_slider->get168() * 700.0f + 140.0f;
        camera_x = clamp(&v, &zero, &g_clamp_hi);   // (v, 0, 0x593948)
        refresh_hex();
    } else if (page_changed(y_slider)) {
        float zero = 0.0f;
        float v = y_slider->get168() * 700.0f + 140.0f;
        camera_y = clamp(&v, &zero, &g_clamp_hi);   // (v, 0, 0x593948)
        refresh_hex();
    } else if (page_changed(z_slider)) {
        camera_z = z_slider->get168() * g_hundred + g_ten;
        refresh_hex();
    } else if (page_changed(fog_slider)) {
        fog = fog_slider->get168();
        refresh_hex();
    } else if (page_changed(level_slider)) {
        water_level = (int)(level_slider->get168() * 100.0f);   // fmul; call __ftol2 (0x535a64)
        refresh_hex();
    } else if (page_changed(height_slider)) {
        water_height = height_slider->get168();
        refresh_hex();
    } else if (page_changed(type_box)) {
        terrain_type = g_region_types[type_box->get15c()];
        refresh_hex();
    } else if (page_changed(size_box)) {
        map_size = size_box->get15c();
        refresh_hex();
    } else if (page_changed(copy_btn)) {
        copy_clipboard();
    } else if (page_changed(paste_btn)) {
        paste_clipboard();
    }
    goto done;
enabled:
    inline_enabled_text(w);
    inline_refresh_hex();
done:
    if (page_changed(regen_btn)) {
        // "regenerate": reset the scene, arm the flash effect, rebuild the terrain and put the camera on it
        world->reset();                      // 0x421390 (rgl::world::reset)
        vector4 clear_colour(0.2f, 0.2f, 0.3f);
        rgl::world* wp = world;
        vector4& amb = wp->ambient;
        amb = clear_colour;
        wp->ambient.w = 1.0f;
        Light* flash = ((rgl::world*)(void*)world)->spawn_wind();     // 0x478f30
        flash->color = vector4(0.9f, 0.9f, 0.9f);
        flash->base_color.x = 1.0f;
        flash->base_color.y = 1.0f;
        flash->base_color.z = 1.0f;
        flash->pos = vector4(-1000.0f, -1000.0f, 1000.0f);
        flash->type = 2;
        flash->range = 20.0f;
        vector4 dir(1.0f, 1.0f, -1.0f);
        flash->dir = dir;
        {
            // normalize, inlined (the original is 0x407d60's body but with `1.0f / len`)
            float len = flash->dir.length();
            if (len > 1e-10f) {
                float inv = 1.0f / len;
                flash->dir.x *= inv;
                flash->dir.y *= inv;
                flash->dir.z *= inv;
            } else {
                flash->dir = vector4(0.0f, 1.0f, 0.0f);
            }
        }
        sync_bits();
        g_terrain.generate((int)world, (int*)0, 0);   // 0x481d10
        void* water_out;                            // 0x482170 stores the new water mesh here; the original never reads it back
        g_terrain.regenerate_water(&water_out, 0);   // 0x482170
        int owner[4];                               // lanes 1..3 are zeroed just before 0x4814a0 (which ignores its argument); lane 0 is never set
        owner[1] = 0; owner[2] = 0; owner[3] = 0;
        g_terrain.destroy_water((int)owner);   // 0x4814a0
        float y = camera_y * 0.5f;
        float x = camera_x * 0.5f;
        view_pos.x = x;
        view_pos.y = y;
        if (owner[1] != 0)
            free((void*)owner[1]);
    }
    simulate(dt);
    world->step(dt);
    path(dt);
    if (page_changed(back_btn) || (g_keyboard.down[15] != 0 && g_keyboard.prev[15] == 0))
        g_windows.set_state(w18);
}

// @ 0x4f34c0
void TerrainPage::copy_clipboard()
{
    tstring s;
    hexbox->put(&s);
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

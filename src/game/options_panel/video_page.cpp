// video_page.cpp (was slice s004d63b0): the options-change handler of the video-options page (the reason is 4 bytes on the stack).
//   0x4d63b0  on every "changed" signal it copies the control's value into the render-config globals
//             (0x5aa0xx / 0x5a9fxx) and then re-captions the page with 0x4d6b80; some pages also rewrite
//             their own captions (gamma, percentage values), and the two "apply"-ish controls write
//             rgl_config.txt, rebuild the resources and, when anything was changed, show the "change
//             graphics settings" explanation and return to the main menu.
// All names provisional.
//
// Control layouts (verified in s004cf7b0/s004d5ba0): +0x30 get_value, +0x38 get_float, +0x44 changed(flag),
// +0x94 set_default, +0xa4 set_text(text[, position]), +0x298 get the value's text (slider labels).
//
// Text helpers: 0x42bbe0/0x42c110 tstring ctors, 0x42bc40 dtor, 0x42bc50 assign, 0x42bd10 equals,
// 0x42c080 find, 0x42c4a0 substr, 0x42c900 insert. 0x5e293c holds the game's ID3DXEffect*: the gamma values go to
// its SetVector (COM, +0x88).
#include <deque>                          // before the headers that pull in <hash_map>: keeps deque::back()'s two loads in the original order
#include "../../include/image_addrs.h"
#include "../../rgl/options.h"   // g_options
#include "../config/config_block.h"   // g_cfg
#include <windows.h>
#include <d3dx9.h>
#include <stdio.h>
#include "../../rgl/rgl_math.h"
#include "../tstring.h"

using rgl::vector4;

#include "../str_table.h"                                                 // StrTable g_strtable (0x5e2a18)
#include "../shared_vars.h"

extern "C" int __cdecl sprintf(char*, const char*, ...);                    // 0x536f8f
extern "C" char* __cdecl _itoa(int, char*, int);                            // 0x5371f7
#include "../../include/crt.h"   // canonical: fclose (0x537168)

// ---- controls ----------------------------------------------------------------------------------------
#include "../ui_widget.h"                 // MissionUi (the UI widget interface)

#include "../config/widget_mgr.h"         // DropDown: the drop-downs keep their value labels in rows (first at +0x190)

// 0x4d63b0 polls changed(+0x44) and reads the value through +0x30 on the same control.
static inline int settings_value(MissionUi* w) { return w->get15c(); }
static inline char w_changed(MissionUi* w, int f) { return w->clear_mask(f); }
static inline float w_float(MissionUi* w) { return w->get168(); }
static inline void w_set_text(MissionUi* w, const tstring& s) { w->set_text(s); }
static inline void w_set_default(MissionUi* w) { w->get_xy(); }

// ---- other pages / objects ---------------------------------------------------------------------------
struct Game;                                                                // 0x5e2930

// 0x5d6fa0: the graphics resource manager: rgl/resource.h's resmgr, g_resmgr.
#include "../../rgl/resource.h"
// 0x5a9f98: the render config (write-out helper 0x415fb0): rgl/resource_manager.h's rgl_config, repeated here (that header's
// g_log clashes with this TU's). The member is the one the dialog's `save` of config_dialog.cpp calls too.
struct rgl_config {
    void save(FILE* f);   // 0x415fb0
};

extern ID3DXEffect* g_effect;                                                // 0x5e293c (renderer.h)

// main menu: choose() 0x495700, reset() 0x494260, 0x48a3a0 sets an explanation record.
#include "../window_mgr.h"                         // the screen id stack (0xa739f8)
#include "../window_mgr.h"                         // g_windows (0xa73980)

// 0x415160: open a user file ("rgl_config.txt").
#include "../../rgl/text_stream.h"             // mb_open_user_file (0x415160)
#include "../../rgl/options.h"   // g_options 0x5aa008
#include "../float_consts.h"   // shared float constants kept in memory
#include "../config/config_block.h"
#include "../../rgl/options.h"
// 0x4b99c0: settings refresh.
void run_settings_probe();                            // 0x4b99c0 (options_dlg.h; that header's winsock2.h cannot follow windows.h)
// 0x4bc090 / 0x4bc100: set the first / second graphics quality index (clamped to 5).

// 0x4d6b80 (Recf0::update_perf) and 0x4d6dd0 (head_rec::set_enabled_text) are keys_page.cpp's.

// 0x4bc090 / 0x4bc100: set the first / second graphics quality index.
static inline void set_quality(int v, int which)
{
    Config* c = (Config*)(void*)&g_options;
    if (which) c->set_texture_quality(v); else c->set_triple_buffering(v);
}

// ---- the page ----------------------------------------------------------------------------------------
#include "../screen_records.h"       // Recf0: the video options page record (slot 3 is update 0x4d63b0)

// @ 0x4d63b0
void Recf0::frame_update(float)
{
    if (blood->clear_mask(0x10000)) {
        g_options.blood = settings_value(blood);
        update_perf();
    }

    if (change->clear_mask(0x10000)) {
        // switching the renderer: replace the two menu captions and always return to the main menu
        {
            tstring explanation("ui_change_directx_explanation");
            tstring name("DirectX");
            tstring* e = g_strtable.lookup(explanation);   // 0x4d6418: the explanation is the key; "DirectX" is the header
            NamedRec* r = g_windows.box();                  // 0x4d641d: edi = the dialog box record, loaded from [0xa73980]
            r->set_inline(&name, e);                        // header (+0x68) = "DirectX", title (+0x1c) = the looked-up text
        }
        g_windows.open(0);
    }

    if (quality[0]->clear_mask(0x10000)) {
        g_options.char_detail = settings_value(quality[0]);
        set_enabled_text(quality[0]);
        update_perf();
    }
    if (quality[1]->clear_mask(0x10000)) {
        g_options.i28 = settings_value(quality[1]);
        set_enabled_text(quality[1]);
        update_perf();
    }
    if (quality[2]->clear_mask(0x10000)) {
        g_options.i2c = settings_value(quality[2]);
        set_enabled_text(quality[2]);
        f_ec = 1;
        update_perf();
    }
    if (plant_shadows->clear_mask(0x10000)) {
        g_options.i7c = settings_value(plant_shadows);
        f_ec = 1;
        update_perf();
    }
    if (quality[3]->clear_mask(0x10000)) {
        g_options.i20 = settings_value(quality[3]);
        set_enabled_text(quality[3]);
        update_perf();
    }
    if (quality[4]->clear_mask(0x10000)) {
        g_options.i38 = settings_value(quality[4]);
        set_enabled_text(quality[4]);
        update_perf();
    }
    if (quality[5]->clear_mask(0x10000)) {
        g_cfg.fake_reflections = settings_value(quality[5]);
        set_enabled_text(quality[5]);
        f_ec = 1;
        update_perf();
    }
    if (shadow_quality->clear_mask(0x10000)) {
        g_cfg.shadow_quality_req = 2 - settings_value(shadow_quality);
        f_ec = 1;
        update_perf();
    }
    if (corpses->clear_mask(0x10000)) {
        set_quality(5 - settings_value(corpses), 0);
        update_perf();
    }
    if (ragdolls->clear_mask(0x10000)) {
        set_quality(5 - settings_value(ragdolls), 1);
        update_perf();
    }

    if (ui_scale->clear_mask(0x10000)) {
        // monitor gamma: hand the value and its inverse to the renderer, and caption it
        g_options.scale_x = ui_scale->get168();
        // the two effect vectors are D3DXVECTOR4s built with the four-float constructor: the gamma lanes are integer
        // copies of the stored option, the inverse is computed once and stored x..w
        D3DXVECTOR4 v(g_options.scale_x, g_options.scale_x, g_options.scale_x, g_options.scale_x);
        float inv = g_one / g_options.scale_x;
        D3DXVECTOR4 vi(inv, inv, inv, inv);
        g_effect->SetVector("output_gamma", (const D3DXVECTOR4*)&v);
        g_effect->SetVector("output_gamma_inv", (const D3DXVECTOR4*)&vi);
        char buf[0x80];
        _itoa((int)(g_options.scale_x * 100.0f), buf, 10);
        tstring text(buf);
        {
            tstring dot(".");
            text.insert(1, dot);          // 0x4d6726: "220" -> "2.20" (the point goes after the first digit)
        }
        if (text.equals("2.20")) {        // 0x4d6734: the default gamma (the literal at 0x596820) shows as ui_default
            tstring k("ui_default");
            text = *g_strtable.lookup(k);
        }
        val_gamma->set_text(text);
        val_gamma->get_xy();
    }

    if (ragdoll_limit->clear_mask(0x10000)) {
        g_options.i34 = (int)ragdoll_limit->get168();
        char buf[0x80];
        {
            tstring key("ui_percentage_value");
            sprintf(buf, g_strtable.lookup(key)->c_str(), g_options.i34);
        }
        {
            tstring text(buf);
            val_grass_density->set_text(text);
        }
        val_grass_density->get_xy();
        f_ec = 1;
        update_perf();
    }

    if (y_scale->clear_mask(0x10000)) {
        g_options.scale_y = y_scale->get168();
        update_perf();
    }
    if (z_scale->clear_mask(0x10000)) {
        g_options.scale_z = z_scale->get168();
        update_perf();
    }
    if (days->clear_mask(0x10000)) {
        g_cfg.texture_detail = (int)days->get168();
        char buf[0x80];
        {
            tstring key("ui_percentage_value");
            sprintf(buf, g_strtable.lookup(key)->c_str(), g_cfg.texture_detail);
        }
        {
            tstring text(buf);
            val_texture_detail->set_text(text);
        }
        val_texture_detail->get_xy();
        update_perf();
    }

    if (resolution->clear_mask(0x10000)) {
        // texture detail: "WxH" labels, split at the 'x' and taken as integers
        int idx = settings_value(resolution);
        tstring label(resolution->rows[idx]);
        int a = label.find('x', 0);
        int b = label.find(' ', 0);
        g_cfg.display_width = atol(label.substr(0, a).c_str());     // _atol, not _atoi
        g_cfg.display_height = atol(label.substr(a + 1, b).c_str());
        run_settings_probe();
        update_perf();
    }

    if (done->clear_mask(0x10000)) {
        f_ed = 1;
        if (day_saved != g_cfg.texture_detail) {
            g_resmgr.unload_textures();
            g_resmgr.load_textures();
        }
        void* f = mb_open_user_file("rgl_config.txt", "w");
        ((rgl_config*)&g_cfg)->save((FILE*)f);
        fclose((FILE*)f);
        if (f_ec) {
            {
                // 0x4d6ab9: the key is looked up and the text goes in as the body; the title is the empty string
                tstring key("ui_change_graphics_settings_explanation");
                tstring title("");
                tstring* e = g_strtable.lookup(key);
                g_windows.box()->setup(&title, e);
            }
            g_windows.open(0);
        }
    }

    if (f_ed) {
        // the last menu entry is checked -> go back to the page that opened the options
        if (current_screen_id() != 0)
            g_windows.set_state(w18);
    }
}

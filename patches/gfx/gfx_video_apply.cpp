// patches/gfx/gfx_video_apply.cpp: the Video Options page's frame_update with the gfx rows (patch set "gfx", opt-in).
//
// CHANGES BEHAVIOUR on purpose: replaces Recf0::frame_update (0x4d63b0), the page's "changed" handler: the faithful body
// of src/game/ui/screens/options/video_page.cpp (its helpers and includes copied as they are) with two additions, marked
// "gfx": the four rows gfx_video_page.cpp adds are read into g_gfx_saved when they change, and after Done has written
// rgl_config.txt (rgl_config::save, gfx_config.cpp, writes g_gfx_saved) gfx_apply_saved hands them to the device,
// which takes them at the next Present: anisotropy re-set on every sampler, the scene targets remade at the new scale.
// So the page applies them at once; it needs neither a restart nor its "new area" message.
#include "../../src/platform/image_addrs.h"
#include "../../src/game/config/options.h"   // g_options
#include "../../src/game/config/config_block.h"   // g_cfg
#include <windows.h>
#include <d3dx9.h>
#include <stdio.h>
#include "../../src/engine/math/rgl_math.h"
#include "../../src/engine/core/tstring.h"

using rgl::vector4;

#include "../../src/game/data/str_table.h"                                                 // StrTable g_strtable (0x5e2a18)
#include "../../src/game/common/shared_vars.h"

extern "C" int __cdecl sprintf(char*, const char*, ...);                    // 0x536f8f
extern "C" char* __cdecl _itoa(int, char*, int);                            // 0x5371f7
#include "../../src/engine/core/crt.h"   // canonical: fclose (0x537168)

// ---- controls ----------------------------------------------------------------------------------------
#include "../../src/game/ui/widgets/ui_widget.h"                 // MissionUi (the UI widget interface)

// sliders keep their value labels at +0x190: rows of 16 bytes, copied with the tstring copy ctor.
static inline const tstring* value_label(MissionUi* w, int index)
{
    return (const tstring*)(*(char**)((char*)w + 0x190) + index * 0x10);
}

// 0x4d63b0 polls changed(+0x44) and reads the value through +0x30 on the same control.
static inline int settings_value(MissionUi* w) { return w->get15c(); }
static inline char w_changed(MissionUi* w, int f) { return w->clear_mask(f); }
static inline float w_float(MissionUi* w) { return w->get168(); }
static inline void w_set_text(MissionUi* w, const tstring& s) { char r[8]; w->set_text((float*)r, &(s)); }
static inline void w_set_default(MissionUi* w) { char r[8]; w->get_xy((float*)r); }

// ---- other pages / objects ---------------------------------------------------------------------------
struct Game;                                                                // 0x5e2930

// 0x5d6fa0: the graphics resource manager: engine/resource/resource.h's resmgr, g_resmgr.
#include "../../src/engine/resource/resource.h"
// 0x5a9f98: the render config (write-out helper 0x415fb0): engine/resource/resource_manager_init.h's rgl_config, repeated here (that header's
// g_log clashes with this TU's). The member is the one the dialog's `save` of launcher_config_dialog.cpp calls too.
struct rgl_config {
    void save(FILE* f);   // 0x415fb0
};

extern ID3DXEffect* g_effect;                                                // 0x5e293c (renderer.h)

// main menu: choose() 0x495700, reset() 0x494260, 0x48a3a0 sets an explanation record.
#include "../../src/game/ui/window_mgr.h"                         // the screen id stack (0xa739f8)
#include "../../src/game/ui/window_mgr.h"                         // g_windows (0xa73980)

// 0x415160: open a user file ("rgl_config.txt").
#include "../../src/engine/io/text_stream.h"             // mb_open_user_file (0x415160)
#include "../../src/game/config/options.h"   // g_options 0x5aa008
#include "../../src/engine/math/float_consts.h"   // shared float constants kept in memory
#include "../../src/game/config/config_block.h"
#include "../../src/game/config/options.h"
// 0x4b99c0: settings refresh.
void run_settings_probe();                            // 0x4b99c0 (launcher_options_dialog.h; that header's winsock2.h cannot follow windows.h)
// 0x4bc090 / 0x4bc100: set the first / second graphics quality index (clamped to 5).

// 0x4d6b80 and 0x4d6dd0 live in slice s004d6b80.
struct VideoPage;
// the keys page (keys_page.cpp) and this page share these two members: the page calls them on itself
struct KeysPage {
    void update_perf();                                     // 0x4d6b80 (thiscall)
    void set_enabled_text(MissionUi* w);                       // 0x4d6dd0 (thiscall, widget pushed)
};
static inline void update_performance_caption(VideoPage* p)
{
    ((KeysPage*)p)->update_perf();
}
static inline void set_enabled_text_local(MissionUi* w, void* page) { ((KeysPage*)page)->set_enabled_text(w); }

// 0x4bc090 / 0x4bc100: set the first / second graphics quality index.
static inline void set_quality(int v, int which)
{
    Config* c = (Config*)(void*)&g_options;
    if (which) c->set_texture_quality(v); else c->set_triple_buffering(v);
}

// ---- the page ----------------------------------------------------------------------------------------
#include "../../src/game/ui/screens/screen_records.h"       // Recf0: the video options page record (slot 3 is update 0x4d63b0)
struct VideoPage {
    char    pad00[0x18];
    int     option;                                         // +0x18 (the option the menu returns to)
    char    pad1c[0x30 - 0x1c];
    MissionUi* w30;                                            // +0x30  apply / ok
    MissionUi* w34;                                            // +0x34  monitor gamma
    MissionUi* w38;                                            // +0x38  contrast
    MissionUi* w3c;                                            // +0x3c  brightness (float)
    MissionUi* w40;                                            // +0x40  percentage-style control (float)
    MissionUi* w44;                                            // +0x44  percentage-style control (float)
    MissionUi* w48;                                            // +0x48  quality (main)
    MissionUi* w4c;                                            // +0x4c  quality (secondary)
    MissionUi* w50;                                            // +0x50  performance / quality
    MissionUi* w54;                                            // +0x54  water quality
    MissionUi* w58;                                            // +0x58  texture detail
    MissionUi* w5c;                                            // +0x5c  shadows count
    MissionUi* w60;                                            // +0x60  DirectX / renderer
    MissionUi* w64;                                            // +0x64
    MissionUi* w68;                                            // +0x68
    MissionUi* w6c;                                            // +0x6c
    MissionUi* w70;                                            // +0x70
    MissionUi* w74;                                            // +0x74
    MissionUi* w78;                                            // +0x78
    MissionUi* w7c;                                            // +0x7c
    MissionUi* w80;                                            // +0x80  percentage-style caption
    MissionUi* w84;                                            // +0x84
    char    pad88[0x98 - 0x88];
    MissionUi* w98;                                            // +0x98  gamma caption
    MissionUi* w9c;                                            // +0x9c
    MissionUi* wa0;                                            // +0xa0  percentage caption
    char    pad_a4[0xe8 - 0xa4];
    int     applied_value;                                  // +0xe8 (the value the config was written with)
    char    changed_config;                                 // +0xec (something was applied)
    char    apply_requested;                                // +0xed (write the config and leave)

};


#include "./gfx.h"

// ---- gfx: the rows of gfx_video_page.cpp -----------------------------------------------------------------------------
struct DropDown;
extern DropDown* g_gfx_page_dd[4];
extern int g_gfx_page_values[4][10];
extern int g_gfx_page_count[4];

static int row_value(int row, int fallback)
{
    int i = ((MissionUi*)(void*)g_gfx_page_dd[row])->get15c();
    return i >= 0 && i < g_gfx_page_count[row] ? g_gfx_page_values[row][i] : fallback;
}

static void gfx_page_poll()
{
    gfx_settings& s = g_gfx_saved;
    for (int row = 0; row < 4; row++) {
        MissionUi* w = (MissionUi*)(void*)g_gfx_page_dd[row];
        if (!w || !w->clear_mask(0x10000))
            continue;
        if (row == 0)
            s.anisotropy = row_value(0, s.anisotropy);
        else if (row == 1)
            s.render_scale = row_value(1, (int)(s.render_scale * 100.0f + 0.5f)) / 100.0f;
        else if (row == 2)
            s.fsr = row_value(2, s.fsr);
        else
            s.fsr_sharpness = row_value(3, (int)(s.fsr_sharpness * 100.0f + 0.5f)) / 100.0f;
        gfx_log("video page: row %d -> anisotropy %d, render_scale %.2f, fsr %d, fsr_sharpness %.2f\n", row,
                s.anisotropy, s.render_scale, s.fsr, s.fsr_sharpness);
    }
}

#define W(x) ((MissionUi*)(void*)(x))
// @ replaces 0x4d63b0
void Recf0::frame_update(float)
{
    if (W(blood)->clear_mask(0x10000)) {
        g_options.blood = settings_value(W(blood));
        update_performance_caption((VideoPage*)this);
    }

    if (W(change)->clear_mask(0x10000)) {
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

    if (W(quality[0])->clear_mask(0x10000)) {
        g_options.char_detail = settings_value(W(quality[0]));
        set_enabled_text_local(W(quality[0]), this);
        update_performance_caption((VideoPage*)this);
    }
    if (W(quality[1])->clear_mask(0x10000)) {
        g_options.i28 = settings_value(W(quality[1]));
        set_enabled_text_local(W(quality[1]), this);
        update_performance_caption((VideoPage*)this);
    }
    if (W(quality[2])->clear_mask(0x10000)) {
        g_options.i2c = settings_value(W(quality[2]));
        set_enabled_text_local(W(quality[2]), this);
        f_ec = 1;
        update_performance_caption((VideoPage*)this);
    }
    if (W(plant_shadows)->clear_mask(0x10000)) {
        g_options.i7c = settings_value(W(plant_shadows));
        f_ec = 1;
        update_performance_caption((VideoPage*)this);
    }
    if (W(quality[3])->clear_mask(0x10000)) {
        g_options.i20 = settings_value(W(quality[3]));
        set_enabled_text_local(W(quality[3]), this);
        update_performance_caption((VideoPage*)this);
    }
    if (W(quality[4])->clear_mask(0x10000)) {
        g_options.i38 = settings_value(W(quality[4]));
        set_enabled_text_local(W(quality[4]), this);
        update_performance_caption((VideoPage*)this);
    }
    if (W(quality[5])->clear_mask(0x10000)) {
        g_cfg.fake_reflections = settings_value(W(quality[5]));
        set_enabled_text_local(W(quality[5]), this);
        f_ec = 1;
        update_performance_caption((VideoPage*)this);
    }
    if (W(shadow_quality)->clear_mask(0x10000)) {
        g_cfg.shadow_quality_req = 2 - settings_value(W(shadow_quality));
        f_ec = 1;
        update_performance_caption((VideoPage*)this);
    }
    if (W(corpses)->clear_mask(0x10000)) {
        set_quality(5 - settings_value(W(corpses)), 0);
        update_performance_caption((VideoPage*)this);
    }
    if (W(ragdolls)->clear_mask(0x10000)) {
        set_quality(5 - settings_value(W(ragdolls)), 1);
        update_performance_caption((VideoPage*)this);
    }

    if (W(ui_scale)->clear_mask(0x10000)) {
        // monitor gamma: hand the value and its inverse to the renderer, and caption it
        g_options.scale_x = W(ui_scale)->get168();
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
        { char r[8]; W(val_gamma)->set_text((float*)r, &(text)); }
        { char r[8]; W(val_gamma)->get_xy((float*)r); }
    }

    if (W(ragdoll_limit)->clear_mask(0x10000)) {
        g_options.i34 = (int)W(ragdoll_limit)->get168();
        char buf[0x80];
        {
            tstring key("ui_percentage_value");
            sprintf(buf, g_strtable.lookup(key)->c_str(), g_options.i34);
        }
        {
            tstring text(buf);
            { char r[8]; W(val_grass_density)->set_text((float*)r, &(text)); }
        }
        { char r[8]; W(val_grass_density)->get_xy((float*)r); }
        f_ec = 1;
        update_performance_caption((VideoPage*)this);
    }

    if (W(y_scale)->clear_mask(0x10000)) {
        g_options.scale_y = W(y_scale)->get168();
        update_performance_caption((VideoPage*)this);
    }
    if (W(z_scale)->clear_mask(0x10000)) {
        g_options.scale_z = W(z_scale)->get168();
        update_performance_caption((VideoPage*)this);
    }
    if (W(days)->clear_mask(0x10000)) {
        g_cfg.texture_detail = (int)W(days)->get168();
        char buf[0x80];
        {
            tstring key("ui_percentage_value");
            sprintf(buf, g_strtable.lookup(key)->c_str(), g_cfg.texture_detail);
        }
        {
            tstring text(buf);
            { char r[8]; W(val_texture_detail)->set_text((float*)r, &(text)); }
        }
        { char r[8]; W(val_texture_detail)->get_xy((float*)r); }
        update_performance_caption((VideoPage*)this);
    }

    if (W(resolution)->clear_mask(0x10000)) {
        // texture detail: "WxH" labels, split at the 'x' and taken as integers
        int idx = settings_value(W(resolution));
        tstring label(*value_label(W(resolution), idx));
        int a = label.find('x', 0);
        int b = label.find(' ', 0);
        g_cfg.display_width = atol(label.substr(0, a).c_str());     // _atol, not _atoi
        g_cfg.display_height = atol(label.substr(a + 1, b).c_str());
        run_settings_probe();
        update_performance_caption((VideoPage*)this);
    }

    gfx_page_poll();            // gfx: the gfx set's rows -> g_gfx_saved

    if (W(done)->clear_mask(0x10000)) {
        f_ed = 1;
        if (day_saved != g_cfg.texture_detail) {
            g_resmgr.unload_textures();
            g_resmgr.load_textures();
        }
        void* f = mb_open_user_file("rgl_config.txt", "w");
        ((rgl_config*)&g_cfg)->save((FILE*)f);
        fclose((FILE*)f);
        gfx_apply_saved();      // gfx: written above by rgl_config::save; the device takes them at the next Present
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
#undef W


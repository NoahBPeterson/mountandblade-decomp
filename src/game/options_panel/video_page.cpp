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
#include "../ui_widget.h"                 // Widget (the UI widget interface)

// sliders keep their value labels at +0x190: rows of 16 bytes, copied with the tstring copy ctor.
static inline const tstring* value_label(Widget* w, int index)
{
    return (const tstring*)(*(char**)((char*)w + 0x190) + index * 0x10);
}

// 0x4d63b0 polls changed(+0x44) and reads the value through +0x30 on the same control.
static inline int settings_value(Widget* w) { return w->get_value(); }
static inline char w_changed(Widget* w, int f) { return w->changed(f); }
static inline float w_float(Widget* w) { return w->get_float(); }
static inline void w_set_text(Widget* w, const tstring& s) { char r[8]; w->set_text(r, s); }
static inline void w_set_default(Widget* w) { char r[8]; w->get_xy(r); }

// ---- other pages / objects ---------------------------------------------------------------------------
struct Game;                                                                // 0x5e2930

// 0x5d6fa0: the graphics resource manager (rebuild its two halves): rgl/resource.h's resource_manager, g_resmgr.
#include "../../rgl/resource.h"
// 0x5a9f98: the render config (write-out helper 0x415fb0): rgl/resource_manager.h's rgl_config, repeated here (that header's
// g_log clashes with this TU's). The member is the one the dialog's `save` of config_dialog.cpp calls too.
struct rgl_config {
    void save(FILE* f);   // 0x415fb0
};

extern ID3DXEffect* g_effect;                                                // 0x5e293c (renderer.h)

// main menu: choose() 0x495700, reset() 0x494260, 0x48a3a0 sets an explanation record.
#include "../id_stack.h"                         // the screen id stack (0xa739f8)
#include "../main_menu.h"                         // MainMenu (0xa73980): the one definition

// 0x415160: open a user file ("rgl_config.txt").
#include "../../rgl/text_stream.h"             // mb_open_user_file (0x415160)
#include "../../rgl/options.h"   // g_options 0x5aa008
#include "../float_consts.h"   // shared float constants kept in memory
#include "../config/config_block.h"
#include "../../rgl/options.h"
// 0x4b99c0: settings refresh.
void run_settings_probe();                            // 0x4b99c0 (options_dlg.h; that header's winsock2.h cannot follow windows.h)
// 0x4bc090 / 0x4bc100: set the first / second graphics quality index (clamped to 5).

// 0x4d6b80 and 0x4d6dd0 live in slice s004d6b80.
// WindowMgrW (world_data/world_data.h; main_menu.h's `reset` is its old name): open(n) 0x494260 on the main menu object
struct WindowMgrW { void open(int n); };
struct VideoPage;
// the keys page (keys_page.cpp) and this page share these two members: the page calls them on itself
struct KeysPage {
    void update_perf();                                     // 0x4d6b80 (thiscall)
    void set_enabled_text(Widget* w);                       // 0x4d6dd0 (thiscall, widget pushed)
};
static inline void update_performance_caption(VideoPage* p)
{
    ((KeysPage*)p)->update_perf();
}
static inline void set_enabled_text_local(Widget* w, void* page) { ((KeysPage*)page)->set_enabled_text(w); }

// 0x4bc090 / 0x4bc100: set the first / second graphics quality index.
static inline void set_quality(int v, int which)
{
    Config* c = (Config*)(void*)&g_options;
    if (which) c->set_texture_quality(v); else c->set_triple_buffering(v);
}

// ---- the page ----------------------------------------------------------------------------------------
#include "../screen_records.h"       // Recf0: the video options page record (slot 3 is update 0x4d63b0)
struct VideoPage {
    char    pad00[0x18];
    int     option;                                         // +0x18 (the option the menu returns to)
    char    pad1c[0x30 - 0x1c];
    Widget* w30;                                            // +0x30  apply / ok
    Widget* w34;                                            // +0x34  monitor gamma
    Widget* w38;                                            // +0x38  contrast
    Widget* w3c;                                            // +0x3c  brightness (float)
    Widget* w40;                                            // +0x40  percentage-style control (float)
    Widget* w44;                                            // +0x44  percentage-style control (float)
    Widget* w48;                                            // +0x48  quality (main)
    Widget* w4c;                                            // +0x4c  quality (secondary)
    Widget* w50;                                            // +0x50  performance / quality
    Widget* w54;                                            // +0x54  water quality
    Widget* w58;                                            // +0x58  texture detail
    Widget* w5c;                                            // +0x5c  shadows count
    Widget* w60;                                            // +0x60  DirectX / renderer
    Widget* w64;                                            // +0x64
    Widget* w68;                                            // +0x68
    Widget* w6c;                                            // +0x6c
    Widget* w70;                                            // +0x70
    Widget* w74;                                            // +0x74
    Widget* w78;                                            // +0x78
    Widget* w7c;                                            // +0x7c
    Widget* w80;                                            // +0x80  percentage-style caption
    Widget* w84;                                            // +0x84
    char    pad88[0x98 - 0x88];
    Widget* w98;                                            // +0x98  gamma caption
    Widget* w9c;                                            // +0x9c
    Widget* wa0;                                            // +0xa0  percentage caption
    char    pad_a4[0xe8 - 0xa4];
    int     applied_value;                                  // +0xe8 (the value the config was written with)
    char    changed_config;                                 // +0xec (something was applied)
    char    apply_requested;                                // +0xed (write the config and leave)

};

#define W(x) ((Widget*)(void*)(x))
// @ 0x4d63b0
void Recf0::scale_3(float)
{
    if (W(blood)->changed(0x10000)) {
        g_options.blood = settings_value(W(blood));
        update_performance_caption((VideoPage*)this);
    }

    if (W(change)->changed(0x10000)) {
        // switching the renderer: replace the two menu captions and always return to the main menu
        {
            tstring explanation("ui_change_directx_explanation");
            tstring name("DirectX");
            tstring* e = g_strtable.lookup(explanation);   // 0x4d6418: the explanation is the key; "DirectX" is the header
            MainMenu* r = g_main_menu.rec;                  // 0x4d641d: edi = the dialog box record, loaded from [0xa73980]
            r->setup(&name, e);                             // header (+0x68) = "DirectX", title (+0x1c) = the looked-up text
            r->f4c = 0;
            r->f50 = 0;
            r->f4 = 0;
        }
        MainMenu& m = g_main_menu;
        ((WindowMgrW*)(void*)&m)->open(0);
    }

    if (W(quality[0])->changed(0x10000)) {
        g_options.char_detail = settings_value(W(quality[0]));
        set_enabled_text_local(W(quality[0]), this);
        update_performance_caption((VideoPage*)this);
    }
    if (W(quality[1])->changed(0x10000)) {
        g_options.i28 = settings_value(W(quality[1]));
        set_enabled_text_local(W(quality[1]), this);
        update_performance_caption((VideoPage*)this);
    }
    if (W(quality[2])->changed(0x10000)) {
        g_options.i2c = settings_value(W(quality[2]));
        set_enabled_text_local(W(quality[2]), this);
        f_ec = 1;
        update_performance_caption((VideoPage*)this);
    }
    if (W(plant_shadows)->changed(0x10000)) {
        g_options.i7c = settings_value(W(plant_shadows));
        f_ec = 1;
        update_performance_caption((VideoPage*)this);
    }
    if (W(quality[3])->changed(0x10000)) {
        g_options.i20 = settings_value(W(quality[3]));
        set_enabled_text_local(W(quality[3]), this);
        update_performance_caption((VideoPage*)this);
    }
    if (W(quality[4])->changed(0x10000)) {
        g_options.i38 = settings_value(W(quality[4]));
        set_enabled_text_local(W(quality[4]), this);
        update_performance_caption((VideoPage*)this);
    }
    if (W(quality[5])->changed(0x10000)) {
        g_cfg.fake_reflections = settings_value(W(quality[5]));
        set_enabled_text_local(W(quality[5]), this);
        f_ec = 1;
        update_performance_caption((VideoPage*)this);
    }
    if (W(shadow_quality)->changed(0x10000)) {
        g_cfg.shadow_quality_req = 2 - settings_value(W(shadow_quality));
        f_ec = 1;
        update_performance_caption((VideoPage*)this);
    }
    if (W(corpses)->changed(0x10000)) {
        set_quality(5 - settings_value(W(corpses)), 0);
        update_performance_caption((VideoPage*)this);
    }
    if (W(ragdolls)->changed(0x10000)) {
        set_quality(5 - settings_value(W(ragdolls)), 1);
        update_performance_caption((VideoPage*)this);
    }

    if (W(ui_scale)->changed(0x10000)) {
        // monitor gamma: hand the value and its inverse to the renderer, and caption it
        float gamma = W(ui_scale)->get_float();
        g_options.scale_x = gamma;
        float inv = orig::g_one / gamma;
        vector4 v, vi;
        v.x = v.y = v.z = v.w = gamma;
        vi.x = vi.y = vi.z = vi.w = inv;
        g_effect->SetVector("output_gamma", (const D3DXVECTOR4*)&v);
        g_effect->SetVector("output_gamma_inv", (const D3DXVECTOR4*)&vi);
        char buf[0x40];
        _itoa((int)(gamma * 100.0f), buf, 10);
        tstring text(buf);
        {
            tstring dot(".");
            text.insert(1, dot);          // 0x4d6726: "220" -> "2.20" (the point goes after the first digit)
        }
        if (text.equals("2.20")) {        // 0x4d6734: the default gamma (the literal at 0x596820) shows as ui_default
            tstring k("ui_default");
            text = *g_strtable.lookup(k);
        }
        { char r[8]; W(val_gamma)->set_text(r, text); }
        { char r[8]; W(val_gamma)->get_xy(r); }
    }

    if (W(ragdoll_limit)->changed(0x10000)) {
        int pct = (int)W(ragdoll_limit)->get_float();
        g_options.i34 = pct;
        char buf[0x40];
        {
            tstring key("ui_percentage_value");
            tstring* e = g_strtable.lookup(key);
            sprintf(buf, e->c_str(), pct);
        }
        {
            tstring text(buf);
            { char r[8]; W(val_grass_density)->set_text(r, text); }
        }
        { char r[8]; W(val_grass_density)->get_xy(r); }
        f_ec = 1;
        update_performance_caption((VideoPage*)this);
    }

    if (W(y_scale)->changed(0x10000)) {
        g_options.scale_y = W(y_scale)->get_float();
        update_performance_caption((VideoPage*)this);
    }
    if (W(z_scale)->changed(0x10000)) {
        g_options.scale_z = W(z_scale)->get_float();
        update_performance_caption((VideoPage*)this);
    }
    if (W(days)->changed(0x10000)) {
        int pct = (int)W(days)->get_float();
        g_cfg.texture_detail = pct;
        char buf[0x40];
        {
            tstring key("ui_percentage_value");
            tstring* e = g_strtable.lookup(key);
            sprintf(buf, e->c_str(), pct);
        }
        {
            tstring text(buf);
            { char r[8]; W(val_texture_detail)->set_text(r, text); }
        }
        { char r[8]; W(val_texture_detail)->get_xy(r); }
        update_performance_caption((VideoPage*)this);
    }

    if (W(resolution)->changed(0x10000)) {
        // texture detail: "WxH" labels, split at the 'x' and taken as integers
        int idx = settings_value(W(resolution));
        tstring label(*value_label(W(resolution), idx));
        int a = label.find('x', 0);
        int b = label.find(' ', 0);
        {
        tstring left = label.substr(0, a);
        g_cfg.display_width = atol(left.c_str());     // 0x4d6900: _atol, not _atoi
        }
        {
        tstring right = label.substr(a + 1, b);
        g_cfg.display_height = atol(right.c_str());
        }
        run_settings_probe();
        update_performance_caption((VideoPage*)this);
    }

    if (W(done)->changed(0x10000)) {
        f_ed = 1;
        if (day_saved != g_cfg.texture_detail) {
            g_resmgr.unload_textures();
            g_resmgr.load_textures();
        }
        void* f = mb_open_user_file("rgl_config.txt", (const char*)orig::g_rgl_name);
        ((rgl_config*)&g_cfg)->save((FILE*)f);
        fclose((FILE*)f);
        if (f_ec) {
            {
                // 0x4d6ab9: the key is looked up and the text goes in as the body; the title is the empty string
                tstring key("ui_change_graphics_settings_explanation");
                tstring title("");
                tstring* e = g_strtable.lookup(key);
                g_main_menu.rec->setup(&title, e);
            }
            ((WindowMgrW*)(void*)&g_main_menu)->open(0);
        }
    }

    if (f_ed) {
        // the last menu entry is checked -> go back to the page that opened the options
        if (current_screen_id() != 0)
            g_main_menu.set_state(w18);
    }
}
#undef W

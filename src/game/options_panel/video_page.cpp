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
#include <windows.h>
#include <d3dx9.h>
#include <stdio.h>
#include "../../rgl/rgl_math.h"
#include "../tstring.h"

using rgl::vector4;

#include "../str_table.h"                                                 // StrTable g_strtable (0x5e2a18)

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
extern rgl_config g_cfg;                                        // 0x5a9f98
// 0x4bc090 / 0x4bc100: Config's (config/config.h, whose headers clash with this TU's) quality setters, called on the options
// block at 0x5aa008 (ecx = 0x5aa008 at 0x4d6610 / 0x4d6642).
struct Config { void set_triple_buffering(int t); void set_texture_quality(int q); };

extern ID3DXEffect* g_effect;                                                // 0x5e293c (renderer.h)
extern Game* g_game;                                                        // 0x5e2930

// main menu: choose() 0x495700, reset() 0x494260, 0x48a3a0 sets an explanation record.
#include "../id_stack.h"                         // the screen id stack (0xa739f8)
#include "../main_menu.h"                         // MainMenu (0xa73980): the one definition

// 0x415160: open a user file ("rgl_config.txt").
#include "../../rgl/text_stream.h"             // mb_open_user_file (0x415160)
// 0x4b99c0: settings refresh.
void run_settings_probe();                            // 0x4b99c0 (options_dlg.h; that header's winsock2.h cannot follow windows.h)
// 0x4bc090 / 0x4bc100: set the first / second graphics quality index (clamped to 5).
struct GraphicsQuality { int a; int b; };
extern GraphicsQuality g_graphics_quality;                                  // 0x5aa008

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
    Config* c = (Config*)(void*)&g_graphics_quality;
    if (which) c->set_texture_quality(v); else c->set_triple_buffering(v);
}

// ---- the page ----------------------------------------------------------------------------------------
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

    void update(int reason);                                // 0x4d63b0
};

// @ 0x4d63b0
void VideoPage::update(int reason)
{
    if (w50->changed(0x10000)) {
        orig::g_opt_blood = settings_value(w50);
        update_performance_caption(this);
    }

    if (w60->changed(0x10000)) {
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

    if (w64->changed(0x10000)) {
        orig::g_char_detail = settings_value(w64);
        set_enabled_text_local(w64, this);
        update_performance_caption(this);
    }
    if (w68->changed(0x10000)) {
        orig::g_shadows_on = settings_value(w68);
        set_enabled_text_local(w68, this);
        update_performance_caption(this);
    }
    if (w6c->changed(0x10000)) {
        orig::g_plant_shadow = settings_value(w6c);
        set_enabled_text_local(w6c, this);
        changed_config = 1;
        update_performance_caption(this);
    }
    if (w54->changed(0x10000)) {
        orig::g_mesh_quality = settings_value(w54);
        changed_config = 1;
        update_performance_caption(this);
    }
    if (w70->changed(0x10000)) {
        orig::g_char_shadow = settings_value(w70);
        set_enabled_text_local(w70, this);
        update_performance_caption(this);
    }
    if (w74->changed(0x10000)) {
        orig::g_particle = settings_value(w74);
        set_enabled_text_local(w74, this);
        update_performance_caption(this);
    }
    if (w78->changed(0x10000)) {
        orig::g_simple_reflect = settings_value(w78);
        set_enabled_text_local(w78, this);
        changed_config = 1;
        update_performance_caption(this);
    }
    if (w5c->changed(0x10000)) {
        orig::g_shadow_quality_req = 2 - settings_value(w5c);
        changed_config = 1;
        update_performance_caption(this);
    }
    if (w48->changed(0x10000)) {
        set_quality(5 - settings_value(w48), 0);
        update_performance_caption(this);
    }
    if (w4c->changed(0x10000)) {
        set_quality(5 - settings_value(w4c), 1);
        update_performance_caption(this);
    }

    if (w34->changed(0x10000)) {
        // monitor gamma: hand the value and its inverse to the renderer, and caption it
        float gamma = w34->get_float();
        orig::g_scale_x = gamma;
        float inv = orig::g_one / gamma;
        vector4 v, vi;
        v.x = v.y = v.z = v.w = gamma;
        vi.x = vi.y = vi.z = vi.w = inv;
        g_effect->SetVector("output_gamma", (const D3DXVECTOR4*)&v);
        g_effect->SetVector("output_gamma_inv", (const D3DXVECTOR4*)&vi);
        char buf[0x40];
        _itoa((int)(gamma * orig::k_100f), buf, 10);
        tstring text(buf);
        {
            tstring dot(".");
            text.insert(1, dot);          // 0x4d6726: "220" -> "2.20" (the point goes after the first digit)
        }
        if (text.equals("2.20")) {        // 0x4d6734: the default gamma (the literal at 0x596820) shows as ui_default
            tstring k("ui_default");
            text = *g_strtable.lookup(k);
        }
        { char r[8]; w98->set_text(r, text); }
        { char r[8]; w98->get_xy(r); }
    }

    if (w44->changed(0x10000)) {
        int pct = (int)w44->get_float();
        orig::g_ragdoll_lim = pct;
        char buf[0x40];
        {
            tstring key("ui_percentage_value");
            tstring* e = g_strtable.lookup(key);
            sprintf(buf, e->c_str(), pct);
        }
        {
            tstring text(buf);
            { char r[8]; wa0->set_text(r, text); }
        }
        { char r[8]; wa0->get_xy(r); }
        changed_config = 1;
        update_performance_caption(this);
    }

    if (w38->changed(0x10000)) {
        orig::g_scale_y = w38->get_float();
        update_performance_caption(this);
    }
    if (w3c->changed(0x10000)) {
        orig::g_scale_z = w3c->get_float();
        update_performance_caption(this);
    }
    if (w40->changed(0x10000)) {
        int pct = (int)w40->get_float();
        orig::g_texture_detail = pct;
        char buf[0x40];
        {
            tstring key("ui_percentage_value");
            tstring* e = g_strtable.lookup(key);
            sprintf(buf, e->c_str(), pct);
        }
        {
            tstring text(buf);
            { char r[8]; w80->set_text(r, text); }
        }
        { char r[8]; w80->get_xy(r); }
        update_performance_caption(this);
    }

    if (w58->changed(0x10000)) {
        // texture detail: "WxH" labels, split at the 'x' and taken as integers
        int idx = settings_value(w58);
        tstring label(*value_label(w58, idx));
        int a = label.find('x', 0);
        int b = label.find(' ', 0);
        {
        tstring left = label.substr(0, a);
        orig::g_iWindowWidth = atol(left.c_str());     // 0x4d6900: _atol, not _atoi
        }
        {
        tstring right = label.substr(a + 1, b);
        orig::g_iWindowHeight = atol(right.c_str());
        }
        run_settings_probe();
        update_performance_caption(this);
    }

    if (w30->changed(0x10000)) {
        apply_requested = 1;
        if (applied_value != orig::g_texture_detail) {
            g_resmgr.unload_textures();
            g_resmgr.load_textures();
        }
        void* f = mb_open_user_file("rgl_config.txt", (const char*)orig::g_rgl_name);
        g_cfg.save((FILE*)f);
        fclose((FILE*)f);
        if (changed_config) {
            {
                // 0x4d6ab9: the key is looked up and the text goes in as the body; the title is the empty string
                tstring key("ui_change_graphics_settings_explanation");
                tstring title((const char*)orig::g_empty_text);
                tstring* e = g_strtable.lookup(key);
                g_main_menu.rec->setup(&title, e);
            }
            ((WindowMgrW*)(void*)&g_main_menu)->open(0);
        }
    }

    if (apply_requested) {
        // the last menu entry is checked -> go back to the page that opened the options
        if (current_screen_id() != 0)
            g_main_menu.set_state(option);
    }
}

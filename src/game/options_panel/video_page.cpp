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
// 0x42c080 find, 0x42c4a0 substr, 0x42c900 insert. The render-config object at 0x5e293c has a
// set(name, vector4) virtual at +0x88.
#include <windows.h>
#include <stdio.h>
#include "../../rgl/rgl_math.h"
#include "../tstring.h"

using rgl::vector4;

#include "./str_table_rgl.h"                                              // StrTableRgl (was StringTable here): the one definition

extern "C" int __cdecl sprintf(char*, const char*, ...);                    // 0x536f8f
extern "C" char* __cdecl _itoa(int, char*, int);                            // 0x5371f7
extern "C" long __cdecl _atol(const char*);                                 // 0x53734d
extern "C" int __cdecl fclose_(void*);                                      // 0x537168

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

// 0x5d6fa0: the graphics resource manager (rebuild its two halves).
struct Resources {
    void rebuild1();                                                        // 0x418e10
    void rebuild2();                                                        // 0x418d60
};
extern Resources g_resources;                                               // 0x5d6fa0

// 0x5a9f98: the render config (write-out helper 0x415fb0).
struct RenderConfig {
    void write(void* file);                                                 // 0x415fb0
};
extern RenderConfig g_render_config;                                        // 0x5a9f98

// 0x5e293c: the renderer; set(name, value) at +0x88.
struct RenderParams {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09(); virtual void v0a(); virtual void v0b();
    virtual void v0c(); virtual void v0d(); virtual void v0e(); virtual void v0f();
    virtual void v10(); virtual void v11(); virtual void v12(); virtual void v13();
    virtual void v14(); virtual void v15(); virtual void v16(); virtual void v17();
    virtual void v18(); virtual void v19(); virtual void v1a(); virtual void v1b();
    virtual void v1c(); virtual void v1d(); virtual void v1e(); virtual void v1f();
    virtual void v20();
    virtual void set(const char* name, const vector4& v);   // +0x88  0x?
};
extern RenderParams g_renderer;                                                 // 0x5e293c
extern Game* g_game;                                                        // 0x5e2930

// main menu: choose() 0x495700, reset() 0x494260, 0x48a3a0 sets an explanation record.
#include "../main_menu.h"                         // MainMenu (0xa73980): the one definition

// 0x415160: open a user file ("rgl_config.txt").
void* open_user_file(const char* name, const char* mode);
// 0x4b99c0: settings refresh.
void settings_refresh();
// 0x4bc090 / 0x4bc100: set the first / second graphics quality index (clamped to 5).
struct GraphicsQuality { int a; int b; };
extern GraphicsQuality g_graphics_quality;                                  // 0x5aa008

// 0x4d6b80 and 0x4d6dd0 live in slice s004d6b80.
struct VideoPage;
struct VideoPage2 {
    void update_perf();                                     // 0x4d6b80 (thiscall)
    void set_enabled_text(Widget* w);                       // 0x4d6dd0 (thiscall, widget pushed)
};
static inline void update_performance_caption(VideoPage* p)
{
    ((VideoPage2*)p)->update_perf();
}
static inline void set_enabled_text_local(Widget* w, void* page) { ((VideoPage2*)page)->set_enabled_text(w); }

// 0x4bc090 / 0x4bc100: set the first / second graphics quality index.
void set_quality1(int v);                                                   // 0x4bc090
void set_quality2(int v);                                                   // 0x4bc100
static inline void set_quality(int v, int which)
{
    if (which) set_quality2(v); else set_quality1(v);
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
        *(int*)0x5aa020 = settings_value(w50);
        update_performance_caption(this);
    }

    if (w60->changed(0x10000)) {
        // switching the renderer: replace the two menu captions and always return to the main menu
        tstring explanation("ui_change_directx_explanation");
        tstring name("DirectX");
        tstring* e = g_string_table.lookup(name);
        MainMenu& m = g_main_menu;
        m.set_texts(&explanation, e);
        m.f4c = 0;
        m.f50 = 0;
        m.f4 = 0;
        name.~tstring();
        explanation.~tstring();
        m.reset(0);
    }

    if (w64->changed(0x10000)) {
        *(int*)0x5aa024 = settings_value(w64);
        set_enabled_text_local(w64, this);
        update_performance_caption(this);
    }
    if (w68->changed(0x10000)) {
        *(int*)0x5aa030 = settings_value(w68);
        set_enabled_text_local(w68, this);
        update_performance_caption(this);
    }
    if (w6c->changed(0x10000)) {
        *(int*)0x5aa034 = settings_value(w6c);
        set_enabled_text_local(w6c, this);
        changed_config = 1;
        update_performance_caption(this);
    }
    if (w54->changed(0x10000)) {
        *(int*)0x5aa084 = settings_value(w54);
        changed_config = 1;
        update_performance_caption(this);
    }
    if (w70->changed(0x10000)) {
        *(int*)0x5aa028 = settings_value(w70);
        set_enabled_text_local(w70, this);
        update_performance_caption(this);
    }
    if (w74->changed(0x10000)) {
        *(int*)0x5aa040 = settings_value(w74);
        set_enabled_text_local(w74, this);
        update_performance_caption(this);
    }
    if (w78->changed(0x10000)) {
        *(int*)0x5a9fc4 = settings_value(w78);
        set_enabled_text_local(w78, this);
        changed_config = 1;
        update_performance_caption(this);
    }
    if (w5c->changed(0x10000)) {
        *(int*)0x5aa004 = 2 - settings_value(w5c);
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
        *(float*)0x5aa088 = gamma;
        float inv = *(float*)0x549cdc / gamma;
        vector4 v, vi;
        v.x = v.y = v.z = v.w = gamma;
        vi.x = vi.y = vi.z = vi.w = inv;
        g_renderer.set("output_gamma", v);
        g_renderer.set("output_gamma_inv", vi);
        char buf[0x40];
        _itoa((int)(gamma * *(float*)0x54ab28), buf, 10);
        tstring text(buf);
        tstring key_state("ui_gamma");
        tstring state("");
        state.insert(1, key_state);   // the state suffix is appended after the first character
        key_state.~tstring();
        if (text.equals("ui_default")) {
            tstring k("ui_default");
            state = *g_string_table.lookup(k);
            k.~tstring();
        }
        { char r[8]; w98->set_text(r, state); }
        { char r[8]; w98->get_xy(r); }
        text.~tstring();
    }

    if (w44->changed(0x10000)) {
        int pct = (int)w44->get_float();
        *(int*)0x5aa03c = pct;
        char buf[0x40];
        {
            tstring key("ui_percentage_value");
            tstring* e = g_string_table.lookup(key);
            sprintf(buf, e->c_str(), pct);
            key.~tstring();
        }
        tstring text(buf);
        tstring key2("ui_percentage_value");
        { char r[8]; wa0->set_text(r, text); }
        key2.~tstring();
        text.~tstring();
        { char r[8]; wa0->get_xy(r); }
        changed_config = 1;
        update_performance_caption(this);
    }

    if (w38->changed(0x10000)) {
        *(float*)0x5aa08c = w38->get_float();
        update_performance_caption(this);
    }
    if (w3c->changed(0x10000)) {
        *(float*)0x5aa090 = w3c->get_float();
        update_performance_caption(this);
    }
    if (w40->changed(0x10000)) {
        int pct = (int)w40->get_float();
        *(int*)0x5a9f9c = pct;
        char buf[0x40];
        {
            tstring key("ui_percentage_value");
            tstring* e = g_string_table.lookup(key);
            sprintf(buf, e->c_str(), pct);
            key.~tstring();
        }
        tstring text(buf);
        tstring key2("ui_percentage_value");
        { char r[8]; w80->set_text(r, text); }
        key2.~tstring();
        text.~tstring();
        { char r[8]; w80->get_xy(r); }
        update_performance_caption(this);
    }

    if (w58->changed(0x10000)) {
        // texture detail: "WxH" labels, split at the 'x' and taken as integers
        int idx = settings_value(w58);
        tstring label(*value_label(w58, idx));
        int a = label.find('x', 0);
        int b = label.find(' ', 0);
        tstring left = label.substr(0, a);
        *(int*)0x5a9fe4 = _atol(left.c_str());
        left.~tstring();
        tstring right = label.substr(a + 1, b);
        *(int*)0x5a9fe8 = _atol(right.c_str());
        right.~tstring();
        settings_refresh();
        update_performance_caption(this);
        label.~tstring();
    }

    if (w30->changed(0x10000)) {
        apply_requested = 1;
        if (applied_value != *(int*)0x5a9f9c) {
            g_resources.rebuild1();
            g_resources.rebuild2();
        }
        void* f = open_user_file("rgl_config.txt", (const char*)0x54b048);
        g_render_config.write(f);
        fclose_(f);
        if (changed_config) {
            tstring explanation("ui_change_graphics_settings_explanation");
            tstring pos((const char*)0x54a480);
            tstring* e = g_string_table.lookup(pos);
            g_main_menu.set_texts(&explanation, e);
            pos.~tstring();
            explanation.~tstring();
            g_main_menu.reset(0);
        }
    }

    if (apply_requested) {
        // the last menu entry is checked -> go back to the page that opened the options
        if (*(int*)(*(int*)(0xa739fc + (((0xa73a04 + 0xa73a08 - 1) / 4 - 0xa73a00)) * 4)
                    + ((0xa73a04 + 0xa73a08 - 1) & 3) * 4) != 0)
            g_main_menu.choose(option);
    }
}

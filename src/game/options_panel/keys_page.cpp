// keys_page.cpp (was slice s004d6b80): the performance caption and the controls page (key assignment table).
//   0x4d6b80  recompute the estimated-performance caption from the render-config globals
//   0x4d6dd0  caption a control with "ui_enabled" / "ui_disabled" after its value changed
//   0x4d6e50  the page update: picks the changed key row, then either closes the key-assignment overlay
//             or re-captions every row (0x4d7120) and handles the three page-level buttons
//   0x4d6f90  a table of 0x25 (key, button) pairs; forgets every pair with that button and stores a new one
//   0x4d6fd0  copies the "original" pairs (at +0x128) back over the current ones
//   0x4d7000  writes the pairs to controls.dat through an inline bin_writer
//   0x4d7120  captions the 7 column headers and the 0x25 key rows and refreshes the two buttons
//   0x4d7270  lays out one key row of the table (scale from the text width, position, text)
// All names provisional. The page is called KeysPage and its key table GameKeys here (the slice called them
// VideoPage and KeyTable, which are other classes: video_page.cpp, src/include/key_table.h).
//
// Rewritten 2026-10-22 from the disassembly: the earlier version used a widget vtable 0x44 bytes off, the
// unverified match/include/rgl.h string layout (buffer at +0, really +4), explicit destructor calls on top of
// the implicit ones, swapped forget() arguments, a restore() two dwords off, a performance formula missing
// four factors, a save() that never wrote the table, and no body for 0x4d7270.
#include "../../include/image_addrs.h"
#include "../float_consts.h"
#include "../../rgl/options.h"   // g_options
#include "../config/config_block.h"   // g_cfg
#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include "../../rgl/rgl_math.h"
#include "../../rgl/stream.h"

using rgl::vector4;
#include "../tstring.h"               // tstring: 0x42bbd0 / 0x42bbe0 ctors, 0x42bc40 dtor, 0x42bcb0 assign(const char*)

#include "../str_table.h"                                                 // StrTable g_strtable (0x5e2a18)

// ---- controls ----------------------------------------------------------------------------------------
#include "../../include/vec.h"                      // vector2 (= Vec2)
// The +0x94 and +0xa4 slots return an 8-byte non-POD object through a hidden pointer that the callers
// discard (same slot shape as s004d7d10 / s004dc630).
#include "../wret.h"

// Slot offsets from the calls in this slice (0x4d6b80, 0x4d6dd0, 0x4d6e50, 0x4d7120, 0x4d7270).
#include "../ui_widget.h"                 // MissionUi (the UI widget interface)

// ---- symbols -----------------------------------------------------------------------------------------
#include "../window_mgr.h"                         // g_windows (0xa73980)
#include "../game_keys.h"                         // GameKeys g_game_keys (0xa73730)
// 0xa170c8: the keyboard state; is_bound(n) is 0x487900.
#include "../key_names.h"
#include "../../rgl/options.h"   // g_options 0x5aa008
#include "../float_consts.h"   // shared float constants kept in memory
#include "../terrain_a/game_state.h"
#include "../../include/render_flags.h"

// 0x4bab70 (byte-exact in s004bab70): build the user path of a file name in place. A static function in the
// original, so cl passes `out` in esi; this is a local copy with the same body.
static void user_path(char* out)                                            // 0x4bab70
{
    char path[0x104];
    char name[0x104];

    SHGetFolderPathA(0, 0x1a, 0, 0, path);              // CSIDL_APPDATA
    sprintf(name, "%s\\Mount&Blade", path);
    CreateDirectoryA(name, 0);
    sprintf(name, "%s", out);
    sprintf(out, "%s\\Mount&Blade\\%s", path, name);
}

// UTF-8 -> int code points (0x487b00; custom register convention in the original: text in edi, count out in
// edx, buf and &capacity on the stack). Local copy from s004dc630, shaped so cl picks the same convention.
static __declspec(noinline) void decode_utf8(const tstring* s, int* out_len, int* buf, const int* maxn)   // 0x487b00
{
    *out_len = 0;
    int n = s->len;
    int i = 0;
    if (n > 0) {
        do {
            if (*out_len >= *maxn)
                break;
            unsigned char c = s->buf[i];
            int ch;
            if (c >= 0xf0 && c < 0xf5) {
                const unsigned char* q = (const unsigned char*)s->buf + i;
                ch = (((((q[0] & 7) << 6) | (q[1] & 0x3f)) << 6 | (q[2] & 0x3f)) << 6) | (q[3] & 0x3f);
                i += 4;
            } else if (c >= 0xe0 && c < 0xf0) {
                const unsigned char* q = (const unsigned char*)s->buf + i;
                ch = ((((q[0] & 0xf) << 6) | (q[1] & 0x3f)) << 6) | (q[2] & 0x3f);
                i += 3;
            } else if (c >= 0xc2 && c < 0xe0) {
                const unsigned char* q = (const unsigned char*)s->buf + i;
                ch = (((q[0] & 0x1f) << 6) | (q[1] & 0x3f));
                i += 2;
            } else {
                i++;
                if (c >= 0x80)
                    continue;
                ch = (signed char)c;
            }
            buf[*out_len] = ch;
            (*out_len)++;
        } while (i < n);
    }
}

// the file writer's ctor is inline here (0x4d7000 stores both vtables), out of line in resource_manager.cpp
inline bin_writer::bin_writer() {}

// ---- the key table (0xa73730: game/game_keys.h) -----------------------------------------------------------
// the one global of the table: the original constructs it in the static initializer 0x547e90 (every code -1, then load() 0x4c2ea0)
GameKeys g_game_keys;                                                              // 0xa73730
GameKeys::GameKeys()
{
    memset(entry, 0xff, sizeof entry);
    load();
}

// ---- the page ----------------------------------------------------------------------------------------
// The page is read from both ends: the two button objects sit at +0x30 (apply / restore) and the widget
// the handler talks to last is at +0xe4, so the layout is declared as two views of the same memory.
struct KeysPageView {
    char    pad00[0x18];
    int     option;                                         // +0x18 (the option the menu returns to)
    char    pad1c[0x30 - 0x1c];
    MissionUi* w30;                                            // +0x30  apply
    MissionUi* w34;                                            // +0x34  cancel / restore
    MissionUi* w38;                                            // +0x38  defaults
    char    pad3c[0xd0 - 0x3c];
    MissionUi* keys[0x25];                                     // +0xd0  the key-assignment rows
    MissionUi* w164;                                           // +0x164 the key-assignment overlay
    char    pad168[0x18c - 0x168];
    MissionUi* captions[7];                                    // +0x18c the column headers
    char    pad1a8[0x1b4 - 0x1a8];
    char    modified;                                       // +0x1b4
    char    pad1b5[0x1b8 - 0x1b5];
    int     editing;                                        // +0x1b8 the row being assigned, -1 for none
};

#include "../screen_records.h"       // Rec1bc: the key-binding page record (slot 3 is update 0x4d6e50)
struct KeysPage {
    char    pad00[0xe4];
    MissionUi* perf_caption;                                   // +0xe4 the performance caption
    char    pad_e8[0x300];

    void update_perf();                                     // 0x4d6b80
    void set_enabled_text(MissionUi* w);                       // 0x4d6dd0
    void refresh();                                         // 0x4d7120
};

static inline KeysPageView* head(KeysPage* p) { return (KeysPageView*)p; }

// @ 0x4d6b80
void KeysPage::update_perf()
{
    float perf = 100.0f;
    if (g_options.i28)
        perf = g_options.scale_z * 5.0f + 100.0f;
    perf += g_options.blood * 5;
    perf += g_options.i20 * 6;
    int limit = *(int*)&g_options.max_dead;
    if (limit >= 100) limit = 100;
    perf += limit * 0.2f;
    limit = g_options.i40;
    if (limit >= 5) limit = 5;
    perf += limit;
    int d3d = g_options.i2c;
    char high = g_render.use_hw_shadow;
    perf += g_options.i34 * 0.1f;
    int shadows = g_use_shaders;
    perf += g_options.scale_y * 10.0f;

    if (d3d) {
        if (g_cfg.shadow_quality_req >= 2)
            perf += 3.0f;
        if (shadows) {
            if (high) perf *= 1.1f;
            else                  perf *= 1.25f;
        } else {
            perf += 4.0f;
        }
    }
    if (g_options.i7c == 2) {
        if (shadows) {
            if (high) perf *= 1.1f;
            else                  perf *= 1.2f;
        }
    }
    if (shadows)
        perf *= (1 - g_cfg.fake_reflections) * g_k065 + g_one;
    perf *= g_options.i38 * 0.03f + 1.0;
    perf *= g_cfg.texture_detail + 400.0f;
    perf *= 0.002f;
    int w = g_cfg.display_width;
    int h = g_cfg.display_height;
    if (w <= 0) { w = 0x500; h = 0x400; }
    perf *= w + 300.0f;
    perf *= 0.0009090909f;
    perf *= h + 250.0f;
    perf *= 0.0011764705f;

    char buf[0x104];
    {
        tstring key("ui_estimated_performance");
        tstring* e = g_strtable.lookup(key);
        sprintf(buf, e->c_str(), (int)(30000.0f / perf));
    }
    {
        tstring text(buf);
        { char r[8]; perf_caption->set_text((float*)r, &(text)); }
    }
    { char r[8]; perf_caption->get_xy((float*)r); }
}

// @ 0x4d6dd0
void KeysPage::set_enabled_text(MissionUi* w)
{
    if (w->get15c()) {
        tstring key("ui_enabled");
        { char r[8]; w->set_text((float*)r, &(*g_strtable.lookup(key))); }
    } else {
        tstring key("ui_disabled");
        { char r[8]; w->set_text((float*)r, &(*g_strtable.lookup(key))); }
    }
}

// @ 0x4d6e50
void Rec1bc::frame_update(float)
{
#define W(x) ((MissionUi*)(void*)(x))
    if (f1b8 >= 0) {
        W(f164)->show();
        int pressed = -1;
        for (int i = 0; i < 0x100; i++)             // the last bound key wins
            if ((char)g_keyboard.is_ready(i))   // the caller tests al only
                pressed = i;
        if (pressed >= 0) {
            if (pressed != 1) {
                g_game_keys.forget(f1b8, pressed);
                f1b4 = 1;
            }
            ((KeysPage*)this)->refresh();
            f1b8 = -1;
        }
    } else {
        W(f164)->hide();
        for (int i = 0; i < 0x25; i++)
            if (W(slot_buttons[i])->clear_mask(0x10000))
                f1b8 = i;
        if (W(f38)->clear_mask(0x10000)) {
            g_game_keys.set_defaults();
            f1b4 = 1;
            ((KeysPage*)this)->refresh();
        }
        if (W(f34)->clear_mask(0x10000)) {
            g_game_keys.restore();
            f1b4 = 0;
            ((KeysPage*)this)->refresh();
        }
        if (W(f30)->clear_mask(0x10000)) {
            if (f1b4)
                g_game_keys.save();
            g_windows.set_state(w18);
        }
    }
#undef W
}

// @ 0x4d6f90
void GameKeys::forget(int index, int button)
{
    int* p = (int*)entry;
    int loop = 0x25;
    int n = 2;
    for (;;) {
        if (*p == button)
            *p = -1;
        p++;
        if (--n) continue;
        if (!--loop) break;
        n = 2;
    }
    entry[index][0] = button;
}

// @ 0x4d7000
void GameKeys::save()
{
    bin_writer w;
    char path[0x104];
    sprintf(path, "controls.dat");
    user_path(path);
    (&w.name)->assign(path);   // 0x42bcb0 (bin_writer::name is an rgl::string, the same object)
    w.is_open = 0;
    w.fp = fopen(path, "wb");
    if (w.fp) {
        w.failed = 0;
        w.is_open = 1;
    } else
        w.failed = 1;
    stream_base& s = w;
    int n = 0x25;
    s.w_int(&n);
    int* e = (int*)entry;
    for (int i = 0x25; i; i--)
        for (int j = 2; j; j--)
            s.w_int(e++);
    if (w.is_open) {
        if (s.flush() == 0)
            fclose(w.fp);
        w.failed = 1;
        w.is_open = 0;
    }
}

// @ 0x4d6fd0
void GameKeys::restore()
{
    for (int i = 0; i < 0x25; i++) {
        entry[i][0] = original[i][0];
        entry[i][1] = original[i][1];
    }
}

// 0x4d7270: one key row: scale the row by the box (shrunk to fit the text width), position it, set the text.
// The box is four float arguments (the caller pushes four immediates).
void layout_row(MissionUi* row, const tstring* text, float bx, float by, float bz, float bw);

// @ 0x4d7120
void KeysPage::refresh()
{
    KeysPageView* p = head(this);
    for (int k = 0; k < 7; k++) {
        tstring text;
        g_game_keys.key_name(k, &text);
        { char r[8]; p->captions[k]->set_text((float*)r, &(text)); }
    }

    for (int i = 0; i < 0x25; i++) {
        char assigned;
        tstring text;
        g_game_keys.key_binding_name(i, &text, (int*)&assigned);   // stores one byte
        layout_row(p->keys[i], &text, 0.034f, 3.7f, 0.072f, 0.085f);
        unsigned colour = 0xffff2200;
        if (!assigned)
            colour = 0xff000000;
        p->keys[i]->set_color(&colour);
        p->keys[i]->set15c(0);
    }

    bool unassigned = false;
    for (int i = 0; i < 0x25; i++)
        if (g_game_keys.entry[i][0] == -1) unassigned = true;
    // 0x4d71e0: Done is greyed while any key is unbound (+0xb8), Undo only while nothing changed. Written when ui_widget.h
    // had the two slots swapped and not flipped with it, so Done was greyed with every key bound.
    if (unassigned) p->w30->flag2_set();
    else            p->w30->flag2_clear();
    if (p->modified) p->w34->flag2_clear();
    else             p->w34->flag2_set();
}

// @ 0x4d7270
void layout_row(MissionUi* row, const tstring* text, float bx, float by, float bz, float bw)
{
    float s = bx * g_k065;
    int maxn = 0x800;
    int len;
    int buf[0x800];
    decode_utf8(text, &len, buf, &maxn);
    float tw = len * s * g_k039;
    float room = bx * by - bx / bz * bw;
    if (tw > room)
        s *= room / tw;
    vector2 scale;
    scale.x = s;
    scale.y = s;
    row->set_size((const float*)&scale);
    vector2 pos;
    pos.x = 0;
    pos.y = bx * 0.5f - s * g_chunk_scale;
    row->set_offset((const float*)&pos);
    { char r[8]; row->set_text((float*)r, &(*text)); }
}

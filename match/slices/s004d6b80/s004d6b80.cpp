// Slice s004d6b80: the performance caption and the controls page (key assignment table).
//   0x4d6b80  recompute the estimated-performance caption from the render-config globals
//   0x4d6dd0  caption a control with "ui_enabled" / "ui_disabled" after its value changed
//   0x4d6e50  the page update: picks the changed key row, then either closes the key-assignment overlay
//             or re-captions every row (0x4d7120) and handles the three page-level buttons
//   0x4d6f90  a table of 0x25 (key, button) pairs; forgets every pair with that button and stores a new one
//   0x4d6fd0  copies the "original" pairs (at +0x12c) back over the current ones
//   0x4d7000  build the path of controls.dat and open it for writing
//   0x4d7120  captions the 7 column headers and the 0x25 key rows and refreshes the two buttons
//   0x4d7270  lays out one key row of the table (row widget + the paperdoll preview + the text)
// All names provisional.
//
// Control layouts (verified in s004d5ba0/s004d63b0): +0x2c activate, +0x30 get_value, +0x44 changed(flag),
// +0x6c/+0x70 (vector2) setters, +0x98 colour, +0xa4 set_text(text[, position]), +0xb0 show / +0xb4 hide /
// +0xb8 enable / +0xbc disable.
//
// Text helpers: 0x42bbe0/0x42bbd0 tstring ctors, 0x42bc40 dtor, 0x42bc50 assign, 0x42bcb0 operator=.
#include <windows.h>
#include <stdio.h>
#include "rgl.h"

using rgl::vector4;

// ---- tstring -----------------------------------------------------------------------------------------
struct tstring : rgl::string {
    tstring(const char* s) : rgl::string(s) {}                              // 0x42bbe0
    tstring();                                                              // 0x42bbd0
    tstring(const tstring& o);                                              // 0x42c110
    tstring& operator=(const tstring& o);                                   // 0x42bcb0
    tstring& assign(rgl::string* o);                                        // 0x42bc50
    ~tstring();                                                             // 0x42bc40
    const char* c_str() const { return p ? p : (const char*)&p; }
};

struct StringTable {
    tstring* lookup(tstring* key);                                          // 0x43a1e0
};
extern StringTable g_string_table;                                          // 0x5e2a18

extern "C" int __cdecl sprintf(char*, const char*, ...);                    // 0x536f8f
extern "C" int __cdecl sscanf(const char*, const char*, ...);               // 0x536f7c
void* file_open(const char* name, const char* mode);                        // 0x537194 (CRT fopen, ret 8)
int   file_close(void* f);                                                  // 0x537168
int ftol(double v);                                                         // 0x535a64 (x87 argument)
void crt_init(int n);                                                       // 0x5363e0

// ---- controls ----------------------------------------------------------------------------------------
struct vector2 { float x, y; };

struct Widget {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09();
    virtual void activate(int v);                           // +0x2c
    virtual void v0c();
    virtual int  get_value();                               // +0x30
    virtual void v0d();
    virtual float get_float();                              // +0x38
    virtual void v10(); virtual void v11();
    virtual char changed(int flag);                         // +0x44
    virtual void v12(); virtual void v13(); virtual void v14(); virtual void v15();
    virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v1a(); virtual void v1b(); virtual void v1c(); virtual void v1d();
    virtual void v1e(); virtual void v1f(); virtual void v20(); virtual void v21();
    virtual void v22(); virtual void v23();
    virtual void v24(); virtual void v25();
    virtual void v26();
    virtual void v27(const vector2& v);                     // +0x6c
    virtual void v28(const vector2& v);                     // +0x70
    virtual void v29(); virtual void v2a();
    virtual void v2b(); virtual void v2c(); virtual void v2d(); virtual void v2e();
    virtual void v2f(); virtual void v30(); virtual void v31(); virtual void v32();
    virtual void v33(); virtual void v34();
    virtual void set_default();                             // +0x90
    virtual void v36();
    virtual void set_colour(int colour);                    // +0x98
    virtual void v38(); virtual void v39();
    virtual void set_text(const tstring& s);                // +0xa4
    virtual void v3a();
    virtual void set_text(const tstring& s, const vector4& pos);    // +0xac
    virtual void show();                                    // +0xb0
    virtual void hide();                                    // +0xb4
    virtual void enable();                                  // +0xb8
    virtual void disable();                                 // +0xbc
};

static inline char w_changed(Widget* w, int f) { return w->changed(f); }
static inline int  w_value(Widget* w) { return w->get_value(); }
static inline void w_activate(Widget* w, int v) { w->activate(v); }
static inline void w_set_text(Widget* w, const tstring& s) { w->set_text(s); }
static inline void w_set_text(Widget* w, const tstring& s, const vector4& p) { w->set_text(s, p); }
static inline void w_set_colour(Widget* w, int c) { w->set_colour(c); }

// the mesh-object widget class used by the key rows (+0x180 is the mesh object)
struct MeshWidget : Widget {
    char    pad00[0x180];
    void*   mesh;                                           // +0x180
    void layout(const tstring* text, void* page, const vector4& box);   // 0x4d7270
};

// ---- symbols -----------------------------------------------------------------------------------------
struct MainMenu {
    void choose(int option);                                // 0x495700 (tail-called)
};
extern MainMenu g_main_menu;                                                // 0xa73980
static inline void main_menu_choose(int option) { g_main_menu.choose(option); }
// 0x4bab70: build the user path of a file name ("%s\\Mount&Blade" + name) into the buffer at esp.
void user_path(char* buf);                                                  // 0x4bab70
// 0xa170c8: the keyboard state; is_bound(n) is 0x487900, name(n) is 0x4d7610.
struct KeyNames {
    char is_bound(int index);                                               // 0x487900
    tstring* name(tstring* out, int n);                                     // 0x4d7610
};
extern KeyNames g_key_names;                                                // 0xa170c8
// 0x487b00: the text width in pixels of a string for a font / size pair.
int text_width(const char* text, int size, int* chars);                     // 0x487b00

// ---- the key table (0xa73730) -------------------------------------------------------------------------
// 0x25 rows of two dwords: {bound key, button}; +0x12c holds the "original" copy of the pairs.
struct KeyTable {
    int entry[0x25][2];                                     // +0x00
    char pad[0x12c - 0x25 * 8];
    int original[0x25][2];                                  // +0x12c
    void forget(int button, int index);                     // 0x4d6f90
    void restore();                                         // 0x4d6fd0
    void save();                                            // 0x4d7000
    void reset_defaults();                                  // 0x4be7c0
    void caption(int index, tstring& out);                  // 0x4d7370
    void column(int index, tstring& text, char* assigned);  // 0x4d7d10
};
extern KeyTable g_key_table;                                                // 0xa73730

// ---- the page ----------------------------------------------------------------------------------------
// The page is read from both ends: the two button objects sit at +0x30 (apply / restore) and the widget
// the handler talks to last is at +0xe4, so the layout is declared as two views of the same memory.
struct VideoPageEnd {
    char    pad00[0x18];
    int     option;                                         // +0x18 (the option the menu returns to)
    char    pad1c[0x30 - 0x1c];
    Widget* w30;                                            // +0x30  apply
    Widget* w34;                                            // +0x34  cancel / restore
    Widget* w38;                                            // +0x38  defaults
    char    pad3c[0xd0 - 0x3c];
    Widget* keys[0x25];                                     // +0xd0  the key-assignment rows
    char    pad164[0x164 - 0xd0 - 0x24];
    Widget* w164;                                           // +0x164 the key-assignment overlay
    char    pad168[0x18c - 0x168];
    Widget* captions[7];                                    // +0x18c the column headers
    char    pad1a8[0x1b4 - 0x1a8];
    char    modified;                                       // +0x1b4
    char    pad1b5[0x1b8 - 0x1b5];
    int     editing;                                        // +0x1b8 the row being assigned, -1 for none
};

struct VideoPage {
    char    pad00[0xe4];
    Widget* perf_caption;                                   // +0xe4 the performance caption
    char    pad_e8[0x300];

    void update_perf();                                     // 0x4d6b80
    void set_enabled_text(Widget* w);                       // 0x4d6dd0
    void update(int reason);                                // 0x4d6e50
    void refresh();                                         // 0x4d7120
};

static inline VideoPageEnd* head(VideoPage* p) { return (VideoPageEnd*)p; }

// @ 0x4d6b80
void VideoPage::update_perf()
{
    double perf = *(float*)0x54ab28;
    if (*(int*)0x5aa030)
        perf = *(float*)0x5aa090 * *(float*)0x59393c + *(float*)0x54ab28;
    perf += *(int*)0x5aa020 * 5;
    perf += *(int*)0x5aa028 * 6;
    int limit = *(int*)0x5aa044;
    if (limit >= 100) limit = 100;
    perf += limit * *(float*)0x54a6e4;
    limit = *(int*)0x5aa048;
    if (limit >= 5) limit = 5;
    perf += limit;

    if (*(int*)0x5aa034) {
        if (*(int*)0x5aa004 >= 2)
            perf += *(float*)0x54a710;
        if (*(int*)0x5e28fc) {
            if (*(char*)0xa17f24) perf *= *(float*)0x54b888;
            else                  perf *= *(float*)0x590c80;
        } else {
            perf += *(float*)0x5935c8;
        }
    }
    if (*(int*)0x5aa084 == 2) {
        if (*(int*)0x5e28fc) {
            if (*(char*)0xa17f24) perf *= *(float*)0x54b888;
            else                  perf *= *(float*)0x590b94;
        }
    }
    if (*(int*)0x5e28fc)
        perf *= 1 - *(int*)0x5a9fc4 * *(float*)0x590b6c + *(float*)0x549cdc;
    perf *= (float)*(int*)0x5aa040 * *(float*)0x54b8b4;
    int w = *(int*)0x5a9fe4;
    int h = *(int*)0x5a9fe8;
    if (w <= 0) { w = 0x500; h = 0x400; }
    perf *= w + *(float*)0x590b20;
    perf *= *(float*)0x59685c;
    perf *= h + *(float*)0x590b60;
    perf *= *(float*)0x596858;

    char buf[0x104];
    tstring key("ui_estimated_performance");
    tstring* e = g_string_table.lookup(&key);
    sprintf(buf, e->c_str(), ftol(*(float*)0x596854 / perf));
    key.~tstring();

    tstring text(buf);
    tstring pos((const char*)0x596860);
    perf_caption->set_text(text, *(const vector4*)&pos);
    text.~tstring();
    perf_caption->set_default();
}

// @ 0x4d6dd0
void VideoPage::set_enabled_text(Widget* w)
{
    tstring* t;
    if (w_value(w)) {
        tstring key("ui_enabled");
        t = g_string_table.lookup(&key);
        key.~tstring();
    } else {
        tstring key("ui_disabled");
        t = g_string_table.lookup(&key);
        key.~tstring();
    }
    tstring text(*t);
    w->set_text(text);
    text.~tstring();
}

// @ 0x4d6e50
void VideoPage::update(int reason)
{
    VideoPageEnd* p = head(this);
    if (p->editing >= 0) {
        p->w164->hide();
        int pressed = -1;
        for (int i = 0; i < 0x100; i++) {
            if (g_key_names.is_bound(i)) {
                pressed = i;
                break;
            }
        }
        if (pressed >= 0) {
            if (pressed != 1) {
                g_key_table.forget(pressed, p->editing);
                p->modified = 1;
            }
            refresh();
            p->editing = -1;
        }
    } else {
        p->w164->show();
        for (int i = 0; i < 0x25; i++)
            if (w_changed(p->keys[i], 0x10000))
                p->editing = i;
        if (w_changed(p->w38, 0x10000)) {
            g_key_table.reset_defaults();
            p->modified = 1;
            refresh();
        }
        if (w_changed(p->w34, 0x10000)) {
            g_key_table.restore();
            p->modified = 0;
            refresh();
        }
        if (w_changed(p->w30, 0x10000)) {
            if (p->modified)
                g_key_table.save();
            main_menu_choose(p->option);
        }
    }
}

// @ 0x4d6f90
void KeyTable::forget(int button, int index)
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

// @ 0x4d6fd0
void KeyTable::restore()
{
    int* dst = (int*)entry;
    int* src = (int*)original;
    for (int i = 0; i < 0x25; i++) {
        dst[0] = src[1];
        dst[1] = src[2];
        dst += 2;
        src += 2;
    }
}

// @ 0x4d7000
void KeyTable::save()
{
    char name[0x104];
    sprintf(name, (const char*)0x54a72c, (const char*)0x54add8);
    char path[0x104];
    user_path(path);
    tstring line(name);
    tstring file(path);
    line = file;
    FILE* f = (FILE*)file_open(line.p, "wb");
    (void)f;
}

// @ 0x4d7120
void VideoPage::refresh()
{
    VideoPageEnd* p = head(this);
    for (int k = 0; k < 7; k++) {
        tstring text;
        g_key_table.caption(k, text);
        tstring pos;
        w_set_text(p->captions[k], text, *(const vector4*)&pos);
        text.~tstring();
    }

    for (int i = 0; i < 0x25; i++) {
        MeshWidget* row = (MeshWidget*)p->keys[i];
        char assigned;
        tstring text;
        g_key_table.column(i, text, &assigned);
        row->layout(&text, this, *(const vector4*)0x54a480);
        if (assigned) w_set_colour(row, (int)0xffff2200);
        else          w_set_colour(row, (int)0xff000000);
        w_activate(row, 0);
        text.~tstring();
    }

    bool unassigned = false;
    for (int i = 0; i < 0x25; i++)
        if (g_key_table.entry[i][0] == -1) unassigned = true;
    if (unassigned) p->w30->enable();
    else            p->w30->disable();
    if (p->modified) p->w34->disable();
    else             p->w34->enable();
}

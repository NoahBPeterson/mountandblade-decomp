#include <windows.h>
#include <stdio.h>
#include <new>

extern "C" int _fltused = 0;
extern "C" __declspec(dllimport) long __stdcall SHGetFolderPathA(void* hwnd, int csidl, void* token, unsigned flags, char* path);

void mb_snprintf(char* dest, unsigned size, const char* fmt, ...);   // 0x415100

// @ 0x4dd7b0
void save_path(char* out)
{
    char dir[0x104];
    char base[0x104];
    SHGetFolderPathA(0, 5, 0, 0, dir);
    sprintf(base, "%s\\Mount&Blade Savegames", dir);
    CreateDirectoryA(base, 0);
    sprintf(base, "%s", out);
    sprintf(out, "%s\\Mount&Blade Savegames\\%s", dir, base);
}

// @ 0x4dd6e0
void scan_saves(int ids, int* count, int* first, const char* prefix)
{
    char buf[0x104];
    char file[0x200];
    *count = 0;
    *first = -1;
    unsigned i = 0;
    do {
        sprintf(buf, "");
        save_path(buf);
        mb_snprintf(file, 0x200, "%s%s\\sg%d%d.sav", buf, prefix, i / 10, i % 10);
        FILE* f = fopen(file, "r");
        if (f) {
            fclose(f);
            *(int*)(ids + *count * 4) = i;
            *count = *count + 1;
        } else if (*first == -1) {
            *first = i;
        }
        i++;
    } while ((int)i < 9);
}

// ============================================================================
// MainMenu::create (0x4dcef0): build the main menu window: background container,
// the six/seven menu rows, the logo and mercenary panels, the version label, the
// save-game scan and the load/exit block.
//
// Layout recovered from the disassembly: +0x2c win, +0x30 the menu block
// (BlockOwner, 0x30 bytes), +0x60..+0x78 seven rows, +0x7c version, +0x80 logo,
// +0x84 mercenary picture, +0x88 first free save slot.
// ============================================================================

struct tstring {
    int pos;            // +0x00
    char* buf;          // +0x04
    int cap;            // +0x08
    int len;            // +0x0c
    tstring(const char* s);                     // 0x42bbe0 (ret 4)
    ~tstring();                                 // 0x42bc40
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

struct StrTable {
    tstring* lookup(const tstring& key) throw();        // 0x43a1e0 (ret 4)
};
extern StrTable g_strtable;                     // 0x5e2a18

struct vector2 {
    float x, y;
    vector2() {}
    vector2(float x_, float y_) : x(x_), y(y_) {}
};

// ---- widgets ---------------------------------------------------------------------------------------
// Slot map verified against the widget vtable 0x591d58 / 0x592848 (see s004dd830):
//   +0x58 pos, +0x64 size, +0x6c scale, +0x74 text position, +0x7c/+0x80 image,
//   +0x98/+0x9c colours, +0xb8 hide, +0xc8 add_child.
struct Widget {
    virtual void w00(); virtual void w01(); virtual void w02(); virtual void w03(); virtual void w04();
    virtual void w05(); virtual void w06(); virtual void w07(); virtual void w08(); virtual void w09();
    virtual void w10(); virtual void w11(); virtual void w12(); virtual void w13(); virtual void w14();
    virtual void w15(); virtual void w16(); virtual void w17(); virtual void w18(); virtual void w19();
    virtual void w20(); virtual void w21();
    virtual void set_pos(const vector2& v);         // +0x58
    virtual void w23(); virtual void w24();
    virtual void set_size(const vector2& v);        // +0x64
    virtual void w26();
    virtual void set_scale(const vector2& v);       // +0x6c
    virtual void w28();
    virtual void set_text_pos(int* v);              // +0x74
    virtual void w30();
    virtual void add_image(int f, void* m);         // +0x7c
    virtual void add_image2(int f, void* m);        // +0x80
    virtual void w34(); virtual void w35(); virtual void w36(); virtual void w37();
    virtual void w38();
    virtual void set_colour(int* c);                // +0x98
    virtual void set_colour2(int* c);               // +0x9c
    virtual void w40();
    virtual void w41(); virtual void w42(); virtual void w43(); virtual void w44(); virtual void w45();
    virtual void hide();                            // +0xb8
    virtual void w47(); virtual void w48(); virtual void w49();
    virtual void add_child(void* c);                // +0xc8

    void base_ctor();                               // 0x48ae20
};

// the container widget (vptr 0x592848, size 0x190)
struct UiContainer : Widget {
    char pad0[0x188 - 0x04];
    char f188;
    void ctor_container();                          // 0x4931a0
};

// the 0x178-byte image/panel widget (vptr set by base_ctor)
struct UiImage : Widget {
    char pad0[0x178 - 0x04];
};

// a menu block owner: keeps the "current kind" index and the block widget
struct BlockOwner {
    char pad0[0x10];
    int clock;                                      // +0x10
    char pad14[0x2c - 0x14];
    Widget* block;                                  // +0x2c
    void init(Widget* parent, int which);           // 0x4dd920 (ret 8)
};

struct gui_manager {
    void reset();                                   // 0x42d1b0
    void push_back(void* const& x);                 // 0x49b120 (ret 4)
};
extern gui_manager g_gui;                           // 0x5d7080

struct ResMgr {
    void* get_object(const tstring& name);          // 0x419540 (ret 4)
};
extern ResMgr g_resmgr;                             // 0x5d6fa0

void* __fastcall mesh_clone(void* m);               // 0x41c6d0 (ecx = mesh)

// the save-prefix global (char*); the c_str idiom falls back to its own address
extern char* g_save_prefix;                         // 0x8ee3c3c

extern int   g_window_flags;                        // 0xa73a10 (set to 0x400000)
extern int   g_tutorial_enabled;                    // 0xa181a4
extern int   g_menu_font_red;                       // 0xa181bc
extern int   g_menu_font_green;                     // 0xa181c0
extern int   g_menu_font_blue;                      // 0xa181c4
extern int   g_main_menu_kind;                      // 0xa75f34
extern char  g_frame_flag;                          // 0x5d8788

Widget* add_row(Widget* parent, const tstring* key, float x, float y, float w, float h);   // 0x4dd830
Widget* add_big_row(Widget* parent, const tstring* text, float x, float y, float w, float h); // 0x4dada0

struct MainMenu {
    char pad0[0x2c];
    UiContainer* win;                               // +0x2c
    BlockOwner block;                               // +0x30
    Widget* row_tutorial;                           // +0x60
    Widget* row_new_game;                           // +0x64
    Widget* row_restore;                            // +0x68
    Widget* row_custom;                             // +0x6c
    Widget* row_options;                            // +0x70
    Widget* row_credits;                            // +0x74
    Widget* row_exit;                               // +0x78
    Widget* version;                                // +0x7c
    Widget* logo;                                   // +0x80
    Widget* pic;                                    // +0x84
    int     first_free;                             // +0x88
    void create();
};

// @ 0x4dcef0 MainMenu::create
void MainMenu::create()
{
    char text[0x200];
    int ids[9];
    char version_digits[0x40];
    int count;

    g_window_flags = 0x400000;
    g_gui.reset();

    UiContainer* p = (UiContainer*)::operator new(0x190);
    if (p != 0) {
        p->ctor_container();
        *(void**)p = (void*)0x592848;
        p->f188 = 0;
    }
    win = p;

    win->set_pos(vector2(-0.001f, -0.001f));
    win->set_size(vector2(1.003f, 0.753f));
    int base_colour = 0xff442323;
    win->set_colour(&base_colour);

    int r = g_menu_font_red;
    r = r < 0 ? 0 : (r >= 0xff ? 0xfe : r);
    int g = g_menu_font_green;
    g = g < 0 ? 0 : (g >= 0xff ? 0xfe : g);
    int b = g_menu_font_blue;
    b = b < 0 ? 0 : (b >= 0xff ? 0xfe : b);
    int colour = ((((unsigned)r | 0xffffff00u) << 8) | (unsigned)g) << 8 | (unsigned)b;
    int down = (int)0xffffdd00;
    int tp = 0;

    float step = g_tutorial_enabled ? 0.06f : 0.08f;

    {
        tstring k("ui_start_tutorial");
        row_tutorial = add_row(win, g_strtable.lookup(k), 0.33f,
                               (g_tutorial_enabled ? 5 : 4) * step + 0.08f, 0.6f, 0.05f);
    }
    {
        tstring k("ui_start_a_new_game");
        row_new_game = add_row(win, g_strtable.lookup(k), 0.33f,
                               (g_tutorial_enabled ? 4 : 3) * step + 0.08f, 0.6f, 0.05f);
    }
    {
        tstring k("ui_restore_a_saved_game");
        row_restore = add_row(win, g_strtable.lookup(k), 0.33f,
                              (g_tutorial_enabled ? 3 : 2) * step + 0.08f, 0.6f, 0.05f);
    }
    if (g_tutorial_enabled) {
        {
            tstring k("ui_custom_battle");
            row_custom = add_row(win, g_strtable.lookup(k), 0.33f, step + step + 0.08f, 0.6f, 0.05f);
        }
        row_custom->set_colour(&colour);
        row_custom->set_colour2(&down);
        row_custom->set_text_pos(&tp);
    } else {
        row_custom = 0;
    }
    {
        tstring k("ui_options");
        row_options = add_row(win, g_strtable.lookup(k), 0.33f, step + 0.08f, 0.6f, 0.05f);
    }
    {
        tstring k("ui_exit_to_windows");
        row_exit = add_row(win, g_strtable.lookup(k), 0.68f, 0.02f, 0.3f, 0.04f);
    }
    {
        tstring k("ui_credits");
        row_credits = add_row(win, g_strtable.lookup(k), 0.33f, 0.08f, 0.5f, 0.05f);
    }

    row_tutorial->set_colour(&colour);
    row_new_game->set_colour(&colour);
    row_restore->set_colour(&colour);
    row_options->set_colour(&colour);
    row_credits->set_colour(&colour);
    row_exit->set_colour(&colour);

    row_tutorial->set_colour2(&down);
    row_new_game->set_colour2(&down);
    row_restore->set_colour2(&down);
    row_options->set_colour2(&down);
    row_credits->set_colour2(&down);
    row_exit->set_colour2(&down);

    row_tutorial->set_text_pos(&tp);
    row_new_game->set_text_pos(&tp);
    row_restore->set_text_pos(&tp);
    row_options->set_text_pos(&tp);
    row_credits->set_text_pos(&tp);
    tp = 8;
    row_exit->set_text_pos(&tp);

    {
        UiImage* p = (UiImage*)::operator new(0x178);
        if (p != 0)
            p->base_ctor();
        else
            p = 0;
        logo = p;
    }
    logo->set_pos(vector2(0.2f, 0.55f));
    logo->set_size(vector2(0.6f, 0.15f));
    {
        tstring k("warrider_logo");
        logo->add_image(0, mesh_clone(g_resmgr.get_object(k)));
    }
    win->add_child(logo);

    {
        UiImage* p = (UiImage*)::operator new(0x178);
        if (p != 0)
            p->base_ctor();
        else
            p = 0;
        pic = p;
    }
    pic->set_pos(vector2(0.015f, 0.02f));
    {
        tstring k("pic_mercenary");
        pic->add_image2(0, mesh_clone(g_resmgr.get_object(k)));
    }
    win->add_child(pic);

    sprintf(version_digits, "%d.%d%d%d", 1, 0, 1, 1);
    {
        tstring k("ui_version_value");
        tstring* fmt = g_strtable.lookup(k);
        sprintf(text, fmt->c_str(), version_digits);
    }
    {
        tstring t(text);
        version = add_big_row(win, &t, 0.67f, 0.5f, 0.3f, 0.04f);
    }
    version->set_colour(&colour);

    const char* prefix = g_save_prefix ? g_save_prefix : (const char*)&g_save_prefix;
    scan_saves((int)ids, &count, &first_free, prefix);
    if (first_free == -1)
        row_new_game->hide();

    block.init(win, g_main_menu_kind);
    if (block.clock <= 0) {
        tstring k("load_window");
        win->add_image(0, mesh_clone(g_resmgr.get_object(k)));
    }

    void* wv = win;
    g_frame_flag = 1;
    g_gui.push_back(wv);
}

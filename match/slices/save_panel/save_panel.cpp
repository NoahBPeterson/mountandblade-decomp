// Slice save_panel: the save/load screen's update and the ini-driven label setup.
//   0x4ea290  SavePanel::update(int): slot buttons, save-name dialog, delete confirm, save / delete file
//   0x4e4c70  label_refresh(MissionUi*, tstring): apply the "<key>_..." ini values to a label widget
// Layouts follow the verified neighbouring slices (s004ea0c0, s004e4c70, s004e85d0).
#include <windows.h>
#include <stdio.h>
#include "rgl.h"

extern "C" int _fltused = 0;

// ---- engine string (16 bytes) ----------------------------------------------------------------------------
struct tstring {
    int pos;            // +0x00
    char* buf;          // +0x04
    int cap;            // +0x08
    int len;            // +0x0c
    tstring();                                          // 0x42bbd0
    tstring(const char* s);                             // 0x42bbe0
    tstring(const tstring& o);                          // 0x42c110
    ~tstring();                                         // 0x42bc40
    void operator=(const tstring& o);                   // 0x42bc50
    void assign(const char* s);                         // 0x42bcb0
    void clear();                                       // 0x42be50
    tstring operator+(const tstring& s) const;          // 0x42c840
};

// ---- widgets ---------------------------------------------------------------------------------------------
// the plain widget: only is_key_down is used here (vtable slot +0x44 = 17)
struct Control {
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
    virtual void v10(); virtual void v14(); virtual void v18(); virtual void v1c();
    virtual void v20(); virtual void v24(); virtual void v28();
    virtual void set_cursor(int on);                    // +0x2c
    virtual void v30(); virtual void v34(); virtual void v38(); virtual void v3c();
    virtual void v40();
    virtual char is_key_down(int key);                   // +0x44
    virtual void v48(); virtual void v4c(); virtual void v50(); virtual void v54();
    virtual void v58(); virtual void v5c(); virtual void v60(); virtual void v64();
    virtual void v68(); virtual void v6c(); virtual void v70(); virtual void v74();
    virtual void v78(); virtual void v7c(); virtual void v80(); virtual void v84();
    virtual void v88(); virtual void v8c(); virtual void v90(); virtual void v94();
    virtual void v98(); virtual void v9c(); virtual void va0(); virtual void va4();
    virtual void va8(); virtual void vac(); virtual void vb0(); virtual void vb4();
    virtual void hide();                                 // +0xb8
    virtual void show();                                 // +0xbc
};

// the label widget of the save screen: the setters the ini keys feed (see s004e4c70)
struct vector2 {
    float x, y;
    vector2() {}
    vector2(float a, float b) { x = a; y = b; }
};
struct MissionUi {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03(); virtual void v04();
    virtual void v05(); virtual void v06(); virtual void v07(); virtual void v08(); virtual void v09();
    virtual void v10(); virtual void v11(); virtual void v12(); virtual void v13(); virtual void v14();
    virtual void v15(); virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v20(); virtual void v21();
    virtual void set_pos(const vector2& v);              // +0x58
    virtual void v23(); virtual void v24();
    virtual void set_scale(const vector2& v);            // +0x64
    virtual void v26();
    virtual void set_text_size(const vector2& v);        // +0x6c
    virtual void set_text_pos(const vector2& v);         // +0x70
    virtual void set_flags(const int& v);                // +0x74
    virtual void v30(); virtual void v31(); virtual void v32(); virtual void v33();
    virtual void v34(); virtual void v35(); virtual void v36(); virtual void v37();
    virtual void set_color(const int& v);                // +0x98
    virtual void set_highlight(const int& v);            // +0x9c
};

// ---- the save panel --------------------------------------------------------------------------------------
struct SaveSlot {                                        // stride 0x30, at this+0x2c
    tstring title;                                       // +0x00 (panel +0x2c)
    char pad10[0x10];                                    // +0x10
    int  used;                                           // +0x20 (panel +0x4c)
    char pad24[8];                                       // +0x24
    char flags[3];                                       // +0x2c (panel +0x58)
};
struct SavePanel {
    char pad0[0x18];
    int  f18;                                            // +0x18 (the menu state to return to)
    char pad1c[0x2c - 0x1c];
    SaveSlot slots[9];                                   // +0x2c
    char pad1dc[0x1e4 - 0x1dc];
    Control* buttons[9];                                 // +0x1e4
    Control* list_panel;                                 // +0x208
    Control* f20c;                                       // +0x20c
    Control* info_panel;                                 // +0x210
    char pad214[4];
    int selected;                                        // +0x218
    int f21c;                                            // +0x21c
    int f220;                                            // +0x220
    tstring f224;                                        // +0x224
    char deleting;                                       // +0x234
    void update(int arg);
    void select(int idx);                                // 0x4ea850
};

// the main menu screen object (0xa73980): its +0 word points at the open dialog's record
struct menu_record {
    char pad0[4];
    int  open;                                           // +0x04
    char pad08[0x1c - 0x08];
    tstring title;                                       // +0x1c
    char pad2c[0x4c - 0x2c];
    int  choice;                                         // +0x4c
    int  f50;                                            // +0x50
    tstring sub;                                         // +0x54
    int  f64;                                            // +0x64
    tstring header;                                      // +0x68
    tstring answer_yes;                                  // +0x78
    tstring answer_no;                                   // +0x88
};
struct main_menu {
    menu_record* rec;                                    // +0x00
    void choose(int option);                             // 0x495700
};
extern main_menu g_main_menu;                            // 0xa73980

// the save-name dialog object behind 0xa7398c
struct SaveDialog {
    char pad0[4];
    int  f4;                                             // +0x04
    char pad8[0x48 - 0x08];
    tstring f48;                                         // +0x48
    char pad58[0x8c - 0x58];
    int  f8c;                                            // +0x8c
};
extern SaveDialog* g_save_dialog;                        // 0xa7398c

struct StrTable { const tstring* lookup(const tstring& key); };   // 0x43a1e0
extern StrTable g_strtable;                              // 0x5e2a18
struct MsgLog { void add(tstring msg, int color); };     // 0x452c00
extern MsgLog g_msglog;                                  // 0x5d7080
void mb_snprintf(char* dst, unsigned cap, const char* fmt, ...);   // 0x415100
int  __cdecl sprintf(char* dst, const char* fmt, ...);   // 0x536f8f
int  __cdecl crt_remove(const char* path);               // 0x539023
void __cdecl save_path(char* path);                      // 0x4dd7b0
int  __cdecl save_game(int slot, tstring* name);         // 0x4e8980
extern tstring g_save_prefix_str;                        // 0x8ee3c38 (buf at +4 -> 0x8ee3c3c)
extern char* g_game;                                     // 0x5e2930
extern int g_a17420;                                     // 0xa17420
extern int g_a17820;                                     // 0xa17820

// the engine stream object the new-save branch builds (0x54a740 -> 0x54ae48, dtor body 0x54ad68)
struct streambase {
    void* vptr;                                          // +0x00
    streambase() { vptr = (void*)0x54a740; }
};
struct stream : streambase {
    tstring name;                                        // +0x04
    int f14;                                             // +0x14
    char f18;                                            // +0x18
    int f1c;                                             // +0x1c
    tstring data;                                        // +0x20
    FILE* file;                                          // +0x30
    char failed;                                         // +0x34
    stream()
    {
        f18 = 0;
        f1c = 0;
        failed = 1;
        file = 0;
        f14 = 0;
        vptr = (void*)0x54ae48;
    }
    ~stream() { vptr = (void*)0x54ad68; }
};

// the ini table at 0xa44240: only the lookups this slice makes
struct config_table {
    int find(const tstring& key);                        // 0x4bd5a0
    int get_hex(int* idx);                               // 0x4e51e0
    float get_float(int* idx);                           // 0x4e5230
};
extern config_table g_config;                            // 0xa44240

static const char* text_of(const tstring* s)
{
    char* b = s->buf;
    return b ? b : (const char*)&s->buf;
}

// @ 0x4ea290
void SavePanel::update(int)
{
    char dir[0x200];
    char name[0x200];

    for (int i = 0; i < 9; i++) {
        if (buttons[i] && buttons[i]->is_key_down(0x10000))
            select(i);
    }
    if (list_panel->is_key_down(0x10000)) {
        if (deleting) {
            f220 = -1;
            {
                tstring title("ui_set_save_file_name");
                const tstring* r = g_strtable.lookup(title);
                mb_snprintf(name, 0x200, text_of(r));
            }
            tstring* slotname;
            if (slots[selected].used)
                slotname = &slots[selected].title;
            else
                slotname = (tstring*)(*(int*)(g_game + 0x8c) * 0x508 +
                                      *(int*)(g_game + 0x13a00));
            {
                tstring cancel("ui_cancel");
                tstring ok("ui_ok");
                tstring title2(name);
                const tstring* rc = g_strtable.lookup(cancel);
                const tstring* ro = g_strtable.lookup(ok);
                menu_record* m = g_main_menu.rec;
                m->header.clear();
                m->title = title2;
                m->answer_yes = *ro;
                m->answer_no = *rc;
                m->sub = *slotname;
                m->choice = (int)&f220;
                m->f50 = (int)&f224;
                m->open = 3;
            }
            g_main_menu.choose(0);
            return;
        } else if (!slots[selected].flags[0] && !slots[selected].flags[1] &&
                   !slots[selected].flags[2]) {
            stream s;
            tstring unused1;
            tstring unused2;
            char* p = g_save_prefix_str.buf ? g_save_prefix_str.buf : (char*)&g_save_prefix_str.buf;
            sprintf(dir, "");
            save_path(dir);
            mb_snprintf(name, 0x200, "%s%s\\sg%d%d.sav", dir, p, selected / 10, selected % 10);
            g_save_dialog->f4 = 1;
            g_save_dialog->f48.assign(name);
            g_save_dialog->f8c = selected;
            g_main_menu.choose(3);
            return;
        }
    }
    if (f20c->is_key_down(0x10000)) {
        g_main_menu.choose(f18);
        return;
    }
    if (info_panel->is_key_down(0x10000) ||
        (g_a17420 != 0 && g_a17820 == 0 &&
         (((unsigned char*)info_panel)[0x104] & 2) == 0)) {
        f21c = -1;
        {
            tstring title("ui_confirm_delete_game");
            const tstring* r = g_strtable.lookup(title);
            mb_snprintf(name, 0x200, text_of(r));
        }
        {
            tstring no("ui_no");
            tstring yes("ui_yes");
            tstring title2(name);
            const tstring* rn = g_strtable.lookup(no);
            const tstring* ry = g_strtable.lookup(yes);
            menu_record* m = g_main_menu.rec;
            m->header.clear();
            m->title = title2;
            m->answer_yes = *ry;
            m->answer_no = *rn;
            m->choice = (int)&f21c;
            m->f50 = 0;
            m->open = 1;
        }
        g_main_menu.choose(0);
    } else {
        if (f21c == 0) {
            char* p = g_save_prefix_str.buf ? g_save_prefix_str.buf : (char*)&g_save_prefix_str.buf;
            sprintf(dir, "");
            save_path(dir);
            mb_snprintf(name, 0x200, "%s%s\\sg%d%d.sav", dir, p, selected / 10, selected % 10);
            if (crt_remove(name) != 0) {
                tstring err("ui_error_removing_file");
                g_msglog.add(*g_strtable.lookup(err), -1);
            }
            g_main_menu.choose(0x15);
            return;
        }
        if (f220 != 0)
            return;
        save_game(selected, &f224);
        *(tstring*)((char*)g_game + 8) = f224;
        tstring ok("ui_game_saved");
        g_msglog.add(*g_strtable.lookup(ok), -1);
        g_main_menu.choose(f18);
    }
}

// The four float pairs each start from a fresh {0,0}: the original copies a returned vector2
// temporary into the local for the later pairs and initializes the first one directly.
// @ 0x4e4c70
void label_refresh(MissionUi* w, tstring key)
{
    int idx;
    tstring name = key + tstring("_text_color");
    if ((idx = g_config.find(name)) >= 0)
        w->set_color(g_config.get_hex(&idx));
    name = key + tstring("_highlight_text_color");
    if ((idx = g_config.find(name)) >= 0)
        w->set_highlight(g_config.get_hex(&idx));
    vector2 v = vector2(0.0f, 0.0f);
    bool any = false;
    name = key + tstring("_position_x");
    if ((idx = g_config.find(name)) >= 0) {
        v.x = g_config.get_float(&idx);
        any = true;
    }
    name = key + tstring("_position_y");
    if ((idx = g_config.find(name)) >= 0) {
        v.y = g_config.get_float(&idx);
        w->set_pos(v);
    } else if (any) {
        w->set_pos(v);
    }
    any = false;
    v = vector2(0.0f, 0.0f);
    name = key + tstring("_size_x");
    if ((idx = g_config.find(name)) >= 0) {
        v.x = g_config.get_float(&idx);
        any = true;
    }
    name = key + tstring("_size_y");
    if ((idx = g_config.find(name)) >= 0) {
        v.y = g_config.get_float(&idx);
        w->set_scale(v);
    } else if (any) {
        w->set_scale(v);
    }
    any = false;
    v = vector2(0.0f, 0.0f);
    name = key + tstring("_text_size_x");
    if ((idx = g_config.find(name)) >= 0) {
        v.x = g_config.get_float(&idx);
        any = true;
    }
    name = key + tstring("_text_size_y");
    if ((idx = g_config.find(name)) >= 0) {
        v.y = g_config.get_float(&idx);
        w->set_text_size(v);
    } else if (any) {
        w->set_text_size(v);
    }
    any = false;
    v = vector2(0.0f, 0.0f);
    name = key + tstring("_text_position_x");
    if ((idx = g_config.find(name)) >= 0) {
        v.x = g_config.get_float(&idx);
        any = true;
    }
    name = key + tstring("_text_position_y");
    if ((idx = g_config.find(name)) >= 0) {
        v.y = g_config.get_float(&idx);
        w->set_text_pos(v);
    } else if (any) {
        w->set_text_pos(v);
    }
    name = key + tstring("_text_flags");
    if ((idx = g_config.find(name)) >= 0)
        w->set_flags(g_config.get_hex(&idx));
}

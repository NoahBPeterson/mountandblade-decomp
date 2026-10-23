// Slice s004d7d10: the options panel's key bindings, cursor, text refresh, faction report and the character
// import/export confirmation flow.
//   0x4d7d10 options_panel::key_binding_name  name of the binding at slot*8 of the panel (+0, +8, +0x10, ...)
//   0x4d7d90 options_panel::on_key_event      panel-wide key handling (refresh / hand over to the main menu)
//   0x4d7de0 options_panel::update_cursor     show the mouse cursor only while no menu is open
//   0x4d7e00 options_panel::refresh_texts     re-read the panel's text entry and rebuild the faction report
//   0x4d7e50 options_panel::draw_faction_report  one line per faction with its renown
//   0x4d8000 options_panel::import_export     confirm and perform the character import/export
// Names are provisional; every layout was derived from the disassembly of this slice.
#include <windows.h>
#include <stdio.h>
#include <math.h>

// ---------------------------------------------------------------------------------------------------------
// tstring (16 bytes): read cursor, heap buffer, capacity, length (layout from s0046ad50/tstring.h).
struct tstring {
    int pos;                                                        // +0
    char* buf;                                                      // +4 (text lives at &buf while empty)
    int cap;                                                        // +8
    int len;                                                        // +0xc
    tstring();                                                      // 0x42bbd0
    tstring(const char* s);                                         // 0x42bbe0
    tstring(const tstring& o);                                      // 0x42c110
    ~tstring();                                                     // 0x42bc40
    void operator=(const tstring& o);                               // 0x42bc50
    tstring* assign(const char* s);                                 // 0x42bcb0 (returns this)
    void clear();                                                   // 0x42be50
    static void release(tstring* s) { s->~tstring(); }              // explicit temporary destruction
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};
extern const char EMPTY_TEXT[];                                     // 0x54a480 ("")
extern const float RENOWN_SCALE;                                    // 0x54ab28 (100.0f)

// the game's localized-string table (0x5e2a18): lookup by a scratch tstring holding the text key
struct StrTable {
    tstring* lookup(tstring* key);                                  // 0x43a1e0 (ret 4)
    tstring* find(tstring* key);                                    // 0x4c6010 (ret 4; null when not translated)
};
extern StrTable g_strtable;                                         // 0x5e2a18

// the keyboard object (0xa170c8); the key-name function only passes it through
struct ctr {
    char pad0[0x100];
    tstring key_name(int key);                                      // 0x4d7610 (slice s004d7370; ret 8: hidden return + key)
};
extern ctr g_keyboard;                                              // 0xa170c8

// ---------------------------------------------------------------------------------------------------------
// a ui control: +0x2c set_cursor(bool), +0x44 is_key_down(int)
struct ui_control {
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
    virtual void v10(); virtual void v14(); virtual void v18(); virtual void v1c();
    virtual void v20(); virtual void v24(); virtual void v28();
    virtual void set_cursor(int show);                              // +0x2c
    virtual void v30(); virtual void v34(); virtual void v38(); virtual void v3c();
    virtual void v40();
    virtual char is_key_down(int key);                              // +0x44
};

// set_text returns an 8-byte object through a hidden pointer that no caller destroys (a non-POD pair; same
// slot as the terrain page's widgets, s004f25a0)
struct WRet { int a, b; WRet(); };
// the panel's text store
struct text_store {
    virtual void t00(); virtual void t04(); virtual void t08(); virtual void t0c();
    virtual void t10(); virtual void t14(); virtual void t18(); virtual void t1c();
    virtual void t20(); virtual void t24(); virtual void t28(); virtual void t2c();
    virtual void t30(); virtual void t34(); virtual void t38(); virtual void t3c();
    virtual void t40(); virtual void t44(); virtual void t48(); virtual void t4c();
    virtual void t50(); virtual void t54(); virtual void t58(); virtual void t5c();
    virtual void t60(); virtual void t64(); virtual void t68(); virtual void t6c();
    virtual void t70(); virtual void t74(); virtual void t78(); virtual void t7c();
    virtual void t80(); virtual void t84(); virtual void t88(); virtual void t8c();
    virtual void t90(); virtual void t94(); virtual void t98(); virtual void t9c();
    virtual void ta0();
    virtual WRet set_text(const tstring& s);                        // +0xa4
    virtual void ta8();
    virtual void add_text(tstring* text, float* pos);               // +0xac
};

// the local text entries of the panel's text store: drop the newest one (0x4d7e00 keeps it as a call)
void text_store_clear_last(text_store* self);

// a troop record (0x508 bytes): the name tstring at +0, the name's text key at +0x20
struct troop_rec {
    tstring name;                                                   // +0x00
    char pad10[0x10];
    tstring key;                                                    // +0x20
    char pad30[0x508 - 0x30];
    tstring* display_name();                                        // 0x43a480
};

// a faction record (0x300 bytes): the displayed name at +0, a "has a translated name" flag at +0x10, the name's
// text key at +0x14, the renown at +0x2c
struct faction_rec {
    tstring name;                                                   // +0x00
    char has_name;                                                  // +0x10
    char pad11[3];
    tstring key;                                                    // +0x14
    char pad24[0x08];
    float renown;                                                   // +0x2c
    char tail[0x300 - 0x30];
};

struct ini_file;
struct outfile;

// the game globals (0x5e2930)
struct game_globals {
    char pad0[0x8c];
    int  player_troop;                                              // +0x8c (index into the troop table)
    char pad90[0x13a00 - 0x90];
    troop_rec* troops;                                              // +0x13a00 (0x508 bytes each)
    char pad04[0x13a48 - 0x13a04];
    faction_rec* factions;                                          // +0x13a48 (0x300 bytes each)
    int  num_factions;                                              // +0x13a4c
    void export_character(outfile* f);                              // 0x4d86d0 (ret 4)
    void import_apply(ini_file* table);                             // 0x4d8c20 (ret 4)
};
extern game_globals* g_game;                                        // 0x5e2930 (pointer to the game globals)
extern int g_trial_mode;                                            // 0x5e29c4

// the main-menu record (g_main_menu.rec): +0x04 "open", +0x1c title, +0x4c/+0x50 selection state, +0x68 header,
// +0x78/+0x88 answers
struct MenuRec {
    char pad0[4];
    int  open;                                                      // +0x04
    char pad08[0x1c - 0x08];
    tstring title;                                                  // +0x1c
    char pad2c[0x4c - 0x2c];
    int  choice;                                                    // +0x4c
    int  v50;                                                       // +0x50
    char pad54[0x68 - 0x54];
    tstring header;                                                 // +0x68
    tstring answer_yes;                                             // +0x78
    tstring answer_no;                                              // +0x88
    void setup(const tstring* hdr, const tstring* text)             // 0x48a3a0 (ret 8)
    {
        header = *hdr;
        title = *text;
        choice = 0;
        v50 = 0;
        open = 0;
    }
    void ask_yes_no(const tstring* text, const tstring* yes, const tstring* no, int* result);  // 0x48a3d0 (ret 0x10)
};
struct MainMenu {
    MenuRec* rec;                                                   // +0
    void reset(int arg);                                            // 0x494260 (ret 4)
    void choose(int choice_id);                                     // 0x495700 (ret 4)
};
extern MainMenu g_main_menu;                                        // 0xa73980

bool mb_file_exists(const char* path);                               // 0x415120

// the text records read by the import (see s004bd170 for the layout)
struct ini_file {
    char pad0[0x2c000];
    int  count;                                                     // +0x2c000
    ini_file();                                                     // 0x4c1be0
    void load(FILE* f);                                             // 0x4bd630
    ~ini_file();                                                    // 0x4be920
};

// the export file object: three classes in a chain (vtables 0x54a740 / 0x595618 / 0x595990); the middle one's
// destructor closes the file (virtual close at slot 26 = +0x68, called through the class's own vtable)
struct outfile_a {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09(); virtual void v10(); virtual void v11();
    virtual void v12(); virtual void v13(); virtual void v14(); virtual void v15();
    virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v20(); virtual void v21(); virtual void v22(); virtual void v23();
    virtual void v24(); virtual void v25();
    virtual int close();                                            // +0x68
    tstring path;                                                   // +0x04
    int f14;                                                        // +0x14
    char f18;                                                       // +0x18
    int f1c;                                                        // +0x1c
    tstring t20;                                                    // +0x20
    FILE* fp;                                                       // +0x30
    char failed;                                                    // +0x34
    char open;                                                      // +0x35
};
struct outfile_b : outfile_a {
    outfile_b() { f18 = 0; f1c = 0; failed = 1; open = 0; f14 = 0; }
    void shut()
    {
        if (open) {
            if (close() == 0)
                fclose(fp);
            failed = 1;
            open = 0;
        }
    }
    ~outfile_b() { shut(); }
    virtual void b_dummy();
};
struct outfile : outfile_b {
    outfile(const char* p) { path.assign(p); }
    virtual void c_dummy();
};

#define MODE_RA " RA"                                               // 0x595990
#define MODE_R  "r"                                                 // 0x54a8fc
#define MODE_W  "w"                                                 // 0x54b058

// the options panel (offsets read by this slice)
struct options_panel;
void options_panel_key_binding_name(options_panel*, int, tstring*, int*);
void options_panel_on_key_event(options_panel*, int);
void options_panel_update_cursor(options_panel*);
void options_panel_refresh_texts(options_panel*);

struct options_panel {
    int  key_menu;                                                  // +0x00
    int  menu_open;                                                 // +0x04
    char pad08[0x18 - 0x08];
    int  menu_choice;                                               // +0x18
    char pad1c[0x2c - 0x1c];
    text_store* texts;                                              // +0x2c
    ui_control* menu_keys;                                          // +0x30
    ui_control* import_key;                                         // +0x34
    ui_control* cursor_control;                                     // +0x38
    ui_control* refresh_key;                                        // +0x3c
    int  export_pending;                                            // +0x40
    int  import_pending;                                            // +0x44

    void key_binding_name(int slot, tstring* out, int* translated);  // 0x4d7d10
    void on_key_event(int);                                         // 0x4d7d90
    __declspec(noinline) void update_cursor();                      // 0x4d7de0
    __declspec(noinline) void refresh_texts();                      // 0x4d7e00
    __declspec(noinline) void draw_faction_report();                // 0x4d7e50
    void export_path(char* out);                                    // 0x4d9260 (ret 4)
    void import_export(int);                                        // 0x4d8000
};

// @ 0x4d7d10
void options_panel::key_binding_name(int slot, tstring* out, int* translated)
{
    const int key = (&key_menu)[slot * 2];
    if (key < 0) {
        {
            tstring no_key("ui_no_key_assigned");                   // 0x59687c
            *out = *g_strtable.lookup(&no_key);
        }
        *(char*)translated = 1;
    } else {
        *out = g_keyboard.key_name(key);
        *(char*)translated = 0;
    }
}

// @ 0x4d7d90
void options_panel::on_key_event(int)
{
    if (cursor_control->is_key_down(0x10000)) {
        menu_open = 0;
        update_cursor();
        refresh_texts();
    }
    if (menu_keys->is_key_down(0x10000))
        g_main_menu.choose(menu_choice);
}

// @ 0x4d7de0
void options_panel::update_cursor()
{
    if (menu_open == 0)
        cursor_control->set_cursor(1);
    else
        cursor_control->set_cursor(0);
}

// @ 0x4d7e00
void options_panel::refresh_texts()
{
    {
        tstring empty(EMPTY_TEXT);                                  // 0x54a480 ""
        texts->set_text(empty);
    }
    if (menu_open == 0)
        draw_faction_report();
}

// @ 0x4d7e50
void options_panel::draw_faction_report()
{
    float row = 0.6f;
    {
        tstring empty(EMPTY_TEXT);                                  // 0x54a480 ""
        texts->set_text(empty);
    }
    {
        tstring title("ui_factions_report");                        // 0x596bac
        float pos[2];
        pos[0] = 0.1f;
        pos[1] = 0.65f;
        texts->add_text(g_strtable.lookup(&title), pos);
    }
    for (int i = 0; i < g_game->num_factions; i++) {
        faction_rec* f = &g_game->factions[i];
        float renown = f->renown * 100.0f;                          // 0x54ab28
        float whole = (float)floor(renown);
        int base = (int)whole;
        int n;
        if (renown - whole <= 0.5f)                                 // 0x549d70
            n = base;
        else
            n = base + 1;
        tstring* shown = &f->name;
        if (f->has_name == 0) {
            tstring* found = g_strtable.find(&f->key);
            if (found)
                shown = found;
        }
        char text[512];
        sprintf(text, "%s:  %d", shown->c_str(), n);                // 0x596ba4
        {
            tstring line(text);
            float xy[2];
            xy[0] = 0.2f;
            xy[1] = row;
            texts->add_text(&line, xy);
        }
        row -= 0.06f;                                               // 0x590e5c
    }
}

// @ 0x4d8000
void options_panel::import_export(int)
{
    char buf_a[512];
    char buf_b[512];
    if (export_pending == 0) {
        export_path(buf_a);
        export_pending = -1;
        outfile file(buf_a);
        file.fp = fopen(buf_a, MODE_W);
        if (file.fp) {
            file.failed = 0;
            file.open = 1;
        } else {
            file.failed = 1;
        }
        g_game->export_character(&file);
        file.shut();
        {
            tstring msg("ui_character_playername_exported_successfully");   // 0x596d20
            troop_rec* t = &g_game->troops[g_game->player_troop];
            tstring* nm = g_strtable.find(&t->key);
            if (!nm)
                nm = &t->name;
            sprintf(buf_b, g_strtable.lookup(&msg)->c_str(), nm->c_str());
        }
        {
            tstring text(buf_b);
            tstring header("");
            g_main_menu.rec->setup(&header, &text);
        }
        g_main_menu.reset(0);
        return;
    }
    if (import_pending == 0) {
        export_path(buf_b);
        import_pending = -1;
        ini_file table;
        FILE* f = fopen(buf_b, MODE_R);
        if (f) {
            table.load(f);
            fclose(f);
            g_game->import_apply(&table);
            {
                tstring msg("ui_character_playername_imported_successfully");   // 0x596cf0
                troop_rec* t = &g_game->troops[g_game->player_troop];
                tstring* nm = g_strtable.find(&t->key);
                if (!nm)
                    nm = &t->name;
                sprintf(buf_a, g_strtable.lookup(&msg)->c_str(), nm->c_str());
            }
            {
                tstring text(buf_a);
                tstring header("");
                g_main_menu.rec->setup(&header, &text);
            }
            g_main_menu.reset(0);
        } else {
            {
                tstring msg("ui_unable_to_open_import_file");       // 0x596cd0
                tstring header("");
                g_main_menu.rec->setup(&header, g_strtable.lookup(&msg));
            }
            g_main_menu.reset(0);
        }
        return;
    }
    if (import_key->is_key_down(0x10000)) {
        if (g_trial_mode) {
            export_path(buf_b);
            if (mb_file_exists(buf_b)) {
                import_pending = -1;
                {
                    tstring msg("ui_are_you_sure_you_want_to_import_the_character");   // 0x596c9c
                    sprintf(buf_a, g_strtable.lookup(&msg)->c_str());
                }
                {
                    tstring no("ui_no");                            // 0x596c94
                    tstring yes("ui_yes");                          // 0x596c8c
                    tstring text(buf_a);
                    g_main_menu.rec->ask_yes_no(&text, g_strtable.lookup(&yes), g_strtable.lookup(&no), &import_pending);
                }
                g_main_menu.choose(0);
            } else {
                {
                    tstring msg("ui_unable_to_find_character_import_file"); // 0x596c64
                    tstring header("");
                    g_main_menu.rec->setup(&header, g_strtable.lookup(&msg));
                }
                g_main_menu.reset(0);
            }
        } else {
            {
                tstring msg("ui_mount_and_blade_is_running_in_trial_mode_please_buy_the_game_for_importing_a_character");
                tstring header("");
                g_main_menu.rec->setup(&header, g_strtable.lookup(&msg));
            }
            g_main_menu.reset(0);
        }
        return;
    }
    if (cursor_control->is_key_down(0x10000)) {
        export_path(buf_b);
        if (mb_file_exists(buf_b)) {
            export_pending = -1;
            {
                tstring msg("ui_export_file_for_character_playername_already_exists_overwrite_it"); // 0x596bc0
                troop_rec* t = &g_game->troops[g_game->player_troop];
                sprintf(buf_a, g_strtable.lookup(&msg)->c_str(), t->display_name()->c_str());
            }
            {
                tstring no("ui_no");
                tstring yes("ui_yes");
                tstring text(buf_a);
                g_main_menu.rec->ask_yes_no(&text, g_strtable.lookup(&yes), g_strtable.lookup(&no), &export_pending);
            }
            g_main_menu.choose(0);
        } else {
            export_pending = 0;
        }
        return;
    }
    if (menu_keys->is_key_down(0x10000))
        g_main_menu.choose(menu_choice);
}

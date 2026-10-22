// Slice s00507400: the character sheet's window builder (0x507400).
//
//   0x507400  CharSheet::init: if the caller did not hand over a character record, take the player's;
//             on the first call clear the mode; reset the gui manager; build the 0x190-byte panel
//             container (vtable 0x592848) at this+0x2c, give it the "character_window" mesh, register
//             it with the gui, then hang the reset/done buttons off it.
//             The skill column (this+0x4c0, vtable 0x597050) gets one 0x188-byte row per
//             "<char_num_skills>" entry (rows vtable 0x591d58: a config-driven label, a level-text
//             child and a "+" increment label).  The weapon-proficiency column then gets its three
//             headings, seven rows (hidden for the weapon classes the save says are unavailable,
//             g_weapon_available @ 0x5e2900), and the attributes column (this+0x4b0) its four rows.
//             The character-name button (this+0x4c4, vtable 0x592208) shows the record's translated
//             name, or "enter your name" plus the configured colour when the player has no name yet.
//             Finally the statistics/next/previous buttons, the skill-description panel, the five
//             layout passes, and the mode-specific dialog (saving policy / tutorial).
//
// Every "<key>" is looked up in the settings table 0xa44240.  Two access idioms occur in the
// original and both are reproduced here: g_config.find (0x4bd5a0) with the value at entry+0x20 of
// a 0x2c-byte record, and a fully inlined key walk (0x42bdd0 tstring::equals per record).  The
// float surface is atof (0x537352).
#include <windows.h>
#include <stdio.h>
#include "rgl.h"

extern "C" int _fltused = 0;

// ---- engine string (16 bytes) ---------------------------------------------------------------------------
struct tstring {
    int pos;                                    // +0
    char* buf;                                  // +4
    int cap;                                    // +8
    int len;                                    // +0xc
    tstring();                                  // 0x42bbd0
    tstring(const char* s);                     // 0x42bbe0
    ~tstring();                                 // 0x42bc40
    tstring& operator=(const tstring& o);       // 0x42bc50
    char equals(const tstring& o) const;        // 0x42bdd0 (ret 4)
    char equals(const char* s) const;           // 0x42bd10
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

struct vector2 {
    float x, y;
    vector2() {}
};

// ---- widgets ------------------------------------------------------------------------------------------
// Slot offsets used here: +0x58 set_rect(vector2*), +0x64 set_scale(vector2*),
// +0x6c set_text_size(vector2*), +0x80 set_mesh(int, void*), +0x98 set_color(int),
// +0xa4 set_text(vector2* out, const tstring& text) (0x488b50, ret 8), +0xb0 show(),
// +0xc8 add_child(Widget*).  Field offsets are the ones this function reads and writes.
struct Widget {
    char pad00[0x10];
    Widget* plus10;                             // +0x10 (attribute row '+' label)
    Widget* plus1c;                             // +0x1c (weapon row '+' label)
    char pad20[0xa8 - 0x20];
    Widget* levels;                             // +0xa8 (skill row level text)
    char pad_ac[0xe8 - 0xac];
    float px;                                   // +0xe8
    float py;                                   // +0xec
    float pw;                                   // +0xf0
    char pad_f4[0x121 - 0xf4];
    char b121;                                  // +0x121
    char pad122[0x14c - 0x122];
    int color;                                  // +0x14c
    Widget* plus150;                            // +0x150 (skill row '+' label)
    char pad154[0x178 - 0x154];
    int f178;                                   // +0x178
    char b180;                                  // +0x180
    char b181;                                  // +0x181
    char pad182[0x184 - 0x182];
    int f184;                                   // +0x184
    char b188;                                  // +0x188
    char pad189[0x1f8 - 0x189];
    int skill_index;                            // +0x1f8

    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03(); virtual void v04();
    virtual void v05(); virtual void v06(); virtual void v07(); virtual void v08(); virtual void v09();
    virtual void v10(); virtual void v11(); virtual void v12(); virtual void v13(); virtual void v14();
    virtual void v15(); virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v20(); virtual void v21();
    virtual void set_rect(const vector2* v);                        // +0x58  (slot 22)
    virtual void v23(); virtual void v24();
    virtual void set_scale(const vector2* v);                       // +0x64  (slot 25)
    virtual void v26();
    virtual void set_text_size(const vector2* v);                   // +0x6c  (slot 27)
    virtual void v28(); virtual void v29(); virtual void v30(); virtual void v31();
    virtual void set_mesh(int slot, void* mesh);                    // +0x80  (slot 32)
    virtual void v33(); virtual void v34(); virtual void v35(); virtual void v36(); virtual void v37();
    virtual void set_color(int color);                              // +0x98  (slot 38)
    virtual void v39(); virtual void v40();
    virtual void set_text(vector2* out, const tstring& text);       // +0xa4  (slot 41)
    virtual void v42(); virtual void v43();
    virtual void show();                                            // +0xb0  (slot 44)
    virtual void v45(); virtual void v46(); virtual void v47(); virtual void v48(); virtual void v49();
    virtual void add_child(Widget* child);                          // +0xc8  (slot 50)
};

// ---- engine globals -----------------------------------------------------------------------------------
struct GameData {
    char pad0[0x8c];
    int player;                                 // +0x8c
    char pad90[0x13a00 - 0x90];
    char* chars;                                // +0x13a00 (records of 0x508 bytes, name tstring at +0x20)
};
extern GameData* g_game;                        // 0x5e2930

struct Mesh { Mesh* clone(); };                 // 0x41c6d0
struct ResMgr { Mesh* get_object(const tstring* name); };               // 0x419540 (ret 4)
extern ResMgr g_resmgr;                         // 0x5d6fa0

struct GuiManager {
    void reset();                               // 0x42d1b0
    void push(Widget** w);                      // 0x49b120 (ret 4)
};
extern GuiManager g_gui;                        // 0x5d7080
extern char g_ui_dirty;                         // 0x5d8788

struct StrTable {
    tstring* lookup(const tstring& key);        // 0x43a1e0 (ret 4, never null)
    tstring* find(const tstring* key);          // 0x4c6010 (null when absent)
};
extern StrTable g_strtable;                     // 0x5e2a18

// settings table at 0xa44240: 0x2c-byte records, key tstring at +0, text pointer at +0x20
struct ConfigTable {
    int find(const tstring& key);               // 0x4bd5a0 (ret 4)
    float get_float(const tstring& key);        // 0x4bd5e0 (ret 4)
    int get_int(const tstring& key);            // 0x5093d0 (ret 4)
};
extern ConfigTable g_config;                    // 0xa44240
extern int g_config_count;                      // 0xa70240

extern "C" double atof(const char* s);          // 0x537352
void* __cdecl operator new(unsigned n);         // 0x53545e
int sscanf(const char* s, const char* fmt, ...);   // 0x535fe4

void ctor_window(void* p);                      // 0x4931a0
void ctor_widget(void* p);                      // 0x48ae20
void mesh_scale(Mesh* m, float f);              // 0x44f0c0

// ---- widget factories ----------------------------------------------------------------------------------
Widget* make_button(Widget* parent, tstring* caption, int a, int b, float h);        // 0x4dce70
Widget* make_short_button(Widget* parent, tstring* caption, int a, int b, float h);  // 0x4e5570
Widget* make_medium_button(Widget* parent, tstring* caption, int a, int b, float h,
                           tstring* up, tstring* down,
                           float a8, float a9, float a10);                           // 0x4dcc50
Widget* make_label(Widget* parent, tstring* text, float x, float y, float sx, float sy);  // 0x4dada0
Widget* make_text(Widget* parent, tstring* text, float x, float y, int flag, float sy);   // 0x509770
void label_refresh(Widget* w, tstring key);                                          // 0x4e4c70
Widget* make_panel(Widget* parent, tstring* key);                                    // 0x4f6160
Widget* make_panel_child(Widget* parent, tstring* key, Widget* child);               // 0x4f62f0
void widget_f488d50(Widget* w);                                                      // 0x488d50

// ---- character sheet ------------------------------------------------------------------------------------
struct CharSheet {
    char pad04[4];
    int flag4;                                  // +0x4  (a record was supplied by the caller)
    int flag8;                                  // +0x8  (already initialised once)
    char pad0c[0x14 - 0xc];
    int player;                                 // +0x14
    char pad18[0x2c - 0x18];
    Widget* window;                             // +0x2c (the 0x190-byte panel)
    Widget* skill_row[7];                       // +0x30 (num_skills rows actually used)
    char pad4c[0x2d0 - 0x4c];
    int num_skills;                             // +0x2d0
    int f2d4;                                   // +0x2d4 (set to -1)
    int f2d8;                                   // +0x2d8
    int mode;                                   // +0x2dc (0 = new game, 1 = tutorial)
    char pad2e0[0x42c - 0x2e0];
    Widget* done;                               // +0x42c
    Widget* reset;                              // +0x430
    Widget* explanation;                        // +0x434
    char pad438[0x440 - 0x438];
    Widget* stats;                              // +0x440
    Widget* next;                               // +0x444
    Widget* prev;                               // +0x448
    Widget* attrib_row[4];                      // +0x44c
    char pad45c[0x46c - 0x45c];
    Widget* weapon_title;                       // +0x46c
    Widget* attrib_title;                       // +0x470
    Widget* skill_title;                        // +0x474
    Widget* weapon_row[7];                      // +0x478
    char pad494[0x4b0 - 0x494];
    Widget* attribs;                            // +0x4b0
    Widget* general;                            // +0x4b4
    Widget* skill_desc;                         // +0x4b8
    Widget* skills_outer;                       // +0x4bc
    Widget* skills;                             // +0x4c0
    Widget* name;                               // +0x4c4
    char f4c8;                                  // +0x4c8
    char pad4c9[0x4d4 - 0x4c9];
    char f4d4;                                  // +0x4d4
    char pad4d5[0x52c - 0x4d5];
    char f52c;                                  // +0x52c

    void init();                                // 0x507400
};

extern int g_game_mode;                         // 0x5a9fb0
extern int g_weapon_available[7];               // 0x5e2900
extern char g_skill_table[];                    // 0x8ee3100 (records of 0x44 bytes)

// the two mode/tutorial objects touched at the end
struct MainMenu {
    char pad4[0x4];
    int f4;                                     // +0x4
    char pad8[0x1c - 0x8];
    tstring a;                                  // +0x1c
    char pad2c[0x4c - 0x2c];
    int f4c;                                    // +0x4c
    int f50;                                    // +0x50
    char pad54[0x68 - 0x54];
    tstring b;                                  // +0x68
};
// 0xa73980 is the menu holder object itself; its first dword points at the menu the tutorial
// branch fills in (tstrings at +0x1c and +0x68, the three ints at +4/+0x4c/+0x50).
struct MainMenuHolder {
    MainMenu* target;                           // +0
    void set(int v);                            // 0x494260
};
extern MainMenuHolder g_menu;                   // 0xa73980
struct DialogMgr {
    void open(tstring* msg, int type, const char* a, const char* b,
              const char* c, const char* d, const char* e, const char* f);   // 0x509410
};
extern DialogMgr* g_dialog;                     // 0xa73984 (pointer variable)

void sheet_509730(CharSheet* s);                // 0x509730
void sheet_4ef7d0(CharSheet* s);                // 0x4ef7d0
void sheet_509980(CharSheet* s);                // 0x509980
void sheet_4ef9f0(CharSheet* s);                // 0x4ef9f0
void sheet_4f0820(CharSheet* s);                // 0x4f0820
void sheet_4f06f0(CharSheet* s);                // 0x4f06f0
void sheet_5094a0(CharSheet* s);                // 0x5094a0

// ---- settings access ------------------------------------------------------------------------------------
static const char* config_text(int i)
{
    char* e = (char*)0xa4425c + i * 0x2c;       // entry base; the text pointer is at +4
    const char* v = *(const char**)(e + 4);
    return v ? v : e + 4;
}

// the inlined key walk of the original: compare every record's key, stop at the first hit
static int config_index(const tstring& key)
{
    int found = -1;
    for (int i = 0; i < g_config_count && found == -1; i++) {
        if (((tstring*)((char*)0xa44240 + i * 0x2c))->equals(key))
            found = i;
    }
    return found;
}

// "read a configured key as float, 0 when the key is absent" via ConfigTable::find
static float config_float(const tstring& key)
{
    float v = 0;
    int i = g_config.find(key);
    if (i >= 0) v = (float)atof(config_text(i));
    return v;
}

// the same, through the inlined walk
static float config_float_walk(const tstring& key)
{
    float v = 0;
    int i = config_index(key);
    if (i >= 0) v = (float)atof(config_text(i));
    return v;
}

// @ 0x507400
void CharSheet::init()
{
    int i;

    if (flag4 == 0)
        player = g_game->player;
    if (flag8 == 0) {
        mode = 0;
        flag8 = 1;
    }

    g_gui.reset();
    stats = 0;

    // the panel: 0x190-byte container, vtable 0x592848, "character_window" mesh
    Widget* w = (Widget*)::operator new(0x190);
    if (w != 0) {
        ctor_window(w);
        *(void**)w = (void*)0x592848;
        w->b188 = 0;
    } else {
        w = 0;
    }
    window = w;

    vector2 v;
    v.x = 0.0f;
    v.y = 0.0f;
    w->set_rect(&v);
    v.x = 1.0f;
    v.y = 0.75f;
    w->set_scale(&v);

    {
        void* mesh;
        {
            tstring n("character_window");
            mesh = ((Mesh*)g_resmgr.get_object(&n))->clone();
        }
        w->set_mesh(0, mesh);
    }

    Widget* pw = window;
    g_ui_dirty = 1;
    g_gui.push(&pw);

    // reset-changes button (this+0x430) and done button (this+0x42c)
    {
        tstring ka("char_b_reset_h");
        tstring kb("ui_reset_changes");
        float h = config_float(ka);
        reset = make_button(window, g_strtable.lookup(kb), 0, 0, h);
    }
    label_refresh(reset, tstring("char_b_reset"));

    {
        tstring ka("char_b_done_h");
        tstring kb("ui_done");
        float h = config_float(ka);
        done = make_short_button(window, g_strtable.lookup(kb), 0, 0, h);
    }
    label_refresh(done, tstring("char_b_done"));

    sheet_509730(this);

    f4c8 = 1;
    f2d4 = -1;
    f2d8 = 0;

    float skills_y = config_float(tstring("char_b_skills_y_a"));

    // do the configured skill column offsets apply to this character?
    float skills_x;
    {
        tstring kn("char_num_skills");
        if (num_skills <= g_config.get_int(kn)) {
            tstring kx("char_b_skills_x_a");
            skills_x = g_config.get_float(kx);
        } else {
            skills_x = 0.0f;
        }
    }

    // ---- the skill column -------------------------------------------------------------------------
    Widget* sk = (Widget*)::operator new(0x188);
    if (sk != 0) {
        ctor_window(sk);
        *(void**)sk = (void*)0x597050;
    } else {
        sk = 0;
    }
    skills = sk;
    label_refresh(sk, tstring("char_bo_skills"));

    v.x = sk->pw;
    v.y = ((float)num_skills + 0.5f) * skills_y;
    sk->set_scale(&v);

    if (num_skills > 0) {
        for (i = 0; i < num_skills; i++) {
            Widget* row = (Widget*)::operator new(0x188);
            if (row != 0) {
                ctor_widget(row);
                row->f178 = 0;
                row->b121 = 0;
                *(void**)row = (void*)0x591d58;
                row->b180 = 0;
                row->f184 = 0;
            } else {
                row = 0;
            }
            skill_row[i] = row;
            label_refresh(row, tstring("char_b_skills_base"));

            int extra;
            {
                tstring kn("char_num_skills");
                if (num_skills < g_config.get_int(kn)) {
                    tstring kn2("char_num_skills");
                    extra = g_config.get_int(kn2) - num_skills;
                } else {
                    extra = 0;
                }
            }

            float rowy = (float)(extra + i) * skills_y + row->py;
            v.x = row->px;
            v.y = rowy;
            row->set_rect(&v);

            float colw = config_float_walk(tstring("char_b_skills_w"));
            v.x = colw + skills_x;
            v.y = skills_y;
            row->set_scale(&v);

            int idx = row->skill_index;
            char* base = (char*)g_skill_table + idx * 0x44;
            tstring* nm = g_strtable.find((tstring*)(base + 0x20));
            if (nm == 0)
                nm = (tstring*)(base + 0x10);
            vector2 tx;
            row->set_text(&tx, *nm);

            sk->add_child(row);

            float levels_w = config_float_walk(tstring("char_b_skill_levels_w"));
            float levels_x = config_float_walk(tstring("char_b_skill_levels_x"));
            tstring empty("");
            row->levels = make_label(sk, &empty, levels_x + skills_x, rowy,
                                     levels_w, skills_y);
            row->levels->set_color(row->color);   // 0x507b9f pushes the row class's vptr+0x14c (an address, not a colour); row->color is the same byte the original reads

            float inc_h = config_float_walk(tstring("char_b_skill_increment_h_a"));
            float inc_y = config_float_walk(tstring("char_b_skill_increment_y_a"));
            float inc_x = config_float_walk(tstring("char_b_skill_increment_x"));
            tstring plus("+");
            row->plus150 = make_text(sk, &plus, inc_x + skills_x, inc_y + rowy,
                                     1, skills_y - inc_h);

            float ts_x = config_float_walk(tstring("char_b_skill_increment_t_s"));
            float ts_y = config_float_walk(tstring("char_b_skill_increment_t_s"));
            vector2 ts;
            ts.x = ts_y;
            ts.y = ts_x;
            row->plus150->set_text_size(&ts);
            row->plus150->set_text(&tx, tstring("+"));
        }
    }

    // the framed "skills" outer panel
    sheet_4ef7d0(this);
    Widget* inner;
    {
        tstring ks("char_s_skills");
        inner = make_panel(window, &ks);
    }
    {
        tstring ko("char_sc_skills_outer");
        skills_outer = make_panel_child(sk, &ko, inner);
    }
    widget_f488d50(skills_outer);
    window->add_child(skills_outer);

    // ---- the three column headings -----------------------------------------------------------------
    {
        Widget* t = (Widget*)::operator new(0x178);
        if (t != 0) {
            ctor_widget(t);
        } else {
            t = 0;
        }
        weapon_title = t;
    }
    label_refresh(weapon_title, tstring("char_t_weapon_proficiencies_title"));
    {
        vector2 tx;
        weapon_title->set_text(&tx, *g_strtable.lookup(tstring("ui_weapon_proficiencies")));
    }
    window->add_child(weapon_title);

    {
        Widget* t = (Widget*)::operator new(0x178);
        if (t != 0) {
            ctor_widget(t);
        } else {
            t = 0;
        }
        attrib_title = t;
    }
    label_refresh(attrib_title, tstring("char_t_attributes_title"));
    {
        vector2 tx;
        attrib_title->set_text(&tx, *g_strtable.lookup(tstring("ui_attributes")));
    }
    window->add_child(attrib_title);

    {
        Widget* t = (Widget*)::operator new(0x178);
        if (t != 0) {
            ctor_widget(t);
        } else {
            t = 0;
        }
        skill_title = t;
    }
    label_refresh(skill_title, tstring("char_t_skills_title"));
    {
        vector2 tx;
        skill_title->set_text(&tx, *g_strtable.lookup(tstring("ui_skills")));
    }
    window->add_child(skill_title);

    // ---- the seven weapon-proficiency rows ----------------------------------------------------------
    for (i = 0; i < 7; i++) {
        Widget* row = (Widget*)::operator new(0x188);
        if (row != 0) {
            ctor_widget(row);
            row->f178 = 0;
            row->b121 = 0;
            *(void**)row = (void*)0x591d58;
            row->b180 = 0;
            row->f184 = 0;
        } else {
            row = 0;
        }
        weapon_row[i] = row;
        label_refresh(row, tstring("char_b_weapon_proficiencies_base"));

        float spacing = config_float_walk(tstring("char_b_weapon_proficiencies_y_a"));
        v.x = row->px;
        v.y = row->py - (float)i * spacing;
        row->set_rect(&v);

        window->add_child(row);

        float inc_h = config_float_walk(tstring("char_b_weapon_proficiency_increment_h"));
        float inc_y = config_float_walk(tstring("char_b_weapon_proficiency_increment_y"));
        float inc_ya = config_float_walk(tstring("char_b_weapon_proficiency_increment_y_a"));
        float inc_x = config_float_walk(tstring("char_b_weapon_proficiency_increment_x"));
        tstring plus("+");
        row->plus1c = make_text(window, &plus, inc_y - inc_ya * (float)i, inc_x,
                                1, inc_h);

        float ts_x = config_float_walk(tstring("char_b_weapon_proficiency_increment_t_s"));
        float ts_y = config_float_walk(tstring("char_b_weapon_proficiency_increment_t_s"));
        vector2 ts;
        ts.x = ts_y;
        ts.y = ts_x;
        row->plus1c->set_text_size(&ts);
        {
            vector2 tx;
            row->plus1c->set_text(&tx, tstring("+"));
        }

        if (g_weapon_available[i] == 0) {
            row->show();
            row->plus1c->show();
        }
    }

    // ---- the attributes / general-info columns -----------------------------------------------------
    {
        Widget* a = (Widget*)::operator new(0x188);
        if (a != 0) {
            ctor_window(a);
            *(void**)a = (void*)0x597050;
        } else {
            a = 0;
        }
        attribs = a;
    }
    label_refresh(attribs, tstring("char_bo_attribs"));

    {
        Widget* g = (Widget*)::operator new(0x188);
        if (g != 0) {
            ctor_window(g);
            *(void**)g = (void*)0x597050;
        } else {
            g = 0;
        }
        general = g;
    }
    label_refresh(general, tstring("char_bo_general_info"));

    // the character-name button
    {
        Widget* n = (Widget*)::operator new(0x188);
        if (n != 0) {
            ctor_widget(n);
            n->f178 = 0;
            n->b121 = 0;
            *(void**)n = (void*)0x592208;
            n->b180 = 0;
            n->b181 = 0;
            n->f184 = 0x1c;
        } else {
            n = 0;
        }
        name = n;
    }
    label_refresh(name, tstring("char_tb_character_name"));

    {
        Mesh* up;
        {
            tstring k("party_button");
            up = ((Mesh*)g_resmgr.get_object(&k))->clone();
        }
        Mesh* down;
        {
            tstring k("party_button_down");
            down = ((Mesh*)g_resmgr.get_object(&k))->clone();
        }
        mesh_scale(up, 0.6666667f);
        mesh_scale(down, 0.6666667f);
        name->set_mesh(2, up);
        name->set_mesh(1, down);
    }

    {
        char* rec = g_game->chars + (unsigned)player * 0x508;
        tstring* nm = g_strtable.find((tstring*)(rec + 0x20));
        if (nm == 0)
            nm = (tstring*)rec;
        vector2 tx;
        name->set_text(&tx, *nm);
    }

    if (player == 0 && ((tstring*)g_game->chars)->equals("")) {
        name->b181 = 1;
        int col = 0;
        int idx = config_index(tstring("char_tb_character_name_t_c"));
        if (idx >= 0)
            sscanf(config_text(idx), "%x", &col);
        name->set_color(col);
        vector2 tx;
        name->set_text(&tx, *g_strtable.lookup(tstring("ui_enter_name_here")));
    } else {
        name->b181 = 0;
    }

    window->add_child(name);
    window->add_child(attribs);
    window->add_child(general);

    // ---- the four attribute rows -------------------------------------------------------------------
    for (i = 0; i < 4; i++) {
        Widget* row = (Widget*)::operator new(0x188);
        if (row != 0) {
            ctor_widget(row);
            row->f178 = 0;
            row->b121 = 0;
            *(void**)row = (void*)0x591d58;
            row->b180 = 0;
            row->f184 = 0;
        } else {
            row = 0;
        }
        attrib_row[i] = row;
        label_refresh(row, tstring("char_b_attribs_base"));

        float spacing = config_float_walk(tstring("char_b_attribs_y_a"));
        float rowy = row->py - (float)i * spacing;
        v.x = row->px;
        v.y = rowy;
        row->set_rect(&v);

        attribs->add_child(row);

        float inc_h = config_float_walk(tstring("char_b_attrib_increment_h"));
        float inc_ya = config_float_walk(tstring("char_b_attrib_increment_y_a"));
        float inc_x = config_float_walk(tstring("char_b_attrib_increment_x"));
        tstring plus("+");
        row->plus10 = make_text(attribs, &plus, rowy - inc_ya, inc_x, 1, inc_h);

        float ts_x = config_float_walk(tstring("char_b_attrib_increment_t_s"));
        float ts_y = config_float_walk(tstring("char_b_attrib_increment_t_s"));
        vector2 ts;
        ts.x = ts_y;
        ts.y = ts_x;
        row->plus10->set_text_size(&ts);
        {
            vector2 tx;
            row->plus10->set_text(&tx, tstring("+"));
        }
    }

    // ---- statistics / next / previous --------------------------------------------------------------
    {
        tstring kh("char_b_stats_h");
        tstring kc("ui_statistics");
        float h = config_float_walk(kh);
        tstring* cap = g_strtable.lookup(kc);
        tstring normal("medium_button");
        tstring pressed("medium_button_down");
        stats = make_medium_button(window, cap, 0, 0, h, &normal, &pressed,
                                   3.7f, 0.072f, 0.085f);
    }
    label_refresh(stats, tstring("char_b_stats"));

    f52c = 1;
    if (g_game_mode == 0 && player != g_game->player)
        f52c = 0;
    if (mode != -1) {
        f52c = 0;
        stats->show();
    }
    prev = 0;
    next = 0;

    if (g_game_mode != 0 && mode == -1) {
        {
            tstring kh("char_b_next_h");
            tstring kc("ui_next");
            float h = config_float_walk(kh);
            tstring* cap = g_strtable.lookup(kc);
            tstring normal("short_button");
            tstring pressed("short_button_down");
            next = make_medium_button(window, cap, 0, 0, h, &normal, &pressed,
                                      3.0f, 0.072f, 0.085f);
        }
        label_refresh(next, tstring("char_b_next"));
    }

    if (g_game_mode != 0 && mode == -1) {
        {
            tstring kh("char_b_prev_h");
            tstring kc("ui_prev");
            float h = config_float_walk(kh);
            tstring* cap = g_strtable.lookup(kc);
            tstring normal("short_button");
            tstring pressed("short_button_down");
            prev = make_medium_button(window, cap, 0, 0, h, &normal, &pressed,
                                      3.0f, 0.072f, 0.085f);
        }
        label_refresh(prev, tstring("char_b_prev"));
    }

    // ---- the skill-description panel ---------------------------------------------------------------
    {
        Widget* d = (Widget*)::operator new(0x188);
        if (d != 0) {
            ctor_window(d);
            *(void**)d = (void*)0x597050;
        } else {
            d = 0;
        }
        skill_desc = d;
    }
    label_refresh(skill_desc, tstring("char_bo_skill_desc"));
    window->add_child(skill_desc);

    {
        Widget* e = (Widget*)::operator new(0x178);
        if (e != 0) {
            ctor_widget(e);
        } else {
            e = 0;
        }
        explanation = e;
    }
    label_refresh(explanation, tstring("char_t_explanation"));
    skill_desc->add_child(explanation);
    explanation->show();

    // ---- the layout passes -------------------------------------------------------------------------
    sheet_509980(this);
    sheet_4ef9f0(this);
    sheet_4f0820(this);
    sheet_4f06f0(this);
    sheet_5094a0(this);

    f4d4 = 0;

    if (mode == 0) {
        tstring k1("ui_saving_policy_nonrealistic");
        tstring k2("ui_saving_policy_realistic");
        tstring k3("ui_question_saving_policy");
        const char* nonreal = g_strtable.lookup(k1)->c_str();
        const char* real = g_strtable.lookup(k2)->c_str();
        g_dialog->open(g_strtable.lookup(k3), 2, real, nonreal, "", "", "", "");
        g_menu.set(1);
        return;
    } else if (mode == 1) {
        tstring k1("ui_tutorial_character_generation");
        tstring k2("ui_tutorial");
        tstring* gen = g_strtable.lookup(k1);
        tstring* tut = g_strtable.lookup(k2);
        MainMenu* m = g_menu.target;
        m->b = *tut;
        m->a = *gen;
        m->f4c = 0;
        m->f50 = 0;
        m->f4 = 0;
        g_menu.set(0);
    }
}

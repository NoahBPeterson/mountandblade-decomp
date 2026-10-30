#include <stddef.h>
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
//             g_display_wp_one_handed @ 0x5e2900), and the attributes column (this+0x4b0) its four rows.
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
#include "../tstring.h"

#include "../../include/vec.h"                      // vector2 (= Vec2)

// ---- widgets ------------------------------------------------------------------------------------------
// Slot offsets used here: +0x58 set_rect(vector2*), +0x64 set_scale(vector2*),
// +0x6c set_text_size(vector2*), +0x80 set_mesh(int, void*), +0x98 set_color(int),
// +0xa4 set_text(vector2* out, const tstring& text) (0x488b50, ret 8), +0xb0 show(),
// +0xc8 add_child(CharSheetWidget*).  Field offsets are the ones this function reads and writes.
struct CharSheetWidget {
    char pad00[0x0c];   // 4 lower than the +0x10 these comments name: the vptr occupies the first 4 bytes
    CharSheetWidget* plus10;                             // +0x10 (attribute row '+' label)
    char pad14[0x1c - 0x14];                             // +0x14..0x1c (without it plus1c landed at +0x14 and
    CharSheetWidget* plus1c;                             // +0x1c  every later field was 8 bytes low)
    char pad20[0xa8 - 0x20];
    CharSheetWidget* levels;                             // +0xa8 (skill row level text)
    // The offset vector is +0xe0/+0xe4 and the size +0xe8/+0xec (ui_widget.h WidgetObj: rect_x, rect_y, rect_w,
    // rect_h). This view had px/py at +0xe8/+0xec - the *size* fields - so every row was placed at the width and
    // height that label_refresh had just set instead of at its configured position. The class is polymorphic, so cl
    // puts the vptr in the first 4 bytes and every member compiles 4 bytes after its declaration (verified with
    // cl 13.10: a stub with px declared at 0xe0 reports offsetof 0xe4). The pads below are therefore 4 smaller than
    // the real offsets, and the pad after the rect gives the 4 bytes back so the members after it keep their offsets.
    // The original's row code reads px/py from +0xe8/+0xec (0x507922 `mov edx,[ecx+0xe8]`, 0x5078ed `fadd
    // [edx+0xec]`, 0x5077cd `mov edx,[ecx+0xf0]`) and the weapon/attribute rows likewise; +0xe0/+0xe4 is the second
    // copy set_pos writes for the child chain. Declared 4 lower, because the vptr shifts every member by +4.
    char pad_ac[0xe8 - 0xac];
    float px;                                   // +0xe8
    float py;                                   // +0xec
    float pw;                                   // +0xf0
    float ph;                                   // +0xf4
    char pad_f0[0x121 - 0xf8];
    char b121;                                  // +0x121
    char pad122[0x14c - 0x122];
    int color;                                  // +0x14c
    CharSheetWidget* plus150;                            // +0x150 (skill row '+' label)
    char pad154[0x178 - 0x154];
    int f178;                                   // +0x178
    char pad17c[4];                             // the 4 bytes the original leaves between f178 and b180
    char b180;                                  // +0x180
    char b181;                                  // +0x181
    char pad182[0x184 - 0x182];
    int f184;                                   // +0x184
    char b188;                                  // +0x188
    char pad189[3];                             // (0x190-byte panels; skill rows are 0x188 bytes)

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
    virtual void set_color(const unsigned* color);                   // +0x98  (slot 38; by address, like
                                                                     // field_panel.h/mission_3.h's same slot)
    virtual void v39(); virtual void v40();
    virtual void set_text(vector2* out, const tstring& text);       // +0xa4  (slot 41)
    virtual void v42(); virtual void v43();
    virtual void show();                                            // +0xb0  (slot 44)
    virtual void v45(); virtual void v46(); virtual void v47(); virtual void v48(); virtual void v49();
    virtual void add_child(CharSheetWidget* child);                          // +0xc8  (slot 50)
};
typedef char chk_csw_px[offsetof(CharSheetWidget, px) == 0xe8 ? 1 : -1];
typedef char chk_csw_py[offsetof(CharSheetWidget, py) == 0xec ? 1 : -1];
typedef char chk_csw_pw[offsetof(CharSheetWidget, pw) == 0xf0 ? 1 : -1];
typedef char chk_csw_color[offsetof(CharSheetWidget, color) == 0x14c ? 1 : -1];
typedef char chk_csw_b180[offsetof(CharSheetWidget, b180) == 0x180 ? 1 : -1];

// ---- engine globals -----------------------------------------------------------------------------------
#include "../../include/game_data.h"

#include "../res_mesh.h"                       // ResMesh (was Mesh), ResMgr: the one definition
#include "../rgl_mesh.h"                       // rgl::mesh (clone 0x41c6d0)
extern ResMgr g_resmgr;                         // 0x5d6fa0

#include "../gui_mgr.h"
extern GuiManager g_gui;                        // 0x5d7080
extern char g_ui_dirty;                         // 0x5d8788

#include "../str_table.h"
extern StrTable g_strtable;                     // 0x5e2a18

// settings table at 0xa44240: 0x2c-byte records, key tstring at +0, text pointer at +0x20
#include "../config_table.h"                  // ConfigEntry, ConfigTable, g_config: the one definition


extern "C" double atof(const char* s);          // 0x537352
void* __cdecl operator new(unsigned n);         // 0x53545e
int sscanf(const char* s, const char* fmt, ...);   // 0x535fe4

// The definitions behind the helpers below live in other modules under their own declarations; the declarations here are
// theirs (class stubs carry only the member this file calls), and the CharSheetWidget views reach them through casts.
struct MissionUi {
    MissionUi();   // 0x48ae20 (the widget base constructor: ui/mission_ui.cpp)
};
struct ContainerW {
    void* construct();   // 0x4931a0 (the list container's constructor body: world_data)
};
struct MissionPanel {
    void draw();   // 0x488d50 (ui/mission_ui.cpp)
};
struct ContainerBase; struct UiAInline; struct UiButton; struct WidgetB; struct WidgetC;

static __forceinline void ctor_window(void* p) { ((ContainerW*)p)->construct(); }       // 0x4931a0
static __forceinline void init_mission_ui(void* p) { new (p) MissionUi; }                // 0x48ae20
static __forceinline void draw(CharSheetWidget* w) { ((MissionPanel*)w)->draw(); }     // 0x488d50

// ---- widget factories ----------------------------------------------------------------------------------
UiAInline* make_short_button(MissionUi* parent, const tstring* caption, float a, float b, float h);  // 0x4e5570
UiAInline* new_medium_button(MissionUi* parent, const tstring* caption, float a, float b, float h);  // 0x4dce70
CharSheetWidget* make_medium_button(CharSheetWidget* parent, tstring* caption, int a, int b, float h,
                           tstring* up, tstring* down,
                           float a8, float a9, float a10);                           // 0x4dcc50
MissionUi* make_hint(ContainerBase* parent, const tstring& text, float x, float y, float sx, float sy);  // 0x4dada0
UiButton* make_button(Widget* parent, const tstring* text, float x, float y, int flag, float sy);   // 0x509770
void label_refresh(MissionUi* w, tstring key);                                                    // 0x4e4c70
WidgetB* make_scrollbar(void* parent, const tstring* key);                                       // 0x4f6160
WidgetC* make_editor_list(void* parent, const tstring* key, int child);                          // 0x4f62f0
#define label_refresh(w, key) label_refresh((MissionUi*)(w), key)
static __forceinline CharSheetWidget* new_medium_button(CharSheetWidget* p, tstring* c, int a, int b, float h)
{ return (CharSheetWidget*)new_medium_button((MissionUi*)p, c, (float)a, (float)b, h); }
static __forceinline CharSheetWidget* make_short_button(CharSheetWidget* p, tstring* c, int a, int b, float h)
{ return (CharSheetWidget*)make_short_button((MissionUi*)p, c, (float)a, (float)b, h); }
static __forceinline CharSheetWidget* make_hint(CharSheetWidget* p, tstring* t, float x, float y, float sx, float sy)
{ return (CharSheetWidget*)make_hint((ContainerBase*)p, *t, x, y, sx, sy); }
static __forceinline CharSheetWidget* make_button(CharSheetWidget* p, tstring* t, float x, float y, int flag, float sy)
{ return (CharSheetWidget*)make_button((Widget*)p, t, x, y, flag, sy); }
static __forceinline CharSheetWidget* make_scrollbar(CharSheetWidget* p, tstring* k)
{ return (CharSheetWidget*)make_scrollbar((void*)p, k); }
static __forceinline CharSheetWidget* make_editor_list(CharSheetWidget* p, tstring* k, CharSheetWidget* c)
{ return (CharSheetWidget*)make_editor_list((void*)p, k, (int)c); }

// ---- character sheet ------------------------------------------------------------------------------------
#include "../char_sheet_panel.h"             // CharSheet (the character sheet panel)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../ini_file.h"

extern int g_game_mode;                         // 0x5a9fb0
extern int g_display_wp_one_handed[7];               // 0x5e2900
extern char g_skill_tbl[];                    // 0x8ee3100 (records of 0x44 bytes)

// the two mode/tutorial objects touched at the end
struct TutorialMenuRec {                        // the menu record the holder's first dword points to (renamed from MainMenu: 0xa73980 itself is main_menu.h's MainMenu)
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
struct WindowMgrW {
    TutorialMenuRec* target;                    // +0
    void open(int v);                           // 0x494260
};
extern WindowMgrW g_menu;                   // 0xa73980

// the character window's methods (src/game/inventory_panel/char_window.cpp) run on this same object
struct char_window {
    void fill_skills();                         // 0x509730
    void load_troop();                          // 0x5094a0
    void open_view();                           // 0x509980
    void init(const tstring& msg, int type, const char* a, const char* b, const char* c, const char* d, const char* e, const char* f);   // 0x509410 (the message box)
};
extern char_window* g_dialog;                     // 0xa73984 (pointer variable)

// ---- settings access ------------------------------------------------------------------------------------
static const char* config_text(int i)
{
    char* e = (char*)&g_game_vars + 0x1c + i * 0x2c;   // entry base; the text pointer is at +4
    const char* v = *(const char**)(e + 4);
    return v ? v : e + 4;
}

// the inlined key walk of the original: compare every record's key, stop at the first hit
static int config_index(const tstring& key)
{
    int found = -1;
    for (int i = 0; i < g_game_vars.count && found == -1; i++) {
        if (((tstring*)((char*)&g_game_vars + i * 0x2c))->equals(key))
            found = i;
    }
    return found;
}

// "read a configured key as float, 0 when the key is absent" via ConfigTable::find
static float config_float(const tstring& key)
{
    float v = 0;
    int i = g_game_vars.find(key);
    if (i >= 0) v = (float)atof(config_text(i));
    { FILE* pf_ = fopen("cf_probe.txt", "at");
      if (pf_) { fprintf(pf_, "key=%-28s idx=%4d v=%g\n", key.c_str() ? key.c_str() : "?", i, v); fclose(pf_); } }
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
    CharSheetWidget* w = (CharSheetWidget*)::operator new(0x190);
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
            mesh = ((rgl::mesh*)g_resmgr.get_object(&n))->clone();
        }
        w->set_mesh(0, mesh);
    }

    CharSheetWidget* pw = window;
    g_ui_dirty = 1;
    g_gui.add_root(&pw);

    // reset-changes button (this+0x430) and done button (this+0x42c)
    {
        tstring ka("char_b_reset_h");
        tstring kb("ui_reset_changes");
        float h = config_float(ka);
        reset = new_medium_button(window, g_strtable.lookup(kb), 0, 0, h);
    }
    label_refresh(reset, tstring("char_b_reset"));

    {
        tstring ka("char_b_done_h");
        tstring kb("ui_done");
        float h = config_float(ka);
        done = make_short_button(window, g_strtable.lookup(kb), 0, 0, h);
    }
    label_refresh(done, tstring("char_b_done"));

    ((char_window*)this)->fill_skills();

    f4c8 = 1;
    f2d4 = -1;
    f2d8 = 0;

    float skills_y = config_float(tstring("char_b_skills_y_a"));

    // do the configured skill column offsets apply to this character?
    float skills_x;
    {
        tstring kn("char_num_skills");
        if (num_skills <= g_game_vars.get_int(kn)) {
            tstring kx("char_b_skills_x_a");
            skills_x = g_game_vars.get_float(kx);
        } else {
            skills_x = 0.0f;
        }
    }

    // ---- the skill column -------------------------------------------------------------------------
    CharSheetWidget* sk = (CharSheetWidget*)::operator new(0x188);
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
            CharSheetWidget* row = (CharSheetWidget*)::operator new(0x188);
            if (row != 0) {
                init_mission_ui(row);
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
                if (num_skills < g_game_vars.get_int(kn)) {
                    tstring kn2("char_num_skills");
                    extra = g_game_vars.get_int(kn2) - num_skills;
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

            int idx = skill_id[i];                   // fill_skills' list of active skills (0x5079f8: [&skill_row[i] + 0x1f8])
            char* base = (char*)g_skill_tbl + idx * 0x44;
            tstring* nm = g_strtable.find((tstring*)(base + 0x20));
            if (nm == 0)
                nm = (tstring*)(base + 0x10);
            vector2 tx;
            row->set_text(&tx, *nm);

            sk->add_child(row);

            float levels_w = config_float_walk(tstring("char_b_skill_levels_w"));
            float levels_x = config_float_walk(tstring("char_b_skill_levels_x"));
            tstring empty("");
            row->levels = make_hint(sk, &empty, levels_x + skills_x, rowy,
                                     levels_w, skills_y);
            row->levels->set_color((const unsigned*)&row->color);
            label[i] = (Widget*)row->levels;                     // +0xd8: the per-row rank text update_skill_rows recolours
   // 0x507b9f pushes the row class's vptr+0x14c (an address, not a colour); row->color is the same byte the original reads

            float inc_h = config_float_walk(tstring("char_b_skill_increment_h_a"));
            float inc_y = config_float_walk(tstring("char_b_skill_increment_y_a"));
            float inc_x = config_float_walk(tstring("char_b_skill_increment_x"));
            tstring plus("+");
            row->plus150 = make_button(sk, &plus, inc_x + skills_x, inc_y + rowy,
                                     1, skills_y - inc_h);

            float ts_x = config_float_walk(tstring("char_b_skill_increment_t_s"));
            float ts_y = config_float_walk(tstring("char_b_skill_increment_t_s"));
            vector2 ts;
            ts.x = ts_y;
            ts.y = ts_x;
            row->plus150->set_text_size(&ts);
            row->plus150->set_text(&tx, tstring("+"));
            btn[i] = (Widget*)row->plus150;                      // +0x180: the per-row upgrade button it shows/hides

        }
    }

    // the framed "skills" outer panel
    update_skill_rows();
    CharSheetWidget* inner;
    {
        tstring ks("char_s_skills");
        inner = make_scrollbar(window, &ks);
    }
    {
        tstring ko("char_sc_skills_outer");
        skills_outer = make_editor_list(sk, &ko, inner);
    }
    draw(skills_outer);
    window->add_child(skills_outer);

    // ---- the three column headings -----------------------------------------------------------------
    {
        CharSheetWidget* t = (CharSheetWidget*)::operator new(0x178);
        if (t != 0) {
            init_mission_ui(t);
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
        CharSheetWidget* t = (CharSheetWidget*)::operator new(0x178);
        if (t != 0) {
            init_mission_ui(t);
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
        CharSheetWidget* t = (CharSheetWidget*)::operator new(0x178);
        if (t != 0) {
            init_mission_ui(t);
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
        CharSheetWidget* row = (CharSheetWidget*)::operator new(0x188);
        if (row != 0) {
            init_mission_ui(row);
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
        row->plus1c = make_button(window, &plus, inc_y - inc_ya * (float)i, inc_x,
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

        rows[i] = (Widget*)row->plus1c;              // +0x494: update_weapons shows/hides this '+' per weapon class
        if (g_display_wp_one_handed[i] == 0) {
            row->show();
            row->plus1c->show();
        }
    }

    // ---- the attributes / general-info columns -----------------------------------------------------
    {
        CharSheetWidget* a = (CharSheetWidget*)::operator new(0x188);
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
        CharSheetWidget* g = (CharSheetWidget*)::operator new(0x188);
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
        CharSheetWidget* n = (CharSheetWidget*)::operator new(0x188);
        if (n != 0) {
            init_mission_ui(n);
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
        rgl::mesh* up;
        {
            tstring k("party_button");
            up = ((rgl::mesh*)g_resmgr.get_object(&k))->clone();
        }
        rgl::mesh* down;
        {
            tstring k("party_button_down");
            down = ((rgl::mesh*)g_resmgr.get_object(&k))->clone();
        }
        up->scale(0.6666667f);
        down->scale(0.6666667f);
        name->set_mesh(2, up);
        name->set_mesh(1, down);
    }

    {
        char* rec = (char*)g_game->chars + (unsigned)player * 0x508;
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
        name->set_color((const unsigned*)&col);
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
        CharSheetWidget* row = (CharSheetWidget*)::operator new(0x188);
        if (row != 0) {
            init_mission_ui(row);
            row->f178 = 0;
            row->b121 = 0;
            *(void**)row = (void*)0x591d58;
            row->b180 = 0;
            row->f184 = 0;
        } else {
            row = 0;
        }
        attrib_row[i] = row;
        skill_pos[i] = (Widget*)row;                 // +0x45c-8: refresh_panels sets each attribute row's text
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
        row->plus10 = make_button(attribs, &plus, inc_x, rowy - inc_ya, 1, inc_h);
        skill_val[i] = (Widget*)row->plus10;         // +0x45c: refresh_panels shows/hides the '+' per attribute

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
        CharSheetWidget* d = (CharSheetWidget*)::operator new(0x188);
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
        CharSheetWidget* e = (CharSheetWidget*)::operator new(0x178);
        if (e != 0) {
            init_mission_ui(e);
        } else {
            e = 0;
        }
        explanation = e;
    }
    label_refresh(explanation, tstring("char_t_explanation"));
    skill_desc->add_child(explanation);
    explanation->show();

    // ---- the layout passes -------------------------------------------------------------------------
    ((char_window*)this)->open_view();
    update_detail();
    refresh_panels();
    update_weapons();
    ((char_window*)this)->load_troop();

    f4d4 = 0;

    if (mode == 0) {
        tstring k1("ui_saving_policy_nonrealistic");
        tstring k2("ui_saving_policy_realistic");
        tstring k3("ui_question_saving_policy");
        const char* nonreal = g_strtable.lookup(k1)->c_str();
        const char* real = g_strtable.lookup(k2)->c_str();
        g_dialog->init(*g_strtable.lookup(k3), 2, real, nonreal, "", "", "", "");
        g_menu.open(1);
        return;
    } else if (mode == 1) {
        tstring k1("ui_tutorial_character_generation");
        tstring k2("ui_tutorial");
        tstring* gen = g_strtable.lookup(k1);
        tstring* tut = g_strtable.lookup(k2);
        TutorialMenuRec* m = g_menu.target;
        m->b = *tut;
        m->a = *gen;
        m->f4c = 0;
        m->f50 = 0;
        m->f4 = 0;
        g_menu.open(0);
    }
}

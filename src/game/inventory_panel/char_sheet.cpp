#include "../../include/image_addrs.h"
#include "../config/config_block.h"      // Config g_cfg (0x5a9f98)
#include "../window_mgr.h"      // g_windows slots (slot_as<T>(i))
#include <stddef.h>
// Slice s00507400: the character sheet's window builder (0x507400).
//
//   0x507400  CharSheet::init_screen: if the caller did not hand over a character record, take the player's;
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
#include "../../rgl/rgl_math.h"
#include "../shared_vars.h"
#include "../window_mgr.h"

extern "C" int _fltused = 0;

// ---- engine string (16 bytes) ---------------------------------------------------------------------------
#include "../tstring.h"

#include "../../include/vec.h"                      // vector2 (= Vec2)
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)

// ---- engine globals -----------------------------------------------------------------------------------
#include "../../include/game_data.h"

#include "../../rgl/mesh.h"                       // rgl::mesh (clone 0x41c6d0)
#include "../res_mgr.h"                        // g_resmgr: the resource manager 0x5d6fa0

#include "../strat/gui_manager.h"

#include "../str_table.h"

// settings table at 0xa44240: 0x2c-byte records, key tstring at +0, text pointer at +0x20
#include "../ini_file.h"                      // ini_file, g_game_vars (0xa44240)
#include "../skill_cap.h"
#include "../ui_globals.h"      // UI / core globals


extern "C" double atof(const char* s);          // 0x537352
void* __cdecl operator new(unsigned n);         // 0x53545e
int sscanf(const char* s, const char* fmt, ...);   // 0x535fe4

// The definitions behind the helpers below live in other modules under their own declarations.
#include "../ui/container.h"                      // MissionUi, ContainerBase, NodeA, text_box
#include "../ui/ui_widgets.h"                     // UiA, UiD
#include "../config/ui_widgets_c.h"               // UiE (the character-name edit box)
#include "../party2/camera_class.h"               // Camera (draw 0x488d50)

// ---- widget factories ----------------------------------------------------------------------------------
UiA* make_short_button(MissionUi* parent, const tstring* caption, float a, float b, float h);  // 0x4e5570
UiA* new_medium_button(MissionUi* parent, const tstring* caption, float a, float b, float h);  // 0x4dce70
UiA* new_sized_button(MissionUi* parent, const tstring& text, float a, float b, float c,
                      const tstring& name, const tstring& down, float k1, float k2, float k3);   // 0x4dcc50
MissionUi* make_hint(ContainerBase* parent, const tstring& text, float x, float y, float sx, float sy);  // 0x4dada0
UiA* make_button(ContainerBase* parent, const tstring* text, float x, float y, int flag, float sy);   // 0x509770
void label_refresh(MissionUi* w, tstring key);                                                    // 0x4e4c70
UiD* make_scrollbar(void* parent, const tstring* key);                                       // 0x4f6160
Camera* make_editor_list(void* parent, const tstring* key, int child);                          // 0x4f62f0

// ---- character sheet ------------------------------------------------------------------------------------
#include "../char_sheet_panel.h"             // CharSheet (the character sheet panel)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../ini_file.h"


#include "../game_globals.h"      // g_module_settings.show_quest_notes, g_module_settings.player_wounded_treshold, g_module_settings.disable_food_slot, g_display_wp_one_handed (canonical, defined in triggers/trigger_vec.cpp)

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
// 0xa73980 is the window manager (window_mgr.h); its slot 0 is the menu the tutorial branch fills in
// (tstrings at +0x1c and +0x68, the three ints at +4/+0x4c/+0x50).

// the character window's methods (src/game/inventory_panel/char_window.cpp) run on this same object
struct char_window {
    void fill_skills();                         // 0x509730
    void load_troop();                          // 0x5094a0
    void open_view();                           // 0x509980
    void init(const tstring& msg, int type, const char* a, const char* b, const char* c, const char* d, const char* e, const char* f);   // 0x509410 (the message box)
};

// ---- settings access ------------------------------------------------------------------------------------
static const char* config_text(int i)
{
    return g_game_vars.entries[i].value.c_str();   // the text pointer is at +4; null: the field itself holds the text
}

// the inlined key walk of the original: compare every record's key, stop at the first hit
static int config_index(const tstring& key)
{
    int found = -1;
    for (int i = 0; i < g_game_vars.count && found == -1; i++) {
        if (g_game_vars.entries[i].key.equals(key))
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
void CharSheet::init_screen()
{
    int i;

    if (w04 == 0)
        w14 = g_game->player;
    if (w08 == 0) {
        mode = 0;
        w08 = 1;
    }

    g_gui.reset();
    stats = 0;

    // the panel: 0x190-byte container, vtable 0x592848, "character_window" mesh
    NodeA* w = new NodeA;
    w->f188 = 0;
    window = w;

    vector2 v;
    v.x = 0.0f;
    v.y = 0.0f;
    w->move_a(&v.x);
    v.x = 1.0f;
    v.y = 0.75f;
    w->set_xy(&v.x);

    {
        rgl::mesh* mesh;
        {
            tstring n("character_window");
            mesh = g_resmgr.get_object(&n)->clone();
        }
        w->set_track(0, mesh);
    }

    MissionUi* pw = window;
    g_gui.dirty = 1;
    g_gui.objs.push_back(pw);

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
    text_box* sk = new text_box;
    skills = sk;
    label_refresh(sk, tstring("char_bo_skills"));

    v.x = sk->screen[0];
    v.y = ((float)num_skills + 0.5f) * skills_y;
    sk->set_xy(&v.x);

    if (num_skills > 0) {
        for (i = 0; i < num_skills; i++) {
            UiA* row = new UiA;
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

            float rowy = (float)(extra + i) * skills_y + row->fEC;
            v.x = row->fE8;
            v.y = rowy;
            row->move_a(&v.x);

            float colw = config_float_walk(tstring("char_b_skills_w"));
            v.x = colw + skills_x;
            v.y = skills_y;
            row->set_xy(&v.x);

            int idx = skill_id[i];                   // fill_skills' list of active skills (0x5079f8: [&skill_row[i] + 0x1f8])
            char* base = (char*)g_skill_tbl + idx * 0x44;
            tstring* nm = g_strtable.find((tstring*)(base + 0x20));
            if (nm == 0)
                nm = (tstring*)(base + 0x10);
            vector2 tx;
            row->set_text(&tx.x, nm);

            sk->add_child(row);

            float levels_w = config_float_walk(tstring("char_b_skill_levels_w"));
            float levels_x = config_float_walk(tstring("char_b_skill_levels_x"));
            tstring empty("");
            // 0x507b6e: the level label goes only into this->label[i] (= [&skill_row[i] + 0xa8]), not into the row
            label[i] = make_hint(sk, empty, levels_x + skills_x, rowy,
                                     levels_w, skills_y);
            label[i]->set_color(&row->color14c);                     // +0xd8: the per-row rank text update_skill_rows recolours
   // 0x507b9f pushes the row class's vptr+0x14c (an address, not a colour); row->color is the same byte the original reads

            float inc_h = config_float_walk(tstring("char_b_skill_increment_h_a"));
            float inc_y = config_float_walk(tstring("char_b_skill_increment_y_a"));
            float inc_x = config_float_walk(tstring("char_b_skill_increment_x"));
            tstring plus("+");
            // 0x507d69: [&skill_row[i] + 0x150] = this->btn[i]; the row object itself is not written
            UiA* plus150 = make_button(sk, &plus, inc_x + skills_x, inc_y + rowy,
                                     1, skills_y - inc_h);
            btn[i] = plus150;

            float ts_x = config_float_walk(tstring("char_b_skill_increment_t_s"));
            float ts_y = config_float_walk(tstring("char_b_skill_increment_t_s"));
            vector2 ts;
            ts.x = ts_y;
            ts.y = ts_x;
            plus150->set_size(&ts.x);
            plus150->set_text(&tx.x, &plus);
                                 // +0x180: the per-row upgrade button it shows/hides

        }
    }

    // the framed "skills" outer panel
    update_skill_rows();
    UiD* inner;
    {
        tstring ks("char_s_skills");
        inner = make_scrollbar(window, &ks);
    }
    {
        tstring ko("char_sc_skills_outer");
        skills_outer = make_editor_list(sk, &ko, (int)inner);
    }
    skills_outer->draw();
    window->add_child(skills_outer);

    // ---- the three column headings -----------------------------------------------------------------
    {
        MissionUi* t = new MissionUi;
        weapon_title = t;
    }
    label_refresh(weapon_title, tstring("char_t_weapon_proficiencies_title"));
    {
        vector2 tx;
        weapon_title->set_text(&tx.x, g_strtable.lookup(tstring("ui_weapon_proficiencies")));
    }
    window->add_child(weapon_title);

    {
        MissionUi* t = new MissionUi;
        attrib_title = t;
    }
    label_refresh(attrib_title, tstring("char_t_attributes_title"));
    {
        vector2 tx;
        attrib_title->set_text(&tx.x, g_strtable.lookup(tstring("ui_attributes")));
    }
    window->add_child(attrib_title);

    {
        MissionUi* t = new MissionUi;
        skill_title = t;
    }
    label_refresh(skill_title, tstring("char_t_skills_title"));
    {
        vector2 tx;
        skill_title->set_text(&tx.x, g_strtable.lookup(tstring("ui_skills")));
    }
    window->add_child(skill_title);

    // ---- the seven weapon-proficiency rows ----------------------------------------------------------
    for (i = 0; i < 7; i++) {
        UiA* row = new UiA;
        weapon_row[i] = row;
        label_refresh(row, tstring("char_b_weapon_proficiencies_base"));

        float spacing = config_float_walk(tstring("char_b_weapon_proficiencies_y_a"));
        v.x = row->fE8;
        v.y = row->fEC - (float)i * spacing;
        row->move_a(&v.x);

        window->add_child(row);

        float inc_h = config_float_walk(tstring("char_b_weapon_proficiency_increment_h"));
        float inc_y = config_float_walk(tstring("char_b_weapon_proficiency_increment_y"));
        float inc_ya = config_float_walk(tstring("char_b_weapon_proficiency_increment_y_a"));
        float inc_x = config_float_walk(tstring("char_b_weapon_proficiency_increment_x"));
        tstring plus("+");
        // 0x50845e: [&weapon_row[i] + 0x1c] = this->rows[i]; the row object itself is not written
        UiA* plus1c = make_button(window, &plus, inc_x, inc_y - inc_ya * (float)i,
                                1, inc_h);      // 0x50844e: x = increment_x, y = increment_y - increment_y_a * i

        float ts_x = config_float_walk(tstring("char_b_weapon_proficiency_increment_t_s"));
        float ts_y = config_float_walk(tstring("char_b_weapon_proficiency_increment_t_s"));
        vector2 ts;
        ts.x = ts_y;
        ts.y = ts_x;
        rows[i] = plus1c;
        plus1c->set_size(&ts.x);
        {
            vector2 tx;
            plus1c->set_text(&tx.x, &plus);
        }

        if (g_display_wp[i] == 0) {
            row->hide();
            plus1c->hide();
        }
    }

    // ---- the attributes / general-info columns -----------------------------------------------------
    {
        text_box* a = new text_box;
        attribs = a;
    }
    label_refresh(attribs, tstring("char_bo_attribs"));

    {
        text_box* g = new text_box;
        general = g;
    }
    label_refresh(general, tstring("char_bo_general_info"));

    // the character-name button
    {
        UiE* n = new UiE(0);
        name = n;
    }
    label_refresh(name, tstring("char_tb_character_name"));

    {
        rgl::mesh* up;
        {
            tstring k("party_button");
            up = g_resmgr.get_object(&k)->clone();
        }
        rgl::mesh* down;
        {
            tstring k("party_button_down");
            down = g_resmgr.get_object(&k)->clone();
        }
        up->scale(0.6666667f);
        down->scale(0.6666667f);
        name->set_track(2, up);
        name->set_track(1, down);
    }

    {
        char* rec = (char*)g_game->chars + (unsigned)w14 * 0x508;
        tstring* nm = g_strtable.find((tstring*)(rec + 0x20));
        if (nm == 0)
            nm = (tstring*)rec;
        vector2 tx;
        name->set_text(&tx.x, nm);
    }

    if (w14 == 0 && ((tstring*)g_game->chars)->equals("")) {
        name->f181 = 1;
        unsigned col = 0;
        int idx = config_index(tstring("char_tb_character_name_t_c"));
        if (idx >= 0)
            sscanf(config_text(idx), "%x", &col);
        name->set_color(&col);
        vector2 tx;
        name->set_text(&tx.x, g_strtable.lookup(tstring("ui_enter_name_here")));
    } else {
        name->f181 = 0;
    }

    window->add_child(name);
    window->add_child(attribs);
    window->add_child(general);

    // ---- the four attribute rows -------------------------------------------------------------------
    for (i = 0; i < 4; i++) {
        UiA* row = new UiA;
        attrib_row[i] = row;
        skill_pos[i] = row;                 // +0x45c-8: refresh_panels sets each attribute row's text
        label_refresh(row, tstring("char_b_attribs_base"));

        float spacing = config_float_walk(tstring("char_b_attribs_y_a"));
        float rowy = row->fEC - (float)i * spacing;
        v.x = row->fE8;
        v.y = rowy;
        row->move_a(&v.x);

        attribs->add_child(row);

        float inc_h = config_float_walk(tstring("char_b_attrib_increment_h"));
        float inc_ya = config_float_walk(tstring("char_b_attrib_increment_y_a"));
        float inc_x = config_float_walk(tstring("char_b_attrib_increment_x"));
        tstring plus("+");
        // [&attrib_row[i] + 0x10] = this->skill_val[i]; the row object itself is not written
        UiA* plus10 = make_button(attribs, &plus, inc_x, rowy - inc_ya, 1, inc_h);
        skill_val[i] = plus10;         // +0x45c: refresh_panels shows/hides the '+' per attribute

        float ts_x = config_float_walk(tstring("char_b_attrib_increment_t_s"));
        float ts_y = config_float_walk(tstring("char_b_attrib_increment_t_s"));
        vector2 ts;
        ts.x = ts_y;
        ts.y = ts_x;
        plus10->set_size(&ts.x);
        {
            vector2 tx;
            plus10->set_text(&tx.x, &plus);
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
        stats = new_sized_button(window, *cap, 0, 0, h, normal, pressed,
                                   3.7f, 0.072f, 0.085f);
    }
    label_refresh(stats, tstring("char_b_stats"));

    f52c = 1;
    if (g_cfg.enable_edit_mode == 0 && w14 != g_game->player)
        f52c = 0;
    if (mode != -1) {
        f52c = 0;
        stats->hide();
    }
    prev = 0;
    next = 0;

    if (g_cfg.enable_edit_mode != 0 && mode == -1) {
        {
            tstring kh("char_b_next_h");
            tstring kc("ui_next");
            float h = config_float_walk(kh);
            tstring* cap = g_strtable.lookup(kc);
            tstring normal("short_button");
            tstring pressed("short_button_down");
            next = new_sized_button(window, *cap, 0, 0, h, normal, pressed,
                                      3.0f, 0.072f, 0.085f);
        }
        label_refresh(next, tstring("char_b_next"));
    }

    if (g_cfg.enable_edit_mode != 0 && mode == -1) {
        {
            tstring kh("char_b_prev_h");
            tstring kc("ui_prev");
            float h = config_float_walk(kh);
            tstring* cap = g_strtable.lookup(kc);
            tstring normal("short_button");
            tstring pressed("short_button_down");
            prev = new_sized_button(window, *cap, 0, 0, h, normal, pressed,
                                      3.0f, 0.072f, 0.085f);
        }
        label_refresh(prev, tstring("char_b_prev"));
    }

    // ---- the skill-description panel ---------------------------------------------------------------
    {
        text_box* d = new text_box;
        skill_desc = d;
    }
    label_refresh(skill_desc, tstring("char_bo_skill_desc"));
    window->add_child(skill_desc);

    {
        MissionUi* e = new MissionUi;
        explanation = e;
    }
    label_refresh(explanation, tstring("char_t_explanation"));
    skill_desc->add_child(explanation);
    explanation->hide();

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
        slot_as<char_window>(1)->init(*g_strtable.lookup(k3), 2, real, nonreal, "", "", "", "");
        g_windows.open(1);
        return;
    } else if (mode == 1) {
        tstring k1("ui_tutorial_character_generation");
        tstring k2("ui_tutorial");
        tstring* gen = g_strtable.lookup(k1);
        tstring* tut = g_strtable.lookup(k2);
        g_windows.box()->set_inline(tut, gen);
        g_windows.open(0);
    }
}

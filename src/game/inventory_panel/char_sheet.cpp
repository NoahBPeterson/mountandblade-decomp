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
static __forceinline int config_index(const tstring& key)
{
    int found = -1;
    for (int i = 0; found == -1 && i < g_game_vars.count; i++) {
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
static __forceinline float config_float_walk(const tstring& key)
{
    float v = 0;
    int i = config_index(key);
    if (i >= 0) v = (float)atof(config_text(i));
    return v;
}

// registers a top-level window with the gui
static inline void gui_add(MissionUi* w)
{
    g_gui.dirty = 1;
    g_gui.objs.push_back(w);
}

// the "+" buttons take their caption by reference in the callers' source
static __forceinline UiA* make_button(ContainerBase* parent, const tstring& text, float x, float y, int flag, float sy)
{
    return make_button(parent, &text, x, y, flag, sy);
}

// the widget virtual's result (the text size) is ignored
static __forceinline void set_caption(MissionUi* w, const tstring& text)
{
    float out[2];
    w->set_text(out, &text);
}

static __forceinline UiD* make_scrollbar(void* parent, const tstring& key)
{
    return make_scrollbar(parent, &key);
}

static __forceinline Camera* make_editor_list(void* parent, const tstring& key, int child)
{
    return make_editor_list(parent, &key, child);
}

static __forceinline void set_color_v(MissionUi* w, const unsigned& c)
{
    w->set_color((unsigned*)&c);
}

static __forceinline float scaled(float k, float sp)
{
    return sp * k;
}

// a "%x" colour value, through the inlined walk
static __forceinline unsigned config_hex_walk(const tstring& key)
{
    unsigned v = 0;
    int i = config_index(key);
    if (i >= 0) sscanf(config_text(i), "%x", &v);
    return v;
}

// the statistics / next / previous buttons: 0x4dcc50 with the button meshes spelled out
static __forceinline UiA* medium_button_h(MissionUi* parent, const tstring& text, float h)
{
    return new_sized_button(parent, text, 0, 0, h, "medium_button", "medium_button_down", 3.7f, 0.072f, 0.085f);
}

static __forceinline UiA* short_button_h(MissionUi* parent, const tstring& text, float h)
{
    return new_sized_button(parent, text, 0, 0, h, "short_button", "short_button_down", 3.0f, 0.072f, 0.085f);
}

// @ 0x507400
void CharSheet::init_screen()
{
    if (w04 == 0)
        w14 = g_game->player;
    if (w08 == 0) {
        mode = 0;
        w08 = 1;
    }

    g_gui.reset();
    stats = 0;

    // the panel: 0x190-byte container, vtable 0x592848, "character_window" mesh
    window = new NodeA((char)0);
    window->move_a((const float*)&vector2(0.0f, 0.0f));
    window->set_xy((const float*)&vector2(1.0f, 0.75f));
    rgl::mesh* mesh;
    {
        tstring n("character_window");
        mesh = g_resmgr.get_object(&n)->clone();
    }
    window->set_track(0, mesh);

    gui_add(window);

    // reset-changes button (this+0x430) and done button (this+0x42c)
    reset = new_medium_button(window, g_strtable.lookup("ui_reset_changes"), 0, 0, config_float("char_b_reset_h"));
    label_refresh(reset, "char_b_reset");
    done = make_short_button(window, g_strtable.lookup("ui_done"), 0, 0, config_float("char_b_done_h"));
    label_refresh(done, "char_b_done");

    ((char_window*)this)->fill_skills();

    f4c8 = 1;
    f2d4 = -1;
    f2d8 = 0;

    float skills_y = config_float("char_b_skills_y_a");

    // do the configured skill column offsets apply to this character?
    float skills_x = num_skills <= g_game_vars.get_int("char_num_skills")
        ? g_game_vars.get_float("char_b_skills_x_a") : 0.0f;

    // ---- the skill column -------------------------------------------------------------------------
    skills = new text_box;
    label_refresh(skills, "char_bo_skills");
    skills->set_xy((const float*)&vector2(skills->screen[0], ((float)num_skills + 0.5f) * skills_y));

    for (int i = 0; i < num_skills; i++) {
        skill_row[i] = new UiA;
        label_refresh(skill_row[i], "char_b_skills_base");

        float rowy = (float)((num_skills < g_game_vars.get_int("char_num_skills")
                              ? g_game_vars.get_int("char_num_skills") - num_skills : 0) + i)
                     * skills_y + skill_row[i]->fEC;
        skill_row[i]->move_a((const float*)&vector2(skill_row[i]->fE8, rowy));
        skill_row[i]->set_xy((const float*)&vector2(config_float_walk("char_b_skills_w") + skills_x, skills_y));

        {
            char* base = (char*)g_skill_tbl + skill_id[i] * 0x44;
            tstring* nm = g_strtable.find((tstring*)(base + 0x20));
            if (nm == 0)
                nm = (tstring*)(base + 0x10);
            float tx[2];
            skill_row[i]->set_text(tx, nm);
        }

        skills->add_child(skill_row[i]);

        // 0x507b6e: the level label goes only into this->label[i] (= [&skill_row[i] + 0xa8]), not into the row
        label[i] = make_hint(skills, "", config_float_walk("char_b_skill_levels_x") + skills_x, rowy,
                             config_float_walk("char_b_skill_levels_w"), skills_y);
        // 0x507b9f pushes the row class's vptr+0x14c (an address, not a colour); row->color is the same byte the original reads
        label[i]->set_color(&skill_row[i]->color14c);

        // 0x507d69: [&skill_row[i] + 0x150] = this->btn[i]; the row object itself is not written
        btn[i] = make_button(skills, "+", config_float_walk("char_b_skill_increment_x") + skills_x,
                             config_float_walk("char_b_skill_increment_y_a") + rowy, 1,
                             skills_y - config_float_walk("char_b_skill_increment_h_a"));
        {
            tstring s1("char_b_skill_increment_t_s");
            float a1 = config_float_walk(s1);
            tstring s2("char_b_skill_increment_t_s");
            float a2 = config_float_walk(s2);
            btn[i]->set_size((const float*)&vector2(a2, a1));
        }
        set_caption(btn[i], "+");
    }

    // the framed "skills" outer panel
    update_skill_rows();
    UiD* inner;
    {
        tstring k("char_s_skills");
        inner = make_scrollbar(window, &k);
    }
    {
        tstring k("char_sc_skills_outer");
        skills_outer = make_editor_list(skills, &k, (int)inner);
    }
    skills_outer->draw();
    window->add_child(skills_outer);

    // ---- the three column headings -----------------------------------------------------------------
    weapon_title = new MissionUi;
    label_refresh(weapon_title, "char_t_weapon_proficiencies_title");
    {
        float tx[2];
        weapon_title->set_text(tx, g_strtable.lookup("ui_weapon_proficiencies"));
    }
    window->add_child(weapon_title);

    attrib_title = new MissionUi;
    label_refresh(attrib_title, "char_t_attributes_title");
    {
        float tx[2];
        attrib_title->set_text(tx, g_strtable.lookup("ui_attributes"));
    }
    window->add_child(attrib_title);

    skill_title = new MissionUi;
    label_refresh(skill_title, "char_t_skills_title");
    {
        float tx[2];
        skill_title->set_text(tx, g_strtable.lookup("ui_skills"));
    }
    window->add_child(skill_title);

    // ---- the seven weapon-proficiency rows ----------------------------------------------------------
    for (int j = 0; j < 7; j++) {
        weapon_row[j] = new UiA;
        label_refresh(weapon_row[j], "char_b_weapon_proficiencies_base");

        float fj;
        {
            tstring k("char_b_weapon_proficiencies_y_a");
            float sp = config_float_walk(k);
            fj = (float)j;
            weapon_row[j]->move_a((const float*)&vector2(weapon_row[j]->fE8, weapon_row[j]->fEC - sp * fj));
        }
        window->add_child(weapon_row[j]);

        // 0x50845e: [&weapon_row[j] + 0x1c] = this->rows[j]; the row object itself is not written
        rows[j] = make_button(window, "+", config_float_walk("char_b_weapon_proficiency_increment_x"),
                              config_float_walk("char_b_weapon_proficiency_increment_y")
                              - config_float_walk("char_b_weapon_proficiency_increment_y_a") * fj,
                              1, config_float_walk("char_b_weapon_proficiency_increment_h"));
        {
            tstring s1("char_b_weapon_proficiency_increment_t_s");
            float a1 = config_float_walk(s1);
            tstring s2("char_b_weapon_proficiency_increment_t_s");
            float a2 = config_float_walk(s2);
            rows[j]->set_size((const float*)&vector2(a2, a1));
        }
        set_caption(rows[j], "+");

        if (g_display_wp[j] == 0) {
            weapon_row[j]->hide();
            rows[j]->hide();
        }
    }

    // ---- the attributes / general-info columns -----------------------------------------------------
    attribs = new text_box;
    label_refresh(attribs, "char_bo_attribs");
    general = new text_box;
    label_refresh(general, "char_bo_general_info");

    // the character-name button
    name = new UiE(0);
    label_refresh(name, "char_tb_character_name");

    rgl::mesh* up;
    rgl::mesh* down;
    {
        {
            tstring k("party_button");
            up = g_resmgr.get_object(&k)->clone();
        }
        // declared in the block that encloses the first key: the scopes nest, so the two keys may not share a slot
        tstring k("party_button_down");
        down = g_resmgr.get_object(&k)->clone();
    }
    up->scale(0.6666667f);
    down->scale(0.6666667f);
    name->set_track(2, up);
    name->set_track(1, down);

    {
        char* rec = (char*)g_game->chars + (unsigned)w14 * 0x508;
        tstring* nm = g_strtable.find((tstring*)(rec + 0x20));
        if (nm == 0)
            nm = (tstring*)rec;
        float tx[2];
        name->set_text(tx, nm);
    }

    if (w14 == 0 && ((tstring*)g_game->chars)->equals("")) {
        name->f181 = 1;
        set_color_v(name, config_hex_walk("char_tb_character_name_t_c"));
        float tx[2];
        name->set_text(tx, g_strtable.lookup("ui_enter_name_here"));
    } else {
        name->f181 = 0;
    }

    window->add_child(name);
    window->add_child(attribs);
    window->add_child(general);

    // ---- the four attribute rows -------------------------------------------------------------------
    for (int k = 0; k < 4; k++) {
        attrib_row[k] = new UiA;
        label_refresh(attrib_row[k], "char_b_attribs_base");

        float rowy = attrib_row[k]->fEC - (float)k * config_float_walk("char_b_attribs_y_a");
        attrib_row[k]->move_a((const float*)&vector2(attrib_row[k]->fE8, rowy));
        attribs->add_child(attrib_row[k]);

        // [&attrib_row[k] + 0x10] = this->skill_val[k]; the row object itself is not written
        skill_val[k] = make_button(attribs, "+", config_float_walk("char_b_attrib_increment_x"),
                                   rowy - config_float_walk("char_b_attrib_increment_y_a"),
                                   1, config_float_walk("char_b_attrib_increment_h"));
        {
            tstring s1("char_b_attrib_increment_t_s");
            float a1 = config_float_walk(s1);
            tstring s2("char_b_attrib_increment_t_s");
            float a2 = config_float_walk(s2);
            skill_val[k]->set_size((const float*)&vector2(a2, a1));
        }
        set_caption(skill_val[k], "+");
    }

    // ---- statistics / next / previous --------------------------------------------------------------
    stats = medium_button_h(window, *g_strtable.lookup("ui_statistics"), config_float_walk("char_b_stats_h"));
    label_refresh(stats, "char_b_stats");

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
        next = short_button_h(window, *g_strtable.lookup("ui_next"), config_float_walk("char_b_next_h"));
        label_refresh(next, "char_b_next");
        prev = short_button_h(window, *g_strtable.lookup("ui_prev"), config_float_walk("char_b_prev_h"));
        label_refresh(prev, "char_b_prev");
    }

    // ---- the skill-description panel ---------------------------------------------------------------
    skill_desc = new text_box;
    label_refresh(skill_desc, "char_bo_skill_desc");
    window->add_child(skill_desc);

    explanation = new MissionUi;
    label_refresh(explanation, "char_t_explanation");
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
        {
            tstring k1("ui_saving_policy_nonrealistic");
            tstring k2("ui_saving_policy_realistic");
            tstring k3("ui_question_saving_policy");
            const char* nonreal = g_strtable.lookup(k1)->c_str();
            const char* real = g_strtable.lookup(k2)->c_str();
            slot_as<char_window>(1)->init(*g_strtable.lookup(k3), 2, real, nonreal, "", "", "", "");
        }
        g_windows.open(1);
        return;
    } else if (mode == 1) {
        {
            tstring k1("ui_tutorial_character_generation");
            tstring k2("ui_tutorial");
            tstring* gen = g_strtable.lookup(k1);
            tstring* tut = g_strtable.lookup(k2);
            g_windows.box()->set_inline(tut, gen);
        }
        g_windows.open(0);
    }
}

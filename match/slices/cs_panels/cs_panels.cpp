// cs_panels: the character sheet's text pass (refresh_panels, 0x4f0820) and the detail pane of the
// skill / attribute list (update_detail, 0x4ef9f0).
//
// refresh_panels takes the edited troop record (g_game(0x5e2930)->chars(+0x13a00)[this+0x14], stride
// 0x508), formats level / experience / health / points / the four attribute lines into a 0x200-byte
// buffer and posts each line, with its configured (x, y) position, on the matching text widget of the
// sheet.  Positions come from the settings table 0xa44240 (find 0x4bd5a0 -> entry index, entries of
// 0x2c bytes from 0xa4425c with the value pointer at +4, atof 0x537352).
//
// update_detail fills the title label (+0x4b8) and the description box (+0x434) of the panel with the
// text of the selected row of the current page (+0x2d8: 0 attributes, 1 skills, 2 proficiencies,
// 3 face; the row itself in +0x2d4, the row ids in +0x228).
#include <windows.h>
#include <stdio.h>
#include <string.h>

// ---- rgl string (16 bytes: position, buffer, capacity, length) --------------------------------------
struct tstring {
    int pos;                                        // +0
    char* buf;                                      // +4
    int cap;                                        // +8
    int len;                                        // +0xc
    tstring();                                      // 0x42bbd0
    tstring(const char* s);                         // 0x42bbe0
    tstring(const tstring& o);                      // 0x42c110
    ~tstring();                                     // 0x42bc40
    tstring operator+(const tstring& o) const;      // 0x42c840
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

// translation table at 0x5e2a18
struct StrTable {
    tstring* lookup(const tstring& key);            // 0x43a1e0 (never null)
    tstring* find(const tstring* key);              // 0x4c6010 (null when absent)
};
extern StrTable g_strtable;                         // 0x5e2a18

// ---- the troop record (0x508 bytes; g_game->chars at +0x13a00) --------------------------------------
struct Troop {
    tstring name;                                   // +0x00
    tstring plural;                                 // +0x10
    tstring key;                                    // +0x20 translation key
    unsigned __int64 flags;                         // +0x30 (bit 0x10 = hero)
    char pad38[0x58 - 0x38];
    int attrs[4];                                   // +0x58 strength / agility / intelligence / charisma
    char pad68[0x7a - 0x68];
    unsigned char nib;                              // +0x7a
    char pad7b[0x9c - 0x7b];
    int xp;                                         // +0x9c
    float hp;                                       // +0xa0
    int skill_points;                               // +0xa4
    int attr_points;                                // +0xa8
    int prof_points;                                // +0xac
    int level;                                      // +0xb0
    char padb4[0x508 - 0xb4];

    int nibble(int i);                              // 0x4351a0 (packed 4-bit skill rank)
};

struct GameData {
    char pad8c[0x8c];
    int player;                                     // +0x8c (hero troop index)
    char pad90[0x13a00 - 0x90];
    Troop* chars;                                   // +0x13a00
    int skill_level(int troop, int skill);          // 0x437b20 (ret 8)
};
extern GameData* g_game;                            // 0x5e2930

// ---- the skill table (0x8ee3100, stride 0x44) / its attribute field ---------------------------------
struct SkillRec {
    tstring desc;                                   // +0x00
    tstring name;                                   // +0x10
    tstring key;                                    // +0x20
    int f30;                                        // +0x30
    int f34;                                        // +0x34
    int cap;                                        // +0x38 (the level cap)
    int f3c;                                        // +0x3c
    unsigned attr;                                  // +0x40 (low nibble: the attribute the skill needs)
    tstring* desc_name();                           // 0x4c6190
};
extern SkillRec g_skills[];                         // 0x8ee3100

// the ini table at 0xa44240
struct ConfigTable {
    int find(const tstring& key);                   // 0x4bd5a0
    float get_float(const tstring& key);            // 0x4bd5e0
};
extern ConfigTable g_config;                        // 0xa44240
struct ConfigEntry {
    int f0;
    const char* value;                              // +4
    char pad[0x2c - 8];
};
extern "C" double atof(const char* s);              // 0x537352

extern int g_level_table[];                         // 0x5937cc
extern int g_skill_clamp;                           // 0x8ee3ac4
extern const char* g_char_t_level_x;                // 0xa18120
extern const char* g_char_t_level_y;                // 0xa18124
void mb_snprintf(char* buf, unsigned size, const char* fmt, ...);   // 0x415100
void mb_vsprintf(char* buf, const char* fmt, ...);                  // 0x536f8f

// "read a configured key as float, 0 when the key is absent"
static __forceinline float config_float(const tstring& key)
{
    float v = 0;
    int i = g_config.find(key);
    if (i >= 0) {
        ConfigEntry* e = &((ConfigEntry*)0xa4425c)[i];
        v = (float)atof(e->value ? e->value : (const char*)&e->value);
    }
    return v;
}

// translated text of a key
static __forceinline const char* tr(const tstring& key)
{
    tstring* r = g_strtable.lookup(key);
    return r->buf ? r->buf : (const char*)&r->buf;
}

// ---- widgets ----------------------------------------------------------------------------------------
// +0xa4 set_text(text) returns a small pair through a hidden return pointer, +0xac set_text_at(text,
// pos) (ret 8), +0xb0 hide, +0xb4 show.
struct WPair { int a, b; };
struct Widget {
    virtual void d00(); virtual void d01(); virtual void d02(); virtual void d03(); virtual void d04();
    virtual void d05(); virtual void d06(); virtual void d07(); virtual void d08(); virtual void d09();
    virtual void d10(); virtual void d11(); virtual void d12(); virtual void d13(); virtual void d14();
    virtual void d15(); virtual void d16(); virtual void d17(); virtual void d18(); virtual void d19();
    virtual void d20(); virtual void d21(); virtual void d22(); virtual void d23(); virtual void d24();
    virtual void d25(); virtual void d26(); virtual void d27(); virtual void d28(); virtual void d29();
    virtual void d30(); virtual void d31(); virtual void d32(); virtual void d33(); virtual void d34();
    virtual void d35(); virtual void d36(); virtual void d37(); virtual void d38(); virtual void d39();
    virtual void d40();
    virtual void set_text(WPair* out, const tstring& text);         // +0xa4
    virtual void d42();
    virtual void set_text_at(const tstring& text, const float* pos); // +0xac
    virtual void hide();                                            // +0xb0
    virtual void show();                                            // +0xb4
    char pad[0x181 - 4];
    char has_label;                                                 // +0x181
};

// ---- the character sheet panel ----------------------------------------------------------------------
struct CharSheet {
    int pad0;
    char pad4[0x14 - 4];
    int player;                                     // +0x14 (edited troop index)
    char pad18[0x228 - 0x18];
    int page_rows[0x2d4 / 4 - 0x228 / 4];           // +0x228 row ids
    int f_2d4;                                      // +0x2d4 selected row (-1 = none)
    int f_2d8;                                      // +0x2d8 page
    char pad2dc[0x434 - 0x2dc];
    Widget* f_434;                                  // +0x434 description box
    char pad438[0x44c - 0x438];
    Widget* skill_pos[4];                           // +0x44c
    Widget* skill_val[4];                           // +0x45c
    char pad46c[0x4b0 - 0x46c];
    Widget* level_at;                               // +0x4b0
    Widget* xp_bar;                                 // +0x4b4
    Widget* f_4b8;                                  // +0x4b8 title label
    char pad4bc[0x4c4 - 0x4bc];
    Widget* name;                                   // +0x4c4

    void refresh_panels();                          // 0x4f0820
    void update_detail();                           // 0x4ef9f0
};

// 0x4f04f0: the caption of a weapon class into buf
void weapon_class_caption(int type, char* out);     // 0x4f04f0

// a line posted with its configured (x, y)
#define POS(W, LINE, KEY_Y, KEY_X)                                   \
    do {                                                             \
        tstring ky(KEY_Y);                                           \
        float y = config_float(ky);                                  \
        tstring kx(KEY_X);                                           \
        float x = config_float(kx);                                  \
        tstring t(LINE);                                             \
        float pos[2];                                                \
        pos[0] = x;                                                  \
        pos[1] = y;                                                  \
        W->set_text_at(t, pos);                                      \
    } while (0)

// @ 0x4f0820
void CharSheet::refresh_panels()
{
    int i = 0;
    WPair out;
    Troop* c = &g_game->chars[player];
    char line[0x200];
    char text[0x200];

    {
        tstring t("");
        level_at->set_text(&out, t);
    }
    {
        tstring t("");
        xp_bar->set_text(&out, t);
    }

    if (name->has_label == 0) {
        tstring* n = g_strtable.find(&c->key);
        if (n == 0) n = &c->name;
        tstring t(n->c_str());
        name->set_text(&out, t);
    }

    {
        tstring k("ui_level");
        mb_vsprintf(line, tr(k), c->level);
    }
    POS(xp_bar, line, "char_t_level_y", "char_t_level_x");

    {
        tstring k("ui_xp");
        mb_vsprintf(line, tr(k), c->xp);
    }
    POS(xp_bar, line, "char_t_xp_y", "char_t_xp_x");

    {
        tstring k("ui_next_level_at");
        mb_vsprintf(line, tr(k), g_level_table[c->level]);
    }
    POS(xp_bar, line, "char_t_next_level_at_y", "char_t_next_level_at_x");

    if ((c->flags & 0x10) != 0) {
        tstring k("ui_health_player");
        i = c->nib & 0xf;
        if (i < 0) i = 0;
        else if (i > g_skill_clamp) i = g_skill_clamp;
        mb_vsprintf(line, tr(k), (int)c->hp, c->attrs[0] + i * 2 + 0x23);
    } else {
        tstring k("ui_health");
        i = c->nib & 0xf;
        if (i < 0) i = 0;
        else if (i > g_skill_clamp) i = g_skill_clamp;
        mb_vsprintf(line, tr(k), c->attrs[0] + i * 2 + 0x23);
    }
    POS(xp_bar, line, "char_t_health_y", "char_t_health_x");

    {
        tstring k("ui_attribute_points");
        mb_snprintf(line, 0x200, tr(k), c->attr_points);
    }
    POS(level_at, line, "char_t_attrib_points_y", "char_t_attrib_points_x");

    {
        tstring k("ui_skill_points");
        mb_snprintf(line, 0x200, tr(k), c->skill_points);
    }
    POS(level_at, line, "char_t_skill_points_y", "char_t_skill_points_x");

    {
        tstring k("ui_weapon_points");
        mb_snprintf(line, 0x200, tr(k), c->prof_points);
    }
    POS(level_at, line, "char_t_weapon_points_y", "char_t_weapon_points_x");

    for (i = 0; i < 4; i++) {
        mb_snprintf(text, 0x200, "%s: %d",
                    tr(i == 0 ? tstring("ui_str") : i == 1 ? tstring("ui_agi")
                       : i == 2 ? tstring("ui_int") : tstring("ui_cha")),
                    g_game->chars[player].attrs[i]);
        {
            tstring t(text);
            skill_pos[i]->set_text(&out, t);
        }
        if (c->attr_points > 0)
            skill_val[i]->show();
        else
            skill_val[i]->hide();
    }
}

// @ 0x4ef9f0
void CharSheet::update_detail()
{
    WPair out;
    char title[0x800];
    char body[0x200];
    char attr[0x200];
    {
        tstring t("");
        f_4b8->set_text(&out, t);
    }
    f_434->hide();
    int row = f_2d4;
    if (row < 0)
        return;

    if (f_2d8 == 1) {
        // skills page
        int id = page_rows[row];
        SkillRec* sk = &g_skills[id];
        tstring sep(": ");
        tstring* nm = g_strtable.find(&sk->key);
        if (nm == 0) nm = &sk->name;
        tstring s = (*nm + sep) + *sk->desc_name();
        const char* anchor = 0;
        if (id == 2) anchor = g_char_t_level_y;
        else if (id == 1) anchor = g_char_t_level_x;
        mb_snprintf(title, 0x400, s.c_str(), anchor);
        {
            tstring t(title);
            f_4b8->set_text(&out, t);
        }
        mb_snprintf(body, 0x200, tr(tstring("ui_current_level_value")),
                    g_game->skill_level(player, id));
        {
            tstring ky("char_t_current_level_y");
            float fy = g_config.get_float(ky);
            tstring kx("char_t_current_level_x");
            float fx = g_config.get_float(kx);
            tstring t(body);
            float pos[2];
            pos[0] = fx;
            pos[1] = fy;
            f_4b8->set_text_at(t, pos);
        }
        mb_snprintf(body, 0x200, tr(tstring("ui_base_attribute_value")),
                    tr((g_skills[id].attr & 0xf) == 0 ? tstring("ui_str")
                       : (g_skills[id].attr & 0xf) == 1 ? tstring("ui_agi")
                       : (g_skills[id].attr & 0xf) == 2 ? tstring("ui_int") : tstring("ui_cha")));
        {
            tstring ky("char_t_base_attrib_y");
            float fy = g_config.get_float(ky);
            tstring kx("char_t_base_attrib_x");
            float fx = g_config.get_float(kx);
            tstring t(body);
            float pos[2];
            pos[0] = fx;
            pos[1] = fy;
            f_4b8->set_text_at(t, pos);
        }
        if (player != g_game->player) {
            f_434->hide();
            return;
        }
        if (g_game->chars[player].nibble(id) < g_skills[id].cap
            && g_game->chars[player].skill_points >= 1
            && (g_game->chars[player].nibble(id) + 1) * 3
               <= g_game->chars[player].attrs[g_skills[id].attr & 0xf]) {
            f_434->hide();
            return;
        }
        if (g_game->chars[player].nibble(id) >= g_skills[id].cap) {
            mb_snprintf(body, 0x200, tr(tstring("ui_at_learning_limit")));
        } else if (g_game->chars[player].skill_points < 1) {
            mb_snprintf(body, 0x200, tr(tstring("ui_not_enough_skill_points_to_learn")));
        } else {
            if ((g_skills[id].attr & 0xf) == 0)
                mb_snprintf(attr, 0x200, tr(tstring("ui_strength")));
            else if ((g_skills[id].attr & 0xf) == 1)
                mb_snprintf(attr, 0x200, tr(tstring("ui_agility")));
            else if ((g_skills[id].attr & 0xf) == 2)
                mb_snprintf(attr, 0x200, tr(tstring("ui_intelligence")));
            else
                mb_snprintf(attr, 0x200, tr(tstring("ui_charisma")));
            mb_snprintf(body, 0x200,
                        tr(tstring("ui_not_enough_attributetype_to_learn_this_skill")), attr);
        }
        {
            tstring t(body);
            f_434->set_text(&out, t);
        }
        f_434->show();
        return;
    }
    if (f_2d8 == 2) {
        // proficiencies page
        weapon_class_caption(row, body);
        row = f_2d4;
        if (row == 0)
            strcpy(attr, tr(tstring("ui_explanation_one_handed_weapon")));
        else if (row == 1)
            strcpy(attr, tr(tstring("ui_explanation_two_handed_weapon")));
        else if (row == 2)
            strcpy(attr, tr(tstring("ui_explanation_polearm")));
        else if (row == 3)
            strcpy(attr, tr(tstring("ui_explanation_archery")));
        else if (row == 4)
            strcpy(attr, tr(tstring("ui_explanation_crossbow")));
        else if (row == 5)
            strcpy(attr, tr(tstring("ui_explanation_throwing")));
        else
            strcpy(attr, tr(tstring("ui_explanation_firearms")));
        mb_snprintf(title, 0x400, "%s: %s", body, attr);
        {
            tstring t(title);
            f_4b8->set_text(&out, t);
        }
        f_434->hide();
        return;
    }
    if (f_2d8 == 0) {
        // attributes page
        if (row == 0)
            strcpy(title, tr(tstring("ui_explanation_strength")));
        else if (row == 1)
            strcpy(title, tr(tstring("ui_explanation_agility")));
        else if (row == 2)
            strcpy(title, tr(tstring("ui_explanation_intelligence")));
        else if (row == 3)
            strcpy(title, tr(tstring("ui_explanation_charisma")));
        {
            tstring t(title);
            f_4b8->set_text(&out, t);
        }
        f_434->hide();
        return;
    }
    if (f_2d8 == 3) {
        // face page
        f_4b8->set_text(&out, *g_strtable.lookup(tstring("ui_edit_face")));
        f_434->hide();
    }
}

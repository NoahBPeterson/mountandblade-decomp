// Slice s004ef9f0: the character panel's detail page (0x4ef9f0).  The panel keeps the row ids of the
// skill/attribute pages at +0x228, the page number in +0x2d8 (0 = attributes, 1 = skills, 2 = proficiencies,
// 3 = face), the selected row in +0x2d4, the title label above the list at +0x4b8 and the description box
// under it at +0x434.  The handler fills the title and the description with text tailored to that row:
// skills get "name: description" plus their level and a learn/spend hint, proficiencies get their group
// name and explanation, attributes their name, base value and explanation; the face page only a title.
// See s004eeb10 for the rest of the panel.  Names are provisional; offsets are read off this slice.
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <string.h>
#include "rgl.h"

extern "C" void __cdecl mb_snprintf(char*, unsigned, const char*, ...);    // 0x415100

// ---- a tstring as stored in the record tables (ctor 0x42bbe0) ----------------------------------------
struct tstring {
    int pos;
    char* buf;
    int cap;
    int len;
    tstring();
    tstring(const char* s);                         // 0x42bbe0
    tstring(const tstring& o);                      // 0x42c110
    ~tstring();                                     // 0x42bc40
    tstring operator+(const tstring& o) const;      // 0x42c840
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

struct StrTable {
    tstring* find(const tstring* key);              // 0x4c6010 (null when absent)
    tstring* lookup(const tstring& key);            // 0x43a1e0 (never null; null buffer for a missing key)
};
extern StrTable g_strtable;                         // 0x5e2a18

// translated text of a key (lookup, falling back to the empty string stored in place)
static const char* str_text(const tstring& key)
{
    tstring* r = g_strtable.lookup(key);
    return r->buf ? r->buf : (const char*)&r->buf;
}

// ---- the virtual interface of the widgets seen through their vptr (0xdword slots, see s0048b760) ------
struct UiVt {
    virtual void d00(); virtual void d01(); virtual void d02(); virtual void d03(); virtual void d04();
    virtual void d05(); virtual void d06(); virtual void d07(); virtual void d08(); virtual void d09();
    virtual void d10(); virtual void d11(); virtual void d12(); virtual void d13(); virtual void d14();
    virtual void d15(); virtual void d16(); virtual void d17();
    virtual void set_key_flags(int f);              // +0x48
    virtual void d19(); virtual void d20(); virtual void d21(); virtual void d22(); virtual void d23();
    virtual void d24();
    virtual void set_xy(float* v);                  // +0x64
    virtual void d26();
    virtual void set_size(float* v);                // +0x6c
    virtual void set_pos(float* v);                 // +0x70
    virtual void d29(); virtual void d30(); virtual void d31(); virtual void d32(); virtual void d33();
    virtual void d34(); virtual void d35();
    virtual void refresh();                         // +0x90
    virtual void get_xy(float* v);                  // +0x94
    virtual void set_color(unsigned* c);            // +0x98
    virtual void d39(); virtual void d40();
    virtual void get_text_size(tstring* s, float* out);  // +0xa4
    virtual void d42();
    virtual void set_text(const tstring* s, float* xy);  // +0xac
    virtual void hide();                            // +0xb0
    virtual void show();                            // +0xb4
    virtual void d46(); virtual void d47(); virtual void d48(); virtual void d49(); virtual void d50();
    virtual void update_children();                 // +0xc4
    virtual void d53(); virtual void d54();
};

struct UiWidget {
    UiVt* vt;                                       // +0x00
};

// ---- game records ------------------------------------------------------------------------------------
struct TroopRec {
    char pad0[0x58];
    int f_58[0x14];                                 // +0x58 (attributes, then proficiencies)
    int skill_points;                               // +0xa4
    char padb0[0x508 - 0xa8];

    int skill_rank(int idx);                        // 0x4351a0 (packed 4-bit rank of proficiency idx)
};

struct Game {
    char pad8c[0x8c];
    int player;                                     // +0x8c (hero troop index)
    char pad90[0x13a00 - 0x90];
    TroopRec* troops;                               // +0x13a00

    int get_prof(int troop, int idx);               // 0x437b20 (packed 4-bit rank)
};
extern Game* g_game;                                // 0x5e2930

// skill table at 0x8ee3100, stride 0x44
struct SkillRec {
    tstring desc;                                   // +0x00
    tstring name;                                   // +0x10
    tstring key;                                    // +0x20
    int cap;                                        // +0x34
    int attr_cap;                                   // +0x38
    char pad3c[4];
    unsigned attr;                                  // +0x40 (low nibble: 0-based attribute the skill needs)
    tstring* translated();                          // 0x4c6190 (the key's "_desc" translation)
};
extern SkillRec g_skills[];                         // 0x8ee3100
#define SKILL_ATTR(id) (g_skills[id].attr & 0xf)

// the ini table at 0xa44240 (0x2c-byte entries, see s004bd170)
struct ini_file {
    float get_float(const tstring& key);            // 0x4bd5e0
};
extern ini_file g_config;                           // 0xa44240

// the character texture's text anchors for the level values; the original loads the pointer stored
// in each global and prints it as a "%s" argument
extern const char* g_char_t_level_x;                // 0xa18120
extern const char* g_char_t_level_y;                // 0xa18124

// the character sheet panel (full layout in s004eeb10)
struct CharSheet {
    int vptr;                                       // +0x00
    int f_4;
    char pad8[0x14 - 0x8];
    int f_14;                                       // +0x14 (edited troop index)
    char pad18[0x228 - 0x18];
    int f_228[10];                                  // +0x228 (row id of each skill/attribute row)
    char pad250[0x2d4 - 0x250];
    int f_2d4;                                      // +0x2d4 (selected row, -1 = none)
    int f_2d8;                                      // +0x2d8 (page: 0 attributes, 1 skills, 2 profs, 3 face)
    char pad2dc[0x434 - 0x2dc];
    UiWidget* f_434;                                // +0x434 (description box)
    char pad438[0x4b8 - 0x438];
    UiWidget* f_4b8;                                // +0x4b8 (title label above the list)

    void update_detail();                           // 0x4ef9f0
};

// 0x4f04f0: the proficiency group name of proficiency idx into buf (another slice)
void prof_group_name(int idx, char* buf);           // 0x4f04f0

// the skill-page hint for a row that cannot be raised
static const char* skill_hint(TroopRec* t, int id)
{
    if (t->skill_rank(id) >= g_skills[id].cap)
        return "ui_at_learning_limit";
    if (t->skill_points < 1)
        return "ui_not_enough_skill_points_to_learn";
    return "ui_not_enough_attributetype_to_learn_this_skill";
}

// the proficiency page's explanation of group idx
static const char* prof_explanation(int idx)
{
    switch (idx) {
    case 0: return "ui_explanation_one_handed_weapon";
    case 1: return "ui_explanation_two_handed_weapon";
    case 2: return "ui_explanation_polearm";
    case 3: return "ui_explanation_archery";
    case 4: return "ui_explanation_crossbow";
    case 5: return "ui_explanation_throwing";
    default: return "ui_explanation_firearms";
    }
}

// the attribute page's explanation of attribute idx
static const char* attrib_explanation(int idx)
{
    switch (idx) {
    case 0: return "ui_explanation_strength";
    case 1: return "ui_explanation_agility";
    case 2: return "ui_explanation_intelligence";
    default: return "ui_explanation_charisma";
    }
}

// @ 0x4ef9f0 -------------------------------------------------------------------------------
void CharSheet::update_detail()
{
    // measure the title label before its text is replaced (the result only feeds the label's layout)
    float sz;
    {
        tstring cur("");
        f_4b8->vt->get_text_size(&cur, &sz);
    }
    f_434->vt->hide();
    int row = f_2d4;
    if (row < 0)
        return;
    char title[0x400];
    char body[0x200];
    char keybuf[0x1e8];
    if (f_2d8 != 1) {
        if (f_2d8 == 2) {                                   // proficiencies page
            prof_group_name(row, keybuf);
            char expl[0x1e8];
            strcpy(expl, str_text(tstring(prof_explanation(row))));
            mb_snprintf(title, 0x400, "%s: %s", keybuf, expl);
            tstring the_title(title);
            f_4b8->vt->get_text_size(&the_title, &sz);
            f_434->vt->hide();
            return;
        }
        if (f_2d8 != 0) {
            if (f_2d8 != 3)                                 // face page
                return;
            strcpy(keybuf, str_text(tstring("ui_edit_face")));
            tstring the_title(keybuf);
            f_4b8->vt->get_text_size(&the_title, &sz);
            f_434->vt->hide();
            return;
        }
        // attributes page
        strcpy(keybuf, str_text(tstring(attrib_explanation(row))));
        tstring the_title(keybuf);
        f_4b8->vt->get_text_size(&the_title, &sz);
        f_434->vt->hide();
        return;
    }
    {                                                       // skills page: one skill row
        int id = f_228[row];
        SkillRec* sk = &g_skills[id];
        tstring* name = g_strtable.find(&sk->key);
        if (!name)
            name = &sk->name;
        tstring s = *name + *sk->translated();
        const char* anchor = 0;
        if (id == 2)
            anchor = g_char_t_level_y;
        else if (id == 1)
            anchor = g_char_t_level_x;
        mb_snprintf(title, 0x400, "%s: %s", s.c_str(), anchor);
        tstring the_title(title);
        f_4b8->vt->get_text_size(&the_title, &sz);
        mb_snprintf(body, 0x200, "%s: %d", str_text(tstring("ui_current_level_value")),
                    g_game->get_prof(f_14, id));
        float xy[2];
        xy[0] = g_config.get_float(tstring("char_t_current_level_x"));
        xy[1] = g_config.get_float(tstring("char_t_current_level_y"));
        tstring desc(body);
        f_4b8->vt->set_text(&the_title, xy);
        TroopRec* t = &g_game->troops[f_14];
        int rank = t->skill_rank(id);
        if (rank < sk->cap && t->skill_points >= 1 && (rank + 1) * 3 <= t->f_58[SKILL_ATTR(id)]) {
            f_434->vt->hide();
            return;
        }
        mb_snprintf(body, 0x200, "%s", str_text(tstring(skill_hint(t, id))));
        tstring hint(body);
        f_434->vt->get_text_size(&hint, &sz);
        f_434->vt->hide();
        return;
    }
}

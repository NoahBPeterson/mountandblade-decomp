// Slice s004f0820: the character-sheet text pass (0x4f0820).  Takes the player's character record
// (g_game(0x5e2930)->chars(+0x13a00)[this+0x14], stride 0x508), formats its level / experience /
// attributes / skill levels / weapon points / health into a 0x200-byte line and posts each line, with
// its configured (x, y) position, on the matching text widget of the sheet.  Every position comes from
// the settings table 0xa44240 (key -> entry index, entries of 0x2c bytes from 0xa4425c, text at +4,
// atof 0x537352).  Widget slots (s004dfd50, s004f04f0): set_text +0xa4, set_position(text, &pos)
// +0xac, show +0xb0, show_selected +0xb4.  The empty-string literal is the shared 0x54a480.
#include <windows.h>
#include <stdio.h>

// rgl string (16 bytes: read cursor, buffer pointer, capacity, length); s00429890 / s004dfd50
struct tstring {
    int pos;
    char* buf;
    int cap;
    int len;
    tstring();                                      // 0x42bbd0
    tstring(const char* s);                         // 0x42bbe0
    ~tstring();                                     // 0x42bc40
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

// translation table object at 0x5e2a18
struct StrTable {
    tstring* lookup(const tstring& key);            // 0x43a1e0 (never null)
    tstring* find(const tstring* key);              // 0x4c6010 (null when absent)
};
extern StrTable g_strtable;                         // 0x5e2a18

// ---- character record (0x508 bytes each, g_game->chars at +0x13a00; cf. s0049f060 / s004b1b20) --------
// Only the fields this panel reads are named; the record is reached through a byte pointer + 0x508 index,
// so the unread tail (0xb4..0x508) does not change the code.
struct CharRec {
    char pad0[0x20];
    char name[0x10];                                // +0x20 (tstring, passed whole to find)
    int flags;                                      // +0x30 (bit 0x10 selects the "player" variant)
    char pad34[0x58 - 0x34];
    int level_at;                                   // +0x58 (index into the 0x5937cc level table)
    char pad5c[0x74 - 0x5c];
    int attrib[2];                                  // +0x74, +0x78 (the sheet's first %s: %d line)
    char pad7c[0x80 - 0x7c];
    float health;                                   // +0x80 (unrounded, for grid_round)
    char pad84[0xa0 - 0x84];
    float weapon_pts;                               // +0xa0 (unrounded)
    int hp;                                         // +0xa4 (health shown as an integer)
    int health_max;                                 // +0xa8 (attribute points)
    int next_level_xp;                              // +0xac (weapon points)
    int xp;                                         // +0xb0 (experience)
};

struct GameData {
    char pad0[0x90];
    int player;                                     // +0x90
    char pad94[0x13a00 - 0x94];
    CharRec* chars;                                 // +0x13a00
};
extern GameData* g_game;                            // 0x5e2930

// settings table at 0xa44240: find() returns an entry index or -1; entries are 0x2c bytes from 0xa4425c
// with the string pointer at +4 (a null pointer there means the field itself holds the text).
struct ConfigTable {
    int find(const tstring& key);                   // 0x4bd5a0 (ret 4)
};
extern ConfigTable g_config;                        // 0xa44240
struct ConfigEntry {
    int f0;                                         // +0
    const char* value;                              // +4
    char pad[0x2c - 8];
};
extern "C" double atof(const char* s);              // 0x537352

extern int g_level_table[];                         // 0x5937cc (char_t_next_level_at_ values)
extern int g_skill_clamp;                           // 0x8ee3ac4 (upper clamp of the skill offset)
int grid_round(const float& f);                     // 0x535a64 (fld f; round)
int mb_sprintf(char* buf, int size, const char* fmt, ...);  // 0x415100 (size-limited formatter)
void mb_vsprintf(char* buf, const char* fmt, ...);          // 0x536f8f (unlimited formatter)

// the inlined "read a configured key as float, 0 when the key is absent" sequence of the original
static float config_float(const char* key)
{
    float v = 0;
    int i = g_config.find(tstring(key));
    if (i >= 0) v = (float)atof(((ConfigEntry*)0xa4425c)[i].value);
    return v;
}

// ---- widgets --------------------------------------------------------------------------------------------
// The three top widgets (name, level_at, xp_bar) read their label flag at +0x181 and pass the word at
// +0xa4 as the text colour.  The four skill widgets have the same slots but pass the word at +0xa8.
struct ColorText {
    char pad0[0xa4];
    void* color;                                                // +0xa4 (colour passed to set_text)
    char pad_a8[0x181 - 0xa8];
    char has_label;                                             // +0x181 (set once the name is captioned)
    virtual void c00(); virtual void c01(); virtual void c02(); virtual void c03(); virtual void c04();
    virtual void c05(); virtual void c06(); virtual void c07(); virtual void c08(); virtual void c09();
    virtual void c10(); virtual void c11(); virtual void c12(); virtual void c13(); virtual void c14();
    virtual void c15(); virtual void c16(); virtual void c17(); virtual void c18(); virtual void c19();
    virtual void c20(); virtual void c21(); virtual void c22(); virtual void c23(); virtual void c24();
    virtual void c25(); virtual void c26(); virtual void c27(); virtual void c28(); virtual void c29();
    virtual void c30(); virtual void c31(); virtual void c32(); virtual void c33(); virtual void c34();
    virtual void c35(); virtual void c36(); virtual void c37(); virtual void c38(); virtual void c39();
    virtual void c40(); virtual void c41(); virtual void c42();
    virtual void set_text(void* colour, const tstring& text);   // +0xa4
    virtual void c44(); virtual void c45();
    virtual void set_position(const tstring& text, float* pos); // +0xac
};

struct SkillRow {
    char pad0[0xa8];
    void* color;                                                // +0xa8 (colour passed to set_text)
    virtual void w00(); virtual void w01(); virtual void w02(); virtual void w03(); virtual void w04();
    virtual void w05(); virtual void w06(); virtual void w07(); virtual void w08(); virtual void w09();
    virtual void w10(); virtual void w11(); virtual void w12(); virtual void w13(); virtual void w14();
    virtual void w15(); virtual void w16(); virtual void w17(); virtual void w18(); virtual void w19();
    virtual void w20(); virtual void w21(); virtual void w22(); virtual void w23(); virtual void w24();
    virtual void w25(); virtual void w26(); virtual void w27(); virtual void w28(); virtual void w29();
    virtual void w30(); virtual void w31(); virtual void w32(); virtual void w33(); virtual void w34();
    virtual void w35(); virtual void w36(); virtual void w37(); virtual void w38(); virtual void w39();
    virtual void w40(); virtual void w41(); virtual void w42();
    virtual void set_text(void* colour, const tstring& text);   // +0xa4
    virtual void w44(); virtual void w45();
    virtual void set_position(const tstring& text, float* pos); // +0xac
    virtual void show();                                        // +0xb0
    virtual void show_selected();                               // +0xb4
};

// ---- the character-sheet panel (only the slots this function touches) -----------------------------------
struct CharSheet {
    char pad0[0x14];
    int player;                                     // +0x14
    char pad18[0x44c - 0x18];
    SkillRow* skill_pos[4];                         // +0x44c (position widgets of the four skill lines)
    SkillRow* skill_val[4];                         // +0x45c (text widgets of the four skill lines)
    char pad46c[0x4b0 - 0x46c];
    ColorText* level_at;                            // +0x4b0 (next-level / level / attribute lines)
    ColorText* xp_bar;                              // +0x4b4 (experience bar)
    char pad4b8[0x4c4 - 0x4b8];
    ColorText* name;                                // +0x4c4
    void update_texts();                            // 0x4f0820
};

// @ 0x4f0820
void CharSheet::update_texts()
{
    int i;
    float y, x;
    CharRec* c = (CharRec*)((char*)g_game->chars + player * 0x508);
    char line[512];

    level_at->set_text(level_at->color, tstring(""));
    xp_bar->set_text(xp_bar->color, tstring(""));

    if (name->has_label == 0) {
        tstring* n = g_strtable.find((const tstring*)c->name);
        if (n == 0) n = (tstring*)c->name;
        name->set_text(name->color, tstring(n->c_str()));
    }

    mb_vsprintf(line, g_strtable.lookup(tstring("ui_level"))->c_str(), c->xp);
    y = config_float("char_t_level_y");
    x = config_float("char_t_level_x");
    xp_bar->set_position(tstring(line), &x);

    mb_vsprintf(line, g_strtable.lookup(tstring("ui_xp"))->c_str(), c->weapon_pts);
    y = config_float("char_t_xp_y");
    x = config_float("char_t_xp_x");
    xp_bar->set_position(tstring(line), &x);

    mb_vsprintf(line, g_strtable.lookup(tstring("ui_next_level_at"))->c_str(), g_level_table[c->xp]);
    y = config_float("char_t_next_level_y");
    x = config_float("char_t_next_level_x");
    xp_bar->set_position(tstring(line), &x);

    if ((c->flags & 0x10) != 0) {
        i = c->attrib[1] & 0xf;
        if (i < 0) i = 0;
        else if (i > g_skill_clamp) i = g_skill_clamp;
        mb_sprintf(line, 0x200, g_strtable.lookup(tstring("ui_health_pyer"))->c_str(),
                   grid_round(c->health));
    } else {
        i = c->attrib[1] & 0xf;
        if (i < 0) i = 0;
        else if (i > g_skill_clamp) i = g_skill_clamp;
        mb_vsprintf(line, g_strtable.lookup(tstring("ui_health"))->c_str(), c->level_at + i * 2 + 0x23);
    }
    y = config_float("char_t_health_y");
    x = config_float("char_t_health_x");
    xp_bar->set_position(tstring(line), &x);

    mb_sprintf(line, 0x200, g_strtable.lookup(tstring("ui_attribute_points"))->c_str(), c->health_max);
    y = config_float("char_t_attribut_points_y");
    x = config_float("char_t_attribut_points_x");
    level_at->set_position(tstring(line), &x);

    mb_sprintf(line, 0x200, g_strtable.lookup(tstring("ui_skillts"))->c_str(), c->hp);
    y = config_float("char_t_skillts_y");
    x = config_float("char_t_skillts_x");
    level_at->set_position(tstring(line), &x);

    mb_sprintf(line, 0x200, g_strtable.lookup(tstring("ui_weapon_points"))->c_str(), c->next_level_xp);
    y = config_float("char_t_weapon_points_y");
    x = config_float("char_t_weapon_points_x");
    level_at->set_position(tstring(line), &x);

    for (i = 0; i < 4; i++) {
        if (i == 0)
            mb_sprintf(line, 0x200, "%s:  %d", g_strtable.lookup(tstring("ui_str"))->c_str(),
                       c->attrib[0]);
        else if (i == 1)
            mb_sprintf(line, 0x200, "%s:  %d", g_strtable.lookup(tstring("ui_agi"))->c_str(),
                       c->attrib[0]);
        else if (i == 2)
            mb_sprintf(line, 0x200, "%s:  %d", g_strtable.lookup(tstring("ui_int"))->c_str(),
                       c->attrib[0]);
        else
            mb_sprintf(line, 0x200, "%s:  %d", g_strtable.lookup(tstring("ui_cha"))->c_str(),
                       c->attrib[0]);
        skill_pos[i]->set_text(skill_pos[i]->color, tstring(line));
        if (c->health_max > 0)
            skill_val[i]->show_selected();
        else
            skill_val[i]->show();
    }
}

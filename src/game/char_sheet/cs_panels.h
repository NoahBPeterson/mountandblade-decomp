#ifndef CS_PANELS_H
#define CS_PANELS_H
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
#include "../tstring.h"

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
    char pad7c[0x9c - 0x7c];
    int xp_val;                                     // +0x9c (the ui_xp line's value)
    float health;                                   // +0xa0 (float, shown truncated by the ui_health_pyer line)
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
int mb_sprintf(char* buf, int size, const char* fmt, ...);  // 0x415100 (size-limited formatter)
void mb_vsprintf(char* buf, const char* fmt, ...);          // 0x536f8f (unlimited formatter)

// the inlined "read a configured key as float, 0 when the key is absent" sequence of the original
static __forceinline float config_float(const tstring& key)
{
    float v = 0;
    int i = g_config.find(key);
    if (i >= 0) v = (float)atof(((ConfigEntry*)0xa4425c)[i].value);
    return v;
}

// ---- widgets --------------------------------------------------------------------------------------------
// One widget type (vtable slots verified at the call sites): +0xa4 post_text(out, text) (ret 8; `out` is an
// uninitialised 16-byte result slot), +0xac set_position(text, &pos) (ret 8), +0xb0 show, +0xb4
// show_selected.  The label flag is the byte at +0x181 (the vptr takes +0).
struct PostSlot { int a, b, c, d; };
struct Widget {
    virtual void s00(); virtual void s01(); virtual void s02(); virtual void s03(); virtual void s04();
    virtual void s05(); virtual void s06(); virtual void s07(); virtual void s08(); virtual void s09();
    virtual void s10(); virtual void s11(); virtual void s12(); virtual void s13(); virtual void s14();
    virtual void s15(); virtual void s16(); virtual void s17(); virtual void s18(); virtual void s19();
    virtual void s20(); virtual void s21(); virtual void s22(); virtual void s23(); virtual void s24();
    virtual void s25(); virtual void s26(); virtual void s27(); virtual void s28(); virtual void s29();
    virtual void s30(); virtual void s31(); virtual void s32(); virtual void s33(); virtual void s34();
    virtual void s35(); virtual void s36(); virtual void s37(); virtual void s38(); virtual void s39();
    virtual void s40();
    virtual void post_text(PostSlot* out, const tstring& text);       // +0xa4
    virtual void s42();
    virtual void set_position(const tstring& text, float* pos);      // +0xac
    virtual void show();                                             // +0xb0
    virtual void show_selected();                                    // +0xb4
    char pad[0x181 - 4];
    char has_label;                                                  // +0x181
};
typedef Widget ColorText;
typedef Widget SkillRow;

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

#endif

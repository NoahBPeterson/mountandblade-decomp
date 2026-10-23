// Slice s004f04f0: the character-sheet weapon-class entries (0x4f04f0: class -> translated "ui_*" caption)
// and the per-class posting of those captions onto the sheet (0x4f06f0). The class names are the same
// seven keys as the agent-side helper 0x43a280 (s0043a280), looked up in the translation table 0x5e2a18.
#include <windows.h>
#include <stdio.h>

#define MB_NOTHROW throw()

// rgl string (16 bytes: read cursor, buffer pointer, capacity, length); s00429890 / s004dfd50
struct tstring {
    int pos;
    char* buf;
    int cap;
    int len;
    tstring(const char* s) MB_NOTHROW;              // 0x42bbe0
    tstring() MB_NOTHROW;                           // 0x42bbd0
    ~tstring() MB_NOTHROW;                          // 0x42bc40
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

// translation table object at 0x5e2a18
struct StrTable {
    tstring* lookup(const tstring& key) MB_NOTHROW; // 0x43a1e0
};
extern StrTable g_strtable;                         // 0x5e2a18

// ---- character record (0x508 bytes each, g_game->chars at +0x13a00; cf. s0049f060 / s004b1b20) --------
struct CharRec {
    char pad0[0x30];
    unsigned __int64 flags;                         // +0x30
    char pad38[0x80 - 0x38];
    float weapon[9];                                // +0x80 (weapon proficiency per class; [7] = +0x9c, [8] = +0xa0)
    int health;                                     // +0xa4
    int health_max;                                 // +0xa8
    int next_level_xp;                              // +0xac
    int xp;                                         // +0xb0
    char padb4[0x508 - 0xb4];
    int weapons_at(int index);                      // 0x4ef6d0 (grid-rounded value of the weapon experience table)
};

struct GameData {
    char pad0[0x90];
    int player;                                     // +0x90
    char pad94[0x13a00 - 0x94];
    CharRec* chars;                                 // +0x13a00
    int skill_level(int troop, int skill);          // 0x437b20 (ret 8)
};
extern GameData* g_game;                            // 0x5e2930

extern int g_weapon_xp[];                           // 0x5938c8[9] (experience needed per weapon class)
extern const char* g_weapon_xp_cats[];              // 0x5e2900[7] (per-class glyph category)

void mb_vsprintf(char* out, const char* fmt, va_list args);   // 0x536f8f (used by the inlined sprintf)

// ---- 0x4f04f0: character-sheet caption of weapon class `type` ------------------------------------------
// (same seven keys as 0x43a280, but a plain function of (type, buffer))
// @ 0x4f04f0
void weapon_class_caption(int type, char* out)
{
    if (type == 0) {
        tstring k("ui_one_handed_weapons");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 1) {
        tstring k("ui_two_handed_weapons");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 2) {
        tstring k("ui_polearms");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 3) {
        tstring k("ui_archery");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 4) {
        tstring k("ui_crossbows");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 5) {
        tstring k("ui_throwing");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 6) {
        tstring k("ui_firearms");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    }
}

// ---- widgets: only what these two functions call ---------------------------------------------------------
// font text object: set_text(out8, text) is vtable slot 41 (+0xa4; out8 = address of 8 uninitialised stack bytes in
// the caller), show / show_selected are slots 44 / 45 (+0xb0 / +0xb4)
struct Widget {
    virtual void w00(); virtual void w01(); virtual void w02(); virtual void w03(); virtual void w04();
    virtual void w05(); virtual void w06(); virtual void w07(); virtual void w08(); virtual void w09();
    virtual void w10(); virtual void w11(); virtual void w12(); virtual void w13(); virtual void w14();
    virtual void w15(); virtual void w16(); virtual void w17(); virtual void w18(); virtual void w19();
    virtual void w20(); virtual void w21(); virtual void w22(); virtual void w23(); virtual void w24();
    virtual void w25(); virtual void w26(); virtual void w27(); virtual void w28(); virtual void w29();
    virtual void w30(); virtual void w31(); virtual void w32(); virtual void w33(); virtual void w34();
    virtual void w35(); virtual void w36(); virtual void w37(); virtual void w38(); virtual void w39();
    virtual void w40();
    virtual void set_text(void* out, const tstring& text);     // slot 41 = +0xa4
    virtual void w42(); virtual void w43();
    virtual void show();                                        // +0xb0
    virtual void show_selected();                               // +0xb4
};

// ---- the character-sheet panel that posts the weapon rows ------------------------------------------------
struct CharSheet {
    char pad0[0x14];
    int player;                                     // +0x14 (index of the character shown; re-read every iteration)
    char pad18[0x478 - 0x18];
    Widget* labels[7];                              // +0x478 (caption line per weapon class; reached as [rows - 0x1c])
    Widget* rows[7];                                // +0x494 (one widget per weapon class)
    void update_weapons();                          // 0x4f06f0
};

// @ 0x4f06f0
void CharSheet::update_weapons()
{
    CharRec* c = &g_game->chars[player];
    int i = 0;
    do {
        char line[512];
        char caption[512];
        weapon_class_caption(i, caption);
        sprintf(line, "%s:  %d", caption, (int)g_game->chars[player].weapon[i]);
        {
            tstring text(line);
            char unset[8];                          // the original passes the address of 8 uninitialised bytes
            labels[i]->set_text(unset, text);
        }
        if (c->next_level_xp > 0
            && (float)g_weapon_xp[g_game->skill_level(player, 0x1b)] >= c->weapon[i]
            && g_weapon_xp_cats[i] != 0
            && c->weapons_at(i) <= c->next_level_xp)
            rows[i]->show_selected();
        else
            rows[i]->show();
        i++;
    } while (i < 7);
}

// Slice s004ef420: the character sheet's "apply" step.  0x4ef420 copies the edited attribute/skill values
// back into the player's troop record, 0x4ef7d0 colours the skill rows by their rank, 0x4ef6d0/0x4ef730 are
// two helpers (attribute learning cost in points, item-type -> event id); 0x4ef730 is plain C code.
// The class is the character-sheet panel handled by 0x4eeb10 (slice s004eeb10) with its edit buffers at
// +0x4b8..+0x52c and the skill rows (widget + skill id) at +0xd8 + i*4; see s004eeb10.
// Names are provisional; offsets are read off this slice.
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "rgl.h"

extern "C" int __cdecl sprintf(char*, const char*, ...);    // 0x536f8f

// ---- a tstring as stored in the record tables (ctor 0x42bbe0) ----------------------------------------
struct tstring {
    int pos;
    char* buf;
    int cap;
    int len;
    tstring();
    tstring(const char* s);                         // 0x42bbe0
    ~tstring();                                     // 0x42bc40
};

// ---- the virtual interface of the widgets seen through their vptr (0xdword slots, see s0048b760) ------
struct WRet { int a, b; WRet(); };                  // 8-byte non-POD result (hidden pointer, never destroyed by callers)
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
    virtual WRet set_text(const tstring& s);        // +0xa4
    virtual void d42(); virtual void d43();
    virtual void hide();                            // +0xb0
    virtual void show();                            // +0xb4
    virtual void d46(); virtual void d47(); virtual void d48(); virtual void d49(); virtual void d50();
    virtual void update_children();                 // +0xc4
    virtual void d53(); virtual void d54();
};

struct UiWidget {
    UiVt* vt;                                       // +0x00
};

struct ItemRec {                                    // 0x238 bytes, table at 0x5e29d0
    char pad0[0x1c8];
    unsigned type;                                  // +0x1c8 (low byte = item type)
    char pad1[0x238 - 0x1cc];
};
extern ItemRec* g_items;                            // 0x5e29d0

// ---- game records ------------------------------------------------------------------------------------
struct TroopRec {
    char pad0[0x58];
    int f_58[4];                                    // +0x58 (weapon proficiencies / attributes)
    int f_68[6];                                    // +0x68 (skill ranks, packed 4-bit)
    int f_80, f_84, f_88, f_8c;                     // +0x80
    int f_90, f_94, f_98;                           // +0x90
    char pad9c[0xa0 - 0x9c];
    int level;                                      // +0xa0
    int skill_points;                               // +0xa4
    int attr_str, attr_agi;                         // +0xa8, +0xac (first two attributes)
    char padb0[0x508 - 0xb0];

    int skill_rank(int idx);                        // 0x4351a0
};

struct Game {
    char pad8c[0x8c];
    int player;                                     // +0x8c (hero troop index)
    char pad90[0x13a00 - 0x90];
    TroopRec* troops;                               // +0x13a00

    int skill_rank(int troop, int idx);             // 0x437b20 (packed 4-bit rank)
};
extern Game* g_game;                                // 0x5e2930

// skill table at 0x8ee3100, stride 0x44: max rank / attribute index at +0x34 / +0x3c / +0x40
#define SKILL_MAX(id)  (*(int*)(0x8ee3134 + (id) * 0x44))
#define SKILL_ATTR(id) (*(int*)(0x8ee3140 + (id) * 0x44))

// ---- panel -------------------------------------------------------------------------------------------
struct SkillRow {
    UiWidget* widget;                               // +0x00
    int skill;                                      // +0x04 (skill id)
};

struct CharSheet {
    int vptr;                                       // +0x00
    int f_4;
    char pad8[0x14 - 0x8];
    int f_14;                                       // +0x14 (edited troop index)
    char pad18[0xd8 - 0x18];
    UiVt* label[0x2a];                          // +0xd8  (rank text per skill row)
    UiVt* btn[0x2a];                            // +0x180 (upgrade button per row)
    int skill_id[0x2a];                             // +0x228 (skill id per row)
    int f_2d0;                                      // +0x2d0 (number of skill rows)
    int f_2d4;
    char pad2d8[0x4c8 - 0x2d8];
    char f_4c8;                                     // +0x4c8 (skills visible)
    char pad4c9[0x4d4 - 0x4c9];
    char f_4d4;                                     // +0x4d4 (skills page)
    char pad4d5[0x4d8 - 0x4d5];
    int f_4d8, f_4dc, f_4e0, f_4e4, f_4e8, f_4ec, f_4f0, f_4f4, f_4f8, f_4fc;
    int f_500;
    int f_504;                                      // +0x504 (level)
    int f_508;                                      // +0x508 (skill points)
    int f_50c;                                      // +0x50c (strength)
    int f_510;                                      // +0x510 (agility)
    int f_514, f_518, f_51c, f_520, f_524, f_528;   // +0x514 (skill values, see s004eeb10)

    void apply_to_troop();                          // 0x4ef420
    void update_skill_rows();                       // 0x4ef7d0
    int  steps_to_point(int idx);                   // 0x4ef6d0
};

// ---- 0x535a64: float -> int (_ftol), emitted by the (int) casts below -------------------------------

// 0x4375b0: distance falloff of one proficiency value
struct TroopExt {
    float falloff(float dist, float k);             // 0x4375b0
};

// @ 0x4ef420 -------------------------------------------------------------------------------
void CharSheet::apply_to_troop()
{
    { TroopRec* t = &g_game->troops[f_14]; t->skill_points = f_508; }
    { TroopRec* t = &g_game->troops[f_14]; t->attr_str = f_50c; }
    { TroopRec* t = &g_game->troops[f_14]; t->attr_agi = f_510; }
    { TroopRec* t = &g_game->troops[f_14]; t->level = f_504; }
    { TroopRec* t = &g_game->troops[f_14]; t->f_58[0] = f_4d8; }
    { TroopRec* t = &g_game->troops[f_14]; t->f_58[1] = f_4dc; }
    { TroopRec* t = &g_game->troops[f_14]; t->f_58[2] = f_4e0; }
    { TroopRec* t = &g_game->troops[f_14]; t->f_58[3] = f_4e4; }
    { TroopRec* t = &g_game->troops[f_14]; t->f_68[0] = f_514; }
    { TroopRec* t = &g_game->troops[f_14]; t->f_68[1] = f_518; }
    { TroopRec* t = &g_game->troops[f_14]; t->f_68[2] = f_51c; }
    { TroopRec* t = &g_game->troops[f_14]; t->f_68[3] = f_520; }
    { TroopRec* t = &g_game->troops[f_14]; t->f_68[4] = f_524; }
    { TroopRec* t = &g_game->troops[f_14]; t->f_68[5] = f_528; }
    { TroopRec* t = &g_game->troops[f_14]; t->f_80 = f_4e8; }
    { TroopRec* t = &g_game->troops[f_14]; t->f_84 = f_4ec; }
    { TroopRec* t = &g_game->troops[f_14]; t->f_88 = f_4f0; }
    { TroopRec* t = &g_game->troops[f_14]; t->f_8c = f_4f4; }
    { TroopRec* t = &g_game->troops[f_14]; t->f_90 = f_4f8; }
    { TroopRec* t = &g_game->troops[f_14]; t->f_94 = f_4fc; }
    { TroopRec* t = &g_game->troops[f_14]; t->f_98 = f_500; }
}

// @ 0x4ef7d0 -------------------------------------------------------------------------------
// One pass over the skill rows: the rank as text, coloured green when it is below the skill's cap,
// red when at the cap; then show the upgrade button only for a spendable row of the player's hero.
void CharSheet::update_skill_rows()
{
    if (f_4c8 != 0) {
        char num[512];
        for (int i = 0; i < f_2d0; i++) {
            int rank;
            int cap;
            int id;
            {
                int troop = f_14;
                id = skill_id[i];
                cap = (int)((unsigned)((int*)g_game->troops)[troop * 0x142 + id / 8 + 0x1a] >> (id % 8 * 4)) & 0xf;
                if (cap < 0)
                    cap = 0;
                else if (cap > SKILL_MAX(id))
                    cap = SKILL_MAX(id);
                rank = g_game->skill_rank(troop, id);
            }
            sprintf(num, "%d", rank);
            {
                unsigned color;
                if (rank <= cap) {
                    color = 0xff990000;
                    if (rank >= cap)
                        color = 0xff000000;
                } else {
                    color = 0xff009900;
                }
                label[i]->set_color(&color);
            }
            {
                tstring str(num);
                label[i]->set_text(str);
            }
            btn[i]->hide();
            {
                int troop = f_14;
                TroopRec* t = &g_game->troops[troop];
                id = skill_id[i];
                if (t->skill_points > 0) {
                    int cur = (int)((unsigned)t->f_68[id / 8] >> (id % 8 * 4)) & 0xf;
                    if (cur < 0)
                        cur = 0;
                    else if (cur > SKILL_MAX(id))
                        cur = SKILL_MAX(id);
                    if (cur < SKILL_MAX(id) &&
                        t->skill_rank(id) < t->f_58[SKILL_ATTR(id) & 0xf] / 3 &&
                        (id != 1 && id != 2 || troop == g_game->player))
                        btn[i]->show();
                }
            }
        }
    }
}

// @ 0x4ef6d0 -------------------------------------------------------------------------------
// Number of falloff steps needed before the proficiency value rounds up one point (max 1000).
int CharSheet::steps_to_point(int idx)
{
    float v = ((float*)this)[0x80 / 4 + idx];
    int r = (int)v;
    int i = 0;
    do {
        v = ((TroopExt*)this)->falloff(v, 1.0f);
        i++;
        int cur = (int)v;
        if (cur > r)
            return i;
    } while (i < 1000);
    return i;
}

// @ 0x4ef730 -------------------------------------------------------------------------------
// Item type -> the (activity, skill) event pair used to learn it; *a = -1 for "not used".
void item_events(int item, int* a, int* b, int* c)
{
    *a = -1;
    *b = -1;
    *c = -1;
    if (item < 0)
        return;
    unsigned t = g_items[item].type & 0xff;
    if (t == 2 || t == 3 || t == 4 || t == 9) {
        *a = 0;
        return;
    }
    if (t == 1) {
        *b = 0x18;
        return;
    }
    if (t == 8) {
        *b = 0x21;
        return;
    }
    if (t == 0xa) {
        *b = 0x22;
        return;
    }
    if (t == 7) {
        *b = 0x1a;
        return;
    }
    if (t == 0xc || t == 0xd || t == 0xe || t == 0xf)
        *a = 0;
}

// Slice s004eeb10: the character sheet's per-frame update (0x4eeb10).  It steps the world, flushes the
// message log, shows/hides the skills page and then handles the clicks on the skill rows, the weapon
// proficiency rows and the four attribute rows (spending skill points), the face page, the toggle buttons and
// the "done"/"cancel" buttons; finally it applies the edits back to the hero record.
// Layout: +0x14 edited troop, +0x228 table of skill-page row ids, +0x2d0 number of skill rows,
// +0x2d4 selected row, +0x2d8 page (0 attributes, 1 skills, 2 proficiencies, 3 face), +0x2dc done state,
// +0x430 skills-page panel, +0x42c buttons, +0x434 description, +0x444/+0x448 previous/next buttons,
// +0x44c attributes' left buttons, +0x45c attributes' right buttons, +0x4b8 title, +0x4bc slider/scroll,
// +0x4c4 hero portrait, +0x4c8 "skills visible", +0x4cc face button, +0x4d4 dirty flag, +0x52c face page
// visible.  See s004ef420 (apply), s004ef9f0 (detail page) and s004ef7d0 (skill rows).
// Names are provisional; offsets are read off this slice.
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "rgl.h"

// the strategy world stepped at the top of the frame (0x427f40)
struct World { void step(float dt); };

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
    tstring& operator=(const tstring& o);           // 0x42bc50
};

// ---- the virtual interface of the widgets seen through their vptr (0xdword slots, see s0048b760) ------
struct UiVt {
    virtual void d00(); virtual void d01(); virtual void d02(); virtual void d03(); virtual void d04();
    virtual void d05(); virtual void d06(); virtual void d07(); virtual void d08(); virtual void d09();
    virtual void d10(); virtual void d11(); virtual void d12(); virtual void d13(); virtual void d14();
    virtual void d15(); virtual void d16();
    virtual char clicked_on(int which);             // +0x44 (2 press, 4 release, 0x10000 click)
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
    virtual void d42(); virtual void d43();
    virtual void hide();                            // +0xb0
    virtual void show();                            // +0xb4
    virtual void enable();                          // +0xb8
    virtual void disable();                         // +0xbc
    virtual void d48();
    virtual void update_children();                 // +0xc4
};

typedef UiVt UiWidget;                              // widget pointers point at the polymorphic object itself

// ---- game records ------------------------------------------------------------------------------------
struct TroopRec {
    tstring name;                                   // +0x0 (record name; assigned from the portrait)
    char pad10[0xa4 - 0x10];
    int skill_points;                               // +0xa4
    int attr_points;                                // +0xa8 (attribute points)
    int prof_points;                                // +0xac (weapon proficiency points)
    char padb0[0x508 - 0xb0];                       // record is 0x508 bytes

    int skill_rank(int idx);                        // 0x4351a0 (packed 4-bit rank)
    void set_skill(int idx, int val);               // 0x48bf90
    void bump_prof(int idx, float amount);          // 0x437770
    void bump_attr(int idx, int amount);            // 0x48bef0
    void auto_equip(int a, int b, int c, int d);    // 0x4687e0 (this = troops + index*0x508)
    int steps_to_point(int idx);                    // 0x4ef6d0 (cost in proficiency points of the next point)
};

struct Game {
    char pad8c[0x8c];
    int player;                                     // +0x8c (hero troop index)
    char pad90[0xb4 - 0x90];
    int f_b4;                                       // +0xb4 (set from the profile's +0x50)
    char padb8[0x13a00 - 0xb8];
    TroopRec* troops;                               // +0x13a00
    int troop_count;                                // +0x13a04

    void step(float dt);                            // 0x427f40
    char sl_query(int kind, int troop, int arg);    // 0x435790 (this = g_game)
    char equip_hero(int troop, int amount);         // 0x4428c0 (this = g_game; extra args unused)
};
extern Game* g_game;                                // 0x5e2930

// skill table at 0x8ee3100, stride 0x44; the max rank of a skill is at +0x34
struct SkillCap { char pad[0x34]; int cap; char pad2[0x44 - 0x38]; };
extern SkillCap g_skill_tbl[];                      // 0x8ee3100

// the character portrait: its face-page visibility check and the name buffer at +0x128
struct Portrait {
    char pad0[0x128];
    tstring name;                                   // +0x128
    char has_face();                                // see +0x181 of the record
    char f181;
};

// the record behind +0x4c4
struct HeroEntry {
    char pad0[0x128];
    tstring name;                                   // +0x128
    char pad138[0x181 - 0x138];
    char f181;                                      // +0x181
};

// the character sheet panel (see s004ef420 / s004ef9f0 for the detail page)
struct CharSheet {
    int vptr;                                       // +0x00
    int f_4;                                        // +0x04
    char pad8[0x14 - 0x8];
    int f_14;                                       // +0x14 (edited troop index)
    int f_18;                                       // +0x18 (sound / message id)
    char pad1c[0x30 - 0x1c];
    UiWidget* f_30[10];                             // +0x30 (skill-row number labels)
    char pad58[0x180 - 0x58];
    UiWidget* f_180[10];                            // +0x180 (skill rows)
    char pad1a8[0x228 - 0x1a8];
    int f_228[10];                                  // +0x228 (skill id of each skill row)
    char pad250[0x2d0 - 0x250];
    int f_2d0;                                      // +0x2d0 (number of skill rows)
    int f_2d4;                                      // +0x2d4 (selected row, -1 = none)
    int f_2d8;                                      // +0x2d8 (page: 0 attributes, 1 skills, 2 profs, 3 face)
    int f_2dc;                                      // +0x2dc (done state: 0 idle, 1 waiting)
    char pad2e0[0x2e4 - 0x2e0];
    int f_2e4;                                      // +0x2e4 (the world stepped each frame)
    char pad2e8[0x42c - 0x2e8];
    UiWidget* f_42c;                                // +0x42c (done/cancel buttons)
    UiWidget* f_430;                                // +0x430 (skills page panel)
    UiWidget* f_434;                                // +0x434 (description box)
    char pad438[0x440 - 0x438];
    UiWidget* f_440;                                // +0x440 (back to the army button)
    UiWidget* f_444;                                // +0x444 (previous troop)
    UiWidget* f_448;                                // +0x448 (next troop)
    UiWidget* f_44c[4];                             // +0x44c (attribute left buttons)
    UiWidget* f_45c[4];                             // +0x45c (attribute right buttons)
    char pad46c[0x478 - 0x46c];
    UiWidget* f_478[7];                             // +0x478 (proficiency number labels)
    UiWidget* f_494[7];                             // +0x494 (proficiency rows)
    char pad4b0[0x4b8 - 0x4b0];
    UiWidget* f_4b8;                                // +0x4b8 (title label)
    UiWidget* f_4bc;                                // +0x4bc (scroll/slider bar)
    char pad4c0[0x4c4 - 0x4c0];
    UiWidget* f_4c4;                                // +0x4c4 (portrait)
    char f_4c8;                                     // +0x4c8 (skills visible)
    char pad4c9[0x4cc - 0x4c9];
    UiWidget* f_4cc;                                // +0x4cc (edit face button)
    char pad4d0[0x4d4 - 0x4d0];
    char f_4d4;                                     // +0x4d4 (dirty flag)
    char pad4d5[0x52c - 0x4d5];
    char f_52c;                                     // +0x52c (face page visible)

    void update(float dt);                          // 0x4eeb10
    void apply_to_troop();                          // 0x4ef420
    void update_skill_rows();                       // 0x4ef7d0
    void update_detail();                           // 0x4ef9f0
    void refresh_panels();                          // 0x4f0820
    void refresh_lists();                           // 0x4f06f0
};
// the skill rows at +0xd8 are the same widgets as +0x480 (the editor keeps them twice)
struct SkillRows { UiWidget* w[5]; };

// ---- world / UI services -----------------------------------------------------------------------------
struct MsgLog { void flush(); };                    // 0x4f1390
extern MsgLog g_msglog;                             // 0x5d7080
struct MainMenu { void choose(int option); };       // 0xa73980 (0x495700)
extern MainMenu g_main_menu;                        // 0xa73980
extern int g_world_time;                            // 0x5d8790 (compared as an int)

// scroll bar / slider (0x48baa0 set_value)
struct UiDsv {
    char pad0[0xf0];
    float stops[0x1e];                              // +0xf0 (the "stops" scratch area, 0xf0..0x168)
    float f_168;                                    // +0x168 (thumb)
    float f_16c;                                    // +0x16c
    float f_170;                                    // +0x170 (track)
    float f_174;                                    // +0x174
    int f_178;                                      // +0x178 (stop index)
    float f_17c;                                    // +0x17c
    void set_value(float v);                        // 0x48baa0
};
struct ScrollHost { char pad[0x484]; UiDsv* bar; }; // +0x484

// the profile record at 0xa73984
struct Profile { char pad[0x50]; int troop; };      // +0x50

extern "C" {
extern Profile* g_profile;                          // 0xa73984
extern int* g_menu_state;                           // 0xa739f4 (menu data, +0x14/+0x18 fields)
extern int* g_army_state;                           // 0xa739e0
extern int g_a170d8;                                // 0xa170d8
extern int g_a17118;                                // 0xa17118
extern int g_a17148;                                // 0xa17148
extern int g_a17188;                                // 0xa17188
extern int g_a17348;                                // 0xa17348
extern int g_a174d8;                                // 0xa174d8
extern int g_a17518;                                // 0xa17518
extern int g_a17588;                                // 0xa17588
extern int g_a5a9fbc;                               // 0x5a9fbc
extern int g_trial_mode;                            // 0x5e29c4
}

// 0x495700 "choose menu option" (tail-called through g_main_menu)
static inline void menu_choose(int option) { g_main_menu.choose(option); }

// @ 0x4eeb10 -------------------------------------------------------------------------------
// The click handling of the rows is `selected |= press on either button` / `released |= release on either`;
// a click on the right button of a row spends the corresponding points.
void CharSheet::update(float dt)
{
    int zero = 0;
    bool selected = false;
    bool released = false;

    if (f_2e4 != 0)
        ((World*)f_2e4)->step(dt);
    g_msglog.flush();
    if (f_4d4 != 0)
        f_430->show();
    else
        f_430->hide();

    if (f_4c8 != 0) {
        if (g_world_time != 0) {
            ScrollHost* host = (ScrollHost*)f_4bc;
            UiDsv* bar = host->bar;
            bar->set_value(((float)g_world_time / bar->stops[bar->f_178]) * bar->f_17c * 0.0005f +
                           (bar->stops[bar->f_178] - bar->f_17c) *
                               ((bar->f_168 - bar->f_16c) / (bar->f_170 - bar->f_16c)));
            ((UiVt*)((ScrollHost*)f_4bc)->bar)->set_key_flags(0x10000);
        }
        // the skill rows: select on press, then learn a point on click
        for (int i = 0; i < f_2d0; i++) {
            if (f_30[i]->clicked_on(2) || f_180[i]->clicked_on(2)) {
                selected = true;
                f_2d4 = i;
                f_2d8 = 1;
                update_detail();
            }
            if (f_30[i]->clicked_on(4) || f_180[i]->clicked_on(4))
                released = true;
            if (f_180[i]->clicked_on(0x10000)) {
                int troop = f_14;
                int id = f_228[i];
                TroopRec* tb = g_game->troops;
                int rank = (int)((unsigned)((int*)tb)[troop * 0x142 + id / 8 + 0x1a] >> (id % 8 * 4)) & 0xf;
                if (rank < 0)
                    rank = 0;
                else if (rank > g_skill_tbl[id].cap)
                    rank = g_skill_tbl[id].cap;
                tb[troop].set_skill(id, rank + 1);
                g_game->troops[f_14].skill_points--;
                f_4d4 = 1;
                update_detail();
                update_skill_rows();
                refresh_panels();
                refresh_lists();
            }
        }
        // the seven proficiency rows: train one point on click, paid with proficiency points
        for (int i = 0; i < 7; i++) {
            if (f_478[i]->clicked_on(2) || f_494[i]->clicked_on(2)) {
                selected = true;
                f_2d4 = i;
                f_2d8 = 2;
                update_detail();
            }
            if (f_478[i]->clicked_on(4) || f_494[i]->clicked_on(4))
                released = true;
            if (f_494[i]->clicked_on(0x10000)) {
                int cost = g_game->troops[f_14].steps_to_point(i);
                g_game->troops[f_14].bump_prof(i, (float)cost);
                g_game->troops[f_14].prof_points -= cost;
                f_4d4 = 1;
                refresh_lists();
                refresh_panels();
            }
        }
    }

    // the four attribute rows
    for (int i = 0; i < 4; i++) {
        if (f_44c[i]->clicked_on(2) || f_45c[i]->clicked_on(2)) {
            selected = true;
            f_2d4 = i;
            f_2d8 = 0;
            update_detail();
        }
        if (f_44c[i]->clicked_on(4) || f_45c[i]->clicked_on(4))
            released = true;
        if (f_45c[i]->clicked_on(0x10000)) {
            g_game->troops[f_14].bump_attr(i, 1);
            g_game->troops[f_14].attr_points--;
            if (i == 1)
                g_game->troops[f_14].prof_points += 5;
            f_4d4 = 1;
            refresh_panels();
            update_detail();
            update_skill_rows();
            refresh_lists();
        }
    }

    // the face page
    if (f_52c != 0 && f_4cc != 0 && f_4cc->clicked_on(2)) {
        selected = true;
        f_2d4 = 0;
        f_2d8 = 3;
        update_detail();
    }
    if ((f_52c != 0 && f_4cc != 0 && f_4cc->clicked_on(4) || released) && !selected) {
        f_2d4 = -1;
        update_detail();
    }
    if (f_14 == 0 && f_4c4->clicked_on(0x10000)) {
        g_game->troops[0].name = tstring(((HeroEntry*)f_4c4)->name);
        g_game->sl_query(0x18, 0, 0);
    }

    // previous / next troop
    int step = 0;
    if (f_444 != 0 && f_444->clicked_on(0x10000))
        step = 1;
    else if (f_448 != 0 && f_448->clicked_on(0x10000))
        step = -1;

    if (f_2dc == 0) {
        if (g_profile->troop != -1) {
            g_game->f_b4 = g_profile->troop;
            f_2dc++;
            menu_choose(0xf);
            return;
        }
    } else if (f_2dc == 1) {
        TroopRec* hero = g_game->troops;
        if (hero->attr_points > 0 || hero->skill_points > 0 || hero->prof_points > 5 ||
            ((HeroEntry*)f_4c4)->f181 != 0)
            f_42c->enable();
        else
            f_42c->disable();
        if (f_42c->clicked_on(0x10000)) {
            f_2dc = -1;
            g_game->troops[g_game->player].auto_equip(-1, 0, 0, 0);
            g_menu_state[5] = f_14;
            menu_choose(0x1d);
            return;
        }
        if (f_430->clicked_on(0x10000)) {
            apply_to_troop();
            f_4d4 = 0;
            update_detail();
            update_skill_rows();
            refresh_panels();
            refresh_lists();
            return;
        }
    } else {
        if (step != 0) {
            f_14 += step;
            f_14 = (f_14 + g_game->troop_count) % g_game->troop_count;
            f_4 = 1;
            menu_choose(0xf);
            return;
        }
        if (f_52c != 0 && f_4cc != 0 && f_4cc->clicked_on(0x10000)) {
            g_menu_state[5] = f_14;
            g_menu_state[6] = 0xf;
            menu_choose(0x1d);
            return;
        }
        if (f_440 != 0 && f_440->clicked_on(0x10000)) {
            g_army_state[5] = f_14;
            g_army_state[6] = 0xf;
            menu_choose(0x18);
            return;
        }
        if (f_430->clicked_on(0x10000)) {
            apply_to_troop();
            f_4d4 = 0;
            update_detail();
            update_skill_rows();
            refresh_panels();
            refresh_lists();
            return;
        }
        if (f_42c->clicked_on(0x10000)) {
            menu_choose(f_18);
            return;
        }
        // game keys: rest / export / learning
        if (g_a170d8 == 0 && g_a174d8 != 0) {
            menu_choose(f_18);
        } else {
            if (g_a17148 == 0 && g_a17348 == 0)
                return;
            if (g_a17188 != 0 && g_a17588 == 0 && g_a5a9fbc != 0) {
                if (g_trial_mode != 0) {
                    g_game->equip_hero(f_14, 1000);
                    menu_choose(0xf);
                }
                return;
            }
            if ((g_a17148 != 0 || g_a17348 != 0) && g_a17118 != 0 && g_a17518 == 0 && g_a5a9fbc != 0) {
                for (int i = 0; i < 7; i++)
                    g_game->troops->bump_prof(i, 50.0f);
                menu_choose(0xf);
                return;
            }
        }
    }
}

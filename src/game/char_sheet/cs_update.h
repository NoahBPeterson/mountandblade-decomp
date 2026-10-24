#ifndef CS_UPDATE_H
#define CS_UPDATE_H
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
#include "../tstring.h"

// ---- the virtual interface of the widgets seen through their vptr (0xdword slots, see s0048b760) ------
#include "../ui_widget.h"                 // Widget (the UI widget interface)
typedef Widget UiVt;                    // the slice's name for it
typedef UiVt UiWidget;                              // widget pointers point at the polymorphic object itself

// ---- game records ------------------------------------------------------------------------------------
#include "../../include/troop.h"

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
#include "../skill_cap.h"
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
#include "../char_sheet_panel.h"             // CharSheet (the character sheet panel)
// the skill rows at +0xd8 are the same widgets as +0x480 (the editor keeps them twice)
struct SkillRows { UiWidget* w[5]; };

// ---- world / UI services -----------------------------------------------------------------------------
#include "../../include/msg_log.h"
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

#endif

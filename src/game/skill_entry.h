// skill_entry.h: one record of the skill attribute table at 0x8ee3110 (42 records of 0x44 bytes, ending at 0x8ee3c38). One
// definition for options_panel/options_panel.cpp (the skill names, tstring first) and inventory_panel/char_window.cpp
// (char_window::fill_skills 0x509730 walks the table from 0x8ee3140 to 0x8ee3c68, i.e. the word at +0x30 of every record).
#ifndef GAME_SKILL_ENTRY_H
#define GAME_SKILL_ENTRY_H
#include "tstring.h"
struct skill_entry {
    tstring name;                                                   // +0
    char pad10[0x30 - 0x10];
    unsigned flags;                                                 // +0x30 (bit 8: hidden; 0x509730)
    char pad34[0x44 - 0x34];
};
extern skill_entry g_skills[];                                      // 0x8ee3110  // old name of g_skill_tbl, removed after the names merge
#endif

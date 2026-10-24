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
#include "../str_table.h"
extern StrTable g_strtable;                         // 0x5e2a18

// ---- the character record (0x508 bytes each, g_game->chars at +0x13a00; cf. s0049f060 / s004b1b20) is the troop record,
// src/include/troop.h (the slice called it CharRec): +0x20 key, +0x30 flags32, +0x58 attrs / lvl58, +0x78 nib[3], +0x9c exp,
// +0xa0 hp, +0xa4 skill_points, +0xa8 attr_points, +0xac prof_points, +0xb0 level.
#include "../../include/troop.h"

#include "../../include/game_data.h"

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
#include "../ui_widget.h"                 // Widget (the UI widget interface)
struct SheetWidget : Widget {
    char pad[0x181 - 4];
    char has_label;                                                  // +0x181
};
typedef SheetWidget ColorText;
typedef SheetWidget SkillLine;

// ---- the character-sheet panel (only the slots this function touches) -----------------------------------
#include "../char_sheet_panel.h"             // CharSheet (the character sheet panel)

#endif

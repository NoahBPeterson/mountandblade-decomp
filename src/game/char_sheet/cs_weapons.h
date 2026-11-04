#ifndef CS_WEAPONS_H
#define CS_WEAPONS_H
// Slice s004f04f0: the character-sheet weapon-class entries (0x4f04f0: class -> translated "ui_*" caption)
// and the per-class posting of those captions onto the sheet (0x4f06f0). The class names are the same
// seven keys as the agent-side helper 0x43a280 (s0043a280), looked up in the translation table 0x5e2a18.
#include <windows.h>
#include <stdio.h>

#define MB_NOTHROW throw()

// rgl string (16 bytes: read cursor, buffer pointer, capacity, length); s00429890 / s004dfd50
#include "../tstring.h"

// translation table object at 0x5e2a18
#include "../str_table.h"

// ---- the character record (0x508 bytes each, g_game->chars at +0x13a00; cf. s0049f060 / s004b1b20) is the troop record,
// src/include/troop.h (the slice called it CharRec): +0x30 flags30, +0x80 f80[] per-class weapon proficiency, +0xa4 skill_points,
// +0xa8 attr_points, +0xac prof_points, +0xb0 level; weapons_at is Troop::steps_to_point (0x4ef6d0).
#include "../../include/troop.h"

#include "../../include/game_data.h"

extern const int g_weapon_xp[11];                   // 0x5938c8 (experience needed per weapon class)


// ---- 0x4f04f0: character-sheet caption of weapon class `type` ------------------------------------------
// (same seven keys as 0x43a280, but a plain function of (type, buffer))
#endif

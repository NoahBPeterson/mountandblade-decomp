#ifndef CS_TROOP_H
#define CS_TROOP_H
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
#include "../tstring.h"

// ---- the virtual interface of the widgets seen through their vptr (0xdword slots, see s0048b760) ------
#include "../ui_widget.h"                 // Widget (the UI widget interface)
typedef Widget UiVt;                    // the slice's name for it

// the item kind table (0x5e29d0, 0x238-byte records): include/item_kind.h
#include "../../include/item_kind.h"

// ---- game records ------------------------------------------------------------------------------------
#include "../../include/troop.h"

#include "../../include/game_data.h"            // GameData (g_game, 0x5e2930): the one definition (was a Game view here)

// skill table at 0x8ee3100, stride 0x44: max rank / attribute index at +0x34 / +0x3c / +0x40
extern char g_skill_tbl[];                      // 0x8ee3100 (a literal address would miss the relinked copy: the data moved)
#define SKILL_MAX(id)  (*(int*)(g_skill_tbl + 0x34 + (id) * 0x44))
#define SKILL_ATTR(id) (*(int*)(g_skill_tbl + 0x40 + (id) * 0x44))

// ---- panel -------------------------------------------------------------------------------------------
#include "../char_sheet_panel.h"             // CharSheet (the character sheet panel)

// ---- 0x535a64: float -> int (_ftol), emitted by the (int) casts below -------------------------------


#endif

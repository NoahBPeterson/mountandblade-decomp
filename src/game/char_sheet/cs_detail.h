#ifndef CS_DETAIL_H
#define CS_DETAIL_H
// Slice s004ef9f0: the character panel's detail page (0x4ef9f0).  The panel keeps the row ids of the
// skill/attribute pages at +0x228, the page number in +0x2d8 (0 = attributes, 1 = skills, 2 = proficiencies,
// 3 = face), the selected row in +0x2d4, the title label above the list at +0x4b8 and the description box
// under it at +0x434.  The handler fills the title and the description with text tailored to that row:
// skills get "name: description" plus their level and a learn/spend hint, proficiencies get their group
// name and explanation, attributes their name, base value and explanation; the face page only a title.
// See s004eeb10 for the rest of the panel.  Names are provisional; offsets are read off this slice.
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <string.h>
#include "rgl.h"

extern "C" void __cdecl mb_snprintf(char*, unsigned, const char*, ...);    // 0x415100

// ---- a tstring as stored in the record tables (ctor 0x42bbe0) ----------------------------------------
#include "../tstring.h"

#include "../str_table.h"
extern StrTable g_strtable;                         // 0x5e2a18

// translated text of a key (lookup, falling back to the empty string stored in place)
static const char* str_text(const tstring& key)
{
    tstring* r = g_strtable.lookup(key);
    return r->buf ? r->buf : (const char*)&r->buf;
}

// ---- the virtual interface of the widgets seen through their vptr (see s0048b760) ----------------------
// set_text (+0xa4) returns an 8-byte non-POD pair through a hidden pointer that no caller destroys.
#include "../../include/vec.h"
#include "../ui_widget.h"                 // Widget (the UI widget interface)
typedef Widget UiWidget;                // the slice's name for it

// ---- game records ------------------------------------------------------------------------------------
#include "../../include/troop.h"

#include "../../include/game_data.h"            // GameData (g_game, 0x5e2930): the one definition (was a Game view here)

// skill table at 0x8ee3100, stride 0x44
#include "../../include/skill_rec.h"            // SkillRec (shared with src/rgl/mesh2/gui_edit.h)
extern SkillRec g_skill_tbl[];                         // 0x8ee3100
#define SKILL_ATTR(id) (g_skill_tbl[id].attr & 0xf)

// the ini table at 0xa44240 (0x2c-byte entries, see s004bd170)
#include "../ini_file.h"
extern ini_file g_config;                           // 0xa44240

// the character texture's text anchors for the level values; the original loads the pointer stored
// in each global and prints it as a "%s" argument
extern const char* g_skill_leadership_bonus;                // 0xa18120
extern const char* g_skill_prisoner_management_bonus;                // 0xa18124

// the character sheet panel (full layout in s004eeb10)
#include "../char_sheet_panel.h"             // CharSheet (the character sheet panel)

// 0x4f04f0: the proficiency group name of proficiency idx into buf (another slice)
void weapon_class_caption(int idx, char* buf);           // 0x4f04f0
void weapon_class_caption(int cls, char* out); // 0x4f04f0

// translated text of a key (lookup, falling back to the key's own storage when the buffer is null)
static inline const char* tr(const tstring& key)
{
    tstring* r = g_strtable.lookup(key);
    return r->buf ? r->buf : (const char*)&r->buf;
}

#endif

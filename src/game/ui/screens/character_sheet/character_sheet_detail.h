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
#include "engine/math/rgl_math.h"


// ---- a tstring as stored in the record tables (ctor 0x42bbe0) ----------------------------------------
#include "engine/core/tstring.h"

#include "game/data/str_table.h"

// translated text of a key (lookup, falling back to the empty string stored in place)
static const char* str_text(const tstring& key)
{
    tstring* r = g_strtable.lookup(key);
    return r->buf ? r->buf : (const char*)&r->buf;
}

// ---- the virtual interface of the widgets seen through their vptr (see s0048b760) ----------------------
// set_text (+0xa4) returns an 8-byte non-POD pair through a hidden pointer that no caller destroys.
#include "engine/math/vec.h"
#include "game/ui/widgets/ui_widget.h"                 // MissionUi (the UI widget interface)

// ---- game records ------------------------------------------------------------------------------------
#include "game/data/troop.h"

#include "game/data/game_data.h"            // GameData (g_game, 0x5e2930): the one definition (was a Game view here)

// skill table at 0x8ee3100, stride 0x44
#include "game/data/skill_rec.h"            // SkillRec (shared with src/game/editor/editor_selection_and_translation.h)
#define SKILL_ATTR(id) (g_skill_tbl[id].attr & 0xf)

// the ini table at 0xa44240 (0x2c-byte entries, see s004bd170)
#include "game/config/ini_file.h"

// the character texture's text anchors for the level values; the original loads the pointer stored
// in each global and prints it as a "%s" argument

// the character sheet panel (full layout in s004eeb10)
#include "game/ui/screens/character_sheet/char_sheet_panel.h"             // CharSheet (the character sheet panel)
#include "game/data/module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in game/app/options_dialog_and_module_tables.cpp)
#include "game/data/skill_cap.h"
#include "engine/io/text_stream.h"

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

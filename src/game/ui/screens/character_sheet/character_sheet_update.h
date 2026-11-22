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
#include "engine/math/rgl_math.h"
#include "game/ui/screens/character_sheet/cs_common.h"

// the strategy world stepped at the top of the frame (0x427f40)
#include "engine/scene/world.h"                      // rgl::world: the 0x344-byte scene (step 0x427f40, reset 0x421390, create_entity 0x428440)

// ---- a tstring as stored in the record tables (ctor 0x42bbe0) ----------------------------------------
#include "engine/core/tstring.h"

// ---- the virtual interface of the widgets seen through their vptr (0xdword slots, see s0048b760) ------
#include "game/ui/widgets/ui_widget.h"                 // MissionUi (the UI widget interface)
#include "game/ui/widgets/text_editor_widget.h"         // UiE: the sheet's name box (its text, its selection flag)
#include "game/input/key_names.h"

// ---- game records ------------------------------------------------------------------------------------
#include "game/data/troop.h"

#include "game/data/game_data.h"            // GameData (g_game, 0x5e2930): the one definition (was a Game view here)

// skill table at 0x8ee3100, stride 0x44; the max rank of a skill is at +0x34
#include "game/data/skill_cap.h"

// the character sheet panel (see s004ef420 / s004ef9f0 for the detail page)
#include "game/ui/screens/character_sheet/char_sheet_panel.h"             // CharSheet (the character sheet panel)
// the skill rows at +0xd8 are the same widgets as +0x480 (the editor keeps them twice)
struct SkillRows { MissionUi* w[5]; };

// ---- world / UI services -----------------------------------------------------------------------------
#include "game/ui/gui_manager.h"
#include "game/ui/window_mgr.h"                         // g_windows (0xa73980)
#include "game/input/key_names.h"      // g_keyboard (0xa170c8)
#include "game/ui/ui_globals.h"      // UI / core globals
#include "game/mission/trial_mode.h"

#include "game/mission/camera_class.h"   // Camera: the skills list (+0x4bc); its scroll bar is follow[1] (a UiD, set_value 0x48baa0)

// the profile record at 0xa73984
struct Profile { char pad[0x50]; int troop; };      // +0x50


// 0x495700 "choose menu option" (tail-called through g_main_menu)
static inline void menu_choose(int option) { g_windows.set_state(option); }

#endif

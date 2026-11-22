// inventory_window.h: the party screen's inventory window (code 0x501a20..0x502cc0; the class is the window object that
// owns the equipment menus, the inventory pages and the armour / encumbrance labels). Names provisional.
//
// The class is declared in party_window_class.h (a head_rec: the layout comment is there); this header adds the Game object and the
// widgets the member functions build.
#ifndef GAME_PARTY_PARTY_WINDOW_H
#define GAME_PARTY_PARTY_WINDOW_H
#include "platform/image_addrs.h"
#include "engine/core/tstring.h"
#include "game/common/game_globals.h"      // canonical g_disable_food_slot
#include "engine/math/rgl_math.h"
#include "game/data/item_ref_game_ui.h"
#include "game/data/troop_inventory.h"
#include "game/ui/widgets/ui_widgets.h"
#include "game/ui/widgets/container.h"      // ContainerBase
#include "game/mission/camera_class.h"   // Camera: the scrolling item lists
#include "game/campaign/game_state_load.h"
#include "game/campaign/externs.h"

// ---- objects of other translation units ----------------------------------------------------------------
#include "game/config/ini_file.h"                   // ini_file, g_game_vars (0xa44240)

#include "engine/resource/res_mgr.h"                    // g_resmgr: the resource manager 0x5d6fa0 (get_object 0x419540)
#include "game/data/tableau.h"           // TableauTbl g_tableaus 0xa75f40
namespace rgl { struct mesh; }
rgl::mesh* __cdecl make_item_mesh(const ItemRef* ref, const float* scale);   // 0x4fba40 (ret 0): the mesh of an item reference (the definition, game/ui/hud/agent_hud.cpp, returns rgl::mesh*)

void __cdecl label_refresh(void* w, tstring name);          // 0x4e4c70 (cdecl, the name by value)


// ---- widgets: PartyPanel / PartySlot / StripIcon are declared in party_window_class.h ----
#include "game/ui/widgets/ui_frame.h"                  // UiFrame (vtable 0x5924a0): the one definition
// 0x41f4b0 is std::vector<T>::_Tidy (entity.cpp: the out-of-line release shared by every trivially destructible element vector); only the member is declared
struct mface;
template <class T> struct vec { void clear(); };

#include "game/campaign/party_window_class.h"      // PartyWindow: the window record (a head_rec)

#endif // GAME_PARTY_PARTY_WINDOW_H

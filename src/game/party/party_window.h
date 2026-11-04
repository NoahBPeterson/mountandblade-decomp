// party_window.h: the party screen's inventory window (code 0x501a20..0x502cc0; the class is the window object that
// owns the equipment menus, the inventory pages and the armour / encumbrance labels). Names provisional.
//
// The class is declared in party_window_class.h (a head_rec: the layout comment is there); this header adds the Game object and the
// widgets the member functions build.
#ifndef GAME_PARTY_PARTY_WINDOW_H
#define GAME_PARTY_PARTY_WINDOW_H
#include "../../include/image_addrs.h"
#include "../tstring.h"
#include "../game_globals.h"      // canonical g_disable_food_slot
#include "../../rgl/rgl_math.h"
#include "../ui/item_ref.h"
#include "../ui/troop_inv.h"
#include "../ui/ui_widgets.h"
#include "../ui/container.h"      // ContainerBase
#include "../party2/camera_class.h"   // Camera: the scrolling item lists
#include "./game.h"
#include "./externs.h"

// ---- objects of other translation units ----------------------------------------------------------------
#include "../ini_file.h"                   // ini_file, g_game_vars (0xa44240)

#include "../res_mgr.h"                    // g_resmgr: the resource manager 0x5d6fa0 (get_object 0x419540)
#include "../world_data/tableau.h"           // TableauTbl g_tableaus 0xa75f40
namespace rgl { struct mesh; }
rgl::mesh* __cdecl make_item_mesh(const ItemRef* ref, const float* scale);   // 0x4fba40 (ret 0): the mesh of an item reference (the definition, agent5/agent_hud.cpp, returns rgl::mesh*)

void __cdecl label_refresh(void* w, tstring name);          // 0x4e4c70 (cdecl, the name by value)
#include "../item_sound.h"               // ItemSound (ItemKind +0x228): the one definition
inline float item_f208(ItemKind* k) { return *(float*)((char*)k + 0x208); }       // +0x208 base armour value   0x502620
inline ItemSound* item_sound(ItemKind* k) { return (ItemSound*)((char*)k + 0x228); }   // 0x502b80

extern int g_equip_page;                    // 0xa75f68 (negative: nothing selected)  0x5024e0

// ---- widgets: PartyPanel / PartySlot / StripIcon are declared in party_window_class.h ----
#include "../ui/ui_frame.h"                  // UiFrame (vtable 0x5924a0): the one definition
// the widget base's destructor body (0x487c00) and the scalar deleting destructor built on it (0x502300, the shared
// slot 0 of vtable 0x591c80; mission_ui.h declares that slot as d00(int) without a body)
// 0x487c00 is MissionUi::~MissionUi (mesh_slot/slot.cpp, virtual; called here as the base destructor, so qualified)
struct WidgetDtor {
    void base_dtor() { ((MissionUi*)this)->MissionUi::~MissionUi(); }      // 0x487c00
};
// 0x41f4b0 is vec<mface>::clear (rgl/shapes/shapes.h: the vector's out-of-line clear, shared by every pointer vector); only the member is declared
struct mface;
template <class T> struct vec { void clear(); };

#include "./party_window_class.h"      // PartyWindow: the window record (a head_rec)

#endif // GAME_PARTY_PARTY_WINDOW_H

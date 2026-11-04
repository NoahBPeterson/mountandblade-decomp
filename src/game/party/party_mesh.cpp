//-----------------------------------------------------------------------------
// File: party_mesh.cpp  (0x502b80)
//
// Desc: the party window's item mesh maker: PartyWindow::make_mesh builds the display mesh of an item (the second argument is
//       not used) and tells the scene and the script about the item. It is its own object because its callers pass it a
//       pointer to a local that it never reads: defined in the callers' translation unit, cl would see that and drop the
//       store of the local (party_window.cpp place_slot, inventory_panel.cpp transfer_slot keep it, as the original does).
//       Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "../../include/image_addrs.h"
#include "./party_window.h"
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object
#include "../game_globals.h"      // canonical declarations (listed directly: the resolution audit reads only the source's own includes)
#include "../item_sound.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/item_kind.h"   // g_items 0x5e29d0

// @ 0x502b80
rgl::mesh* PartyWindow::make_mesh(ItemRef* item, void* unused, int t)
{
    rgl::mesh* m = 0;
    if (item->item >= 0) {
        float s = 0.088f;
        m = make_item_mesh(item, &s);
        g_game_G->f1a1f4 = -1;
        g_game_G->f1a1f8 = t;
        g_game_G->f1a204 = -10000;
        g_game_G->f1a200 = (int)m;
        g_game_G->f1a1fc = 0;
        g_script.param1_lo = -1;
        g_script.param1_hi = -1;
        g_script.param2 = t;
        item_sound(&g_items[item->item])->play(-0x32);
    }
    return m;
}

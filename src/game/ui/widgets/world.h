// world.h: the small records next to the GameData members of widget_base.cpp (0x489c30..0x48a3d0). The layouts come from
// those functions (cited). Names provisional.
#ifndef GAME_UI_WORLD_H
#define GAME_UI_WORLD_H
#include <new>
#include "engine/core/tstring.h"
#include "game/ui/widgets/ui_common.h"
#include "game/data/item_ref_game_ui.h"
#include "game/data/troop_inventory.h"
#include "game/data/game_data.h"   // GameData (formerly World here)

#include "game/data/game_recs.h"   // QuestRec: the 0x138-byte record at game+0x13a58 (this header called it PartyRec; the bit-field views of +0x50 / +0x64 are in the shared class)

// The world object is the canonical GameData (../../game/data/game_data.h; global g_game 0x5e2930). Proof: the same members at the same
// offsets (+0x78 default party, +0x8c player troop, +0xc8, +0x13a00 troop table, +0x13a58 quest table, +0x13b44 ...) and the same
// member function 0x48a150 (GameData::template_count; the slices called it sl_cell_coord). The ui members of it are defined in widget_base.cpp.

#include "game/data/game_recs.h"   // PartyTemplate* g_party_templates (0x5e29f8)

#include "game/ui/screens/result_window.h"

struct Named16 {                            // 16-byte name + flag (0x489ab0)
    tstring name;                           // +0x00
    char flag10;                            // +0x10
    void set(const tstring* s);             // 0x489ab0
};


#endif // GAME_UI_WORLD_H

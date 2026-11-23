// troop_inventory.h: classes of troop_inventory.cpp (0x462ac0..0x4636d0): the troop record's inventory (90 slots of
// (item, modifier) pairs plus ten equipment slots), the placed-object stream reader and the 0x5374-byte view
// state. Names provisional; offsets cite the function that reads or writes them.
#ifndef GAME_UI_TROOP_INV_H
#define GAME_UI_TROOP_INV_H
#include <stddef.h>
#include <new>             // declares operator new[]/delete[]: without it cl calls operator new/delete for POD arrays
#include "engine/math/rgl_math.h"
#include "engine/io/stream.h"
#include "engine/core/tstring.h"
#include "game/ui/widgets/ui_common.h"
#include "game/data/item_ref_game_ui.h"

// The troop record: a 0x508-byte entry of the table behind GameData+0x13a00 (Troop in src/game/data/troop.h, one definition for
// src/game and src/engine). The slices called it `Agent` (0x462b60..0x462da0) and `Inv` (0x462e20..0x463100): both are this record.
#include "game/data/troop.h"

// A 0x78-byte record of the sprite table (g_propkinds 0x5e29dc, count 0x5e29e0): src/game/mission/mission_obj.h (MissionObj)
#include "game/mission/mission_obj.h"      // MissionObj: the scene prop kind (g_propkinds)
extern int g_propkind_count;                   // 0x5e29e0
int find_in_table78(const tstring* name);   // 0x4352a0 (cdecl)

stream_base& r_rotation(stream_base& s, rgl::matrix* m);    // 0x415e70 (cdecl)

// The placed object read from a stream (reader 0x4631c0): PlacedObj of src/game/mission/placed_obj.h
#include "game/mission/placed_obj.h"

#include "game/agent/ai/agent_ai_game.h"              // AgentAi: init 0x463370 is defined in troop_inventory.cpp

#endif // GAME_UI_TROOP_INV_H

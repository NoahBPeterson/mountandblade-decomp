// troop_inventory_stats.h: classes of troop_inventory_stats.cpp (0x48be00..0x48c330): the placeholder "Event" object. The inventory
// compaction, the attribute / skill setters and the inventory sort are members of ItemRef / Troop (src/game/data/item_ref.h,
// src/game/data/troop.h). Names provisional; offsets cite the function that reads them.
#ifndef GAME_MISSION_UI2_STAT_PANEL_X_H
#define GAME_MISSION_UI2_STAT_PANEL_X_H
#include "engine/core/tstring.h"
#include "game/common/iface.h"   // IfaceB: the record base (vtable 0x5908f8)

// highest skill rank / tier, read by Troop::set_attr and Troop::set_skill (bound through symbols/slices/troop_inventory_stats.txt: outside the image)
#include "game/data/skill_cap.h"

// the event record (constructor 0x48c210, slot 1 0x48c260): src/game/campaign/event.h, ONE definition with the map event pool (engine/core/pool.h)
#include "game/campaign/event.h"

#endif // GAME_MISSION_UI2_STAT_PANEL_X_H

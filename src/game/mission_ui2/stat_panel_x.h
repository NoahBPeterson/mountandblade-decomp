// stat_panel_x.h: classes of stat_panel_x.cpp (0x48be00..0x48c330): the placeholder "Event" object. The inventory
// compaction, the attribute / skill setters and the inventory sort are members of ItemRef / Troop (src/game/item_ref.h,
// src/include/troop.h). Names provisional; offsets cite the function that reads them.
#ifndef GAME_MISSION_UI2_STAT_PANEL_X_H
#define GAME_MISSION_UI2_STAT_PANEL_X_H
#include "../tstring.h"
#include "../../include/iface.h"   // IfaceB: the record base (vtable 0x5908f8; Event is the same family as RecF)

// highest skill rank / tier, read by Troop::set_attr and Troop::set_skill (bound through symbols/slices/stat_panel_x.txt: outside the image)
#include "../skill_cap.h"

// the event record (constructor 0x48c210, slot 1 0x48c260): src/game/event.h, ONE definition with the map event pool (include/pool.h)
#include "../event.h"

#endif // GAME_MISSION_UI2_STAT_PANEL_X_H

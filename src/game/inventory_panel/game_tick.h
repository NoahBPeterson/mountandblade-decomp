// game_tick.h: the campaign map's frame tick and its helpers (code 0x516ac0..0x5191e0): the game data object (GameData,
// src/include/game_data.h; the slices called it Game and GameData) with the methods of this region, the paged pool of
// map events, the small records the tick walks. The methods are members of GameData (declared in game_data.h). Names provisional.
#ifndef GAME_INVENTORY_PANEL_GAME_TICK_H
#define GAME_INVENTORY_PANEL_GAME_TICK_H
#include "./inv_common.h"
#include <vector>                          // (the marker lists are std::vector<rgl::mesh*> members of g_script)

// the map event pool at game+0xcc is GameData::events (Pool<Event>, include/pool.h); its cursor 0x517960 / 0x5179b0 is
// EventPool::next / first (../event_pool.h, defined in game_tick.cpp); the records are game/event.h's Event
#include "../event_pool.h"

// the party-track ribbons (GameData::tracks): src/game/track_rec.h
#include "../track_rec.h"
struct ItemIter {                       // party2/party_2.h's result of ItemArray::at (a constructor makes it a hidden-return class)
    TrackRec* p;
    ItemIter(TrackRec* q) : p(q) {}
};
struct ItemArray {                      // the iterator helper on a bare base pointer (0x4b22b0, party2/party_2.h)
    TrackRec* base;
    ItemIter at(int idx);               // 0x4b22b0 (ret 8, hidden return pointer)
};


// the objects at game+0x139e8 (snd_bank::update_all 0x462760, ui/item_ref.h) and game+0x139f4 (SimpleTriggerVec::update 0x48add0,
// game/trigger_vecs.h) are called through those classes
#include "../ui/item_ref.h"
#include "../trigger_vecs.h"   // SimpleTriggerVec: the simple-trigger list

inline Party* GameData::player_party()
{
    PartyMgr* mgr = &parties;
    int i = mgr->lookup(main_party);
    return mgr->pages[i >> 4] + (i & 0xf);
}

// the map scene object (global 0xa732b4): a colour at +0x68 (its w lane is the alpha at +0x74) and the per-frame step
#include "../../rgl/scene/world.h"                      // rgl::world: the 0x344-byte scene (step 0x427f40, reset 0x421390, create_entity 0x428440)
#include "../../include/level_global.h"      // g_level (0xa732b4)
#include "../light.h"                                    // Light: GameData::wind, the map sun (update_colors 0x517350)

#endif // GAME_INVENTORY_PANEL_GAME_TICK_H

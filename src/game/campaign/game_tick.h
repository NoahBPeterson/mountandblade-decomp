// game_tick.h: the campaign map's frame tick and its helpers (code 0x516ac0..0x5191e0): the game data object (GameData,
// src/game/data/game_data.h; the slices called it Game and GameData) with the methods of this region, the paged pool of
// map events, the small records the tick walks. The methods are members of GameData (declared in game_data.h). Names provisional.
#ifndef GAME_INVENTORY_PANEL_GAME_TICK_H
#define GAME_INVENTORY_PANEL_GAME_TICK_H
#include "game/mission/inv_common.h"
#include <vector>                          // (the marker lists are std::vector<rgl::mesh*> members of g_script)

// the map event pool at game+0xcc is GameData::events (Pool<Event>, engine/core/pool.h); its cursor 0x517960 / 0x5179b0 is
// EventPool::next / first (../event_pool.h, defined in game_tick.cpp); the records are game/campaign/event.h's Event
#include "game/campaign/event_pool.h"

// the party-track ribbons (GameData::tracks): src/game/campaign/track_rec.h
#include "game/campaign/track_rec.h"
struct ItemIter {                       // game/party/party_map.h's result of ItemArray::at (a constructor makes it a hidden-return class)
    TrackRec* p;
    ItemIter(TrackRec* q) : p(q) {}
};
struct ItemArray {                      // the iterator helper on a bare base pointer (0x4b22b0, game/party/party_map.h)
    TrackRec* base;
    ItemIter at(int idx);               // 0x4b22b0 (ret 8, hidden return pointer)
};


// the objects at game+0x139e8 (snd_bank::update_all 0x462760, game/data/item_ref_game_ui.h) and game+0x139f4 (SimpleTriggerVec::update 0x48add0,
// game/data/trigger_vecs.h) are called through those classes
#include "game/data/item_ref_game_ui.h"
#include "game/data/trigger_vecs.h"   // SimpleTriggerVec: the simple-trigger list

inline Party* GameData::player_party()
{
    PartyMgr* mgr = &parties;
    int i = mgr->lookup(main_party);
    return mgr->pages[i >> 4] + (i & 0xf);
}

// the map scene object (global 0xa732b4): a colour at +0x68 (its w lane is the alpha at +0x74) and the per-frame step
#include "engine/scene/world.h"                      // rgl::world: the 0x344-byte scene (step 0x427f40, reset 0x421390, create_entity 0x428440)
#include "game/common/level_global.h"      // g_level (0xa732b4)
#include "engine/render/light.h"                                    // Light: GameData::wind, the map sun (update_colors 0x517350)

#endif // GAME_INVENTORY_PANEL_GAME_TICK_H

// game_tick.h: the campaign map's frame tick and its helpers (code 0x516ac0..0x5191e0): the game data object (GameData,
// src/include/game_data.h; the slices called it Game and GameData) with the methods of this region, the paged pool of
// map events, the small records the tick walks. The methods are members of GameG, GameData's derived class that adds
// them (the canonical header has no room for them); the fields they read are GameData's members. Names provisional.
#ifndef GAME_INVENTORY_PANEL_GAME_TICK_H
#define GAME_INVENTORY_PANEL_GAME_TICK_H
#include "./inv_common.h"
#include "../int_vec.h"                   // IntVec / RawVec (the marker lists are std::vector<rgl::mesh*> members of g_script)

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

struct GameG : GameDataV {
    void tick(float dt);                            // 0x516ac0 (ret 4)
    __declspec(noinline) void resume_strings(tstring* a, tstring* b);   // 0x517110 (ret 8)
    void update_markers();                          // 0x517140
    void update_colors();                           // 0x517350
    rgl::vector4* sun_color(rgl::vector4* out);     // 0x5176e0 (ret 4)
    rgl::vector4* sky_color(rgl::vector4* out);     // 0x517770 (ret 4)
    rgl::vector4* fog_color(rgl::vector4* out);     // 0x517820 (ret 4)
    void set_grid_color(unsigned* c);               // 0x5178d0 (ret 4)
    void update_cursor(float dt);                   // 0x5179f0 (ret 4: party / settlement visibility)
    void update_weather();                          // 0x517e70
    void weekly_report(bool arg);                   // 0x5181c0 (ret 4)
    void track_parties(float dt);                   // 0x518680 (ret 4)
    __declspec(noinline) float skill_plus_8();      // 0x518940
    void update_tracks();                           // 0x5189e0
    void train_troops();                            // 0x518d70
    // helpers of the game object this region calls
    Party* player_party()
    {
        PartyMgr* mgr = &parties;
        int i = mgr->lookup(main_party);
        Party** pp = mgr->pages._Myfirst + (i >> 4);
        return *pp + (i & 0xf);
    }
};

// the map scene object (global 0xa732b4): a colour at +0x68 (its w lane is the alpha at +0x74) and the per-frame step
#include "../../rgl/scene/world.h"                      // rgl::world: the 0x344-byte scene (step 0x427f40, reset 0x421390, create_entity 0x428440)
#include "../../include/level_global.h"      // g_level (0xa732b4)
#include "../light.h"                                    // Light: GameData::wind, the map sun (update_colors 0x517350)

#endif // GAME_INVENTORY_PANEL_GAME_TICK_H

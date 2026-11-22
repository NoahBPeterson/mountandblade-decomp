// pool.h: Pool<T>, the game's paged record pool (0x2c bytes): records live in pages of 16 (`new T[16]`), index i names
// pages[i >> 4][i & 0xf], released indices go on a free list, and each record handed out is stamped with a running serial.
// ONE definition for its instances:
//   * Pool<MAgent> / Pool<Missile>: the mission's agent table at Mission +4 and its 0xb0-byte record pool at Mission +0x30
//     (terrain_b: reset 0x477f00 / 0x477f90, alloc 0x478020, next_used 0x474300; agent_obj.h's AgentTable views the first);
//   * Pool<Event>: the map event / battle records at GameData +0xcc (game_data.h `events`; alloc 0x493700, game_data_party_quest.cpp);
//   * Pool<Party>: the party records of PartyMgr (party.h), after its id hash, at PartyMgr +0x138b8. Its serial is the next
//     party id (PartyMgr::add_slot 0x493960 hashes it), its records' +4 the in-use type, +8 the id.
// The size: every instance ends in a word zeroed with count and serial (Game's view of the +0xcc pool, game/campaign/game_state_load.h
// SpriteTable 0x51d220; PartyMgr 0x51d250, after its hash loop) and the mission's two pools sit 0x2c apart (+4, +0x30).
// The constructor (inline: Mission::Mission 0x4d3880 zeroes each pool's words right after its two vectors; SpriteTable
// 0x51d220 is the same body out of line) zeroes count, serial and f28; PartyMgr (0x51d250) zeroes them itself after its
// hash loop, so it builds its pool through the unzeroed constructor.
// The other members are per-instance explicit specialisations (the bodies name the record's own in-use / id / serial fields).
#ifndef MB_POOL_H
#define MB_POOL_H
#include <vector>

template<class T> struct Pool {
    int count;                                  // +0x00 records allocated: pages * 16 (0x478020, 0x493700, PartyMgr::grow 0x493830)
    std::vector<int> free_ids;                  // +0x04 indices of the unused records (allocator word; first at +0x08)
    std::vector<T*> pages;                      // +0x14 the pages of 16 records (first at +0x18). A std::vector: the readers index it
                                                // through operator[] (lea + load: 0x4739f0, 0x49e9c0); the raw readers use (&*pages.begin())[...]
    unsigned serial;                            // +0x24 stamped into each new record (0x478020, 0x493700: +0x0c; PartyMgr 0x493960: the id)
    int f28;                                    // +0x28 zeroed with count and serial (0x51d220, 0x51d250); no reader known

    struct unzeroed {};                         // selects the constructor that leaves count, serial and f28 to the owner
    Pool() { count = 0; serial = 0; f28 = 0; }  // inline (0x4d3880, 0x51d220)
    explicit Pool(unzeroed) {}                  // PartyMgr 0x51d250

    bool valid(int i) { return i < count; }
    T* at(int i) { return pages[i >> 4] + (i & 0xf); }            // the record of index i (a signed index: sar)
    T* atu(unsigned i) { return pages[i >> 4] + (i & 0xf); }      // the same for an unsigned index (shr)
    T* get(unsigned i);                         // 0x48ef20 (Event, game/ui/widgets/widget_overlay_api.cpp): at() out of line (thiscall, ret 4)
    void next_used(unsigned* i);                // 0x474300 (Missile); the agent table's iterator 0x43b160 is AgentTable::next_used (agent_obj.h)
    __forceinline unsigned first_used()         // 0x43b1b0 (inlined here)
    {
        unsigned i = 0;
        if (0 < count) {
            do {
                if (pages[i >> 4][i & 0xf].in_use != 0) break;
                i++;
            } while ((int)i < count);
        }
        return i;
    }
    void reset();                               // 0x477f00 (MAgent), 0x477f90 (Missile)
    void write_table(struct stream_base* out);    // 0x4e99d0 (Event, game/save/save_records.cpp: the used records of the save)
    int alloc();                                // 0x478020 (MAgent), 0x493700 (Event): pops a free index, adding a page of 16 when none is left
    __declspec(noinline) ~Pool();               // 0x4c6e30: deletes the pages (`delete[]`: the record's vector deleting destructor), then the
                                                // two vectors free their storage. One body for every T (identical-code folding): the copy
                                                // the original keeps is Pool<Event>'s (game/editor/editor_selection_and_translation.cpp). The owners' teardowns
                                                // Mission::~Mission 0x4d51d0 and GameData::world_free 0x492f40 call it
};

template<class T> Pool<T>::~Pool()
{
    for (typename std::vector<T*>::iterator p = pages.begin(); p < pages.end(); ++p) {
        delete[] *p;
        *p = 0;
    }
}

#endif // MB_POOL_H

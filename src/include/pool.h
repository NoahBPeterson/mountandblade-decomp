// pool.h: Pool<T>, the game's paged record pool (0x2c bytes): records live in pages of 16 (`new T[16]`), index i names
// pages[i >> 4][i & 0xf], released indices go on a free list, and each record handed out is stamped with a running serial.
// ONE definition for its instances:
//   * Pool<MAgent> / Pool<RecJ>: the mission's agent table at Mission +4 and its 0xb0-byte record pool at Mission +0x30
//     (terrain_b: reset 0x477f00 / 0x477f90, alloc 0x478020, next_used 0x474300; agent_obj.h's AgentTable views the first);
//   * Pool<Event>: the map event / battle records at GameData +0xcc (game_data.h `events`; alloc 0x493700, world_data.cpp);
//   * Pool<Party>: the party records of PartyMgr (party.h), after its id hash, at PartyMgr +0x138b8. Its serial is the next
//     party id (PartyMgr::add_slot 0x493960 hashes it), its records' +4 the in-use type, +8 the id.
// The size: every instance is followed by a word its owner's constructor zeroes with count and serial (Game's view of the
// +0xcc pool, party/game.h SpriteTable 0x51d220; PartyMgr 0x51d250, after its hash loop) and the mission's two pools sit
// 0x2c apart (+4, +0x30). The pool has no constructor of its own: those owners zero its words in their own bodies.
// The other members are per-instance explicit specialisations (the bodies name the record's own in-use / id / serial fields).
#ifndef MB_POOL_H
#define MB_POOL_H
#include <vector>

template<class T> struct Pool {
    int count;                                  // +0x00 records allocated: pages * 16 (0x478020, 0x493700, PartyMgr::grow 0x493830)
    std::vector<int> free_ids;                  // +0x04 indices of the unused records (allocator word; first at +0x08)
    std::vector<T*> pages;                      // +0x14 the pages of 16 records (first at +0x18). A std::vector: the readers index it
                                                // through operator[] (lea + load: 0x4739f0, 0x49e9c0); the raw readers use pages._Myfirst[...]
    unsigned serial;                            // +0x24 stamped into each new record (0x478020, 0x493700: +0x0c; PartyMgr 0x493960: the id)
    int f28;                                    // +0x28 zeroed with count and serial (0x51d220, 0x51d250); no reader known

    bool valid(int i) { return i < count; }
    T* at(int i) { return pages[i >> 4] + (i & 0xf); }            // the record of index i (a signed index: sar)
    T* atu(unsigned i) { return pages[i >> 4] + (i & 0xf); }      // the same for an unsigned index (shr)
    T* get(unsigned i);                         // 0x48ef20 (Event, mission_ui2/slot_api.cpp): at() out of line (thiscall, ret 4)
    void next_used(unsigned* i);                // 0x474300 (RecJ); the agent table's iterator 0x43b160 is AgentTable::next_used (agent_obj.h)
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
    void reset();                               // 0x477f00 (MAgent), 0x477f90 (RecJ)
    int alloc();                                // 0x478020 (MAgent), 0x493700 (Event): pops a free index, adding a page of 16 when none is left
};

#endif // MB_POOL_H
